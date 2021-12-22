#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include"mynet/task.h"
#include<bits/stdc++.h>
using namespace mynet;
using   namespace std;

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
        EventLoop g_loop;
        // auto g = simple();
        // loop.run_until_done(g.get_resumable());        
        for(int i = 0;i < 100;i++)
            g_loop.create_task(simple());
        g_loop.run();
    });
}