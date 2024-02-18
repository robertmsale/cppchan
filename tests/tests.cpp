#include "../library.h"
#include <string>
#include <iostream>
#include <chrono>
#include <random>
#include <thread>

struct DoNothing {};
struct Print { std::string msg; };
struct MultiplyThese { int a; int b; };
struct WaitAWhile{};

int main(int argc, char** argv) {
    std::atomic_bool terminate_threads{false};
    std::atomic_size_t jobs_left{0};
    std::random_device rd;
    std::uniform_int_distribution<uint16_t> dist;
    CppChan::Channel<DoNothing, Print> printer_channel{ [](const DoNothing& d){},
        [] (const Print& to_printer) {
            std::cout << to_printer.msg << "\n";
        }
    };
    CppChan::Channel<DoNothing, MultiplyThese, WaitAWhile> work_channel{ [](const DoNothing& d){},
        [&] (const MultiplyThese& nums) {
            std::string msg = std::to_string(nums.a) + " * " + std::to_string(nums.b) + " = " + std::to_string(nums.a * nums.b);
            printer_channel.transmitter.send(Print{msg});
            jobs_left--;
        },
        [&] (const WaitAWhile& wait) {
            uint16_t time = dist(rd);
            std::this_thread::sleep_for(std::chrono::nanoseconds(time));
            printer_channel.transmitter.send(Print{"Worker slept for " + std::to_string(time) + " nanoseconds"});
            jobs_left--;
        }
    };
    std::thread printer{[&] {
        while (!terminate_threads) {
            printer_channel.receiver.process_next();
        }
    }};
    std::vector<std::thread> workers;
    for(int i = 0; i < 8; ++i) {
        workers.emplace_back([&](){
            while(!terminate_threads) {
                work_channel.receiver.process_next();
            }
        });
    }
    for(int i = 0; i < 50; ++i) {
        work_channel.transmitter.send(MultiplyThese{3, i});
        jobs_left++;
        work_channel.transmitter.send(WaitAWhile{});
        jobs_left++;
    }
    while (jobs_left > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    terminate_threads = true;
    printer_channel.transmitter.send(DoNothing{});
    for(int i = 0; i < 8; ++i) work_channel.transmitter.send(DoNothing{});

    printer.join();
    for(auto& t: workers) t.join();

    std::cout << "All threads joined! Test complete!";

    return 0;
}