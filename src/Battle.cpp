#include "Battle.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <thread>

namespace {
enum class PlayerAction {
    Attack = 1,
    Defend = 2,
    Retreat = 3
};

PlayerAction promptPlayerAction() {
    while (true) {
        std::cout << "행동을 선택하세요 (1. 공격  2. 방어  3. 후퇴): ";
        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "숫자를 입력해주세요.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice >= static_cast<int>(PlayerAction::Attack) && choice <= static_cast<int>(PlayerAction::Retreat)) {
            return static_cast<PlayerAction>(choice);
        }
        std::cout << "올바른 행동을 선택해주세요.\n";
    }
}
} // 익명 네임스페이스 종료

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
    bool playerRetreated = false;
    using namespace std::chrono_literals;
    while (enemy.hp > 0) {
        bool anyAlive = std::any_of(team.begin(), team.end(), [](const Character* c) { return c && c->isAlive(); });
        if (!anyAlive) {
            break;
        }

        std::cout << "\n-- 라운드 " << round++ << " --\n";
        PlayerAction action = promptPlayerAction();

        bool defending = false;
        switch (action) {
            case PlayerAction::Attack:
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
                break;
            case PlayerAction::Defend:
                defending = true;
                std::cout << "팀이 방어 자세를 취했습니다! 적의 공격이 약화됩니다.\n";
                break;
            case PlayerAction::Retreat: {
                std::bernoulli_distribution retreatChance(0.6);
                if (retreatChance(rng_)) {
                    std::cout << "성공적으로 후퇴했습니다!\n";
                    playerRetreated = true;
                } else {
                    std::cout << "후퇴에 실패했습니다...\n";
                }
                break;
            }
        }

        if (enemy.hp <= 0 || playerRetreated) {
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
        int incomingDamage = enemy.attack;
        if (defending) {
            incomingDamage = std::max(1, incomingDamage / 2);
        }
        target->currentHP = std::max(0, target->currentHP - incomingDamage);
        std::cout << enemy.name << "의 반격! " << target->name << "에게 " << incomingDamage
                  << " 피해 -> 남은 HP: " << target->currentHP << "\n";
        std::this_thread::sleep_for(200ms);
    }

    if (playerRetreated) {
        std::cout << "\n전투에서 안전하게 빠져나왔습니다.\n";
    } else if (enemy.hp <= 0) {
        std::cout << "\n승리! 적을 물리쳤습니다.\n";
    } else {
        std::cout << "\n패배... 팀이 전멸했습니다.\n";
    }
}
