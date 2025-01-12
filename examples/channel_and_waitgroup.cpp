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

int main()
{
    auto fs_write_ch = new chan_t<int>(3);
    auto kafka_produce_ch = new chan_t<int>;
    auto wg = new wait_group_t;

    wg->add(1);
    auto fs_write = go(
        [=](co_t *__self, state_t *_st) {
            auto st = dynamic_cast<fs_write_state_t *>(_st);
            bool ok = false;
            while (true) {
                coco_begin();
                coco_read_chan(fs_write_ch, st->i, ok);
                if (ok) {
                    printf("---> FS WRITE, i=%d\n", st->i);
                } else {
                    printf("---> fs_write_ch is closed.\n");
                    goto out;
                }
                coco_end();
            }
        out:
            wg->done();
            coco_done();
        },
        new fs_write_state_t);

    wg->add(1);
    auto kafka_produce = go(
        [=](co_t *__self, state_t *_st) {
            auto st = dynamic_cast<kafka_produce_state_t *>(_st);
            bool ok = false;
            while (true) {
                coco_begin();
                coco_read_chan(kafka_produce_ch, st->i, ok);
                if (ok) {
                    printf("---> KAFKA produce message, i=%d\n", st->i);
                } else {
                    printf("---> kafka_produce_ch is closed.\n");
                    goto out;
                }
                coco_end();
            }
        out:
            wg->done();
            coco_done();
        },
        new kafka_produce_state_t);

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
    printf("---> ALL DONE! check errors if any.\n");

    delete sock_read;
    delete fs_write;
    delete fs_write_ch;
    delete kafka_produce;
    delete kafka_produce_ch;
    delete wg;
}
