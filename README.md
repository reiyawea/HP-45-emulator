# HP-45 emulator
Simulates the processor of an HP-45 calculator.
Can be ported to embedded project like 8051, AVR, STM32 etc.

# Features
* Written in plain C: platform ndependent.
* All memory and CPU states are wrapped in struct: supports multi-instance.
* Easy to use: only 4 functions in user interface.
  * `hp45_key_down` and `hp45_key_up`: notify CPU that a key is pressed/released.
  * `hp45_init`: initializes instance struct.
  * `hp45_run`: performs a single step.

# Usage
To simulate the HP-45 at actual speed:
1. call `hp45_init` once
2. call `hp45_run` once per 286us (precisely speaking, 35 times per 10ms).
