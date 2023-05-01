#include <iostream>
#include <unordered_map>
#include <coroutine>

std::unordered_map<std::string, int> myMap = {{"one", 1}, {"two", 2}, {"three", 3}};

struct coroutine_lookup {
    std::string key;
    std::coroutine_handle<> handle;
    int result;

    coroutine_lookup(std::string k, std::coroutine_handle<> h) : key(k), handle(h) {}

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        handle = awaiting;
    }
    int await_resume() noexcept {
        return result;
    }
};

coroutine_lookup lookup(std::string key) {
    auto it = myMap.find(key);
    if (it != myMap.end()) {
        co_return it->second;
    } else {
        co_return -1;
    }
}
˜
int main() {
    std::coroutine_handle<> mainHandle = std::coroutine_handle<>::from_address(nullptr);
    auto lookupHandle = lookup("one").handle;
    int result = 0;

    while (lookupHandle) {
        lookupHandle.resume();
        result = lookupHandle.promise().result;
        lookupHandle = lookupHandle.promise().handle;
    }

    std::cout << "Result: " << result << std::endl;

    return 0;
}
