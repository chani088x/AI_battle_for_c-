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
    src/HttpClient.cpp
)

target_include_directories(ai_ascii_gacha PRIVATE src external)

find_package(Threads REQUIRED)
target_link_libraries(ai_ascii_gacha PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(ai_ascii_gacha PRIVATE winhttp)
endif()

option(USE_LIBCURL "Enable libcurl integration" ON)
option(AI_GACHA_FETCH_CURL "Automatically fetch and build libcurl if it is missing" ON)
if(USE_LIBCURL)
    find_package(CURL QUIET)
    if(CURL_FOUND)
        target_compile_definitions(ai_ascii_gacha PRIVATE USE_LIBCURL=1)
        target_link_libraries(ai_ascii_gacha PRIVATE CURL::libcurl)
    elseif(AI_GACHA_FETCH_CURL)
        set(_ai_gacha_curl_ready FALSE)
        set(_ai_gacha_curl_version "8.7.1")
        set(_ai_gacha_curl_archive "${CMAKE_BINARY_DIR}/curl-${_ai_gacha_curl_version}.tar.gz")
        set(_ai_gacha_curl_src_dir "${CMAKE_BINARY_DIR}/curl-${_ai_gacha_curl_version}")
        set(_ai_gacha_curl_build_dir "${CMAKE_BINARY_DIR}/curl-${_ai_gacha_curl_version}-build")

        if(EXISTS "${_ai_gacha_curl_src_dir}/CMakeLists.txt")
            set(_ai_gacha_curl_ready TRUE)
        else()
            file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}")
            message(STATUS "Attempting to download libcurl ${_ai_gacha_curl_version}...")
            file(DOWNLOAD
                "https://curl.se/download/curl-${_ai_gacha_curl_version}.tar.gz"
                "${_ai_gacha_curl_archive}"
                SHOW_PROGRESS
                STATUS _ai_gacha_curl_dl_status
                EXPECTED_HASH SHA256=5bdc541cf74c6cb0cb21e078ff857f440be3c6ba4b543f34816f21024ab2d5a3
            )
            list(LENGTH _ai_gacha_curl_dl_status _ai_gacha_curl_status_len)
            if(_ai_gacha_curl_status_len GREATER 0)
                list(GET _ai_gacha_curl_dl_status 0 _ai_gacha_curl_status_code)
            else()
                set(_ai_gacha_curl_status_code -1)
            endif()

            if(_ai_gacha_curl_status_code EQUAL 0)
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${_ai_gacha_curl_archive}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                    RESULT_VARIABLE _ai_gacha_curl_extract_result
                )
                if(_ai_gacha_curl_extract_result EQUAL 0)
                    set(_ai_gacha_curl_ready TRUE)
                else()
                    message(WARNING "Failed to extract libcurl archive (error ${_ai_gacha_curl_extract_result}). libcurl support will be disabled. Set AI_GACHA_FETCH_CURL=OFF to silence this message.")
                endif()
            else()
                list(GET _ai_gacha_curl_dl_status 1 _ai_gacha_curl_status_msg)
                message(WARNING "Failed to download libcurl automatically: ${_ai_gacha_curl_status_msg}. libcurl support will be disabled. Set AI_GACHA_FETCH_CURL=OFF to silence this message.")
            endif()
        endif()

        if(_ai_gacha_curl_ready)
            set(BUILD_CURL_EXE OFF CACHE BOOL "Disable curl command line build" FORCE)
            set(BUILD_TESTING OFF CACHE BOOL "Disable curl tests" FORCE)
            add_subdirectory("${_ai_gacha_curl_src_dir}" "${_ai_gacha_curl_build_dir}")
            target_compile_definitions(ai_ascii_gacha PRIVATE USE_LIBCURL=1)
            target_link_libraries(ai_ascii_gacha PRIVATE libcurl)
        else()
            message(STATUS "Proceeding without libcurl; only placeholder ASCII art will be used.")
        endif()
    else()
        message(WARNING "libcurl was not found and automatic fetching is disabled; AI image services will be unavailable.")
    endif()
endif()

option(USE_STB_IMAGE "Enable stb_image based PNG decoding" OFF)
if(USE_STB_IMAGE)
    target_compile_definitions(ai_ascii_gacha PRIVATE USE_STB_IMAGE=1)
endif()
