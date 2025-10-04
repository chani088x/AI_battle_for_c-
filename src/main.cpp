#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "AIArtManager.h"
#include "Battle.h"
#include "Gacha.h"
#include "Inventory.h"

namespace {

void printMenu() {
    std::cout << "\n===== AI ASCII 가챠 RPG =====\n";
    std::cout << "1. 가챠 뽑기\n";
    std::cout << "2. 인벤토리 보기\n";
    std::cout << "3. 팀 편성\n";
    std::cout << "4. 전투 시작\n";
    std::cout << "0. 종료\n";
    std::cout << "선택: ";
}

void showCharacter(const Character& character) {
    std::cout << "ID: " << character.id << " | " << rarityToString(character.rarity)
              << " | HP: " << character.maxHP << " | ATK: " << character.attack
              << " | 스킬: " << character.skill << "\n";
}

void showInventory(const Inventory& inventory) {
    const auto& characters = inventory.getCharacters();
    if (characters.empty()) {
        std::cout << "인벤토리가 비어 있습니다.\n";
        return;
    }
    std::cout << "\n--- 인벤토리 ---\n";
    for (const auto& character : characters) {
        showCharacter(character);
    }

    std::cout << "\n--- 팀 ---\n";
    if (inventory.getTeamIds().empty()) {
        std::cout << "편성된 팀이 없습니다.\n";
    } else {
        for (int id : inventory.getTeamIds()) {
            const Character* c = inventory.findById(id);
            if (c) {
                std::cout << "* ";
                showCharacter(*c);
            }
        }
    }
}

std::vector<int> parseIds(const std::string& input) {
    std::vector<int> ids;
    std::istringstream iss(input);
    int id = 0;
    while (iss >> id) {
        ids.push_back(id);
    }
    return ids;
}

void configureTeam(Inventory& inventory) {
    const auto& characters = inventory.getCharacters();
    if (characters.empty()) {
        std::cout << "먼저 캐릭터를 뽑아주세요.\n";
        return;
    }

    std::cout << "편성할 캐릭터의 ID를 공백으로 입력하세요 (최대 3명): ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
        std::getline(std::cin, line);
    }
    std::vector<int> ids = parseIds(line);
    if (ids.size() > 3) {
        ids.resize(3);
    }

    std::vector<int> filtered;
    for (int id : ids) {
        if (inventory.findById(id)) {
            filtered.push_back(id);
        } else {
            std::cout << "ID " << id << " 캐릭터는 존재하지 않습니다.\n";
        }
    }
    inventory.setTeam(filtered);
    std::cout << "팀 편성이 완료되었습니다.\n";
}

void startBattle(Inventory& inventory, BattleSystem& battleSystem) {
    std::vector<Character*> team = inventory.getTeamMembers();
    if (team.empty()) {
        std::cout << "팀이 비어 있습니다. 팀 편성 후 시도해주세요.\n";
        return;
    }
    battleSystem.runBattle(team);
}

} // 익명 네임스페이스 종료

int main() {
    GachaSystem gacha;
    Inventory inventory;
    AIArtManager artManager;
    BattleSystem battleSystem;

    bool running = true;
    while (running) {
        printMenu();
        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                std::cout << "이미지 생성을 위한 프롬프트를 입력하세요:\n> ";
                std::string prompt;
                std::getline(std::cin, prompt);
                Character character = gacha.rollCharacter(artManager, prompt);
                inventory.addCharacter(character);
                std::cout << "\n새로운 영웅 등장!\n";
                showCharacter(character);
                std::cout << "\n" << character.asciiArt << "\n";
                break;
            }
            case 2:
                showInventory(inventory);
                break;
            case 3:
                configureTeam(inventory);
                break;
            case 4:
                startBattle(inventory, battleSystem);
                break;
            case 0:
                running = false;
                break;
            default:
                std::cout << "올바른 메뉴를 선택하세요.\n";
                break;
        }
    }

    std::cout << "게임을 종료합니다.\n";
    return 0;
}
