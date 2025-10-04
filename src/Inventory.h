#pragma once

#include <optional>
#include <vector>

#include "Character.h"

class Inventory {
public:
    void addCharacter(const Character& character);

    std::vector<Character>& getCharacters();
    const std::vector<Character>& getCharacters() const;

    Character* findById(int id);
    const Character* findById(int id) const;

    void setTeam(const std::vector<int>& teamIds);
    const std::vector<int>& getTeamIds() const;

    std::vector<Character*> getTeamMembers();
    std::vector<const Character*> getTeamMembers() const;

private:
    std::vector<Character> characters_;
    std::vector<int> teamIds_;
};
