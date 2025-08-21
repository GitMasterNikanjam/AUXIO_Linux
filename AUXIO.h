// ##################################################################################
#pragma once

// ##################################################################################
// Include libraries:
#include <iostream>
#include <gpiod.h>
#include <cstdint>
#include <string>

// ##################################################################################
// AUX Base Class

/**
* @brief Common base for simple GPIO input/output helpers using libgpiod v1.x.
*/
class AUX
{
    public:

        /**
         * @brief Holds an error message if operations fail.
         */
        std::string errorMessage;

        /**
        * @brief Release the requested line and close the chip handle.
        *
        * Safe to call multiple times.
        */
        void clean(void);

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

class AUXO : public AUX
{
    public:

        /**
        * @param gpiodChip_path Path to the GPIO chip device (e.g. "/dev/gpiochip0").
        * @param line_offset GPIO line offset number.
        * @param mode Active level: 0 = active-low, 1 = active-high.
        */
        AUXO(const char* gpiodChip_path, unsigned int line_offset, uint8_t mode = 1);

        /** Apply the settings on hardware (request the line as output). */
        bool begin();


        /** Drive the line to its active level. */
        void on();


        /** Drive the line to its inactive level. */
        void off();


        /** Toggle the current line level. */
        void toggle();


        /**
        * @brief Return the current sampled value of the line (1/0) or -1 on error.
        * Note: For outputs this reads back the state (if the hardware reflects it).
        */
        int value() const;

    private:

        uint8_t _on = 1;    ///< Digital value considered "on" (active)

};

// ##################################################################################
// AUXI: Simple digital input

class AUXI : public AUX
{
    public:
        /**
        * @param gpiodChip_path Path to the GPIO chip device (e.g. "/dev/gpiochip0").
        * @param line_offset GPIO line offset number.
        * @param pud Bias: 0=off, 1=pulldown, 2=pullup (if hardware supports it).
        */
        AUXI(const char* gpiodChip_path, unsigned int line_offset, uint8_t pud = 1);


        /** Request the line as input with the configured bias. */
        bool begin();


        /** Read and return the input value. Also updates internal _state. */
        bool read();


        /** Return the last cached value (updated by read()). */
        bool get() const { return _state; }


        /** Placeholder for external IRQ integration hooks (not used here). */
        void EXTI_Callback();


    private:
    
        bool _initState = false; ///< True if begin() succeeded
        volatile bool _state = false; ///< Last sampled state
};


