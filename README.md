# AI ASCII Gacha RPG

C++17 콘솔 환경에서 동작하는 가챠 기반 RPG 예제입니다. AI 이미지 생성 파이프라인(HTTP 호출 → ASCII 변환)과 가챠/전투 시스템을 모두 콘솔에서 체험할 수 있도록 구현했습니다.

## 주요 특징

- **가챠 시스템**: `std::mt19937`와 `std::discrete_distribution`을 사용해 1~5성 캐릭터를 확률적으로 뽑습니다.
- **AI 아트 파이프라인**: 캐릭터를 뽑을 때 콘솔에서 입력한 프롬프트를 기반으로 Stability AI 또는 로컬 Automatic1111 WebUI를 호출하고(`std::async` 비동기 처리), 실패 시 내부 Placeholder ASCII 아트를 생성해 캐시합니다.
- **ASCII 캐싱**: `.cache/ascii/` 폴더에 캐릭터별 ASCII 아트를 저장하여 동일 캐릭터를 다시 뽑아도 즉시 로딩됩니다.
- **턴제 전투**: 매 라운드마다 공격/방어/후퇴 중 하나를 직접 선택해 전략적으로 전투를 진행할 수 있습니다.

## 빌드 및 실행

```bash
cmake -S . -B build
cmake --build build
./build/ai_ascii_gacha
```

> 💡 **Visual Studio (Windows)**
>
> Visual Studio 2022에서 폴더를 열면 `AI_GACHA_FETCH_CURL` 옵션이 기본값 `ON`으로 적용되어, 시스템에 libcurl이 설치되어 있지 않아도
> CMake가 자동으로 소스를 내려받아 빌드합니다. 별도의 vcpkg 설정 없이도 HTTP 기능을 사용할 수 있으며, 이미 설치된 libcurl을 쓰고
> 싶다면 CMake 설정에서 `AI_GACHA_FETCH_CURL=OFF`로 변경하면 됩니다.

## 환경 변수

### AI 서비스 선택

- `AI_GACHA_PROVIDER`: `stability`, `automatic1111`, `none` 중 하나를 지정합니다. 기본값은 키가 설정되어 있으면 Stability, 그렇지 않으면 로컬 Automatic1111 WebUI(`http://127.0.0.1:7860`)를 시도합니다.
- `A1111_API_HOST` (선택): Automatic1111 WebUI의 기본 URL. 미설정 시 `http://127.0.0.1:7860`을 사용합니다.
- `A1111_NEGATIVE_PROMPT` (선택): 모든 요청에 공통으로 붙일 네거티브 프롬프트 문자열.
- `A1111_API_AUTH` (선택): Automatic1111을 `--api-auth 사용자:비밀번호`로 실행한 경우 동일한 문자열을 지정하면 Basic 인증 헤더를 자동으로 붙입니다.
- `A1111_API_KEY` / `A1111_API_KEY_HEADER` (선택): 일부 포크(예: ReForge)에서 요구하는 API 키 헤더 값을 설정합니다. 헤더 이름을 생략하면 기본으로 `X-API-Key`를 사용합니다.

### Stability API 사용 시

- `STABILITY_API_KEY`: [Stability API](https://platform.stability.ai/)에서 발급한 개인 키. `Authorization: Bearer <키>` 헤더로 전송됩니다.
- `STABILITY_ENGINE_ID` (선택): 사용할 엔진 ID. 기본값은 `stable-diffusion-v1-6`입니다.
- `STABILITY_API_HOST` (선택): API 호스트. 기본값은 `https://api.stability.ai`입니다.

정상적인 PNG → ASCII 변환을 위해서는 `USE_STB_IMAGE` 옵션을 켜고(기본 OFF) `stb_image.h`를 포함한 상태로 빌드해야 합니다. 헤더가 없거나 디코딩이 실패하면 Placeholder ASCII 아트를 사용합니다. Automatic1111/ReForge의 실제 이미지를 ASCII로 확인하려면 다음과 같이 옵션을 켜 주세요.

```bash
cmake -S . -B build -DUSE_STB_IMAGE=ON
cmake --build build
```

## 캐시 위치

- ASCII 아트 캐시: `.cache/ascii/*.txt`

## 의존성

- C++17 이상 컴파일러
- CMake 3.16+
- (선택) libcurl – 기본적으로 CMake가 자동으로 내려받아 빌드하며, 시스템에 설치된 라이브러리를 사용하고 싶다면 `AI_GACHA_FETCH_CURL=OFF`
  후 `find_package(CURL)`이 성공하도록 환경을 구성하세요.

프로젝트에 포함된 `external/nlohmann/json.hpp`는 nlohmann/json 스타일과 호환되는 경량 구현체입니다.
