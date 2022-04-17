# HP-45 emulator
Simulates the hardware of an HP-45 calculator. Can be ported to embedded project like STM32 etc.

# Features
* Written in plain C: Platform Independent.
* All memory and CPU states are wrapped in struct: Supports multi-instance.
* Easy to use: only 4 functions in user interface.
  * `hp45_key_down` and `hp45_key_up`: notify CPU that a key is pressed and released.
  * `hp45_init`: initializes instance struct.
  * `hp45_run`: performs a single step.

# Usage
To simulate the HP-45 at actual speed:
1. call `hp45_init` once
2. call `hp45_run` per 286us (precisely, 35 times per 10ms).
