#pragma once

// Dual-licensed under MIT License and Boost Software License 1.0
// Copyright (c) 2024-2025 jinhua luo, luajit.io@gmail.com
// SPDX-License-Identifier: MIT OR BSL-1.0
//
// You may use this software under the terms of either the MIT License
// or the Boost Software License 1.0. See the LICENSE file for details.

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

struct resched_t {};
inline static constexpr resched_t resched{};

struct no_sched_t {};
inline static constexpr no_sched_t no_sched{};

struct co_t
{
    struct promise_type
    {
        std::queue<std::coroutine_handle<>> join_waiters;
        bool completed = false;
        std::exception_ptr exception = nullptr;

        co_t get_return_object()
        {
            return co_t{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept {
            completed = true;
            // Resume all waiting coroutines
            while (!join_waiters.empty()) {
                auto waiter = join_waiters.front();
                join_waiters.pop();
                if (waiter && !waiter.done()) {
                    scheduler_t::instance().schedule(waiter);
                }
            }
            return {};
        }
        void return_void() {}
        void unhandled_exception() {
            exception = std::current_exception();
        }
        // Explicit no-reschedule yield
        std::suspend_always yield_value(no_sched_t) noexcept { return {}; }

        // Explicit reschedule yield - this is the default behavior for co_yield
        std::suspend_always yield_value(resched_t) noexcept {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            scheduler_t::instance().schedule(handle);
            return {};
        }
    };

    struct join_awaiter
    {
        co_t* coro;

        join_awaiter(co_t* c) : coro(c) {}

        bool await_ready() const noexcept {
            return !coro->handle || coro->handle.done() || coro->handle.promise().completed;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept {
            if (coro->handle && !coro->handle.promise().completed) {
                coro->handle.promise().join_waiters.push(handle);
            }
        }

        void await_resume() {
            if (coro->handle && coro->handle.promise().exception) {
                std::rethrow_exception(coro->handle.promise().exception);
            }
        }
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

    // Join method to await coroutine completion
    join_awaiter join() { return join_awaiter(this); }

    // Utility method to check if coroutine is done
    bool is_done() const {
        return !handle || handle.done() || (handle && handle.promise().completed);
    }

    // get exception which is captured in unhandled_exception
    std::exception_ptr get_exception() const {
        return handle ? handle.promise().exception : nullptr;
    }
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

        void await_suspend(std::coroutine_handle<> handle) noexcept { ch->rq.push(handle); }

        data_t await_resume() noexcept
        {
            // For unbuffered channels, check handoff queue first
            if (ch->cap() == 0 && !ch->handoff_q.empty()) {
                auto data = ch->handoff_q.front();
                ch->handoff_q.pop();
                wake_up_writer();
                return data;
            }
            // For buffered channels, read from main queue
            if (!ch->dataq.empty()) {
                auto data = ch->dataq.front();
                ch->dataq.pop();

                // Direct handoff: if there are waiting writers, move their data to buffer
                if (!ch->handoff_q.empty()) {
                    auto writer_data = ch->handoff_q.front();
                    ch->handoff_q.pop();
                    ch->dataq.push(writer_data);
                    wake_up_writer();
                }

                return data;
            }
            // Channel is closed and empty
            return std::nullopt;
        }

    private:
        chan_t<T> *ch;

    private:
        void wake_up_writer() {
            if (!ch->wq.empty()) {
                auto writer = ch->wq.front();
                ch->wq.pop();
                if (writer && !writer.done()) {
                    scheduler_t::instance().schedule(writer);
                }
            }
        }
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
            // Put data in handoff queue for both unbuffered and buffered channels
            // For unbuffered: always use handoff
            // For buffered: use handoff when buffer is full (writer is suspending)
            ch->handoff_q.push(data_);

            // Common logic for both buffered and unbuffered channels
            is_waked_up_ = true;
            ch->wq.push(handle);
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
                    wake_up_reader();
                    return true;
                }
            } else {
                // Buffered channels
                if (is_waked_up_) {
                    // We were woken up by a reader
                    // Our data was already moved to buffer via direct handoff
                    // No need to check buffer size or add data again
                    return true;
                } else {
                    // Direct write without suspension (await_ready returned true)
                    ch->dataq.push(data_);
                    wake_up_reader();
                    return true;
                }
            }
        }

    private:
        chan_t<T> *ch;
        T data_;
        bool is_waked_up_{false};

    private:
        void wake_up_reader() {
            if (!ch->rq.empty()) {
                auto reader = ch->rq.front();
                ch->rq.pop();
                if (reader && !reader.done()) {
                    scheduler_t::instance().schedule(reader);
                }
            }
        }
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

class wg_guard_t {
    wg_t *wg;

public:
    wg_guard_t(wg_t *wg) : wg(wg) {}
    ~wg_guard_t() { wg->done(); }
};
} // namespace coco
