
// #########################################################################################
// Iclude libraries:

#include "AUXIO.h"

// ########################################################################################
// AUXO Class:

AUXO::AUXO(const char* gpiodChip_path, unsigned int line_offset, uint8_t mode)
{
    _mode = mode;
    _line_offset = line_offset;
    _gpiodChip_path = gpiodChip_path;
    _consumer = "AUXO";

    if(_mode == 0)
    {
        _on = 0;

    }
    else
    {
        _on = 1;
    }
}

bool AUXO::begin(void)
{
    _chip = gpiod_chip_open(_gpiodChip_path);
    if (!_chip) 
    {
        errorMessage =  "Failed to open chip.";
        return false;
    }

    _line = gpiod_chip_get_line(_chip, _line_offset);
    if (!_line) 
    {
        errorMessage = "Failed to get line at offset " + std::to_string(_line_offset);
        return false;
    }

    if (gpiod_line_request_output(_line, _consumer, !_on) < 0) 
    {
        errorMessage = "Request line as output failed.";
        return false;
    }

    return true;
}


void AUXO::on(void)
{
    gpiod_line_set_value(_line, _on);
}

void AUXO::off(void)
{
    gpiod_line_set_value(_line, !_on);
}

void AUXO::toggle(void)
{
    

}

// #################################################################################
// AUXI Class:

AUXI::AUXI(const char* gpiodChip_path, unsigned int line_offset, uint8_t pud = 1)
{
    _line_offset = line_offset;
    _gpiodChip_path = gpiodChip_path;
    _consumer = "AUXI";
    _mode = pud;
}

bool AUXI::begin(void)
{


    // bcm2835_gpio_fsel(_pin, BCM2835_GPIO_FSEL_INPT);
    // bcm2835_gpio_set_pud(_pin, _mode);

    return true;
}

// ####################################################################################