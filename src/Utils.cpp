#include "Utils.h"

#include "Character.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <sstream>

std::string sanitizeFileName(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            result.push_back(c);
        } else if (c == ' ' || c == '_' || c == '-') {
            result.push_back('_');
        }
    }
    if (result.empty()) {
        result = "character";
    }
    return result;
}

char brightnessToAscii(double value) {
    static const std::string ramp = "@%#*+=-:. ";
    value = std::clamp(value, 0.0, 1.0);
    size_t index = static_cast<size_t>(value * (ramp.size() - 1));
    return ramp[index];
}

std::string renderAsciiFromGrayscale(const std::vector<float>& grayscale, int width, int height, int targetWidth) {
    if (width <= 0 || height <= 0 || grayscale.empty()) {
        return "";
    }
    targetWidth = std::max(1, targetWidth);
    double scaleX = static_cast<double>(width) / static_cast<double>(targetWidth);
    double scaleY = scaleX * 2.0; // approximate correction for character aspect ratio
    int targetHeight = std::max(1, static_cast<int>(std::round(static_cast<double>(height) / scaleY)));

    std::ostringstream oss;
    for (int y = 0; y < targetHeight; ++y) {
        for (int x = 0; x < targetWidth; ++x) {
            int srcX = std::clamp(static_cast<int>(std::round(x * scaleX)), 0, width - 1);
            int srcY = std::clamp(static_cast<int>(std::round(y * scaleY)), 0, height - 1);
            double value = grayscale[srcY * width + srcX];
            oss << brightnessToAscii(value);
        }
        if (y + 1 < targetHeight) {
            oss << '\n';
        }
    }
    return oss.str();
}

std::string generatePlaceholderAsciiArt(const std::string& name, int rarity) {
    constexpr int width = 64;
    constexpr int height = 32;
    std::vector<float> grayscale(width * height, 0.0f);
    double centerX = width / 2.0;
    double centerY = height / 2.0;
    double radius = std::min(width, height) / 2.0;
    double rarityBoost = 0.1 * rarity;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double dx = (x - centerX) / radius;
            double dy = (y - centerY) / radius;
            double dist = std::sqrt(dx * dx + dy * dy);
            double brightness = std::clamp(1.2 - dist * (1.2 - rarityBoost), 0.0, 1.0);
            grayscale[y * width + x] = static_cast<float>(brightness);
        }
    }

    std::string art = renderAsciiFromGrayscale(grayscale, width, height, 64);

    // Add name banner at top
    std::ostringstream banner;
    banner << name << " (" << rarityToString(rarity) << ")";
    std::string bannerText = banner.str();

    std::ostringstream oss;
    oss << bannerText << "\n";
    oss << art;
    return oss.str();
}

void ensureDirectory(const std::string& path) {
    if (path.empty()) {
        return;
    }
    std::filesystem::create_directories(path);
}
