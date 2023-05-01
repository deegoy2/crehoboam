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
  void await_suspend(std::coroutine_handle<> h) { *hp_ = h; }
  constexpr void await_resume() const noexcept {}
};

ReturnObject counter1(std::coroutine_handle<> *continuation_out)
{
  Awaiter a{continuation_out};
  for (unsigned i = 0;; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000-100*i));
    co_await a;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000-100*i));
    std::cout << "counter1: " << i << std::endl;
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

struct ReturnObject2 {
  struct promise_type {
    ReturnObject2 get_return_object() {
      return {
        // Uses C++20 designated initializer
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> h_;
  operator std::coroutine_handle<promise_type>() const { return h_; }
  // A coroutine_handle<promise_type> converts to coroutine_handle<>
  operator std::coroutine_handle<>() const { return h_; }
};

ReturnObject2 counter2()
{
  for (unsigned i = 0;; ++i) {
    co_await std::suspend_always{};
    std::cout << "counter2: " << i << std::endl;
  }
}

void main2()
{
  std::coroutine_handle<> h = counter2();
  for (int i = 0; i < 8; ++i) {
    //std::cout << "In main2 function\n";
    h();
  }
  h.destroy();
}

struct ReturnObject3 {
  struct promise_type {
    unsigned value_;

    ReturnObject3 get_return_object() {
      return ReturnObject3 {
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> h_;
  operator std::coroutine_handle<promise_type>() const { return h_; }
};

template<typename PromiseType> struct GetPromise {
  PromiseType *p_;
  bool await_ready() { return false; } // says yes call await_suspend
  bool await_suspend(std::coroutine_handle<PromiseType> h) {
    p_ = &h.promise();
    return false;         // says no don't suspend coroutine after all
  }
  PromiseType *await_resume() { return p_; }
};

ReturnObject3 counter3() {
  auto pp = co_await GetPromise<ReturnObject3::promise_type>{};

  for (unsigned i = 0;; ++i) {
    pp->value_ = i;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000-i*100));
    co_await std::suspend_always{};
  }
}

void main3()
{
  
  std::coroutine_handle<ReturnObject3::promise_type> h = counter3();
  ReturnObject3::promise_type &promise = h.promise();
  for (int i = 0; i < 10; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    h();
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "counter3: " << promise.value_  << " in "  << microseconds/1000 << std::endl;
  }
  h.destroy();
}

struct ReturnObject4 {
  struct promise_type {
    unsigned value_;

    ReturnObject4 get_return_object() {
      return {
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    std::suspend_always yield_value(unsigned value) {
      value_ = value;
      return {};
    }
  };

  std::coroutine_handle<promise_type> h_;
};

ReturnObject4 counter4()
{
  for (unsigned i = 0;; ++i)
    co_yield i;       // co yield i => co_await promise.yield_value(i)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void main4()
{
  auto h = counter4().h_;
  auto &promise = h.promise();
  for (int i = 0; i < 8; ++i) {
    std::cout << "counter4: " << promise.value_ << std::endl;
    h();
  }
  h.destroy();
}

struct ReturnObject5 {
  struct promise_type {
    unsigned value_;

    ~promise_type() { std::cout << "promise_type destroyed" << std::endl; }
    ReturnObject5 get_return_object() {
      return {
        .h_ = std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    std::suspend_always yield_value(unsigned value) {
      value_ = value;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000 -value*100));
      return {};
    }
    void return_void() {}
  };

  std::coroutine_handle<promise_type> h_;
};

ReturnObject5 counter5(){
  for (unsigned i = 0; i < 8; ++i)
    co_yield i;
  // falling off end of function or co_return; => promise.return_void();
  // (co_return value; => promise.return_value(value);)
}

void main5()
{
  auto h = counter5().h_;
  auto &promise = h.promise();
  while (!h.done()) { // Do NOT use while(h) (which checks h non-NULL)
    std::cout << "counter5: " << promise.value_ << std::endl;
    h();
  }
  h.destroy();
}


int main()
{
  main1();std::cout << std::endl;
  main2();std::cout << std::endl;
  main3();std::cout << std::endl;
  main4();std::cout << std::endl;
  main5();std::cout << std::endl;
}
