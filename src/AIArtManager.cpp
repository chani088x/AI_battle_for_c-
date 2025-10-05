#include "AIArtManager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <sstream>
#include <memory>
#include <thread>
#include <vector>

#include "Utils.h"
#include "HttpClient.h"

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>

#ifdef USE_STB_IMAGE
#include "stb_image.h"
#endif

namespace {
std::string toLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string ensureTrailingSlash(const std::string& base) {
    if (base.empty() || base.back() == '/') {
        return base;
    }
    return base + "/";
}

bool looksLikeBase64(const std::string& value) {
    if (value.size() < 16) {
        return false;
    }
    size_t usefulChars = 0;
    for (char ch : value) {
        if (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t') {
            continue;
        }
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '+' || ch == '/' || ch == '=' || ch == '-' || ch == '_') {
            ++usefulChars;
            continue;
        }
        return false;
    }
    return usefulChars >= 16;
}

std::string stripDataUriPrefix(const std::string& value) {
    auto delimiterPos = value.find(',');
    if (delimiterPos != std::string::npos) {
        return value.substr(delimiterPos + 1);
    }
    return value;
}

std::optional<std::string> findBase64Image(const nlohmann::json& node) {
    if (node.is_string()) {
        std::string candidate = node.get<std::string>();
        std::string stripped = stripDataUriPrefix(candidate);
        if (looksLikeBase64(stripped)) {
            return stripped;
        }
        return std::nullopt;
    }

    if (node.is_array()) {
        const auto elements = node.get<nlohmann::json::array_t>();
        for (const auto& element : elements) {
            auto found = findBase64Image(element);
            if (found) {
                return found;
            }
        }
        return std::nullopt;
    }

    if (node.is_object()) {
        const auto object = node.get<nlohmann::json::object_t>();
        static const std::vector<std::string> preferredKeys = {
            "image", "data", "bytes", "base64", "payload", "content"
        };

        for (const std::string& key : preferredKeys) {
            auto it = object.find(key);
            if (it != object.end()) {
                auto found = findBase64Image(it->second);
                if (found) {
                    return found;
                }
            }
        }

        for (const auto& item : object) {
            auto found = findBase64Image(item.second);
            if (found) {
                return found;
            }
        }
    }

    return std::nullopt;
}

std::string providerToString(AIServiceProvider provider) {
    switch (provider) {
    case AIServiceProvider::Stability:
        return "Stability";
    case AIServiceProvider::Automatic1111:
        return "Automatic1111";
    case AIServiceProvider::None:
    default:
        return "None";
    }
}
} // 익명 네임스페이스 종료

AIArtManager::AIArtManager(const std::string& cacheDir)
    : cacheDir_(cacheDir), asciiDir_(cacheDir_ + "/ascii"), logPath_(cacheDir_ + "/ai_art.log") {
    ensureDirectory(cacheDir_);
    ensureDirectory(asciiDir_);

    std::string providerChoice;
    if (const char* providerEnv = std::getenv("AI_GACHA_PROVIDER")) {
        providerChoice = toLower(providerEnv);
    }

    if (providerChoice == "automatic1111") {
        config_.provider = AIServiceProvider::Automatic1111;
    } else if (providerChoice == "stability") {
        config_.provider = AIServiceProvider::Stability;
    } else if (providerChoice == "none") {
        config_.provider = AIServiceProvider::None;
    }

    if (const char* apiKey = std::getenv("STABILITY_API_KEY")) {
        config_.apiKey = apiKey;
        if (config_.provider == AIServiceProvider::None && !config_.apiKey.empty()) {
            config_.provider = AIServiceProvider::Stability;
        }
    }

    if (config_.provider == AIServiceProvider::None) {
        // 기본값은 로컬 Automatic1111 WebUI를 사용한다고 가정한다.
        config_.provider = AIServiceProvider::Automatic1111;
    }

    if (config_.provider == AIServiceProvider::Stability) {
        if (const char* host = std::getenv("STABILITY_API_HOST")) {
            config_.host = host;
        } else {
            config_.host = "https://api.stability.ai";
        }
        if (const char* engine = std::getenv("STABILITY_ENGINE_ID")) {
            config_.engineId = engine;
        } else {
            config_.engineId = "stable-diffusion-v1-6";
        }
    } else if (config_.provider == AIServiceProvider::Automatic1111) {
        if (const char* host = std::getenv("A1111_API_HOST")) {
            config_.host = host;
        } else {
            config_.host = "http://127.0.0.1:7860";
        }
        if (const char* auth = std::getenv("A1111_API_AUTH")) {
            config_.basicAuth = auth;
        }
        if (const char* apiKey = std::getenv("A1111_API_KEY")) {
            config_.apiKeyValue = apiKey;
        }
        if (const char* apiKeyHeader = std::getenv("A1111_API_KEY_HEADER")) {
            config_.apiKeyHeader = apiKeyHeader;
        } else if (!config_.apiKeyValue.empty()) {
            config_.apiKeyHeader = "X-API-Key";
        }
    }

    if (const char* negative = std::getenv("A1111_NEGATIVE_PROMPT")) {
        config_.negativePrompt = negative;
    }

    logMessage("AIArtManager 초기화: provider=" + providerToString(config_.provider) + ", host=" + config_.host);
}

void AIArtManager::setVerbose(bool verbose) {
    verbose_ = verbose;
}

std::string AIArtManager::cacheFilePath(const CharacterTemplate& tmpl, const std::string& userPrompt) const {
    std::string name = sanitizeFileName(tmpl.name + "_" + std::to_string(tmpl.rarity));
    std::string hashSuffix = userPrompt.empty() ? "default" : std::to_string(std::hash<std::string>{}(userPrompt));
    return asciiDir_ + "/" + name + "_" + hashSuffix + ".txt";
}

bool AIArtManager::loadFromCache(const std::string& path, std::string& asciiArt) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    asciiArt.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
}

void AIArtManager::saveToCache(const std::string& path, const std::string& asciiArt) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return;
    }
    file << asciiArt;
}

void AIArtManager::attachAsciiArt(Character& character, const CharacterTemplate& tmpl, const std::string& userPrompt) {
    std::string cachePath = cacheFilePath(tmpl, userPrompt);
    std::string fallback = placeholderAscii(tmpl);
    std::string asciiArt;
    bool cacheValid = false;

    if (loadFromCache(cachePath, asciiArt)) {
        if (asciiArt == fallback) {
            logMessage("캐시에 플레이스홀더 ASCII가 저장되어 있어 무시합니다: " + cachePath);
        } else {
            cacheValid = true;
            logMessage("캐시 히트: " + cachePath);
        }
    }

    if (!cacheValid) {
        logMessage("캐시 미스 발생: " + cachePath + ", 새로운 ASCII 아트를 생성합니다.");
        auto placeholderFlag = std::make_shared<std::atomic<bool>>(true);
        auto future = std::async(std::launch::async, [this, tmpl, userPrompt, fallback, placeholderFlag]() {
            bool usedPlaceholder = true;
            std::string ascii = requestAsciiFromService(tmpl, userPrompt, fallback, usedPlaceholder);
            placeholderFlag->store(usedPlaceholder);
            return ascii;
        });
        showLoadingAnimation(future);
        asciiArt = future.get();
        bool usedPlaceholder = placeholderFlag->load();
        if (!usedPlaceholder) {
            saveToCache(cachePath, asciiArt);
            logMessage("새 ASCII 아트를 캐시에 저장했습니다: " + cachePath);
            character.artCachePath = cachePath;
        } else {
            logMessage("플레이스홀더 ASCII는 캐시에 저장하지 않습니다: " + cachePath);
            character.artCachePath.clear();
        }
    } else {
        character.artCachePath = cachePath;
    }

    if (asciiArt.empty()) {
        asciiArt = fallback;
    }

    character.asciiArt = asciiArt;
}

std::string AIArtManager::placeholderAscii(const CharacterTemplate& tmpl) const {
    return generatePlaceholderAsciiArt(tmpl.name, tmpl.rarity);
}

namespace {
std::vector<unsigned char> decodeBase64(const std::string& input) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> lookup(256, -1);
    for (size_t i = 0; i < chars.size(); ++i) {
        lookup[static_cast<unsigned char>(chars[i])] = static_cast<int>(i);
    }

    std::vector<unsigned char> output;
    std::string sanitized;
    sanitized.reserve(input.size());
    for (unsigned char c : input) {
        if (c == '-') {
            sanitized.push_back('+');
        } else if (c == '_') {
            sanitized.push_back('/');
        } else if (c != '\r' && c != '\n' && c != '\t' && c != ' ') {
            sanitized.push_back(static_cast<char>(c));
        }
    }

    int val = 0;
    int valb = -8;
    for (unsigned char c : sanitized) {
        if (lookup[c] == -1) {
            if (c == '=') {
                break;
            }
            continue;
        }
        val = (val << 6) + lookup[c];
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

std::string encodeBase64(const std::string& input) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    int val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        output.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (output.size() % 4 != 0) {
        output.push_back('=');
    }
    return output;
}

std::string convertImageToAscii(const std::vector<unsigned char>& image) {
    if (image.empty()) {
        return {};
    }
#ifdef USE_STB_IMAGE
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load_from_memory(image.data(), static_cast<int>(image.size()), &width, &height, &channels, 3);
    if (!data) {
        return {};
    }

    std::vector<float> grayscale(width * height, 0.0f);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            float r = static_cast<float>(data[index]) / 255.0f;
            float g = static_cast<float>(data[index + 1]) / 255.0f;
            float b = static_cast<float>(data[index + 2]) / 255.0f;
            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
            grayscale[y * width + x] = luminance;
        }
    }

    stbi_image_free(data);

    return renderAsciiFromGrayscale(grayscale, width, height, 64);
#else
    // stb_image가 없으면 바이너리 이미지 데이터를 디코딩할 수 없다.
    // 빈 문자열을 반환해 호출 측에서 플레이스홀더 아트로 대체하도록 한다.
    return {};
#endif
}
} // 익명 네임스페이스 종료

std::string AIArtManager::buildPrompt(const CharacterTemplate& tmpl, const std::string& userPrompt) const {
    std::ostringstream oss;
    if (!tmpl.prompt.empty()) {
        oss << tmpl.prompt;
    }
    if (!userPrompt.empty()) {
        if (oss.tellp() > 0) {
            oss << ", ";
        }
        oss << userPrompt;
    }
    if (oss.tellp() > 0) {
        oss << ", ";
    }
    oss << "hero portrait, dramatic lighting";
    return oss.str();
}

std::string AIArtManager::requestAsciiFromService(const CharacterTemplate& tmpl, const std::string& userPrompt,
    const std::string& fallbackAscii, bool& usedPlaceholder) {
    std::string ascii = fallbackAscii;
    usedPlaceholder = true;

    if (config_.provider == AIServiceProvider::Stability) {
        logMessage("Stability API 요청 시작: " + tmpl.name);
        ascii = requestViaStability(tmpl, userPrompt, ascii);
    } else if (config_.provider == AIServiceProvider::Automatic1111) {
        logMessage("Automatic1111 API 요청 시작: " + tmpl.name);
        ascii = requestViaAutomatic1111(tmpl, userPrompt, ascii);
    }

    usedPlaceholder = (ascii == fallbackAscii);
    return ascii;
}

std::string AIArtManager::requestViaStability(const CharacterTemplate& tmpl, const std::string& userPrompt,
    const std::string& fallbackAscii) {
    if (config_.apiKey.empty() || config_.host.empty() || config_.engineId.empty()) {
        logMessage("Stability API 구성 값이 부족해 플레이스홀더를 사용합니다.");
        return fallbackAscii;
    }

    std::string url = ensureTrailingSlash(config_.host) + "v1/generation/" + config_.engineId + "/text-to-image";
    nlohmann::json payload = nlohmann::json::object();
    nlohmann::json prompt = nlohmann::json::object();
    prompt["text"] = buildPrompt(tmpl, userPrompt);
    nlohmann::json prompts = nlohmann::json::array();
    prompts.push_back(prompt);
    payload["text_prompts"] = prompts;
    payload["cfg_scale"] = 7.0;
    payload["steps"] = 30;
    payload["height"] = 512;
    payload["width"] = 512;

    std::string ascii = fallbackAscii;
    HttpResponse httpResponse;
    std::string errorMessage;
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "application/json"},
        {"Accept", "application/json"},
        {"Authorization", "Bearer " + config_.apiKey}
    };

    if (httpPost(url, payload.dump(), headers, httpResponse, errorMessage)) {
        if (httpResponse.status != 200) {
            std::string statusMsg = "Stability API 응답 코드: " + std::to_string(httpResponse.status);
            if (verbose_) {
                std::cerr << "[AIArt] " << statusMsg << '\n';
            }
            logMessage(statusMsg);
        }
        bool convertedSuccessfully = false;
        try {
            auto json = nlohmann::json::parse(httpResponse.body);
            if (json.contains("artifacts")) {
                const auto& artifacts = json["artifacts"];
                if (artifacts.is_array()) {
                    for (size_t i = 0; i < artifacts.size(); ++i) {
                        const auto& artifact = artifacts[i];
                        if (!artifact.contains("base64")) {
                            continue;
                        }
                        std::string finishReason = artifact.value<std::string>("finishReason", "SUCCESS");
                        if (finishReason != "SUCCESS") {
                            continue;
                        }
                        std::string base64Data = artifact.value<std::string>("base64", "");
                        std::vector<unsigned char> decoded = decodeBase64(base64Data);
                        std::string converted = convertImageToAscii(decoded);
                        if (!converted.empty()) {
                            ascii = converted;
                            convertedSuccessfully = true;
                            break;
                        }
                    }
                }
            }
        } catch (const std::exception&) {
            // 파싱/변환 오류는 무시하고 플레이스홀더 아트로 되돌아간다.
        }
        if (!convertedSuccessfully) {
            std::string warn = "Stability API 응답에서 유효한 이미지를 찾지 못했습니다. 플레이스홀더를 사용합니다.";
            if (verbose_) {
                std::cerr << "[AIArt] " << warn << '\n';
            }
            logMessage(warn);
        }
    } else {
        std::string err = "Stability API 호출 실패: " + errorMessage;
        if (verbose_) {
            std::cerr << "[AIArt] " << err << '\n';
        }
        logMessage(err);
    }
    if (ascii == fallbackAscii) {
        logMessage("Stability API 결과가 플레이스홀더로 유지되었습니다.");
    } else {
        logMessage("Stability API 결과를 ASCII 아트로 변환했습니다.");
    }
    return ascii;
}

std::string AIArtManager::requestViaAutomatic1111(const CharacterTemplate& tmpl, const std::string& userPrompt,
    const std::string& fallbackAscii) {
    if (config_.host.empty()) {
        logMessage("Automatic1111 호스트가 비어 있어 플레이스홀더를 사용합니다.");
        return fallbackAscii;
    }

    std::string url = ensureTrailingSlash(config_.host) + "sdapi/v1/txt2img";
    nlohmann::json payload = nlohmann::json::object();
    payload["prompt"] = buildPrompt(tmpl, userPrompt);
    if (!config_.negativePrompt.empty()) {
        payload["negative_prompt"] = config_.negativePrompt;
    }
    payload["cfg_scale"] = 7.0;
    payload["steps"] = 30;
    payload["width"] = 512;
    payload["height"] = 512;

    std::string ascii = fallbackAscii;
    HttpResponse httpResponse;
    std::string errorMessage;
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "application/json"}
    };
    if (!config_.basicAuth.empty()) {
        headers.emplace_back("Authorization", "Basic " + encodeBase64(config_.basicAuth));
    }
    if (!config_.apiKeyHeader.empty() && !config_.apiKeyValue.empty()) {
        headers.emplace_back(config_.apiKeyHeader, config_.apiKeyValue);
    }

    if (httpPost(url, payload.dump(), headers, httpResponse, errorMessage)) {
        if (httpResponse.status != 200) {
            std::string statusMsg = "Automatic1111 응답 코드: " + std::to_string(httpResponse.status);
            if (verbose_) {
                std::cerr << "[AIArt] " << statusMsg << '\n';
            }
            logMessage(statusMsg);
        }
        bool convertedSuccessfully = false;
        try {
            auto json = nlohmann::json::parse(httpResponse.body);
            auto handleCandidate = [&](const nlohmann::json& candidateNode, const char* debugSource) {
                auto base64Candidate = findBase64Image(candidateNode);
                if (!base64Candidate) {
                    return false;
                }

                std::vector<unsigned char> decoded = decodeBase64(*base64Candidate);
                if (decoded.empty()) {
                    std::string where = debugSource ? std::string(debugSource) : std::string("알 수 없는 경로");
                    logMessage("Automatic1111 Base64 디코딩이 실패했습니다 (" + where + ")");
                    return false;
                }

                std::string converted = convertImageToAscii(decoded);
                if (converted.empty()) {
                    std::string where = debugSource ? std::string(debugSource) : std::string("알 수 없는 경로");
#ifdef USE_STB_IMAGE
                    logMessage("Automatic1111 PNG 디코딩에 실패했습니다 (" + where + ")");
#else
                    logMessage("USE_STB_IMAGE가 비활성화되어 PNG 데이터를 ASCII로 변환하지 못했습니다 (" + where + ")");
#endif
                    return false;
                }

                ascii = converted;
                convertedSuccessfully = true;
                if (debugSource) {
                    logMessage(std::string("Automatic1111 응답에서 Base64 이미지를 추출했습니다 (출처: ") + debugSource + ")");
                }
                return true;
            };

            if (json.contains("images")) {
                const auto& images = json["images"];
                if (images.is_array()) {
                    const auto imageArray = images.get<nlohmann::json::array_t>();
                    for (const auto& imageEntry : imageArray) {
                        if (handleCandidate(imageEntry, "images 배열")) {
                            break;
                        }
                    }
                } else {
                    handleCandidate(images, "images 객체");
                }
            }

            if (!convertedSuccessfully) {
                handleCandidate(json, "최상위 응답");
            }
        } catch (const std::exception&) {
            // 파싱/변환 오류는 무시하고 플레이스홀더 아트로 되돌아간다.
        }
        if (!convertedSuccessfully) {
            std::string warn = "Automatic1111 응답에서 유효한 이미지를 찾지 못했습니다. 플레이스홀더를 사용합니다.";
            if (verbose_) {
                std::cerr << "[AIArt] " << warn << '\n';
            }
            logMessage(warn);
        }
    } else {
        std::string err = "Automatic1111 호출 실패: " + errorMessage;
        if (verbose_) {
            std::cerr << "[AIArt] " << err << '\n';
        }
        logMessage(err);
    }
    if (ascii == fallbackAscii) {
        logMessage("Automatic1111 결과가 플레이스홀더로 유지되었습니다.");
    } else {
        logMessage("Automatic1111 결과를 ASCII 아트로 변환했습니다.");
    }
    return ascii;
}


void AIArtManager::showLoadingAnimation(std::future<std::string>& future) const {
    if (!verbose_) {
        future.wait();
        return;
    }

    static const char spinner[] = {'|', '/', '-', '\\'};
    size_t index = 0;
    using namespace std::chrono_literals;
    while (future.wait_for(200ms) != std::future_status::ready) {
        std::cout << "\rAI 아트를 생성 중... " << spinner[index % 4] << std::flush;
        ++index;
    }
    std::cout << "\rAI 아트를 생성 중... 완료!    \n";
}

void AIArtManager::logMessage(const std::string& message) const {
    std::lock_guard<std::mutex> lock(logMutex_);
    std::ofstream file(logPath_, std::ios::app);
    if (!file.is_open()) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo{};

#if defined(_WIN32)
    localtime_s(&timeInfo, &nowTime);
#else
    localtime_r(&nowTime, &timeInfo);
#endif

    file << '[' << std::put_time(&timeInfo, "%F %T") << "] " << message << '\n';
}
