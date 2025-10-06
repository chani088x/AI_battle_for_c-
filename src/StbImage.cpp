#ifdef USE_STB_IMAGE
// stb_image 구현체를 한 번만 포함하기 위한 전용 소스 파일입니다.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#endif // USE_STB_IMAGE
