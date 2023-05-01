#include <iostream>
#include <fstream>
#include <coroutine>
#include <future>

using namespace std;
// using namespace std::experimental;

struct FileReader {
    const char* filename;
    ifstream file;

    FileReader(const char* fn) : filename(fn), file(filename, ios::binary) {}

    struct promise_type {
        char* buffer;

        FileReader get_return_object() {
            return FileReader(nullptr);
        }

        auto initial_suspend() {
            return suspend_never{};
        }

        auto final_suspend() {
            return suspend_always{};
        }

        void unhandled_exception() {
            terminate();
        }

        auto yield_value(char* buf) {
            buffer = buf;
            return suspend_always{};
        }

        char* value() {
            return buffer;
        }
    };

    FileReader(FileReader&& other) : filename(other.filename), file(move(other.file)) {}

    bool await_ready() {
        return file.good();
    }

    void await_suspend(coroutine_handle<promise_type> handle) {
        char* buffer = new char[1024];
        if (file.read(buffer, 1024)) {
            handle.promise().yield_value(buffer);
        } else {
            handle.promise().yield_value(nullptr);
        }
    }

    char* await_resume() {
        return nullptr;
    }
};

int main() {
    FileReader reader("filename.ext"); // replace filename.ext with the name of your file

    auto coroutine = [&]() -> coroutine_handle<> {
        while (true) {
            char* data = co_await reader;
            if (!data) break;
            // do something with the data
            delete[] data;
        }
        co_return {};
    };

    coroutine();

    return 0;
}
