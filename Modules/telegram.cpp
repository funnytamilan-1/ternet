#include "ternet_telegram.hpp"
#include <utility>

namespace ternet::telegram {

BotApiClient::BotApiClient(std::string token) : token_(std::move(token)) {
    if (token_.empty()) throw Error("Telegram bot token must not be empty");
}

std::string BotApiClient::endpoint(const std::string& method) const {
    if (method.empty()) throw Error("Telegram API method must not be empty");
    return "https://api.telegram.org/bot" + token_ + "/" + method;
}

ApiRequest BotApiClient::request(const std::string& method) const {
    if (method.empty()) throw Error("Telegram API method must not be empty");
    return ApiRequest{method, {}};
}

} // namespace ternet::telegram
