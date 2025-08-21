
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

// ================= Event-driven API =================

bool AUXI::beginInterrupt(Edge edge, uint32_t debounce_us, GpioCallback cb) 
{
    // If already running, stop first
    stopInterrupt();

    // Open chip + line fresh (release any prior request)
    if (_line) { gpiod_line_release(_line); _line = nullptr; }
    if (!_chip) 
    {
        _chip = gpiod_chip_open(_gpiodChip_path);
        if (!_chip) { errorMessage = "Failed to open chip."; return false; }
    }
    _line = gpiod_chip_get_line(_chip, _line_offset);
    if (!_line) { errorMessage = "Failed to get line."; return false; }

    // Compute flags for bias
    unsigned int flags = 0;
    #ifdef GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE
        if (_mode == 0) flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
        else if (_mode == 1) flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
        else if (_mode == 2) flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    #endif

    // Request appropriate edge events
    int rc = -1;
    #ifdef gpiod_line_request_rising_edge_events_flags
        if (edge == Edge::Rising) rc = gpiod_line_request_rising_edge_events_flags(_line, _consumer, flags);
        else if (edge == Edge::Falling)rc = gpiod_line_request_falling_edge_events_flags(_line, _consumer, flags);
        else rc = gpiod_line_request_both_edges_events_flags(_line, _consumer, flags);
    #else
        // Fallback without *_flags() (bias may be ignored on very old libgpiod)
        if (edge == Edge::Rising) rc = gpiod_line_request_rising_edge_events(_line, _consumer);
        else if (edge == Edge::Falling)rc = gpiod_line_request_falling_edge_events(_line, _consumer);
        else rc = gpiod_line_request_both_edges_events(_line, _consumer);
    #endif

    if (rc < 0) { errorMessage = "Request edge events failed."; return false; }

    _edgeSel = edge;
    _debounce_us = debounce_us;
    _cb = cb;
    _running.store(true);

    // Prime cached state
    int v = gpiod_line_get_value(_line);
    _state = (v > 0);

    _thread = std::thread(&AUXI::_pollLoop, this);
    return true;
}


void AUXI::stopInterrupt() 
{
    if (_running.exchange(false)) 
    {
        // Join thread if it exists
        if (_thread.joinable()) _thread.join();
    }
    // Keep line requested so caller may still read(); release only in clean()
}

void AUXI::_pollLoop() 
{
    using clock = std::chrono::steady_clock;
    auto last_time = clock::time_point::min();
    const auto debounce = std::chrono::microseconds(_debounce_us);

    // 100 ms wait slices so we can check _running periodically
    timespec ts; ts.tv_sec = 0; ts.tv_nsec = 100000000; // 100ms

    while (_running.load()) 
    {
        int ret = gpiod_line_event_wait(_line, &ts);
        if (!_running.load()) break; // allow timely exit
        if (ret <= 0) continue; // timeout or error -> loop (errors are transient here)

        gpiod_line_event ev{};
        if (gpiod_line_event_read(_line, &ev) < 0) 
        {
            // Reading failed; keep looping
            continue;
        }

        // Debounce (time since last delivered event)
        auto now = clock::now();
        if (_debounce_us > 0 && last_time != clock::time_point::min()) 
        {
            if (now - last_time < debounce) 
            {
                continue; // drop bounces within window
            }
        }
        last_time = now;

        bool rising = (ev.event_type == GPIOD_LINE_EVENT_RISING_EDGE);

        // Update cached state (best-effort)
        int v = gpiod_line_get_value(_line);
        if (v >= 0) _state = (v != 0);
        else _state = rising; // fallback guess

        // Fire callback if provided
        if (_cb) 
        {
            _cb(rising, ev.ts.tv_sec, ev.ts.tv_nsec);
        }
    }
}

// ####################################################################################