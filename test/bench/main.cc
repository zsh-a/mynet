#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include"mynet/task.h"
using namespace mynet;
int main() {
    // double d = 1.0;
    // ankerl::nanobench::Bench().run("some double ops", [&] {
    //     d += 1.0 / d * 3.5;
    //     if (d > 5.0) {
    //         d -= 5.0;
    //     }
    //     ankerl::nanobench::doNotOptimizeAway(d);
    // });

    auto simple = []() -> Task<int>{
        co_return 9;
    };

    ankerl::nanobench::Bench().run("simple task", [&] {
        // auto& loop = EventLoop::get();
        // auto g = simple();
        // loop.run_until_done(g.get_resumable());        
        mynet::create_task(simple());
    });
}