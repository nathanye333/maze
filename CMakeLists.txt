cmake_minimum_required(VERSION 3.16)
project(qwen-maze LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(httplib)

set(CORE_SOURCES
    src/Maze.cpp
    src/MazeGenerator.cpp
    src/Vision.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/Shrine.cpp
    src/Event.cpp
    src/EventSystem.cpp
    src/WorldState.cpp
    src/God.cpp
    src/MockGod.cpp
)

add_library(qwen_maze_core STATIC ${CORE_SOURCES})
target_include_directories(qwen_maze_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(qwen_maze_core PUBLIC nlohmann_json::nlohmann_json)

add_executable(qwen-maze src/main.cpp src/Game.cpp src/OllamaGod.cpp src/EnvFile.cpp)
target_link_libraries(qwen-maze PRIVATE qwen_maze_core raylib httplib::httplib)
if(WIN32)
    target_link_libraries(qwen-maze PRIVATE ws2_32)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/.env")
    set(QWEN_MAZE_ENV_FILE "${CMAKE_SOURCE_DIR}/.env")
else()
    set(QWEN_MAZE_ENV_FILE "${CMAKE_SOURCE_DIR}/.env.example")
endif()

add_custom_command(
    TARGET qwen-maze
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QWEN_MAZE_ENV_FILE}"
            "$<TARGET_FILE_DIR:qwen-maze>/.env"
    COMMENT "Copy .env next to qwen-maze"
)

add_executable(qwen-maze-tests
    tests/TestMain.cpp
    tests/MazeTests.cpp
    tests/EventTests.cpp
    tests/GodTests.cpp
)
target_include_directories(qwen-maze-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(qwen-maze-tests PRIVATE qwen_maze_core)

enable_testing()
add_test(NAME qwen-maze-tests COMMAND qwen-maze-tests)
