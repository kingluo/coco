#include "../coco.h"
#include <iostream>

using namespace coco;

co_t producer(chan_t<int> &ch) {
    for (int i = 0; i < 3; i++) {
        std::cout << "Sending: " << i << std::endl;
        bool ok = co_await ch.write(i);
        if (!ok) {
            std::cout << "Channel closed, stopping producer" << std::endl;
            break;
        }
    }
    ch.close();
    std::cout << "Producer finished" << std::endl;
}

co_t consumer(chan_t<int> &ch, const std::string &name) {
    while (true) {
        auto result = co_await ch.read();
        if (result.has_value()) {
            std::cout << name << " received: " << result.value() << std::endl;
        } else {
            std::cout << name << " channel closed" << std::endl;
            break;
        }
    }
}

int main() {
    chan_t<int> ch(1); // Buffered channel with capacity 1

    auto prod = producer(ch);
    auto cons1 = consumer(ch, "Consumer1");
    auto cons2 = consumer(ch, "Consumer2");

    // Simple round-robin scheduler
    for (int i = 0; i < 100; i++) { // Limit iterations to prevent infinite loop
        bool any_active = false;

        if (prod.handle && !prod.handle.done()) {
            prod.resume();
            any_active = true;
        }
        if (cons1.handle && !cons1.handle.done()) {
            cons1.resume();
            any_active = true;
        }
        if (cons2.handle && !cons2.handle.done()) {
            cons2.resume();
            any_active = true;
        }

        // Run the scheduler to process any scheduled coroutines
        scheduler_t::instance().run();

        // Check if all coroutines are done
        if (prod.handle.done() && cons1.handle.done() && cons2.handle.done()) {
            break;
        }

        if (!any_active)
            break;
    }

    std::cout << "---> ALL DONE! check errors if any." << std::endl;
    return 0;
}
