#include "AIArtManager.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>

#include "Utils.h"

#include <nlohmann/json.hpp>

#ifdef USE_LIBCURL
#include <curl/curl.h>
#endif

AIArtManager::AIArtManager(const std::string& cacheDir)
    : cacheDir_(cacheDir), asciiDir_(cacheDir_ + "/ascii") {
    ensureDirectory(asciiDir_);

    if (const char* endpoint = std::getenv("AI_GACHA_API_URL")) {
        config_.endpoint = endpoint;
    }
    if (const char* apiKey = std::getenv("AI_GACHA_API_KEY")) {
        config_.apiKey = apiKey;
    }
}

void AIArtManager::setVerbose(bool verbose) {
    verbose_ = verbose;
}

std::string AIArtManager::cacheFilePath(const CharacterTemplate& tmpl) const {
    std::string name = sanitizeFileName(tmpl.name + "_" + std::to_string(tmpl.rarity));
    return asciiDir_ + "/" + name + ".txt";
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

void AIArtManager::attachAsciiArt(Character& character, const CharacterTemplate& tmpl) {
    std::string cachePath = cacheFilePath(tmpl);
    std::string asciiArt;
    if (!loadFromCache(cachePath, asciiArt)) {
        auto future = std::async(std::launch::async, [this, tmpl]() { return requestAsciiFromService(tmpl); });
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
} // namespace
#endif

std::string AIArtManager::requestAsciiFromService(const CharacterTemplate& tmpl) {
    std::string ascii = placeholderAscii(tmpl);

#ifdef USE_LIBCURL
    if (!config_.endpoint.empty()) {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string response;
            nlohmann::json payload = nlohmann::json::object();
            payload["prompt"] = tmpl.prompt + " rendered as hero portrait";

            std::string payloadStr = payload.dump();

            curl_easy_setopt(curl, CURLOPT_URL, config_.endpoint.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payloadStr.size());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            std::string authHeader;
            if (!config_.apiKey.empty()) {
                authHeader = "Authorization: Bearer " + config_.apiKey;
                headers = curl_slist_append(headers, authHeader.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                try {
                    auto json = nlohmann::json::parse(response);
                    if (json.contains("ascii")) {
                        ascii = json["ascii"].get<std::string>();
                    } else if (json.contains("art")) {
                        ascii = json["art"].get<std::string>();
                    }
                } catch (const std::exception&) {
                    // Ignore parsing errors and fallback to placeholder art.
                }
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    }
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
