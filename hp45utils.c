/* hp45sim - emulator of HP-45 scientific calculator.
 * Copyright (C) 2022 reiyawea
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "hp45sim.h"
#include "hp45utils.h"

#define BIT_NONE 0x00
#define BIT_A 0x01
#define BIT_B 0x02
#define BIT_C 0x04
#define BIT_D 0x08
#define BIT_E 0x10
#define BIT_F 0x20
#define BIT_G 0x40
#define BIT_H 0x80

const uint8_t SevenSegmentTable[] = {
  BIT_F | BIT_E | BIT_D | BIT_C | BIT_B | BIT_A,
  BIT_C | BIT_B,
  BIT_G | BIT_E | BIT_D | BIT_B | BIT_A,
  BIT_G | BIT_D | BIT_C | BIT_B | BIT_A,
  BIT_G | BIT_F | BIT_C | BIT_B,
  BIT_G | BIT_F | BIT_D | BIT_C | BIT_A,
  BIT_G | BIT_F | BIT_E | BIT_D | BIT_C | BIT_A,
  BIT_C | BIT_B | BIT_A,
  BIT_G | BIT_F | BIT_E | BIT_D | BIT_C | BIT_B | BIT_A,
  BIT_G | BIT_F | BIT_D | BIT_C | BIT_B | BIT_A,
}

/* Public functions  ---------------------------------------------------------*/
/**
 * @brief  Convert HP-45 registers into LED scan buffer.
 * Register A held the BCD digits for the display while register B was used as a mask register.
 * Nibbles in register B were decoded such that 9 = Digit Off, 0 = Digit On, and 2 = Decimal Point On.
 * Numbers were formatted in 10’s complement notation with 9’s in the sign digits indicating negative sign.
 * @param  instance: HP-45 memory object
 * @param  disp_buf: LED scan buffer, whose length should be at least 14.
 * @retval None
 */
void make_display(hp45inst_t *instance, uint8_t *disp_buf)
{
  int i;
  uint8_t digit;

  for (i = 13; i >= 0; i--){
    if (instance->B.nibble[i] == 9){
      *disp_buf++ = BIT_NONE;
      continue;
    }
    if (i == 13 || i == 2){
      digit = (instance->A.nibble[i] == 9) ? BIT_G : BIT_NONE;
    }else{
      digit = SevenSegmentTable[instance->A.nibble[i]];
    }
    if (instance->B.nibble[i] == 2){
      digit |= BIT_H;
    }
    *disp_buf++ = digit;
  }
}
