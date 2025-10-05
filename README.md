# AI ASCII Gacha RPG

C++17 콘솔 환경에서 동작하는 가챠 기반 RPG 예제입니다. AI 이미지 생성 파이프라인(HTTP 호출 → ASCII 변환)과 가챠/전투 시스템을 모두 콘솔에서 체험할 수 있도록 구현했습니다.

## 주요 특징

- **가챠 시스템**: `std::mt19937`와 `std::discrete_distribution`을 사용해 1~5성 캐릭터를 확률적으로 뽑습니다.
- **AI 아트 파이프라인**: 캐릭터를 뽑을 때 콘솔에서 입력한 프롬프트를 기반으로 Stability AI 또는 로컬 Automatic1111 WebUI를 호출하고(`std::async` 비동기 처리), 실패 시 내부 Placeholder ASCII 아트를 생성해 캐시합니다.
- **ASCII 캐싱**: `.cache/ascii/` 폴더에 캐릭터별 ASCII 아트를 저장하여 동일 캐릭터를 다시 뽑아도 즉시 로딩됩니다.
- **이미지 캐싱**: Automatic1111에서 내려받은 PNG 이미지를 `.cache/images/`에 저장하고, ASCII 캐시가 비어 있을 때도 PNG만으로 다시 변환할 수 있습니다.
- **턴제 전투**: 매 라운드마다 공격/방어/후퇴 중 하나를 직접 선택해 전략적으로 전투를 진행할 수 있습니다.

## 빌드 및 실행

```bash
cmake -S . -B build
cmake --build build
./build/ai_ascii_gacha
```

> 💡 **Visual Studio (Windows)**
>
> 기본 CMake 설정에서는 Windows에서 `USE_LIBCURL=OFF`로 동작하며, 내부 WinHTTP 클라이언트를 사용해 자동으로 HTTP 요청을 처리합니다.
> 따라서 libcurl이 설치되어 있지 않아도 오류 없이 실행되며, Automatic1111 WebUI와 HTTPS 기반 Stability API 모두 정상적으로 호출됩니다.
> 만약 명시적으로 libcurl을 사용하고 싶다면 구성 단계에서 `-DUSE_LIBCURL=ON`을 전달하고, 필요하다면 `-DAI_GACHA_FETCH_CURL=ON`으로 소스
> 다운로드를 허용하세요.

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

> 🔁 **Automatic1111에서 Base64 출력이 비활성화된 경우**
>
> 일부 포크(예: ReForge)나 사용자 설정에서 API 응답의 `images` 항목이 Base64 문자열 대신 이미지 파일 경로만 제공될 수 있습니다. 이제 응답이 Base64이든 파일 경로이든 상관없이 PNG 데이터를 `.cache/images/`에 복사한 뒤 ASCII로 변환합니다. PNG만 남아 있는 상황에서도 동일 폴더의 파일을 읽어 ASCII 캐시를 재생성하므로, WebUI가 이미지를 저장하는 디렉터리에 대한 읽기 권한만 확보하면 됩니다.

## 캐시 위치

- ASCII 아트 캐시: `.cache/ascii/*.txt`
- PNG 이미지 캐시: `.cache/images/*.png`

## 의존성

- C++17 이상 컴파일러
- CMake 3.16+
- (선택) libcurl – Linux/macOS 등에서 HTTP 클라이언트로 사용합니다. Windows에서는 기본적으로 WinHTTP가 활성화되어 별도 설치가 필요 없
  습니다. libcurl을 명시적으로 사용하려면 `cmake -DUSE_LIBCURL=ON`으로 재구성하고 `find_package(CURL)`이 성공하도록 환경을 준비하거나
  `AI_GACHA_FETCH_CURL=ON`으로 소스 다운로드를 허용하면 됩니다.

프로젝트에 포함된 `external/nlohmann/json.hpp`는 nlohmann/json 스타일과 호환되는 경량 구현체입니다.
