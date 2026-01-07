cmake_minimum_required(VERSION 3.20)
project(cpp_playground VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# ============================================
# Dependencies
# ============================================

# Find Protobuf
find_package(Protobuf REQUIRED)

# Find Boost
find_package(Boost 1.74 REQUIRED COMPONENTS
    system
    thread
    chrono
    filesystem
    program_options
)

# Find Google Test (only if building tests)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Keep existing hello example
add_executable(hello hello.cpp)

# ============================================
# MMORPG Server Libraries
# ============================================

# Proto library (generated from .proto files)
add_library(mmorpg_proto STATIC
    src/proto/messages.pb.h
    src/proto/messages.pb.cc
)
target_include_directories(mmorpg_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mmorpg_proto PUBLIC protobuf::libprotobuf)

# Core library (types, events, utilities)
add_library(mmorpg_core STATIC
    src/core/Event.hpp
    src/core/EventBus.hpp
    src/core/EventBus.cpp
)
target_include_directories(mmorpg_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mmorpg_core PUBLIC
    Boost::system
    Boost::thread
    Boost::chrono
    Boost::filesystem
)

# Skills library
add_library(mmorpg_skills STATIC
    src/skills/SkillEffect.hpp
    src/skills/Skill.hpp
    src/skills/Skill.cpp
    src/skills/SkillTree.hpp
    src/skills/SkillTree.cpp
)
target_link_libraries(mmorpg_skills PUBLIC mmorpg_core)

# Actors library
add_library(mmorpg_actors STATIC
    src/actors/Stats.hpp
    src/actors/Actor.hpp
    src/actors/Actor.cpp
    src/actors/Character.hpp
    src/actors/Character.cpp
    src/actors/ActorManager.hpp
    src/actors/ActorManager.cpp
)
target_link_libraries(mmorpg_actors PUBLIC mmorpg_core mmorpg_skills)

# Combat library
add_library(mmorpg_combat STATIC
    src/combat/CombatAction.hpp
    src/combat/DamageCalculator.hpp
    src/combat/DamageCalculator.cpp
    src/combat/CombatSystem.hpp
    src/combat/CombatSystem.cpp
)
target_link_libraries(mmorpg_combat PUBLIC mmorpg_actors)

# Network library
add_library(mmorpg_network STATIC
    src/network/Socket.hpp
    src/network/Socket.cpp
    src/network/Connection.hpp
    src/network/Connection.cpp
    src/network/TcpServer.hpp
    src/network/TcpServer.cpp
)
target_link_libraries(mmorpg_network PUBLIC mmorpg_proto)

# Server library
add_library(mmorpg_server STATIC
    src/server/GameServer.hpp
    src/server/GameServer.cpp
)
target_link_libraries(mmorpg_server PUBLIC
    mmorpg_network
    mmorpg_combat
    mmorpg_actors
    mmorpg_skills
    Boost::program_options
)

# Client library (for test bot)
add_library(mmorpg_client STATIC
    src/client/TestBot.hpp
    src/client/TestBot.cpp
)
target_link_libraries(mmorpg_client PUBLIC mmorpg_network)

# ============================================
# Executables
# ============================================

# Main server
add_executable(mmorpg_server_exe src/main.cpp)
target_link_libraries(mmorpg_server_exe PRIVATE mmorpg_server)
set_target_properties(mmorpg_server_exe PROPERTIES OUTPUT_NAME "mmorpg_server")

# Test bot
add_executable(test_bot examples/bot_test.cpp)
target_link_libraries(test_bot PRIVATE mmorpg_client)

# ============================================
# Examples
# ============================================
if(BUILD_EXAMPLES)
    # Actor demo - test actors and stats
    add_executable(actor_demo examples/actor_demo.cpp)
    target_link_libraries(actor_demo PRIVATE mmorpg_actors)

    # Combat demo - test combat system
    add_executable(combat_demo examples/combat_demo.cpp)
    target_link_libraries(combat_demo PRIVATE mmorpg_combat)

    # Skill demo - test skill tree system
    add_executable(skill_demo examples/skill_demo.cpp)
    target_link_libraries(skill_demo PRIVATE mmorpg_actors)
endif()

# ============================================
# Unit Tests (Google Test)
# ============================================
if(BUILD_TESTS)
    # Stats tests
    add_executable(test_stats tests/test_stats.cpp)
    target_link_libraries(test_stats PRIVATE mmorpg_actors GTest::gtest GTest::gtest_main)
    add_test(NAME StatsTest COMMAND test_stats)

    # Actor tests
    add_executable(test_actor tests/test_actor.cpp)
    target_link_libraries(test_actor PRIVATE mmorpg_actors GTest::gtest GTest::gtest_main)
    add_test(NAME ActorTest COMMAND test_actor)

    # Combat tests
    add_executable(test_combat tests/test_combat.cpp)
    target_link_libraries(test_combat PRIVATE mmorpg_combat GTest::gtest GTest::gtest_main)
    add_test(NAME CombatTest COMMAND test_combat)

    # Skill tests
    add_executable(test_skills tests/test_skills.cpp)
    target_link_libraries(test_skills PRIVATE mmorpg_actors GTest::gtest GTest::gtest_main)
    add_test(NAME SkillsTest COMMAND test_skills)

    # EventBus tests
    add_executable(test_eventbus tests/test_eventbus.cpp)
    target_link_libraries(test_eventbus PRIVATE mmorpg_core GTest::gtest GTest::gtest_main)
    add_test(NAME EventBusTest COMMAND test_eventbus)

    # Network tests
    add_executable(test_network tests/test_network.cpp)
    target_link_libraries(test_network PRIVATE mmorpg_network GTest::gtest GTest::gtest_main)
    add_test(NAME NetworkTest COMMAND test_network)

    # Server tests
    add_executable(test_server tests/test_server.cpp)
    target_link_libraries(test_server PRIVATE mmorpg_server GTest::gtest GTest::gtest_main)
    add_test(NAME ServerTest COMMAND test_server)
endif()
