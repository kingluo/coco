#pragma once

// MIT License
//
// Copyright (c) 2024 jinhua luo, luajit.io@gmail.com
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

#include <functional>
#include <map>
#include <queue>
#include <string>

#define coco_begin()                                                \
    {                                                               \
        auto __tag = __LINE__;                                      \
        auto __it = _st->state.find(__tag);                         \
        auto __st = (__it == _st->state.end()) ? -1 : __it->second; \
        switch (__st) {                                             \
        case -1: {                                                  \
            (void)1;

#define coco_end()              \
    }                           \
    break;                      \
    default:                    \
        break;                  \
        }                       \
        ;                       \
        _st->state[__tag] = -1; \
        }

#define coco_done() return co_t::DONE;

#define coco_yield()              \
    _st->state[__tag] = __LINE__; \
    return co_t::YIELD;           \
    break;                        \
    }                             \
    case __LINE__: {              \
        (void)1;

#define COCO_TOKEN_(x, y) x##y
#define COCO_TOKEN(x, y) COCO_TOKEN_(x, y)

#define COCO_CHAN_OP_(op, ch, t, ok)                 \
    COCO_TOKEN(tag1, __LINE__) :                     \
    {                                                \
        auto ret = ch->op(__self, t);                \
        if (ret != co_t::YIELD) {                    \
            ok = (ret == co_t::TRUE) ? true : false; \
            goto COCO_TOKEN(tag2, __LINE__);         \
        }                                            \
    }                                                \
    coco_yield() goto COCO_TOKEN(tag1, __LINE__);    \
    COCO_TOKEN(tag2, __LINE__) : (void)1;

#define coco_write_chan(ch, t, ok) COCO_CHAN_OP_(put, ch, t, ok)

#define coco_read_chan(ch, t, ok) COCO_CHAN_OP_(get, ch, t, ok)

#define coco_wait(wg)                                  \
    COCO_TOKEN(tag1, __LINE__) : if (wg->wait(__self)) \
    {                                                  \
        goto COCO_TOKEN(tag2, __LINE__);               \
    }                                                  \
    coco_yield() goto COCO_TOKEN(tag1, __LINE__);      \
    COCO_TOKEN(tag2, __LINE__) : (void)1;

namespace coco
{
struct state_t {
    std::map<int, int> state;
    virtual ~state_t() {}
};

class co_t
{
public:
    enum ret_t {
        FALSE,
        TRUE,
        YIELD,
        DONE,
    };
    typedef std::function<ret_t(co_t *, state_t *)> fn_t;

private:
    state_t *st;
    fn_t fn;

public:
    ret_t ret = TRUE;
    co_t(fn_t const &fn, state_t *st = new state_t) : st(st), fn(fn) {}
    bool done() { return ret == DONE; }
    ret_t resume()
    {
        if (done())
            return DONE;
        ret = fn(this, st);
        return ret;
    }
    ~co_t() { delete st; }
};

inline co_t *go(co_t::fn_t const &fn, state_t *st = new state_t)
{
    auto co = new co_t(fn, st);
    co->resume();
    return co;
}

template <typename T> class chan_t
{
    typedef std::function<void(T)> close_fn_t;
    size_t cap = 1;
    std::queue<T> data;
    std::queue<co_t *> rq;
    std::queue<co_t *> wq;
    bool closed_ = false;

public:
    chan_t(int cap = 0) : cap(cap) {}

    bool ready() { return !data.empty(); }

    bool closed() { return closed_; }

    co_t::ret_t get(co_t *co, T &t)
    {
        if (!data.empty()) {
            t = data.front();
            data.pop();
            if (!wq.empty()) {
                auto waiter = wq.front();
                wq.pop();
                waiter->resume();
            }
            return co_t::TRUE;
        }

        if (closed())
            return co_t::FALSE;

        rq.push(co);
        return co_t::YIELD;
    }

    co_t::ret_t put(co_t *co, T t)
    {
        if (closed())
            return co_t::FALSE;

        data.push(t);

        if (!rq.empty()) {
            auto waiter = rq.front();
            rq.pop();
            waiter->resume();
            return co_t::TRUE;
        }

        if (data.size() > cap) {
            rq.push(co);
            return co_t::YIELD;
        }

        return co_t::TRUE;
    }

    void close(close_fn_t fn = nullptr)
    {
        closed_ = true;
        while (!rq.empty()) {
            auto w = rq.front();
            rq.pop();
            w->resume();
        }
        while (!wq.empty()) {
            auto w = wq.front();
            wq.pop();
            w->resume();
        }
        if (fn) {
            while (!data.empty()) {
                auto &v = data.front();
                fn(v);
            }
        }
    }
};

class wait_group_t
{
    co_t *waiter = nullptr;
    int cnt = 0;

public:
    void add(int delta) { cnt += delta; }
    void done()
    {
        cnt--;
        if (cnt == 0 && waiter) {
            auto co = waiter;
            waiter = nullptr;
            co->resume();
        }
    }
    bool wait(co_t *w)
    {
        if (cnt == 0)
            return true;
        waiter = w;
        return false;
    }
};
} // namespace coco
