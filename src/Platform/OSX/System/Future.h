// Copyright (c) 2021-2022, The TuringX Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2016 The Cryptonote developers

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace System {

namespace Detail {

namespace { 

enum class State : unsigned {
  STARTED,
  COMPLETED,
  CONSUMED
};

}

// Simplest possible future implementation. The reason why this class even exist is because currenty std future has a
// memory corrupting bug on OSX. Spawn a new thread, execute procedure in it, get result, and wait thread to shut down.
// Actualy, this is what libstdc++'s std::future is doing.
template<class T> class Future {
public:
  // Create a new thread, and run `operation` in it.
  explicit Future(std::function<T()>&& operation) : procedure(std::move(operation)), state(State::STARTED), worker{[this] { asyncOp(); }} {
  }

  // Wait for async op to complete, then if thread wasn't detached, join it.
  ~Future() {
    wait();
    if (worker.joinable()) {
      worker.join();
    }
  }

  // Get result of async operation. UB if called more than once.
  T get() const {
    assert(state != State::CONSUMED);
    wait();
    state = State::CONSUMED;
    if (currentException != nullptr) {
      std::rethrow_exception(currentException);
    }

    return std::move(result);
  }

  // Wait for async operation to complete, if op is already completed, return immediately.
  void wait() const {
    std::unique_lock<std::mutex> guard(operationMutex);
    while (state == State::STARTED) {
      operationCondition.wait(guard);
    }
  }

  bool valid() const {
    std::unique_lock<std::mutex> guard(operationMutex);
    return state != State::CONSUMED;
  }

private:
  // This function is executed in a separate thread.
  void asyncOp() {
    try {
      assert(procedure != nullptr);
      result = procedure();
    } catch (...) {
      currentException = std::current_exception();
    }

    std::unique_lock<std::mutex> guard(operationMutex);
    state = State::COMPLETED;
    operationCondition.notify_one();
  }

  mutable T result;
  std::function<T()> procedure;
  std::exception_ptr currentException;
  mutable std::mutex operationMutex;
  mutable std::condition_variable operationCondition;
  mutable State state;
  std::thread worker;
};

template<> class Future<void> {
public:
  // Create a new thread, and run `operation` in it.
  explicit Future(std::function<void()>&& operation) : procedure(std::move(operation)), state(State::STARTED), worker{[this] { asyncOp(); }} {
  }

  // Wait for async op to complete, then if thread wasn't detached, join it.
  ~Future() {
    wait();
    if (worker.joinable()) {
      worker.join();
    }
  }

  // Get result of async operation. UB if called more than once.
  void get() const {
    assert(state != State::CONSUMED);
    wait();
    state = State::CONSUMED;
    if (currentException != nullptr) {
      std::rethrow_exception(currentException);
    }
  }

  // Wait for async operation to complete, if op is already completed, return immediately.
  void wait() const {
    std::unique_lock<std::mutex> guard(operationMutex);
    while (state == State::STARTED) {
      operationCondition.wait(guard);
    }
  }

  bool valid() const {
    std::unique_lock<std::mutex> guard(operationMutex);
    return state != State::CONSUMED;
  }

private:
  // This function is executed in a separate thread.
  void asyncOp() {
    try {
      assert(procedure != nullptr);
      procedure();
    } catch (...) {
      currentException = std::current_exception();
    }

    std::unique_lock<std::mutex> guard(operationMutex);
    state = State::COMPLETED;
    operationCondition.notify_one();
  }

  std::function<void()> procedure;
  std::exception_ptr currentException;
  mutable std::mutex operationMutex;
  mutable std::condition_variable operationCondition;
  mutable State state;
  std::thread worker;
};

template<class T> std::function<T()> async(std::function<T()>&& operation) {
  return operation;
}

}

}
