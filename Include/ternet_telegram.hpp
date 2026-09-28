#pragma once

#include <map>
#include <stdexcept>
#include <string>

namespace ternet::telegram {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct ApiRequest {
    std::string method;
    std::map<std::string, std::string> parameters;
};

class BotApiClient {
public:
    explicit BotApiClient(std::string token);
    const std::string& token() const noexcept { return token_; }
    std::string endpoint(const std::string& method) const;
    ApiRequest request(const std::string& method) const;
private:
    std::string token_;
};

} // namespace ternet::telegram
