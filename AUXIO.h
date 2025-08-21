#ifndef AUXIO_H
#define AUXIO_H

#include <iostream>
#include <bcm2835.h>

class AUX
{
    public:

        std::string errorMessage;

        void clean(void);

    protected:

       // GPIO pin number
        uint8_t _pin;
    
        uint8_t _mode;

};

class AUXO : public AUX
{
    public:

        AUXO(uint8_t pin, uint8_t mode = HIGH);

        bool begin(void);

    private:

        // Digital value for LED turn on state.
        uint8_t _on;

};

class AUXI : public AUX
{
    public:

        /*
            @param pud: Pullup/Pulldown mode. PUD_OFF:0, PUD_DOWN:1, PUD_UP:2 
        */
        AUXI(uint8_t pin, uint8_t pud = BCM2835_GPIO_PUD_OFF);

        bool begin(void);

};


#endif