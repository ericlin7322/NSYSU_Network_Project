#include <iostream>
#include <thread>
#include <chrono>

class Timer {
    bool clear = false;

public:
    template<typename T>
    void setTimeout(T function, int delay);
    template<typename T>
    void setInterval(T function, int interval);
    void stop();

};

template<typename T>
void Timer::setTimeout(T function, int delay) {
    this->clear = false;
    std::thread t([=]() {
        if (this->clear) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        if (this->clear) return;
        function();
    });
    t.detach();
}

template<typename T>
void Timer::setInterval(T function, int interval) {
    this->clear = false;
    std::thread t([=]() {
        while (true) {
            if (this->clear) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if (this->clear) return;
            function();
        }
    });
    t.detach();
}

void Timer::stop() {
    this->clear = true;
}