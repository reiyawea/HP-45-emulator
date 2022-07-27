# HP-45 emulator
Simulates the CPU of an HP-45 calculator.
Can be ported to embedded project like 8051, AVR, ARM CM3, etc.

# Features
* Written in plain C: platform independent.
* All memory and CPU states are wrapped in struct: supports multi-instance.
* Easy to use: only 5 functions in user interface.
  * `hp45_key_down` and `hp45_key_up`: notify CPU that a key is pressed/released.
  * `hp45_init`: initializes instance struct.
  * `hp45_run`: performs a single step.

# Usage
To simulate the HP-45 at actual speed:
1. call `hp45_init` once
2. call `hp45_run` once per 286us (precisely speaking, 35 times per 10ms).
3. (optional) call `make_display` to convert CPU registers into display buffer for LED scanning.
