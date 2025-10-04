#pragma once

#include <future>
#include <string>

#include "Character.h"

enum class AIServiceProvider {
    None,
    Stability,
    Automatic1111
};

struct AIServiceConfig {
    std::string host;
    std::string engineId;
    std::string apiKey;
    std::string negativePrompt;
    std::string basicAuth;
    std::string apiKeyHeader;
    std::string apiKeyValue;
    AIServiceProvider provider{AIServiceProvider::None};
};

class AIArtManager {
public:
    explicit AIArtManager(const std::string& cacheDir = ".cache");

    void attachAsciiArt(Character& character, const CharacterTemplate& tmpl, const std::string& userPrompt);
    void setVerbose(bool verbose);

private:
    std::string cacheDir_;
    std::string asciiDir_;
    AIServiceConfig config_;
    bool verbose_{true};

    std::string cacheFilePath(const CharacterTemplate& tmpl, const std::string& userPrompt) const;
    bool loadFromCache(const std::string& path, std::string& asciiArt) const;
    void saveToCache(const std::string& path, const std::string& asciiArt) const;

    std::string requestAsciiFromService(const CharacterTemplate& tmpl, const std::string& userPrompt);
    std::string buildPrompt(const CharacterTemplate& tmpl, const std::string& userPrompt) const;
    std::string placeholderAscii(const CharacterTemplate& tmpl) const;

    void showLoadingAnimation(std::future<std::string>& future) const;

#ifdef USE_LIBCURL
    std::string requestViaStability(const CharacterTemplate& tmpl, const std::string& userPrompt,
        const std::string& fallbackAscii);
    std::string requestViaAutomatic1111(const CharacterTemplate& tmpl, const std::string& userPrompt,
        const std::string& fallbackAscii);
#endif
};
