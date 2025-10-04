#include "Inventory.h"

#include <algorithm>

void Inventory::addCharacter(const Character& character) {
    characters_.push_back(character);
}

std::vector<Character>& Inventory::getCharacters() {
    return characters_;
}

const std::vector<Character>& Inventory::getCharacters() const {
    return characters_;
}

Character* Inventory::findById(int id) {
    auto it = std::find_if(characters_.begin(), characters_.end(), [id](const Character& c) { return c.id == id; });
    if (it == characters_.end()) {
        return nullptr;
    }
    return &(*it);
}

const Character* Inventory::findById(int id) const {
    auto it = std::find_if(characters_.cbegin(), characters_.cend(), [id](const Character& c) { return c.id == id; });
    if (it == characters_.cend()) {
        return nullptr;
    }
    return &(*it);
}

void Inventory::setTeam(const std::vector<int>& teamIds) {
    teamIds_ = teamIds;
}

const std::vector<int>& Inventory::getTeamIds() const {
    return teamIds_;
}

std::vector<Character*> Inventory::getTeamMembers() {
    std::vector<Character*> result;
    for (int id : teamIds_) {
        if (Character* character = findById(id)) {
            result.push_back(character);
        }
    }
    return result;
}

std::vector<const Character*> Inventory::getTeamMembers() const {
    std::vector<const Character*> result;
    for (int id : teamIds_) {
        if (const Character* character = findById(id)) {
            result.push_back(character);
        }
    }
    return result;
}
