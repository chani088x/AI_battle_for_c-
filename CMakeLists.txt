cmake_minimum_required(VERSION 3.16)
project(AIAsciiGachaRPG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(ai_ascii_gacha
    src/main.cpp
    src/Character.cpp
    src/Inventory.cpp
    src/Gacha.cpp
    src/AIArtManager.cpp
    src/Battle.cpp
    src/Persistence.cpp
    src/Utils.cpp
)

target_include_directories(ai_ascii_gacha PRIVATE src external)

find_package(Threads REQUIRED)
target_link_libraries(ai_ascii_gacha PRIVATE Threads::Threads)

option(USE_LIBCURL "Enable libcurl integration" ON)
if(USE_LIBCURL)
    find_package(CURL)
    if(CURL_FOUND)
        target_compile_definitions(ai_ascii_gacha PRIVATE USE_LIBCURL=1)
        target_link_libraries(ai_ascii_gacha PRIVATE CURL::libcurl)
    endif()
endif()
