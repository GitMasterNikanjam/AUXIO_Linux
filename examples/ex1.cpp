/**
 * @file auxo_demo.cpp
 * @brief Example program demonstrating AUXO (GPIO digital output).
 *
 * This example shows how to:
 * - Initialize a GPIO line as digital output
 * - Configure polarity (active-high or active-low)
 * - Drive the line ON and OFF
 * - Toggle the output in a loop
 * - Gracefully clean up the GPIO line before exiting
 *
 * Polarity (mode):
 * - 1 = active-high (default): logical ON = raw 1
 * - 0 = active-low            : logical ON = raw 0
 *
 * Build:
 * @code
 * g++ -std=c++17 -O2 -lpthread -lgpiod -o auxo_demo auxo_demo.cpp
 * @endcode
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "AUXIO.h"

/**
 * @brief Program entry point.
 *
 * Configures /dev/gpiochip0 line 27 as digital output (active-high by default),
 * blinks it in a loop for 10 cycles, then cleans up before exit.
 *
 * @return 0 on success, nonzero on error.
 */
int main() {
    const char* chip = "/dev/gpiochip0";   ///< GPIO chip device path
    const unsigned int line = 27;          ///< GPIO line offset

    // Construct digital output with active-high polarity
    AUXO led(chip, line, /*mode=*/1);

    // Initialize hardware (request line as output)
    if (!led.begin()) {
        std::cerr << "begin() failed: " << led.errorMessage << "\n";
        return 1;
    }

    std::cout << "Blinking line " << line << " on " << chip << "...\n";

    // Blink for 10 cycles
    for (int i = 0; i < 10; ++i) {
        led.on();
        std::cout << "ON  (value=" << led.value() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        led.off();
        std::cout << "OFF (value=" << led.value() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Toggle demo (just flips current state)
    std::cout << "Toggling output 3 times...\n";
    for (int i = 0; i < 3; ++i) {
        led.toggle();
        std::cout << "Toggled -> value=" << led.value() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    // Clean up (release line and chip)
    led.clean();
    std::cout << "Done.\n";

    return 0;
}
