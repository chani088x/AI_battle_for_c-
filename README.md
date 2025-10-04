# AI ASCII Gacha RPG

C++17 콘솔 환경에서 동작하는 가챠 기반 RPG 예제입니다. AI 이미지 생성 파이프라인(HTTP 호출 → ASCII 변환)과 가챠/전투 시스템을 모두 콘솔에서 체험할 수 있도록 구현했습니다.

## 주요 특징

- **가챠 시스템**: `std::mt19937`와 `std::discrete_distribution`을 사용해 1~5성 캐릭터를 확률적으로 뽑습니다.
- **AI 아트 파이프라인**: 캐릭터를 뽑을 때 콘솔에서 입력한 프롬프트를 기반으로 Stability AI 텍스트-이미지 API를 호출하고(`std::async` 비동기 처리), 실패 시 내부 Placeholder ASCII 아트를 생성해 캐시합니다.
- **ASCII 캐싱**: `.cache/ascii/` 폴더에 캐릭터별 ASCII 아트를 저장하여 동일 캐릭터를 다시 뽑아도 즉시 로딩됩니다.
- **턴제 전투**: 매 라운드마다 공격/방어/후퇴 중 하나를 직접 선택해 전략적으로 전투를 진행할 수 있습니다.

## 빌드 및 실행

```bash
cmake -S . -B build
cmake --build build
./build/ai_ascii_gacha
```

## 환경 변수

Stability API와 연동하려면 다음 환경 변수를 설정하세요.

- `STABILITY_API_KEY`: [Stability API](https://platform.stability.ai/)에서 발급한 개인 키. `Authorization: Bearer <키>` 헤더로 전송됩니다.
- `STABILITY_ENGINE_ID` (선택): 사용할 엔진 ID. 기본값은 `stable-diffusion-v1-6`입니다.
- `STABILITY_API_HOST` (선택): API 호스트. 기본값은 `https://api.stability.ai`입니다.

정상적인 PNG → ASCII 변환을 위해서는 `USE_STB_IMAGE` 옵션을 켜고(기본 OFF) `stb_image.h`를 포함한 상태로 빌드해야 합니다. 헤더가 없거나 디코딩이 실패하면 Placeholder ASCII 아트를 사용합니다.

## 캐시 위치

- ASCII 아트 캐시: `.cache/ascii/*.txt`

## 의존성

- C++17 이상 컴파일러
- CMake 3.16+
- (선택) libcurl – API 연동 시 필요

프로젝트에 포함된 `external/nlohmann/json.hpp`는 nlohmann/json 스타일과 호환되는 경량 구현체입니다.
