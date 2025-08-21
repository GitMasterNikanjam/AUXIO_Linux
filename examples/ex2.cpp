#include "../AUXIO.h"
#include <iostream>

static void my_cb(bool rising, long sec, long nsec) {
    std::cout << (rising ? "RISING" : "FALLING")
              << " at " << sec << "." << nsec << "\n";
}

int main() {
    AUXI btn("/dev/gpiochip0", 17, /*pull-up*/ 2);

    // Start interrupt mode: both edges, 2 ms debounce, with callback
    if (!btn.beginInterrupt(AUXI::Edge::Both, 2000, my_cb)) {
        std::cerr << "ERR: " << btn.errorMessage << "\n";
        return 1;
    }

    // ... do work ...
    std::this_thread::sleep_for(std::chrono::seconds(10));

    btn.stopInterrupt(); // optional; clean() also releases resources
    btn.clean();
}
