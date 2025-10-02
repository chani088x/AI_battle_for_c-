#include "Battle.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

BattleSystem::BattleSystem()
    : rng_(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count())) {}

Enemy BattleSystem::createEnemy(const std::vector<Character*>& team) const {
    double avgRarity = 1.0;
    if (!team.empty()) {
        double sum = 0.0;
        for (const Character* c : team) {
            sum += c->rarity;
        }
        avgRarity = sum / static_cast<double>(team.size());
    }
    Enemy enemy;
    enemy.name = "Void Wraith";
    enemy.hp = static_cast<int>(200 + avgRarity * 60 + team.size() * 30);
    enemy.attack = static_cast<int>(35 + avgRarity * 8);
    return enemy;
}

void BattleSystem::runBattle(std::vector<Character*> team) {
    if (team.empty()) {
        std::cout << "팀에 편성된 캐릭터가 없습니다.\n";
        return;
    }

    for (Character* member : team) {
        if (member) {
            member->resetHP();
        }
    }

    Enemy enemy = createEnemy(team);
    std::cout << "\n적이 등장했다! " << enemy.name << " (HP: " << enemy.hp << ", ATK: " << enemy.attack << ")\n";

    int round = 1;
    using namespace std::chrono_literals;
    while (enemy.hp > 0) {
        bool anyAlive = std::any_of(team.begin(), team.end(), [](const Character* c) { return c && c->isAlive(); });
        if (!anyAlive) {
            break;
        }

        std::cout << "\n-- 라운드 " << round++ << " --\n";
        for (Character* member : team) {
            if (!member || !member->isAlive()) {
                continue;
            }
            int damage = member->attack;
            enemy.hp = std::max(0, enemy.hp - damage);
            std::cout << member->name << "의 공격! (" << damage << " 피해) -> 적 HP: " << enemy.hp << "\n";
            std::this_thread::sleep_for(200ms);
            if (enemy.hp <= 0) {
                break;
            }
        }
        if (enemy.hp <= 0) {
            break;
        }

        std::vector<Character*> aliveMembers;
        for (Character* member : team) {
            if (member && member->isAlive()) {
                aliveMembers.push_back(member);
            }
        }
        if (aliveMembers.empty()) {
            break;
        }

        std::uniform_int_distribution<size_t> dist(0, aliveMembers.size() - 1);
        Character* target = aliveMembers[dist(rng_)];
        target->currentHP = std::max(0, target->currentHP - enemy.attack);
        std::cout << enemy.name << "의 반격! " << target->name << "에게 " << enemy.attack << " 피해 -> 남은 HP: " << target->currentHP << "\n";
        std::this_thread::sleep_for(200ms);
    }

    if (enemy.hp <= 0) {
        std::cout << "\n승리! 적을 물리쳤습니다.\n";
    } else {
        std::cout << "\n패배... 팀이 전멸했습니다.\n";
    }
}
