# coco

coco is a simple stackless, single-threaded, and header-only C++11 coroutine library.

## Design

1. No depends on C++20 coroutine, C++11 is enough.
2. No compiler dependencies, simple macros, header-only.
3. coroutine like Lua, stackless async/await implementation just like Rust.
4. channel and waitgroup like Go.
5. single-threaded, no locks.
6. No performance overhead.

## Synopsis

channel and waitgroup:

```cpp
    auto sock_read = new co_t(
        [=](co_t *__self, state_t *_st) {
            auto st = dynamic_cast<sock_read_state_t *>(_st);
            bool ok = false;
            coco_begin();
            for (; st->cnt < 3; st->cnt++) {
                coco_begin();
                coco_write_chan(fs_write_ch, st->cnt, ok);
                (void)ok;
                coco_write_chan(kafka_produce_ch, st->cnt, ok);
                (void)ok;
                coco_end();
            }
            fs_write_ch->close();
            kafka_produce_ch->close();
            coco_wait(wg);
            coco_end();
            coco_done();
        },
        new sock_read_state_t);

    while (!sock_read->done())
        sock_read->resume();
```

webserver:

```cpp
    go([=](co_t* __self, state_t* _st) {
        auto st = dynamic_cast<iouring_state_t*>(_st);
        st->co = __self;
        while (true) {
            coco_begin();
            do_accept(server_socket, nullptr, nullptr, st);
            coco_yield();
            auto sk = st->res;
            go([=](co_t* __self, state_t* _st) {
                auto req = dynamic_cast<conn_state_t*>(_st);
                req->co = __self;
                coco_begin();

                read_request(req);
                coco_yield();

                handle_request(req);
                send_response(req);
                coco_yield();
                delete __self;

                coco_end();
                coco_done();
            }, new conn_state_t(sk));
            coco_end();
        }
        coco_done();
    }, new iouring_state_t);
```

## Coding restrictions

Since Coco is simple and has no compiler support, it inevitably has some coding limitations.

1. coroutine lambda functions should be reentrant, so the statements should be reentrant and produce the same flow after reentry.
2. yield, wait, or channel r/w should be placed in an async block at the first level.
3. variables declared cannot escape yield, so if you need some variables to survive from a yield, use the state or global variables.
4. exceptions must be caught inside the lambda function.
5. state subclasses must inherit the `state_t` base class and `dynamic_cast` it to the subclass when using it.
6. The state is managed by the coroutine, so the state will be deleted when the coroutine exits.
7. You must delete all created coroutines, channels, and waitgroups yourself.
