#pragma once

// MIT License
//
// Copyright (c) 2024-2025 jinhua luo, luajit.io@gmail.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <climits>
#include <coroutine>
#include <functional>
#include <optional>
#include <queue>
#include <stdint.h>
#include <string>
#include <utility>
#include <variant>

namespace coco {
struct scheduler_t {
    std::queue<std::coroutine_handle<>> q;
    void schedule(std::coroutine_handle<> handle) {
        if (handle && !handle.done()) {
            q.push(handle);
        }
    }
    void run() {
        while (!q.empty()) {
            auto handle = q.front();
            q.pop();
            // Check if handle is still valid before resuming
            if (handle && !handle.done()) {
                handle.resume();
            }
        }
    }

    void clear() {
        while (!q.empty()) {
            q.pop();
        }
    }

    static scheduler_t &instance() {
        static thread_local scheduler_t scheduler;
        return scheduler;
    }
};

struct co_t
{
    struct promise_type
    {
        co_t get_return_object()
        {
            return co_t{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
        std::suspend_always yield_value(std::monostate) noexcept { return {}; }
    };
    std::coroutine_handle<promise_type> handle;

    co_t(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~co_t() { if (handle) handle.destroy(); }

    // Delete copy constructor and assignment
    co_t(const co_t&) = delete;
    co_t& operator=(const co_t&) = delete;

    // Move constructor and assignment
    co_t(co_t&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    co_t& operator=(co_t&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, {});
        }
        return *this;
    }

    void resume() { scheduler_t::instance().schedule(handle); }
};

inline co_t go(std::function<co_t()> fn) {
    auto co = fn();
    co.resume();
    return co;
}

template <typename T> class chan_t
{
    typedef std::optional<T> data_t;

    struct read_wait_t
    {
        read_wait_t(chan_t<T> *ch) : ch(ch) {}

        bool await_ready() const noexcept
        {
            // Ready if: has buffered data, has handoff data (unbuffered), or channel is closed
            return !ch->dataq.empty() || !ch->handoff_q.empty() || ch->closed();
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            is_waked_up_ = true;
            ch->rq.push(handle);
        }

        data_t await_resume() noexcept
        {
            // For unbuffered channels, check handoff queue first
            if (ch->cap() == 0 && !ch->handoff_q.empty()) {
                auto data = ch->handoff_q.front();
                ch->handoff_q.pop();
                // Wake up the waiting writer
                if (!ch->wq.empty()) {
                    auto writer = ch->wq.front();
                    ch->wq.pop();
                    if (writer && !writer.done()) {
                        scheduler_t::instance().schedule(writer);
                    }
                }
                return data;
            }
            // For buffered channels, read from main queue
            if (!ch->dataq.empty()) {
                auto data = ch->dataq.front();
                ch->dataq.pop();
                // Wake up a waiting writer if buffer has space now
                if (!ch->wq.empty()) {
                    auto writer = ch->wq.front();
                    ch->wq.pop();
                    if (writer && !writer.done()) {
                        scheduler_t::instance().schedule(writer);
                    }
                }
                return data;
            }
            // Channel is closed and empty
            return std::nullopt;
        }

    private:
        chan_t<T> *ch;
        bool is_waked_up_{false};
    };

    struct write_wait_t
    {
        write_wait_t(chan_t<T> *ch, T data) : ch(ch), data_(data) {}

        bool await_ready() const noexcept
        {
            if (ch->closed()) return true; // Always ready if closed (will return false)

            if (ch->cap() == 0) {
                // Unbuffered: ready if there's a waiting reader
                return !ch->rq.empty();
            } else {
                // Buffered: ready if buffer has space
                return ch->dataq.size() < ch->cap();
            }
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            if (ch->cap() == 0) {
                // For unbuffered channels, put data in handoff queue
                ch->handoff_q.push(data_);
                is_waked_up_ = true;
                ch->wq.push(handle);
            } else {
                // For buffered channels, buffer must be full if we reach here
                // (because await_ready would have returned true if space was available)
                is_waked_up_ = true;
                ch->wq.push(handle);
            }
        }

        bool await_resume() noexcept
        {
            if (ch->closed())
                return false;

            if (ch->cap() == 0) {
                // Unbuffered channels
                if (is_waked_up_) {
                    // We were woken up by a reader, data should have been consumed
                    return true;
                } else {
                    // Direct write without suspension (await_ready returned true)
                    ch->handoff_q.push(data_);
                    if (!ch->rq.empty()) {
                        auto reader = ch->rq.front();
                        ch->rq.pop();
                        if (reader && !reader.done()) {
                            scheduler_t::instance().schedule(reader);
                        }
                    }
                    return true;
                }
            } else {
                // Buffered channels
                if (is_waked_up_) {
                    // We were woken up by a reader, try to add to buffer
                    if (ch->dataq.size() < ch->cap()) {
                        ch->dataq.push(data_);
                        // Wake up a waiting reader if any
                        if (!ch->rq.empty()) {
                            auto reader = ch->rq.front();
                            ch->rq.pop();
                            if (reader && !reader.done()) {
                                scheduler_t::instance().schedule(reader);
                            }
                        }
                        return true;
                    }
                    // Buffer is still full, this shouldn't happen
                    return false;
                } else {
                    // Direct write without suspension (await_ready returned true)
                    ch->dataq.push(data_);
                    // Wake up a waiting reader if any
                    if (!ch->rq.empty()) {
                        auto reader = ch->rq.front();
                        ch->rq.pop();
                        if (reader && !reader.done()) {
                            scheduler_t::instance().schedule(reader);
                        }
                    }
                    return true;
                }
            }
        }

    private:
        chan_t<T> *ch;
        T data_;
        bool is_waked_up_{false};
    };

    size_t cap_{};
    std::queue<T> dataq;
    std::queue<T> handoff_q;
    bool closed_{};
    std::queue<std::coroutine_handle<>> rq;
    std::queue<std::coroutine_handle<>> wq;

private:
    chan_t(chan_t &&other) = delete;
    chan_t(const chan_t &other) = delete;
    chan_t &operator=(chan_t &&other) = delete;
    chan_t &operator=(const chan_t &other) = delete;

public:
    chan_t(int cap = 0) : cap_(cap) {}

    size_t size() { return dataq.size(); }

    size_t cap() { return cap_; }

    bool ready() { return !dataq.empty(); }

    bool closed() { return closed_; }

    read_wait_t read()
    {
        return read_wait_t(this);
    }

    write_wait_t write(T data)
    {
        return write_wait_t(this, data);
    }

    void close()
    {
        closed_ = true;

        while (!rq.empty()) {
            auto reader = std::move(rq.front());
            rq.pop();
            if (reader && !reader.done()) {
                scheduler_t::instance().schedule(reader);
            }
        }
        while (!wq.empty()) {
            auto writer = std::move(wq.front());
            wq.pop();
            if (writer && !writer.done()) {
                scheduler_t::instance().schedule(writer);
            }
        }


    }
};

class wg_t
{
    struct async_wait_t
    {
        async_wait_t(wg_t *wg) : wg(wg) {}
        bool await_ready() const noexcept { return wg->cnt == 0; }
        void await_suspend(std::coroutine_handle<> handle) noexcept { wg->waiters.push(handle); }
        void await_resume() noexcept {}

    private:
        wg_t *wg;
    };

    std::queue<std::coroutine_handle<>> waiters;
    uint64_t cnt{};

public:
    void add(uint64_t delta = 1) {
        cnt += delta;
    }

    void done()
    {
        if (cnt > 0) {
            cnt--;
        }
        if (cnt == 0)
        {
            // Resume all waiting coroutines
            while (!waiters.empty()) {
                auto waiter = std::move(waiters.front());
                waiters.pop();
                if (waiter && !waiter.done()) {
                    scheduler_t::instance().schedule(waiter);
                }
            }
        }
    }

    async_wait_t wait() { return async_wait_t(this); }
};

class wg_guard_t
{
    wg_t *wg;

public:
    wg_guard_t(wg_t *wg) : wg(wg) { wg->add(); }
    ~wg_guard_t() { wg->done(); }
};
} // namespace coco
