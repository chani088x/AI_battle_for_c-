#include "HttpClient.h"

#ifdef USE_LIBCURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

namespace {
#ifdef USE_LIBCURL
size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}
#endif

#ifdef _WIN32
std::wstring widen(const std::string& input) {
    if (input.empty()) {
        return {};
    }
    int required = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (required <= 0) {
        return {};
    }
    std::wstring output(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, output.data(), required);
    if (!output.empty() && output.back() == L'\0') {
        output.pop_back();
    }
    return output;
}
#endif
}

bool httpPost(const std::string& url,
    const std::string& payload,
    const std::vector<std::pair<std::string, std::string>>& headers,
    HttpResponse& response,
    std::string& errorMessage) {
#ifdef USE_LIBCURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        errorMessage = "curl_easy_init 실패";
        return false;
    }

    std::string responseBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    struct curl_slist* headerList = nullptr;
    for (const auto& header : headers) {
        std::string line = header.first + ": " + header.second;
        headerList = curl_slist_append(headerList, line.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        errorMessage = curl_easy_strerror(res);
        if (headerList) {
            curl_slist_free_all(headerList);
        }
        curl_easy_cleanup(curl);
        return false;
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    response.status = status;
    response.body = std::move(responseBuffer);

    if (headerList) {
        curl_slist_free_all(headerList);
    }
    curl_easy_cleanup(curl);
    return true;
#elif defined(_WIN32)
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(URL_COMPONENTS);
    components.dwHostNameLength = -1;
    components.dwUrlPathLength = -1;
    components.dwSchemeLength = -1;

    std::wstring wUrl = widen(url);
    if (wUrl.empty() || !WinHttpCrackUrl(wUrl.c_str(), 0, 0, &components)) {
        errorMessage = "URL 파싱 실패";
        return false;
    }

    bool secure = components.nScheme == INTERNET_SCHEME_HTTPS;
    INTERNET_PORT port = components.nPort;
    std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);

    HINTERNET session = WinHttpOpen(L"AIAsciiGacha/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session) {
        errorMessage = "WinHttpOpen 실패";
        return false;
    }

    HINTERNET connection = WinHttpConnect(session, host.c_str(), port, 0);
    if (!connection) {
        errorMessage = "WinHttpConnect 실패";
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connection, L"POST", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        errorMessage = "WinHttpOpenRequest 실패";
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    std::wstring headerBlock;
    for (const auto& header : headers) {
        headerBlock += widen(header.first + ": " + header.second + "\r\n");
    }

    BOOL sendResult = WinHttpSendRequest(request,
        headerBlock.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerBlock.c_str(),
        headerBlock.empty() ? 0 : static_cast<DWORD>(headerBlock.size()),
        (LPVOID)payload.data(),
        static_cast<DWORD>(payload.size()),
        static_cast<DWORD>(payload.size()),
        0);

    if (!sendResult) {
        errorMessage = "WinHttpSendRequest 실패";
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        errorMessage = "WinHttpReceiveResponse 실패";
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX)) {
        response.status = static_cast<long>(statusCode);
    }

    response.body.clear();
    DWORD bytesAvailable = 0;
    do {
        if (!WinHttpQueryDataAvailable(request, &bytesAvailable)) {
            break;
        }
        if (bytesAvailable == 0) {
            break;
        }
        std::string buffer(bytesAvailable, '\0');
        DWORD bytesRead = 0;
        if (!WinHttpReadData(request, buffer.data(), bytesAvailable, &bytesRead)) {
            break;
        }
        buffer.resize(bytesRead);
        response.body.append(buffer);
    } while (bytesAvailable > 0);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return true;
#else
    (void)url;
    (void)payload;
    (void)headers;
    (void)response;
    errorMessage = "HTTP 백엔드가 활성화되어 있지 않습니다.";
    return false;
#endif
}

