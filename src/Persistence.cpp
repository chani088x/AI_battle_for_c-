#include "Persistence.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

bool Persistence::save(const Inventory& inventory, const std::string& filePath) {
    nlohmann::json root = nlohmann::json::object();
    nlohmann::json characters = nlohmann::json::array();

    for (const auto& character : inventory.getCharacters()) {
        characters.push_back(character.toJson());
    }

    nlohmann::json team = nlohmann::json::array();
    for (int id : inventory.getTeamIds()) {
        team.push_back(id);
    }

    root["characters"] = characters;
    root["team"] = team;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "저장 파일을 열 수 없습니다: " << filePath << "\n";
        return false;
    }
    file << root.dump(2);
    return true;
}

bool Persistence::load(Inventory& inventory, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "저장 파일을 찾을 수 없습니다: " << filePath << "\n";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (content.empty()) {
        return false;
    }

    try {
        nlohmann::json root = nlohmann::json::parse(content);
        std::vector<Character>& characters = inventory.getCharacters();
        characters.clear();

        if (root.contains("characters")) {
            for (size_t i = 0; i < root["characters"].size(); ++i) {
                characters.push_back(Character::fromJson(root["characters"][i]));
            }
        }

        std::vector<int> teamIds;
        if (root.contains("team")) {
            for (size_t i = 0; i < root["team"].size(); ++i) {
                teamIds.push_back(root["team"][i].get<int>());
            }
        }
        inventory.setTeam(teamIds);

    } catch (const std::exception& ex) {
        std::cerr << "저장 데이터를 불러오는 중 오류: " << ex.what() << "\n";
        return false;
    }
    return true;
}
