#pragma once

#include <random>
#include <string>
#include <vector>

#include "Character.h"

struct Enemy {
    std::string name;
    int hp{0};
    int attack{0};
};

class BattleSystem {
public:
    BattleSystem();

    void runBattle(std::vector<Character*> team);

private:
    Enemy createEnemy(const std::vector<Character*>& team) const;
    mutable std::mt19937 rng_;
};
