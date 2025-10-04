#include "Gacha.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <stdexcept>

#include "AIArtManager.h"
#include "Inventory.h"

GachaSystem::GachaSystem()
    : rng_(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count())) {
    rarityLevels_ = {1, 2, 3, 4, 5};
    std::vector<double> weights = {40.0, 30.0, 15.0, 10.0, 5.0};
    rarityDistribution_ = std::discrete_distribution<int>(weights.begin(), weights.end());

    templatesByRarity_[1] = {
        {"Glimmer Slime", 1, 120, 18, "Sticky Tackle", "A cheerful slime made of shimmering jelly"},
        {"Rusty Squire", 1, 140, 16, "Shield Bash", "A rookie knight with dented armor but a bright smile"}
    };
    templatesByRarity_[2] = {
        {"Wind Ranger", 2, 180, 28, "Gale Arrow", "An agile archer controlling a swirl of leaves"},
        {"Tide Cleric", 2, 200, 24, "Healing Wave", "A priestess calling tides of healing water"}
    };
    templatesByRarity_[3] = {
        {"Arc Forge", 3, 240, 36, "Thunder Hammer", "A cyborg blacksmith forging lightning"},
        {"Crimson Dancer", 3, 220, 40, "Blazing Waltz", "A warrior dancing amid petals of flame"}
    };
    templatesByRarity_[4] = {
        {"Nebula Oracle", 4, 280, 48, "Starfall Prophecy", "A mystic floating in a cosmic observatory"},
        {"Aurora Valkyrie", 4, 300, 52, "Radiant Strike", "A valkyrie wielding aurora colored wings"}
    };
    templatesByRarity_[5] = {
        {"Eclipse Dragon", 5, 360, 72, "Solar Eclipse", "A dragon woven from sun and moon"},
        {"Chrono Empress", 5, 320, 68, "Time Collapse", "A regal mage bending time around her"}
    };
}

const CharacterTemplate& GachaSystem::randomTemplateForRarity(int rarity) {
    auto it = templatesByRarity_.find(rarity);
    if (it == templatesByRarity_.end() || it->second.empty()) {
        throw std::runtime_error("No templates for rarity");
    }
    std::uniform_int_distribution<size_t> dist(0, it->second.size() - 1);
    return it->second[dist(rng_)];
}

Character GachaSystem::rollCharacter(AIArtManager& artManager, const std::string& userPrompt) {
    int rarityIndex = rarityDistribution_(rng_);
    int rarity = rarityLevels_[rarityIndex];
    const CharacterTemplate& tmpl = randomTemplateForRarity(rarity);

    Character character(nextId_++, tmpl);
    artManager.attachAsciiArt(character, tmpl, userPrompt);
    return character;
}

void GachaSystem::syncNextId(const Inventory& inventory) {
    int maxId = 0;
    for (const auto& character : inventory.getCharacters()) {
        maxId = std::max(maxId, character.id);
    }
    nextId_ = std::max(nextId_, maxId + 1);
}
