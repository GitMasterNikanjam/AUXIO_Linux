
// #########################################################################################
// Iclude libraries:

#include "AUXIO.h"

// ########################################################################################
// AUX (base)

void AUX::clean() 
{
    if (_line) 
    {
        gpiod_line_release(_line);
        _line = nullptr;
    }
    if (_chip) 
    {
        _chip = nullptr;
    }
        errorMessage.clear();
}

// ########################################################################################
// AUXO (output)

AUXO::AUXO(const char* gpiodChip_path, unsigned int line_offset, uint8_t mode) 
{
    _mode = mode ? 1 : 0;
    _line_offset = line_offset;
    _gpiodChip_path = gpiodChip_path;
    _consumer = "AUXO";
    _on = _mode ? 1 : 0; // active-high => 1, active-low => 0
}

bool AUXO::begin() 
{
    _chip = gpiod_chip_open(_gpiodChip_path);
    if (!_chip) 
    {
        errorMessage = "Failed to open chip.";
        return false;
    }

    _line = gpiod_chip_get_line(_chip, _line_offset);
    if (!_line) 
    {
        errorMessage = std::string("Failed to get line at offset ") + std::to_string(_line_offset);
        return false;
    }

    // Start from inactive level
    if (gpiod_line_request_output(_line, _consumer, _on ? 0 : 1) < 0) 
    {
        errorMessage = "Request line as output failed.";
        return false;
    }

    return true;
}

void AUXO::on() 
{
    if (!_line) return;
    gpiod_line_set_value(_line, _on);
}

void AUXO::off() 
{
    if (!_line) return;
    gpiod_line_set_value(_line, _on ? 0 : 1);
}

void AUXO::toggle() 
{
    if (!_line) return;
    int cur = gpiod_line_get_value(_line);
    if (cur < 0) return; // ignore errors silently; consider setting errorMessage
    int next;
    if (cur == _on) 
    {
        next = _on ? 0 : 1; // go inactive
    } 
    else 
    {
        next = _on; // go active
    }
    gpiod_line_set_value(_line, next);
}

int AUXO::value() const 
{
    if (!_line) return -1;
    return gpiod_line_get_value(_line);
}

// #################################################################################
// AUXI (input)

AUXI::AUXI(const char* gpiodChip_path, unsigned int line_offset, uint8_t pud) 
{
    _line_offset = line_offset;
    _gpiodChip_path = gpiodChip_path;
    _consumer = "AUXI";
    _mode = pud; // 0=off, 1=down, 2=up
}

bool AUXI::begin() {
    _chip = gpiod_chip_open(_gpiodChip_path);
    if (!_chip) 
    {
        errorMessage = "Failed to open chip.";
        return false;
    }

    _line = gpiod_chip_get_line(_chip, _line_offset);
    if (!_line) 
    {
        errorMessage = std::string("Failed to get line at offset ") + std::to_string(_line_offset);
        return false;
    }

    // Map _mode to bias flags (libgpiod v1.6+). If unsupported by HW, the request may fail.
    unsigned int flags = 0;
    #ifdef GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE
    if (_mode == 0) flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
    else if (_mode == 1) flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
    else if (_mode == 2) flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    #endif

    #ifdef gpiod_line_request_input_flags
    if (gpiod_line_request_input_flags(_line, _consumer, flags) < 0) 
    {
        errorMessage = "Request line as input failed.";
        return false;
    }
    #else
    // Fallback for very old libgpiod without *_input_flags()
    (void)flags;
    if (gpiod_line_request_input(_line, _consumer) < 0) 
    {
        errorMessage = "Request line as input failed.";
        return false;
    }
    #endif

    _initState = true;
    // Prime the cached state
    int v = gpiod_line_get_value(_line);
    _state = (v > 0);
    return true;
}

bool AUXI::read() 
{
    if (!_line) 
    {
        errorMessage = "Line not initialized.";
        return false;
    }
    int v = gpiod_line_get_value(_line);
    if (v < 0) 
    {
        errorMessage = "gpiod_line_get_value() failed.";
        return _state; // keep previous
    }
    _state = (v != 0);
    return _state;
}

void AUXI::EXTI_Callback() 
{
    // Placeholder: integrate with your own IRQ/event thread and update _state.
    // For libgpiod event-driven reads, you would:
}

// ####################################################################################