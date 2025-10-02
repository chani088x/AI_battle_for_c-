#pragma once

#include <string>

#include "Inventory.h"

class Persistence {
public:
    static bool save(const Inventory& inventory, const std::string& filePath);
    static bool load(Inventory& inventory, const std::string& filePath);
};
