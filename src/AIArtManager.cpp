#include "AIArtManager.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "Utils.h"

#include <nlohmann/json.hpp>

#ifdef USE_LIBCURL
#include <curl/curl.h>
#endif

#include <functional>

#ifdef USE_STB_IMAGE
#include "stb_image.h"
#endif

AIArtManager::AIArtManager(const std::string& cacheDir)
    : cacheDir_(cacheDir), asciiDir_(cacheDir_ + "/ascii") {
    ensureDirectory(asciiDir_);

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
    if (const char* apiKey = std::getenv("STABILITY_API_KEY")) {
        config_.apiKey = apiKey;
    }
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
    std::string asciiArt;
    if (!loadFromCache(cachePath, asciiArt)) {
        auto future = std::async(std::launch::async, [this, tmpl, userPrompt]() {
            return requestAsciiFromService(tmpl, userPrompt);
        });
        showLoadingAnimation(future);
        asciiArt = future.get();
        saveToCache(cachePath, asciiArt);
    }
    character.asciiArt = asciiArt;
    character.artCachePath = cachePath;
}

std::string AIArtManager::placeholderAscii(const CharacterTemplate& tmpl) const {
    return generatePlaceholderAsciiArt(tmpl.name, tmpl.rarity);
}

#ifdef USE_LIBCURL
namespace {
size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}
std::vector<unsigned char> decodeBase64(const std::string& input) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> lookup(256, -1);
    for (size_t i = 0; i < chars.size(); ++i) {
        lookup[static_cast<unsigned char>(chars[i])] = static_cast<int>(i);
    }

    std::vector<unsigned char> output;
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
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
#endif

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

std::string AIArtManager::requestAsciiFromService(const CharacterTemplate& tmpl, const std::string& userPrompt) {
    std::string ascii = placeholderAscii(tmpl);

#ifdef USE_LIBCURL
    if (config_.apiKey.empty() || config_.host.empty() || config_.engineId.empty()) {
        return ascii;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return ascii;
    }

    std::string url = config_.host + "/v1/generation/" + config_.engineId + "/text-to-image";
    std::string response;

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

    std::string payloadStr = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payloadStr.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    std::string authHeader = "Authorization: Bearer " + config_.apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        try {
            auto json = nlohmann::json::parse(response);
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
                            break;
                        }
                    }
                }
            }
        } catch (const std::exception&) {
            // 파싱/변환 오류는 무시하고 플레이스홀더 아트로 되돌아간다.
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
#endif

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
