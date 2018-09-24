#include "future.hh"

#include <cassert>
#include <thread>
#include <atomic>

int main(int argc, const char *argv[])
{
    experimental::promise<int> p;
    std::atomic<bool> pred{false};
    auto f1 = p.get_future();

    auto f2 = f1.then([&pred](int input) -> void {
        pred = (input == 42);
    });

    std::thread t([&p]() {
        p.set_value(42);
    });

    f2.get();
    assert(pred);
    t.join();

    return 0;
}
