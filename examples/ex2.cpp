/**
 * @file auxi_demo.cpp
 * @brief Example program demonstrating AUXI (GPIO input with interrupts, polarity + bias).
 *
 * This example shows how to:
 * - Initialize a GPIO line as input with pull-up bias
 * - Use active-low polarity mapping (raw 0 -> logical 1)
 * - Perform a one-time read of the logical input state
 * - Register an event-driven handler for rising/falling edges
 * - Use kernel timestamps delivered by libgpiod events
 * - Gracefully stop on Ctrl+C
 *
 * Polarity vs. Bias:
 * - Polarity (mode): 0=active-low, 1=active-high  -> affects logical state returned by read()/get()
 * - Bias: 0=off, 1=pulldown, 2=pullup            -> hardware pull configuration
 *
 * Build:
 * @code
 * g++ -std=c++17 -O2 -lpthread -lgpiod -o auxi_demo auxi_demo.cpp
 * @endcode
 */

#include <csignal>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "AUXIO.h"

/**
 * @brief Example GPIO callback function.
 *
 * Called from AUXI's internal event thread whenever an edge occurs.
 * Prints edge type and the kernel timestamp of the event.
 *
 * @param is_rising True if rising edge, false if falling.
 * @param sec       Seconds part of kernel timestamp.
 * @param nsec      Nanoseconds part of kernel timestamp.
 */
void my_gpio_cb(bool is_rising, long sec, long nsec) {
    std::cout << (is_rising ? "[EDGE] Rising " : "[EDGE] Falling ")
              << "at " << sec << "." << nsec << " (kernel ts)\n";
}

/// @brief Global running flag controlled by SIGINT handler (atomic for thread-safety).
static std::atomic<bool> running{true};

// -------------------------------
/*
std::atomic tells the compiler:
“This variable may change in another thread (or signal context). Always read/write it directly, and do it safely.”

It guarantees:

No caching issues (the main loop will see the new value quickly).

No data races (safe concurrent access without undefined behavior).

In practice, this makes sure that when on_sigint() sets it to false, the while (running.load()) in the main loop really sees it and exits.

bool running → ❌ might not update properly, undefined behavior.

volatile bool running → 😬 works sometimes, but not guaranteed safe in multithreaded/signal code.

std::atomic<bool> running → ✅ guaranteed safe and portable.
*/
// ------------------------------

/**
 * @brief Signal handler for SIGINT (Ctrl+C).
 *
 * Sets the @ref running flag to false so the main loop can exit cleanly.
 *
 * @param sig Signal number (unused).
 */
void on_sigint(int sig) {
    (void)sig;
    running.store(false);
}

/**
 * @brief Program entry point.
 *
 * Configures /dev/gpiochip0 line 27 as input with pull-up bias and active-low polarity,
 * starts edge interrupts with 1 ms debounce, and prints events until Ctrl+C.
 *
 * @return 0 on success, nonzero on error.
 */
int main() {
    std::signal(SIGINT, on_sigint);

    const char* chip = "/dev/gpiochip0";   ///< GPIO chip device path
    const unsigned int line = 27;          ///< GPIO line offset

    // Polarity (mode) and bias:
    // mode=0 => active-low  (raw 0 -> logical 1)
    // bias=2 => pull-up
    AUXI in(chip, line, /*mode=*/0, /*bias=*/2);

    // Request line as input with configured bias
    if (!in.begin()) {
        std::cerr << "begin() failed: " << in.errorMessage << "\n";
        return 1;
    }

    // One-time logical read (after polarity mapping)
    bool level = in.read();
    std::cout << "Initial logical level: " << (level ? "HIGH (active)" : "LOW (inactive)") << "\n";

    // Start interrupts: Both edges, 1000 us debounce, with our callback
    if (!in.beginInterrupt(AUXI::Edge::Both, /*debounce_us=*/1000, my_gpio_cb)) {
        std::cerr << "beginInterrupt() failed: " << in.errorMessage << "\n";
        return 1;
    }

    std::cout << "Listening for edges on chip=" << chip
              << " line=" << line << " (Ctrl+C to quit)\n";

    // Main thread can do other work; here we just idle and optionally check cached state
    while (running.load()) {
        bool cached = in.get(); // logical (polarity-applied) state
        (void)cached;           // placeholder for application logic
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Stop interrupt thread and release resources
    in.stopInterrupt();
    in.clean();
    std::cout << "Stopped.\n";
    return 0;
}
