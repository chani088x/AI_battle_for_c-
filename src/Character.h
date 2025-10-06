#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct CharacterTemplate {
    std::string name;
    int rarity{};
    int baseHP{};
    int baseATK{};
    std::string skill;
    std::string prompt;
};

struct Character {
    int id{0};
    std::string name;
    int rarity{1};
    int maxHP{0};
    int attack{0};
    std::string skill;
    std::string asciiArt;
    std::string artCachePath;
    std::string artImagePath;
    int currentHP{0};

    Character() = default;
    Character(int id, const CharacterTemplate& tmpl);

    void resetHP();
    bool isAlive() const;

    nlohmann::json toJson() const;
    static Character fromJson(const nlohmann::json& data);
};

std::string rarityToString(int rarity);
