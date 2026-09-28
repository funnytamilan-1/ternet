#include "ternet_telegram.hpp"
#include <cassert>
#include <string>

int main() {
    ternet::telegram::BotApiClient bot("123456:TEST_TOKEN");
    assert(bot.endpoint("getMe") == "https://api.telegram.org/bot123456:TEST_TOKEN/getMe");
    assert(bot.request("getMe").method == "getMe");

    bool rejected = false;
    try {
        ternet::telegram::BotApiClient empty("");
    } catch (const ternet::telegram::Error&) {
        rejected = true;
    }
    assert(rejected);
    return 0;
}
