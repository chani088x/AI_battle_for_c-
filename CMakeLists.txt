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
    src/Utils.cpp
)

target_include_directories(ai_ascii_gacha PRIVATE src external)

find_package(Threads REQUIRED)
target_link_libraries(ai_ascii_gacha PRIVATE Threads::Threads)

option(USE_LIBCURL "Enable libcurl integration" ON)
option(AI_GACHA_FETCH_CURL "Automatically fetch and build libcurl if it is missing" ON)
if(USE_LIBCURL)
    find_package(CURL QUIET)
    if(CURL_FOUND)
        target_compile_definitions(ai_ascii_gacha PRIVATE USE_LIBCURL=1)
        target_link_libraries(ai_ascii_gacha PRIVATE CURL::libcurl)
    elseif(AI_GACHA_FETCH_CURL)
        include(FetchContent)
        FetchContent_Declare(
            ai_gacha_libcurl
            URL https://curl.se/download/curl-8.7.1.tar.gz
            URL_HASH SHA256=5bdc541cf74c6cb0cb21e078ff857f440be3c6ba4b543f34816f21024ab2d5a3
        )
        FetchContent_GetProperties(ai_gacha_libcurl)
        if(NOT ai_gacha_libcurl_POPULATED)
            FetchContent_Populate(ai_gacha_libcurl)
            set(BUILD_CURL_EXE OFF CACHE BOOL "Disable curl command line build" FORCE)
            set(BUILD_TESTING OFF CACHE BOOL "Disable curl tests" FORCE)
            add_subdirectory(${ai_gacha_libcurl_SOURCE_DIR} ${ai_gacha_libcurl_BINARY_DIR})
        endif()
        target_compile_definitions(ai_ascii_gacha PRIVATE USE_LIBCURL=1)
        target_link_libraries(ai_ascii_gacha PRIVATE libcurl)
    else()
        message(WARNING "libcurl was not found and automatic fetching is disabled; AI image services will be unavailable.")
    endif()
endif()

option(USE_STB_IMAGE "Enable stb_image based PNG decoding" OFF)
if(USE_STB_IMAGE)
    target_compile_definitions(ai_ascii_gacha PRIVATE USE_STB_IMAGE=1)
endif()
