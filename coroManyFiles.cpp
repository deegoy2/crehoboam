#include <iostream>
#include <fstream>
#include <vector>
#include <coroutine>

// using namespace std::experimental;

struct file_reader {
    std::string filename;
    std::ifstream file;
    std::coroutine_handle<> handle;
    std::vector<char> buffer;

    file_reader(std::string fn, std::coroutine_handle<> h) : filename(fn), handle(h) {}

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        handle = awaiting;
        file.open(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open file " << filename << std::endl;
            handle.resume();
        }
    }
    std::vector<char> await_resume() noexcept {
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            buffer.resize(size);
            file.seekg(0, std::ios::beg);
            file.read(buffer.data(), size);
            file.close();
        }
        return buffer;
    }
};

struct async_file_reader {
    std::vector<std::string> filenames;
    std::vector<file_reader> readers;
    std::vector<std::coroutine_handle<>> handles;

    async_file_reader(std::vector<std::string> fns) : filenames(fns) {
        readers.reserve(filenames.size());
        handles.reserve(filenames.size());
        for (const auto& filename : filenames) {
            readers.emplace_back(filename, std::coroutine_handle<>{nullptr});
            handles.push_back(readers.back().handle);
        }
    }

    struct promise_type {
        async_file_reader reader;

        async_file_reader get_return_object() { return reader; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() { return {}; }
        void unhandled_exception() { std::terminate(); }
    };

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> awaiting) noexcept {
        for (auto& handle : handles) {
            handle.resume();
        }
        awaiting.resume();
    }
    std::vector<std::vector<char>> await_resume() noexcept {
        std::vector<std::vector<char>> results;
        results.reserve(readers.size());
        for (auto& reader : readers) {
            results.push_back(reader.handle.promise().buffer);
        }
        return results;
    }
};

async_file_reader read_files_async(std::vector<std::string> filenames) {
    co_return co_await async_file_reader{filenames};
}

int main() {
    std::vector<std::string> filenames = {"file1.txt", "file2.txt", "file3.txt"};
    auto async_reader = read_files_async(filenames);
    auto results = async_reader.handle.promise().await_resume();
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "Contents of " << filenames[i] << ":" << std::endl;
        std::cout.write(results[i].data(), results[i].size());
    }
    return 0;
}
: