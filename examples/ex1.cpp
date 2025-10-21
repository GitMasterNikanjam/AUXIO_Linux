/**
 * @file ex1.cpp
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
 * mkdir -p ./bin && g++ -o ./bin/ex1 ex1.cpp ../AUXIO.cpp -std=c++17 -O2 -lpthread -lgpiod
 * @endcode
 * 
 * Run:
 * @code
 * sudo ./bin/ex1
 * @endcode
 */

// ##############################################################################
// Include Libraries:

#include "../AUXIO.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

// ##############################################################################
// Global Variables:

const char* chipPath = "/dev/gpiochip0";
const uint32_t pinNum = 27;                     // GPIO27

// Construct digital output with active-high polarity
AUXO led(chipPath, pinNum, /*mode=*/1);

/// @brief Global running flag controlled by SIGINT handler (atomic for thread-safety).
static std::atomic<bool> running{true};

// #############################################################################

/**
 * @brief Signal handler for SIGINT (Ctrl+C).
 *
 * Sets the @ref running flag to false so the main loop can exit cleanly.
 *
 * @param sig Signal number (unused).
 */
void on_sigint(int sig) 
{
    (void)sig;
    running.store(false);
    led.clean();
}

// #############################################################################
/**
 * @brief Program entry point.
 *
 * Configures /dev/gpiochip0 line 27 as digital output (active-high by default),
 * blinks it in a loop for 10 cycles, then cleans up before exit.
 *
 * @return 0 on success, nonzero on error.
 */
int main() 
{
    std::signal(SIGINT, on_sigint);

    // Initialize hardware (request line as output)
    if (!led.begin()) {
        std::cerr << "begin() failed: " << led.errorMessage << "\n";
        return 1;
    }

    std::cout << "Blinking line " << pinNum << " on " << chipPath << "...\n";

    // Blink for 10 cycles
    for (int i = 0; i < 10; ++i) {
        led.on();
        std::cout << "ON  (value=" << led.value() << ")\n";
        std::cout << led.value() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        led.off();
        std::cout << "OFF (value=" << led.value() << ")\n";
        std::cout << led.value() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        if(running.load() == false)
        {
            break;
        }
    }

    // Toggle demo (just flips current state)
    std::cout << "Toggling output 3 times...\n";
    for (int i = 0; i < 3; ++i) {
        led.toggle();
        std::cout << "Toggled -> value=" << led.value() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        if(running.load() == false)
        {
            break;
        }
    }

    // Clean up (release line and chip)
    led.clean();
    std::cout << "Done.\n";

    return 0;
}
