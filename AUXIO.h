/**
 * @file AUXIO.h
 * @brief Simple GPIO input/output wrapper classes using libgpiod v1.x.
 *
 * This header defines three classes:
 * - AUX   (base class providing common members and cleanup)
 * - AUXO  (digital output control)
 * - AUXI  (digital input with optional event-driven callback + debounce)
 *
 * These classes simplify using libgpiod for embedded Linux projects.
 * Tested with libgpiod v1.6.2.
 */

// ##################################################################################
#pragma once

// ##################################################################################
// Include libraries:

#include <iostream>     // Standard C++ I/O stream library.
#include <gpiod.h>      // libgpiod C API. Provides low-level access to GPIO chips and lines (v1.x API).
#include <cstdint>      // Fixed-width integer types. Used for explicit-sized variables (e.g. uint8_t).
#include <string>       // C++ standard string class. Used for storing error messages and consumer labels.
#include <thread>       // C++ thread support. Used by AUXI for the internal polling/event-driven interrupt thread.
#include <atomic>       // C++ atomic operations library. Provides thread-safe flags (e.g., to control the interrupt polling loop).

// ##################################################################################
// Define Macros:

#ifndef HIGH
    #define HIGH 1
#endif

#ifndef LOW
    #define LOW 0
#endif

// ##################################################################################
// AUX Base Class

/**
 * @brief Common base for simple GPIO input/output helpers using libgpiod v1.x.
 *
 * The AUX class is not intended to be used directly. It provides:
 * - errorMessage string for reporting failures
 * - clean() method for releasing lines
 * - protected members for subclasses (chip, line, consumer, etc.)
 */
class AUX
{
    public:

        /**
         * @brief Holds an error message if operations fail.
         */
        std::string errorMessage;

        /**
         * @brief Release the requested line handle.
         *
         * Safe to call multiple times. After this call, the AUX object
         * is reset to its default state.
         */
        bool clean(void);

        /**
         * @brief Return the current sampled value of the line.
         *
         * @return 1 if high, 0 if low, or -1 on error.
         * @note - For outputs this may reflect hardware state if supported.
         * @note - For inputs this reflect digital level of the line.
         */
        int value() const;

    protected:

        gpiod_chip* _chip = nullptr;            ///< GPIO chip handle
        const char* _gpiodChip_path = nullptr;  ///< Path to the GPIO chip (e.g. "/dev/gpiochip0")
        gpiod_line* _line = nullptr;            ///< Line handle
        const char* _consumer = "AUX";          ///< Consumer label
        unsigned int _line_offset = 0;          ///< Line offset
        uint8_t _mode = 0;                      ///< Mode/bias or active level depending on subclass

};

// ##################################################################################
// AUXO: Simple digital output

/**
 * @brief Digital output wrapper for libgpiod.
 *
 * Provides simple methods to set, clear, and toggle a GPIO line.
 * Handles active-high vs. active-low logic transparently.
 */
class AUXO : public AUX
{
    public:

        /**
         * @brief Construct a digital output object.
         *
         * @param gpiodChip_path Path to the GPIO chip device (e.g. "/dev/gpiochip0").
         * @param line_offset GPIO line offset number.
         * @param mode Active level: 0 = active-low, 1 = active-high (default).
         */
        AUXO(const char* gpiodChip_path, unsigned int line_offset, uint8_t mode = 1);

        /**
         * @brief Request the GPIO line as output.
         *
         * Initializes the chip and line handles and sets the line
         * initially to inactive state.
         *
         * @return true on success, false on failure (see errorMessage).
         */
        bool begin();

        /** @brief Drive the line to its active level. */
        void on();

        /** @brief Drive the line to its inactive level. */
        void off();

        /** @brief Toggle the current line level. */
        void toggle();

        /**
         * @brief Set digital level of output line.
         * @param level true = digital level be 1, false = digital level be 0.
         */
        void digitalWrite(bool level);

    private:

        uint8_t _on = 1;    ///< Digital value considered "on" (active)

};

// ##################################################################################
// AUXI: Simple digital input (with optional event thread + debounce)

/**
 * @brief Digital input wrapper for libgpiod.
 *
 * Polarity and bias are independent:  
 *  - _mode: 0 = active-low, 1 = active-high (affects read()/get())
 *  - _bias: 0 = off, 1 = pull-down, 2 = pull-up (hardware support required)
 */
class AUXI : public AUX
{
    public:

        /** @brief Callback type: triggered on rising/falling edge with timestamp. */
        using GpioCallback = void(*)(bool is_rising, long sec, long nsec);

        /** @brief Edge selection for interrupts. */
        enum class Edge : uint8_t { Both = 0, Rising = 1, Falling = 2 };

        /**
         * @brief Construct a digital input object.
         *
         * @param gpiodChip_path Path to the GPIO chip device (e.g. "/dev/gpiochip0").
         * @param line_offset GPIO line offset number.
         * @param mode Polarity: 0=active-low, 1=active-high (default).
         * @param bias Bias: 0=off, 1=pulldown (default), 2=pullup.
         */
        AUXI(const char* gpiodChip_path, unsigned int line_offset, uint8_t mode = 1, uint8_t bias = 0);

        /**
         * @brief Request the line as input with the configured bias.
         * @return true on success, false on failure (see errorMessage).
         */
        bool begin();

        /**
         * @brief Sample the line value and apply polarity.
         *
         * Converts the raw hardware level (0/1) into logical state according to
         * @ref _mode: 0=active-low (invert), 1=active-high (as-is).
         *
         * @return true if logical HIGH, false if logical LOW. On error, returns
         *         previous cached state and sets @ref errorMessage.
         */
        bool read();

        /** @brief Return the last cached logical value (after polarity). */
        bool get() const { return _state; }

        /**
         * @brief Placeholder for external interrupt integration.
         *
         * Intended for projects that integrate with native IRQ handling.
         */
        void EXTI_Callback();

        // ================= Event-driven API =================

        /**
         * @brief Start an event-driven input monitor.
         *
         * Requests edge events and starts an internal polling thread
         * with optional debounce filtering. Callback is invoked from
         * the internal thread.
         *
         * @param edge Which edges to listen for (Both, Rising, Falling).
         * @param debounce_us Debounce interval in microseconds (0 disables debounce).
         * @param cb Optional C-style callback function pointer.
         * @return true on success, false on failure (see errorMessage).
         */
        bool beginInterrupt(Edge edge = Edge::Both, uint32_t debounce_us = 1000, GpioCallback cb = nullptr);

        /**
         * @brief Stop the internal event monitoring thread.
         *
         * Safe to call even if no thread is running.
         */
        void stopInterrupt();

        /** @brief Return whether the internal interrupt thread is running. */
        bool isInterruptRunning() const { return _running.load(); }

    private:

        volatile bool _state = false;           ///< Last cached logical state  

        // Interrupt machinery
        std::thread _thread;                    ///< Poll thread
        std::atomic<bool> _running{false};      ///< Event thread status flag
        uint32_t _debounce_us = 0;              ///< Debounce window (us)
        GpioCallback _cb = nullptr;             ///< Optional user callback
        Edge _edgeSel = Edge::Both;             ///< Which edge(s) are enabled

        uint8_t _bias = 0;                      ///< 0=off, 1=pulldown, 2=pullup

        /** @brief Apply polarity mapping (_mode: 0=active-low, 1=active-high). */
        inline bool _applyPolarity(int raw) const 
        {
            // raw < 0 means "keep previous"; caller handles that.
            const bool lvl = (raw != 0);
            return _mode ? lvl : !lvl;
        }

        /** @brief Internal thread loop for event-driven monitoring. */
        void _pollLoop();
};


