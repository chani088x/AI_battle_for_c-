#include "Character.h"

#include <sstream>

Character::Character(int idValue, const CharacterTemplate& tmpl)
    : id(idValue),
      name(tmpl.name),
      rarity(tmpl.rarity),
      maxHP(tmpl.baseHP),
      attack(tmpl.baseATK),
      skill(tmpl.skill),
      currentHP(tmpl.baseHP) {}

void Character::resetHP() {
    currentHP = maxHP;
}

bool Character::isAlive() const {
    return currentHP > 0;
}

nlohmann::json Character::toJson() const {
    nlohmann::json data = nlohmann::json::object();
    data["id"] = id;
    data["name"] = name;
    data["rarity"] = rarity;
    data["maxHP"] = maxHP;
    data["attack"] = attack;
    data["skill"] = skill;
    data["asciiArt"] = asciiArt;
    data["artCachePath"] = artCachePath;
    return data;
}

Character Character::fromJson(const nlohmann::json& data) {
    Character result;
    result.id = data.value<int>("id", 0);
    result.name = data.value<std::string>("name", "Unknown");
    result.rarity = data.value<int>("rarity", 1);
    result.maxHP = data.value<int>("maxHP", 0);
    result.attack = data.value<int>("attack", 0);
    result.skill = data.value<std::string>("skill", "");
    result.asciiArt = data.value<std::string>("asciiArt", "");
    result.artCachePath = data.value<std::string>("artCachePath", "");
    result.currentHP = result.maxHP;
    return result;
}

std::string rarityToString(int rarity) {
    switch (rarity) {
        case 5: return "★★★★★";
        case 4: return "★★★★";
        case 3: return "★★★";
        case 2: return "★★";
        default: return "★";
    }
}
