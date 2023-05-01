/*
g++ -fcoroutines -std=c++20 -Wall -Werror corodemo.cc
clang++ -std=c++20 -stdlib=libc++ -fcoroutines-ts -Wall -Werror corodemo.cc
*/

#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <chrono>
#include <thread>

struct ReturnObject {
  struct promise_type {
    ReturnObject get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };
};

struct Awaiter {
  std::coroutine_handle<> *hp_;
  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) { 
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    *hp_ = h; 
  }
  constexpr void await_resume() const noexcept {}
};

ReturnObject counter1(std::coroutine_handle<> *continuation_out)
{
  Awaiter a{continuation_out};
  for (unsigned i = 0;; i++) {
    co_await a;
    std::cout << "counter1: " << i << "\ta:"  << std::endl;
  }
}

void main1()
{
  std::coroutine_handle<> h;
  counter1(&h);
  for (int i = 0; i < 8; ++i) {
    //std::cout << "In main1 function\n";
    h();
  }
  h.destroy();
}

int main()
{
  main1();std::cout << std::endl;
}
