# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Purpose

C++ MMORPG server framework for learning modern C++ patterns. Includes:
- Actor/Character system with stats
- Combat system with damage calculation
- Skill tree with upgradeable skills
- TCP networking with Protocol Buffers
- Test bot for server testing

## Build Commands

```bash
# Configure
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug

# Build all
cmake --build build

# Regenerate proto (if messages.proto changes)
protoc --cpp_out=src/proto --proto_path=proto proto/messages.proto
```

## Run Commands

```bash
# Start server (default port 7777)
./build/mmorpg_server [port]

# Run test bot
./build/test_bot localhost 7777 single       # Single bot test
./build/test_bot localhost 7777 multi 5      # 5 bots test
./build/test_bot localhost 7777 stress 10 30 # 10 bots, 30 sec stress test

# Run demos
./build/actor_demo
./build/combat_demo
./build/skill_demo
```

## Project Structure

```
src/
├── core/       # Types, Event, EventBus
├── actors/     # Actor, Character, ActorManager, Stats
├── combat/     # DamageCalculator, CombatSystem
├── skills/     # Skill, SkillTree, SkillDatabase, SkillEffect
├── network/    # Socket, Connection, TcpServer
├── server/     # GameServer
├── client/     # TestBot
├── proto/      # Generated protobuf (messages.pb.h/cc)
└── main.cpp    # Server entry point
proto/          # Protocol definitions (messages.proto)
examples/       # Demo programs + bot_test.cpp
```

## Libraries

- `mmorpg_proto` - Protobuf messages
- `mmorpg_core` - Events, types
- `mmorpg_skills` - Skill system
- `mmorpg_actors` - Actor/Character classes
- `mmorpg_combat` - Combat system
- `mmorpg_network` - TCP socket, connection, server
- `mmorpg_server` - Game server integration
- `mmorpg_client` - Test bot client

## Key Design Patterns

- **EventBus**: Publish/subscribe for decoupled events
- **Strategy**: Swappable DamageFormula
- **Builder**: Fluent Skill construction
- **RAII**: Socket wrapper
- **std::variant**: Type-safe SkillEffect, GameEvent, CombatAction

## Network Protocol

- TCP only, single-threaded poll-based
- Packet format: `[4 bytes length][protobuf data]`
- Message types defined in `proto/messages.proto`

## Environment

- Compiler: clang++ 18.1.3 (C++17)
- Build system: CMake 3.28
- Protobuf: 3.21.12
- Debugger: gdb 15.0
