// ##################################################################################
#pragma once

// ##################################################################################
// Include libraries:
#include <iostream>
#include <gpiod.h>

// ##################################################################################
// AUX Class:

class AUX
{
    public:

        /**
         * @brief Holds an error message if operations fail.
         */
        std::string errorMessage;

        /**
         * @brief Clean up hardware resources.
         */
        void clean(void);

    protected:

        gpiod_chip* _chip;           ///< Handle to the GPIO chip.
        const char* _gpiodChip_path;    ///< Path to the GPIO chip device.
        gpiod_line* _line;           ///< Handle to the GPIO line.
        const char* _consumer;       ///< Consumer label for GPIO request.
        unsigned int _line_offset;   ///< GPIO line offset used by the LED.
        uint8_t _mode;

};

// ##################################################################################
// AUXO Class:

class AUXO : public AUX
{
    public:

        /**
         * @brief Construct a new AUXO object.
         *
         * This only sets up the AUXO pin and its active mode (low/high).
         * Hardware configuration is not applied until begin() is called.
         *
         * @param gpiodChip_path Path to the GPIO chip device (e.g. "/dev/gpiochip0").
         * @param line_offset GPIO line offset number for the AUXO.
         * @param mode Active mode: 0 = active-low, 1 = active-high.
         *
         * @note Call begin() after construction to apply the settings on hardware.
         */
        AUXO(const char* gpiodChip_path, unsigned int line_offset, uint8_t mode = 1); 

        /**
         * @brief Apply settings on the hardware and enable AUXO control.
         *
         * @return true if the hardware setup was successful, false otherwise.
         */
        bool begin(void);

        /// @brief Turn on the LED.
        void on(void);

        /// @brief Turn off the LED.
        void off(void);

        /// @brief Toggle the LED.
        void toggle(void);

    private:

        // Digital value for LED turn on state.
        uint8_t _on;

};

// ##################################################################################
// AUXI Class:

class AUXI : public AUX
{
    public:

        /*
            @param pud: Pullup/Pulldown mode. PUD_OFF:0, PUD_DOWN:1, PUD_UP:2 
        */
        AUXI(const char* gpiodChip_path, unsigned int line_offset, uint8_t pud = 1); 

        bool begin(void);

        /**
         * @brief Read and return input value.
         * @note - Update state value.
         * @return true if the input is trigged.
         * @return false if the input is not trigged.
         */
        bool read(void);

        /**
         * @brief Return last state of input that updated.
         * @return true if the input trigged.
         * @return false if the input is not trigged.
         * @note - This function useful when object is in interrupt mode.
         */
        bool get(void) {return _state;};

        /**
         * @brief External interrupt callback function.
         * @note - If object set in interrupt mode, then use this function, otherwise dont use.
         */
        void EXTI_Callback(void);

    private:

        /// @brief The init state for init successful or not.
        bool _initState;

        /**
         * @brief GPIO pin state.
         */
        volatile bool _state;
};


