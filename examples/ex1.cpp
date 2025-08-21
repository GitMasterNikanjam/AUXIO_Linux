// g++ -std=c++17 -O2 -o demo main.cpp AUXIO.cpp -lgpiod


// Output (active-high)
AUXO led("/dev/gpiochip0", 27, /*active-high*/ 1);
if (!led.begin()) { std::cerr << led.errorMessage << "\n"; }
led.on();
led.toggle();
led.off();
led.clean();

// Input with pull-up
AUXI btn("/dev/gpiochip0", 17, /*pull-up*/ 2);
if (!btn.begin()) { std::cerr << btn.errorMessage << "\n"; }
bool pressed = btn.read();
btn.clean();
