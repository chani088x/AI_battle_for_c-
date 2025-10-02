# AI ASCII Gacha RPG

C++17 콘솔 환경에서 동작하는 가챠 기반 RPG 예제입니다. AI 이미지 생성 파이프라인(HTTP 호출 → ASCII 변환)과 가챠/전투/저장 기능을 모두 콘솔에서 체험할 수 있도록 구현했습니다.

## 주요 특징

- **가챠 시스템**: `std::mt19937`와 `std::discrete_distribution`을 사용해 1~5성 캐릭터를 확률적으로 뽑습니다.
- **AI 아트 파이프라인**: 캐릭터를 뽑을 때 비동기(`std::async`)로 AI 아트를 생성합니다. 환경 변수(`AI_GACHA_API_URL`, `AI_GACHA_API_KEY`)가 설정되어 있으면 libcurl을 통해 외부 API를 호출하고, 그렇지 않으면 내부 Placeholder ASCII 아트를 생성해 캐시합니다.
- **ASCII 캐싱**: `.cache/ascii/` 폴더에 캐릭터별 ASCII 아트를 저장하여 동일 캐릭터를 다시 뽑아도 즉시 로딩됩니다.
- **턴제 전투**: 팀으로 편성한 캐릭터가 적과 라운드별로 교대로 공격하는 간단한 전투 시스템을 제공합니다.
- **저장/불러오기**: `nlohmann::json` 호환 경량 구현체를 이용해 인벤토리와 팀 정보를 JSON으로 저장/불러오기 합니다.

## 빌드 및 실행

```bash
cmake -S . -B build
cmake --build build
./build/ai_ascii_gacha
```

## 환경 변수

외부 AI ASCII API와 연동하고 싶다면 다음 환경 변수를 설정하세요.

- `AI_GACHA_API_URL`: POST 요청을 보낼 엔드포인트(URL). 응답 JSON에 `ascii` 또는 `art` 필드로 ASCII 문자열을 반환한다고 가정합니다.
- `AI_GACHA_API_KEY`: 필요하다면 Bearer 토큰 방식으로 Authorization 헤더에 삽입할 키.

환경 변수가 없으면 프로젝트는 자동으로 Placeholder ASCII 아트를 생성합니다.

## 저장 위치

- 게임 데이터: `data/save.json`
- ASCII 아트 캐시: `.cache/ascii/*.txt`

## 의존성

- C++17 이상 컴파일러
- CMake 3.16+
- (선택) libcurl – API 연동 시 필요

프로젝트에 포함된 `external/nlohmann/json.hpp`는 nlohmann/json 스타일과 호환되는 경량 구현체입니다.
