#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "piper.hpp"
#include "serial.hpp"

void consumer(SerialReader &someReader) {
    std::cout << "consumer start" << std::endl;
    while (auto returns = someReader.get(2)) {
        for (auto c : returns.value()) {
            std::cout << c << std::endl;
        }
    }
    std::cout << "consumer end" << std::endl;
}

void producer(SerialWriter &writeThing) {
    std::vector<char> data;
    data.push_back('A');
    data.push_back('B');
    data.push_back('C');
    data.push_back('D');
    data.push_back('E');

    for (char &c : data) {
        int retval = writeThing.send(c);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (retval != 1) {
            throw std::logic_error{"pipe from data failed to write a byte"};
        }
    }

    writeThing.terminate();
}

int main() {

    Piper myPiper{};
    TxPipe txPipe = myPiper.getTx();
    RxPipe rxPipe = myPiper.getRx();

    std::thread t1{consumer, std::ref(rxPipe)};
    std::thread t2{producer, std::ref(txPipe)};

    t1.join();
    t2.join();
}
