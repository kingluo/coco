#include "coco.h"

using namespace coco;
struct sock_read_state_t : public state_t {
    int cnt = 0;
};

struct fs_write_state_t : public state_t {
    int i = 0;
};

struct kafka_produce_state_t : public state_t {
    int i = 0;
};

int main() {
    auto fs_write_ch = new chan_t<int>(3);
    auto kafka_produce_ch = new chan_t<int>;
    auto wg = new wait_group_t;

    wg->add(1);
    auto fs_write = go([=](co_t* __self, state_t* _st) {
        auto st = dynamic_cast<fs_write_state_t*>(_st);
        while (true) {
            COCO_ASYNC_BEGIN(loop);
            COCO_READ_CHAN(fs_write_ch, st->i);
            if (!fs_write_ch->closed()) {
                printf("---> FS WRITE, i=%d\n", st->i);
            } else {
                printf("---> fs_write_ch is closed.\n");
                goto out;
            }
            COCO_ASYNC_END();
        }
out:
        wg->done();
        COCO_DONE();
    }, new fs_write_state_t);

    wg->add(1);
    auto kafka_produce = go([=](co_t* __self, state_t* _st) {
        auto st = dynamic_cast<kafka_produce_state_t*>(_st);
        while (true) {
            COCO_ASYNC_BEGIN(loop);
            COCO_READ_CHAN(kafka_produce_ch, st->i);
            if (!kafka_produce_ch->closed()) {
                printf("---> KAFKA produce message, i=%d\n", st->i);
            }
            else {
                printf("---> kafka_produce_ch is closed.\n");
                goto out;
            }
            COCO_ASYNC_END();
        }
out:
        wg->done();
        COCO_DONE();
    }, new kafka_produce_state_t);

    auto sock_read = new co_t([=](co_t* __self, state_t* _st) {
        auto st = dynamic_cast<sock_read_state_t*>(_st);
        COCO_ASYNC_BEGIN(sock_read);
        for (; st->cnt < 3; st->cnt++) {
            COCO_ASYNC_BEGIN(loop);
            COCO_WRITE_CHAN(fs_write_ch, st->cnt);
            COCO_WRITE_CHAN(kafka_produce_ch, st->cnt);
            COCO_ASYNC_END();
        }
        fs_write_ch->close();
        kafka_produce_ch->close();
        COCO_WAIT(wg);
        COCO_ASYNC_END();
        COCO_DONE();
    }, new sock_read_state_t);

    while (!sock_read->done())
        sock_read->resume();
    printf("---> ALL DONE! check errors if any.\n");

    delete sock_read;
    delete fs_write;
    delete fs_write_ch;
    delete kafka_produce;
    delete kafka_produce_ch;
    delete wg;
}
