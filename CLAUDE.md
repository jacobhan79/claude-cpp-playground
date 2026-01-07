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

---

## Coding Policies

### Boost Usage

사용하는 Boost 라이브러리:
- `boost::asio` - 비동기 네트워킹, io_context, socket
- `boost::endian` - 네트워크 바이트 오더 변환 (big_to_native, native_to_big)

사용하지 않는 것:
- `boost::thread` - std::thread 사용
- `boost::smart_ptr` - std::unique_ptr, std::shared_ptr 사용
- `boost::optional` - std::optional 사용

### C++ Coding Convention

**Naming Style** (Google C++ Style 기반):
- 클래스/구조체: `PascalCase` (예: `TestBot`, `CombatStats`)
- 함수/메서드: `camelCase` (예: `sendPacket`, `handlePacket`)
- 멤버 변수: `camelCase_` with trailing underscore (예: `socket_`, `actorId_`)
- 지역 변수: `camelCase` (예: `packetLen`, `targetId`)
- 상수/enum: `UPPER_SNAKE_CASE` 또는 `PascalCase` (예: `MSG_LOGIN_REQUEST`)
- 네임스페이스: `lowercase` (예: `mmorpg::client`)

**Braces**: K&R style (opening brace on same line)
```cpp
if (condition) {
    // code
} else {
    // code
}
```

### Function Length

- **최대 50줄** 권장, 100줄 초과 금지
- 50줄 초과 시 헬퍼 함수로 분리 고려
- 단일 책임 원칙: 한 함수는 한 가지 일만

### Switch-Case Guidelines

- **7개 이상** case 시 별도 핸들러 함수로 분리
- 각 case는 가능하면 5줄 이내
- 복잡한 로직은 `handleXxxMessage()` 형태로 추출

```cpp
// Good: 복잡한 case를 함수로 분리
case MSG_LOGIN_RESPONSE:
    handleLoginResponse(packet);
    break;

// Bad: case 내부에 긴 로직
case MSG_LOGIN_RESPONSE: {
    // 20줄의 코드...
}
```

### String Key Policy

**string을 key로 사용하는 경우:**
- 설정 파일에서 읽어온 동적 데이터
- 사용자 입력 기반 조회 (예: 캐릭터 이름)
- 디버깅/로깅 목적

**enum/int를 key로 사용하는 경우:**
- 메시지 타입, 스킬 ID 등 고정된 식별자
- 성능이 중요한 핫 패스
- 컴파일 타임에 알 수 있는 값

```cpp
// Good: 고정 ID는 정수 사용
std::unordered_map<uint32_t, Skill> skills_;

// Good: 동적 이름은 문자열 사용
std::unordered_map<std::string, ActorPtr> actorsByName_;
```

### Packet Payload Policy

**Protobuf 메시지 사용 (권장):**
- 구조화된 데이터 전송
- 타입 안전성 필요 시
- 버전 호환성 필요 시

**Raw string/bytes 사용하는 경우:**
- 단순 텍스트 메시지 (채팅)
- 바이너리 블롭 (파일 전송)
- 레거시 호환

```cpp
// Good: 구조화된 데이터는 protobuf
proto::LoginRequest req;
req.set_username(name);
req.set_password(password);

// Acceptable: 단순 채팅은 문자열
proto::Chat chat;
chat.set_message("Hello");  // 단순 문자열 필드
```

### Error Handling

- 네트워크 에러: 연결 종료 및 재연결 로직
- 파싱 에러: 로그 후 패킷 무시
- 치명적 에러: `std::cerr` 출력 후 graceful shutdown

### Memory Management

- RAII 패턴 사용
- `std::unique_ptr` 기본, 공유 필요 시만 `std::shared_ptr`
- raw pointer는 non-owning 참조로만 사용
