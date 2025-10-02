#pragma once

#include <future>
#include <string>

#include "Character.h"

struct AIServiceConfig {
    std::string endpoint;
    std::string apiKey;
};

class AIArtManager {
public:
    explicit AIArtManager(const std::string& cacheDir = ".cache");

    void attachAsciiArt(Character& character, const CharacterTemplate& tmpl);
    void setVerbose(bool verbose);

private:
    std::string cacheDir_;
    std::string asciiDir_;
    AIServiceConfig config_;
    bool verbose_{true};

    std::string cacheFilePath(const CharacterTemplate& tmpl) const;
    bool loadFromCache(const std::string& path, std::string& asciiArt) const;
    void saveToCache(const std::string& path, const std::string& asciiArt) const;

    std::string requestAsciiFromService(const CharacterTemplate& tmpl);
    std::string placeholderAscii(const CharacterTemplate& tmpl) const;

    void showLoadingAnimation(std::future<std::string>& future) const;
};
