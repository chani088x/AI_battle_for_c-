#pragma once

#include <string>
#include <utility>
#include <vector>

struct HttpResponse {
    long status{0};
    std::string body;
};

// headers are provided as {"Header-Name", "value"}
// Returns true on successful request and fills response/status.
// On failure returns false and fills errorMessage for logging.
bool httpPost(const std::string& url,
    const std::string& payload,
    const std::vector<std::pair<std::string, std::string>>& headers,
    HttpResponse& response,
    std::string& errorMessage);

