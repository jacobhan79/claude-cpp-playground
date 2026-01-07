# MMORPG Server Framework

C++ MMORPG 서버 프레임워크 - 모던 C++ 패턴 학습용 프로젝트

## Features

- **Actor/Character System** - 스탯, 레벨링, 캐릭터 관리
- **Combat System** - 데미지 계산, 버프/디버프
- **Skill Tree** - 3티어 9개 스킬, 업그레이드 시스템
- **TCP Networking** - Protocol Buffers 기반 통신
- **Test Bot** - 13개 테스트 시나리오 (네트워크, 전투, 게임플레이)

## Quick Start

### Prerequisites

- clang++ 18+ (C++17)
- CMake 3.28+
- Protobuf 3.21+
- Boost (asio, endian)

### Build

```bash
# Configure
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build
```

### Run

```bash
# Start server
./build/mmorpg_server 7777

# Run test bot (in another terminal)
./build/test_bot localhost 7777 single
```

## Project Structure

```
src/
├── core/       # Types, Event, EventBus
├── actors/     # Actor, Character, Stats
├── combat/     # DamageCalculator, CombatSystem
├── skills/     # Skill, SkillTree, SkillDatabase
├── network/    # Socket, Connection, TcpServer
├── server/     # GameServer
├── client/     # TestBot
└── proto/      # Generated protobuf files
```

## Test Bot Scenarios

```bash
# Basic
./build/test_bot localhost 7777 single       # Single bot
./build/test_bot localhost 7777 multi 5      # Multiple bots
./build/test_bot localhost 7777 stress 10 30 # Stress test

# Network
./build/test_bot localhost 7777 reconnect    # Reconnection test
./build/test_bot localhost 7777 concurrent   # Concurrent connections

# Combat & Gameplay
./build/test_bot localhost 7777 combat       # Combat simulation
./build/test_bot localhost 7777 dungeon      # Dungeon scenario
./build/test_bot localhost 7777 boss         # Boss raid
```

## Development

### IDE Setup

```bash
# Copy VSCode config templates
cp .vscode/settings.json.example .vscode/settings.json
cp .vscode/tasks.json.example .vscode/tasks.json
cp .vscode/launch.json.example .vscode/launch.json
cp .vscode/c_cpp_properties.json.example .vscode/c_cpp_properties.json

# Copy Claude Code config template
cp .claude/settings.local.json.example .claude/settings.local.json
```

### Coding Guidelines

See [CLAUDE.md](CLAUDE.md) for detailed coding conventions and architecture documentation.

## License

MIT
