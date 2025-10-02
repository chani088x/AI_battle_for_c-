#pragma once

#include <map>
#include <random>
#include <vector>

#include "Character.h"

class AIArtManager;
class Inventory;

class GachaSystem {
public:
    GachaSystem();

    Character rollCharacter(AIArtManager& artManager);
    void syncNextId(const Inventory& inventory);

private:
    std::map<int, std::vector<CharacterTemplate>> templatesByRarity_;
    std::vector<int> rarityLevels_;
    std::mt19937 rng_;
    std::discrete_distribution<int> rarityDistribution_;
    int nextId_{1};

    const CharacterTemplate& randomTemplateForRarity(int rarity);
};
