# AUXIO – Simple C++ GPIO Helper Library (libgpiod v1.x)

`AUXIO` is a lightweight C++ wrapper around **libgpiod v1.x** for Linux GPIO access.  
It provides simple, object-oriented classes for **digital input** and **digital output**, with optional **event-driven interrupts** and **debounce filtering**.

Tested with **libgpiod v1.6.2** on Linux SBCs (Raspberry Pi, BeagleBone, x86).

---

## ✨ Features

- **AUXO** – digital output helper
  - Active-high / active-low support
  - Simple `on()`, `off()`, `toggle()`, and `value()` methods
- **AUXI** – digital input helper
  - Configurable polarity (active-high / active-low)
  - Configurable bias (off / pull-down / pull-up)
  - Simple polling with `read()` and cached `get()`
  - Event-driven edge interrupts with:
    - Rising, falling, or both edges
    - Optional debounce (µs resolution)
    - C-style callback with **kernel timestamp**

---

## 📂 File Overview

- `AUXIO.h` – class declarations
- `AUXIO.cpp` – implementation
- `ex1.cpp` – output example (`AUXO`)
- `ex2.cpp` – input example with interrupts (`AUXI`)

---

## ⚙️ Dependencies

- Linux with `/dev/gpiochipN` devices
- [libgpiod v1.x](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git)  
  (install with `sudo apt install libgpiod-dev` on Debian/Ubuntu)

---

## 🔨 Build Examples

```bash
# Build output demo
g++ -std=c++17 -O2 -lpthread -lgpiod -o auxo_demo ex1.cpp AUXIO.cpp

# Build input demo
g++ -std=c++17 -O2 -lpthread -lgpiod -o auxi_demo ex2.cpp AUXIO.cpp
````

Run as root (GPIO access requires privileges):

```bash
sudo ./auxo_demo
sudo ./auxi_demo
```

---

## 🚦 Usage

### 1. Digital Output (AUXO)

```cpp
#include "AUXIO.h"

int main() {
    AUXO led("/dev/gpiochip0", 27, /*mode=*/1); // active-high output

    if (!led.begin()) {
        std::cerr << "Error: " << led.errorMessage << "\n";
        return 1;
    }

    led.on();   // set line active
    led.off();  // set line inactive
    led.toggle(); // flip current state

    led.clean(); // release GPIO
    return 0;
}
```

➡️ See [ex1.cpp](ex1.cpp) for full blinking example.

---

### 2. Digital Input with Interrupts (AUXI)

```cpp
#include "AUXIO.h"
#include <iostream>
#include <csignal>
#include <atomic>

static std::atomic<bool> running{true};

void sig_handler(int) { running.store(false); }

// Callback triggered on edges
void my_gpio_cb(bool rising, long sec, long nsec) {
    std::cout << (rising ? "Rising" : "Falling")
              << " at " << sec << "." << nsec << "\n";
}

int main() {
    std::signal(SIGINT, sig_handler);

    AUXI button("/dev/gpiochip0", 27, /*mode=*/0, /*bias=*/2); // active-low, pull-up
    if (!button.begin()) {
        std::cerr << "Error: " << button.errorMessage << "\n";
        return 1;
    }

    if (!button.beginInterrupt(AUXI::Edge::Both, 1000, my_gpio_cb)) {
        std::cerr << "Interrupt setup failed: " << button.errorMessage << "\n";
        return 1;
    }

    while (running.load()) {
        // check cached logical state if needed
        bool state = button.get();
        (void)state;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    button.stopInterrupt();
    button.clean();
}
```

➡️ See [ex2.cpp](ex2.cpp) for full input/interrupt example.

---

## 📝 Notes

* **Polarity vs. Bias**:

  * `mode = 1` → active-high (raw 1 = logical HIGH)
  * `mode = 0` → active-low (raw 0 = logical HIGH)
  * `bias = 0` → no bias
  * `bias = 1` → pull-down
  * `bias = 2` → pull-up
* Use `value()` (AUXO) and `get()` (AUXI) for cached logical state.
* Use `clean()` to release GPIO lines before exiting.

---

## 📜 License

This library is released under the **MIT License**.
You are free to use it in personal and commercial projects.

---


