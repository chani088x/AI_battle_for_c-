#pragma once

#include <string>
#include <vector>

std::string sanitizeFileName(const std::string& input);
char brightnessToAscii(double value);
std::string renderAsciiFromGrayscale(const std::vector<float>& grayscale, int width, int height, int targetWidth);
std::string generatePlaceholderAsciiArt(const std::string& name, int rarity);
void ensureDirectory(const std::string& path);
