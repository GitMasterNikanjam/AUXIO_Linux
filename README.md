# AUXIO — Simple C++ GPIO Helper Library (libgpiod v1.x)

`AUXIO` is a tiny, dependency‑light C++ wrapper around **libgpiod v1.x** for Linux GPIO.
It gives you two small classes:
- **AUXO** — digital **O**utput
- **AUXI** — digital **I**nput (with optional edge interrupts + software debounce)

It’s designed for single‑board computers and embedded Linux (Raspberry Pi, BeagleBone, x86 SBCs).  
Tested with **libgpiod v1.6.2**.

> ⚠️ **libgpiod v2.x is not supported** by this header/implementation. If your distro ships v2, install v1.x or adapt the calls accordingly.

---

## ✨ What’s new (this version)

- New base helper `AUX::value()` returning the **current sampled level** for both inputs and outputs (1/0, or −1 on error).
- Clearer docs and comments; small cleanups in interrupt loop and cleanup path.

---

## ✨ Features

- **Output (AUXO)**
  - Active‑high or active‑low logic (transparent `on()/off()`)
  - `toggle()` and **`value()`** via the base class (`AUX::value()`)
- **Input (AUXI)**
  - Polarity mapping (active‑high / active‑low)
  - Bias control (off / pull‑down / pull‑up; if supported by hardware)
  - Simple polling via `read()` and cached `get()`
  - **Event‑driven** edge detection (rising/falling/both) with:
    - Optional debounce (µs)
    - C‑style callback carrying **kernel timestamps**

---

## 📁 Repo Layout

```
AUXIO.h      # API (classes AUX, AUXO, AUXI)
AUXIO.cpp    # Implementation
ex1.cpp      # AUXO example (digital output)
ex2.cpp      # AUXI example (input + interrupts)
```

---

## 🔧 Requirements

- Linux with `/dev/gpiochipN` character devices
- **libgpiod v1.x** runtime & headers (e.g. `libgpiod-dev` on Debian/Ubuntu)
- A compiler with C++17 support

Install on Debian/Ubuntu (v1.x):
```bash
sudo apt update
sudo apt install libgpiod-dev gpiod
# Check version
gpiodetect --version
# -> gpiodetect (libgpiod) v1.6.2
```

---

## 🛠️ Build

### Using g++ (single file examples)

```bash
# Output demo
g++ -std=c++17 -O2 -lpthread -lgpiod -o auxo_demo ex1.cpp AUXIO.cpp

# Input demo
g++ -std=c++17 -O2 -lpthread -lgpiod -o auxi_demo ex2.cpp AUXIO.cpp
```

### Using pkg-config
```bash
g++ -std=c++17 -O2 \
    $(pkg-config --cflags libgpiod) \
    -o auxo_demo ex1.cpp AUXIO.cpp \
    $(pkg-config --libs libgpiod) -lpthread
```

### Using CMake (minimal)
```cmake
cmake_minimum_required(VERSION 3.10)
project(auxio_demo CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

add_executable(auxo_demo ex1.cpp AUXIO.cpp)
target_link_libraries(auxo_demo PRIVATE PkgConfig::GPIOD pthread)

add_executable(auxi_demo ex2.cpp AUXIO.cpp)
target_link_libraries(auxi_demo PRIVATE PkgConfig::GPIOD pthread)
```

Run with permissions (GPIO usually requires root or udev rules):
```bash
sudo ./auxo_demo
sudo ./auxi_demo
```

---

## 🚀 Quick Start

### 1) Digital Output — `AUXO`

```cpp
#include "AUXIO.h"
#include <iostream>

int main() {
    AUXO led("/dev/gpiochip0", 27, /*mode=*/1); // 1 = active-high

    if (!led.begin()) {
        std::cerr << "begin() failed: " << led.errorMessage << "\n";
        return 1;
    }

    led.on();      // drive to active level
    led.off();     // drive to inactive level
    led.toggle();  // flip current level

    std::cout << "Current level: " << led.value() << "\n"; // AUX::value()
    led.clean();   // release resources
}
```
👉 Full example: [`ex1.cpp`](ex1.cpp)

---

### 2) Digital Input + Interrupts — `AUXI`

```cpp
#include "AUXIO.h"
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

static std::atomic<bool> running{true};
void on_sigint(int){ running.store(false); }

// Called from AUXI's internal thread on edges
void my_gpio_cb(bool rising, long sec, long nsec) {
    std::cout << (rising ? "Rising" : "Falling")
              << " at " << sec << "." << nsec << "\n";
}

int main() {
    std::signal(SIGINT, on_sigint);

    // mode=0 => active-low (raw 0 -> logical HIGH)
    // bias=2 => pull-up
    AUXI btn("/dev/gpiochip0", 18, /*mode=*/0, /*bias=*/2);

    if (!btn.begin()) {
        std::cerr << "begin() failed: " << btn.errorMessage << "\n";
        return 1;
    }

    bool first = btn.read(); // one-shot logical read (after polarity)
    std::cout << "First logical state: " << (first ? "HIGH" : "LOW") << "\n";

    // Edges=Both, debounce=1000us, callback=my_gpio_cb
    if (!btn.beginInterrupt(AUXI::Edge::Both, 1000, my_gpio_cb)) {
        std::cerr << "beginInterrupt() failed: " << btn.errorMessage << "\n";
        return 1;
    }

    while (running.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200));

    btn.stopInterrupt();
    btn.clean();
}
```
👉 Full example: [`ex2.cpp`](ex2.cpp)

---

## 🧠 Key Concepts

**Polarity vs. Bias (AUXI)**
- `mode = 1` → active‑high (raw 1 = logical HIGH)  
- `mode = 0` → active‑low  (raw 0 = logical HIGH)  
- `bias = 0` → no bias  
- `bias = 1` → pull‑down (if supported)  
- `bias = 2` → pull‑up   (if supported)

**Cached vs. live reads**
- `AUX::value()` returns the current sampled level (works for both AUXO and AUXI).
- `AUXI::read()` queries hardware and updates internal cache after applying polarity.
- `AUXI::get()` returns the last cached logical state without touching hardware.

**Cleanup**
- Always call `clean()` to release the line and close the chip handle.
- `AUX::clean()` makes a **best effort** to leave the line as **input** before release.

**Interrupts & Debounce**
- `beginInterrupt()` requests kernel edge events and spawns a lightweight polling thread.
- Debounce is time‑based in the library. Hardware glitches faster than your `debounce_us`
  will be filtered in software.

---

## 🔍 API Overview (high level)

### `class AUX` (base)
- `std::string errorMessage;`
- `bool clean();` — release line/chip; best‑effort revert to input
- `int value() const;` — sampled line level (1/0, or −1 on error)

### `class AUXO : public AUX`
- `AUXO(const char* chip, unsigned line, uint8_t mode=1);`
- `bool begin();`
- `void on(); void off(); void toggle();`

### `class AUXI : public AUX`
- `AUXI(const char* chip, unsigned line, uint8_t mode=1, uint8_t bias=0);`
- `bool begin();`
- `bool read();` — returns logical state; on error keeps previous and sets `errorMessage`
- `bool get() const;` — cached logical state
- `void EXTI_Callback();` — placeholder for external IRQ integration
- `enum class Edge { Both, Rising, Falling };`
- `bool beginInterrupt(Edge, uint32_t debounce_us=1000, GpioCallback cb=nullptr);`
- `void stopInterrupt();`
- `bool isInterruptRunning() const;`

---

## 🧩 Compatibility & Notes

- Works with **libgpiod v1.x** APIs (`gpiod_line_*` functions). For v2 you must port the
  requests/events calls (names & types changed).
- Bias flags require a reasonably new v1.x (e.g., 1.5+). If not available, the code falls
  back to `gpiod_line_request_input()` and the bias request may be ignored by older kernels.
- On some platforms, reading back an output’s level depends on hardware support.
- Make sure the selected `line_offset` matches your board’s numbering scheme
  (it’s **not** necessarily the header pin number). Use `gpioinfo` to inspect lines.

---

## 🧯 Troubleshooting

- **`Failed to open chip.`** — Check path (`/dev/gpiochip0`), permissions, or existence of character device.
- **`Failed to get line at offset N`** — The line number is wrong or already requested by another process.
- **`Request edge events failed.` / `Request line as input/output failed.`** — Another process holds the line, or your user lacks permission.
- **No interrupts firing** — Ensure correct edge selection, wiring, pull configuration, and that your source actually toggles.
- **Bounces/noise** — Increase `debounce_us` or add hardware debouncing.

---

## 📜 License

**MIT License** — use freely in personal and commercial projects.

---

## 🙌 Acknowledgements

Powered by **libgpiod** and the Linux GPIO character device API.
