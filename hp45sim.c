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
#include <string.h>
#include "hp45sim.h"

/* Private variables ---------------------------------------------------------*/
static const uint16_t ROM[2048] = {
  #include "hp45rom.c"
};
static const reg_t zero = {
  .nibble = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
static reg_t temp;

/* Private function prototypes -----------------------------------------------*/
static void word_select(hp45inst_t* instance, uint8_t *s, uint8_t *e);
static void mov(hp45inst_t* instance, reg_t *dst, const reg_t *src);
static void exch(hp45inst_t* instance, reg_t *r1, reg_t *r2);
static void shift(hp45inst_t* instance, reg_t *r, uint8_t left);
static void add(hp45inst_t* instance, const reg_t *x, const reg_t *y, reg_t *z);
static void sub(hp45inst_t* instance, const reg_t *x, const reg_t *y, reg_t *z);
static void set1(hp45inst_t* instance, reg_t *r);
static void ifge(hp45inst_t* instance, const reg_t *r1, const reg_t *r2);
static void ifeq0(hp45inst_t* instance, const reg_t *r);

/* Public functions  ---------------------------------------------------------*/
/**
  * @brief  Calculate start and end index according to word-select field.
  * @param  instance: HP-45 memory object
  * @param  s: pointer to store start index
  * @param  e: pointer to store end index
  * @retval None
  */
static void word_select(hp45inst_t* instance, uint8_t *s, uint8_t *e)
{
  switch(instance->ws & 7){
    case 0: *s = *e = instance->P; break;    // 0 = p (The nibble indicated by P register)
    case 1: *s = 3; *e = 12; break;          // 1 = m (mantissa)
    case 2: *s = 0; *e = 2; break;           // 2 = x (exponent)
    case 3: *s = 0; *e = 13; break;          // 3 = w (word (entire register))
    case 4: *s = 0; *e = instance->P; break; // 4 = wp(word up to and including P register)
    case 5: *s = 3; *e = 13; break;          // 5 = ms(mantissa and sign)
    case 6: *s = 2; *e = 2; break;           // 6 = xs(exponent sign)
    case 7: *s = 13; *e = 13; break;         // 7 = s ((mantissa) sign)
  }
}

/**
  * @brief  Move (copy) the specified field of a register to another.
  * @param  instance: HP-45 memory object
  * @param  dst: pointer to destination register
  * @param  src: pointer to source register
  * @retval None
  */
static void mov(hp45inst_t* instance, reg_t *dst, const reg_t *src)
{
  uint8_t i, s, e;

  word_select(instance, &s, &e);
  for(i = s; i <= e; i++){
    dst->nibble[i] = src->nibble[i];
  }
}

/**
  * @brief  Exhchange the specified field of 2 registers.
  * @param  instance: HP-45 memory object
  * @param  r1: pointer to one register
  * @param  r2: pointer to the other register
  * @retval None
  */
static void exch(hp45inst_t* instance, reg_t *r1, reg_t *r2)
{
  uint8_t i, s, e, t;

  word_select(instance, &s, &e);
  for(i = s; i <= e; i++){
    t = r1->nibble[i];
    r1->nibble[i] = r2->nibble[i];
    r2->nibble[i] = t;
  }
}

/**
  * @brief  Shift the specified field of a register.
  * @param  instance: HP-45 memory object
  * @param  r: pointer to register
  * @param  left: direction. non-zero for left and 0 for right.
  * @retval None
  */
static void shift(hp45inst_t* instance, reg_t *r, uint8_t left)
{
  uint8_t i, s, e;

  word_select(instance, &s, &e);
  if(left){
    for(i = e; i > s; i--){
      r->nibble[i] = r->nibble[i-1];
    }
    r->nibble[s] = 0;
  }else{
    for(i = s; i < e; i++){
      r->nibble[i] = r->nibble[i+1];
    }
    r->nibble[e] = 0;
  }
}

/**
  * @brief  Perform BCD addition z = x+y on the specified field of 3 registers.
  * @param  instance: HP-45 memory object
  * @param  x, y: pointer to operand registers
  * @param  z: pointer to result register
  * @retval None
  */
static void add(hp45inst_t* instance, const reg_t *x, const reg_t *y, reg_t *z)
{
  uint8_t a, b, c, i, s, e, cy = 0;

  word_select(instance, &s, &e);
  for(i = s; i <= e; i++){
    a = x->nibble[i];
    b = y->nibble[i];
    c = a+b+cy;
    if(c >= 10){
      cy = 1;
      c -= 10;
    }else{
      cy = 0;
    }
    z->nibble[i] = c;
  }
  instance->CY = cy;
}

/**
  * @brief  Perform BCD subtraction z = x-y on the specified field of 3 registers.
  * @param  instance: HP-45 memory object
  * @param  x, y: pointer to operand registers
  * @param  z: pointer to result register
  * @retval None
  */
static void sub(hp45inst_t* instance, const reg_t *x, const reg_t *y, reg_t *z)
{
  uint8_t a, b, c, i, s, e, cy = 0;

  word_select(instance, &s, &e);
  for(i = s; i <= e; i++){
    a = x->nibble[i];
    b = y->nibble[i];
    c = a-b-cy;
    if(c & 0x80){
      cy = 1;
      c += 10;
    }else{
      cy = 0;
    }
    z->nibble[i] = c;
  }
  instance->CY = cy;
}

/**
  * @brief  Set value of 1 to the specified field of a register.
  * @param  instance: HP-45 memory object
  * @param  r: pointer to the register
  * @retval None
  */
static void set1(hp45inst_t* instance, reg_t *r)
{
  uint8_t i, s, e;

  word_select(instance, &s, &e);
  r->nibble[s] = 1;
  for(i = s+1; i <= e; i++){
    r->nibble[i] = 0;
  }
}

/**
  * @brief  Perform r1> = r2 comparison on the specified field of 2 registers.
            Carry flag is cleared if r1> = r2, or set if r1<r2.
  * @param  instance: HP-45 memory object
  * @param  r1, r2: pointer to operand registers
  * @retval None
  */
static void ifge(hp45inst_t* instance, const reg_t *r1, const reg_t *r2)
{
  uint8_t a, b, i, s, e;

  word_select(instance, &s, &e);
  for(i = e; ; i--){
    a = r1->nibble[i];
    b = r2->nibble[i];
    if(a > b){
      break;
    }else if(a < b){
      instance->CY = 1;
      break;
    }
    if(i == s)break;
  }
}

/**
  * @brief  Perform r =  = 0 comparison on the specified field of a register.
            Carry flag is cleared if r =  = 0, or set if r! = 0.
  * @param  instance: HP-45 memory object
  * @param  r: pointer to operand register
  * @retval None
  */
static void ifeq0(hp45inst_t* instance, const reg_t *r)
{
  uint8_t i, s, e;

  word_select(instance, &s, &e);
  for(i = s; i<= e; i++){
    if(r->nibble[i]){
      instance->CY = 1;
      break;
    }
  }
}

/**
  * @brief  Decode type 2 (arithmatic/register) instructions (with opcode of xxxx_xxxx_10).
  * @param  instance: HP-45 memory object
  * @param  opcode: bit 2 to 9 of opcode
  * @retval int: negative value for undefined opcode.
  */
int opcode10(hp45inst_t* instance, uint8_t opcode)
{
  instance->ws = opcode & 7;
  switch(opcode>>3){
    /* === 1) clear === */
    case 23: // 0->A
      mov(instance, &instance->A, &zero);
      break;
    case 1:  // 0->B
      mov(instance, &instance->B, &zero);
      break;
    case 6:  // 0->C 
      mov(instance, &instance->CX, &zero);
      break;
    /* === 2) transfer/exchange === */
    case 9:  // A->B
      mov(instance, &instance->B, &instance->A);
      break;
    case 4:  // B->C
      mov(instance, &instance->CX, &instance->B);
      break;
    case 12: // C->A
      mov(instance, &instance->A, &instance->CX);
      break;
    case 25: // A<->B
      exch(instance, &instance->A, &instance->B);
      break;
    case 17: // B<->C
      exch(instance, &instance->B, &instance->CX);
      break;
    case 29: // C<->A
      exch(instance, &instance->A, &instance->CX);
      break;
    /* === 3) add/subtract === */
    case 14: // A+C->C
      add(instance, &instance->A, &instance->CX, &instance->CX);
      break;
    case 10: // A-C->C
      sub(instance, &instance->A, &instance->CX, &instance->CX);
      break;
    case 28: // A+B->A
      add(instance, &instance->A, &instance->B, &instance->A);
      break;
    case 24: // A-B->A
      sub(instance, &instance->A, &instance->B, &instance->A);
      break;
    case 30: // A+C->A
      add(instance, &instance->A, &instance->CX, &instance->A);
      break;
    case 26: // A-C->A
      sub(instance, &instance->A, &instance->CX, &instance->A);
      break;
    case 21: // C+C->C
      add(instance, &instance->CX, &instance->CX, &instance->CX);
      break;
    /* === 4) compare === */
    case 0:  // 0-B
      ifeq0(instance, &instance->B);
      break;
    case 13: // 0-C
      ifeq0(instance, &instance->CX);
      break;
    case 2:  // A-C
      ifge(instance, &instance->A, &instance->CX);
      break;
    case 16: // A-B
      ifge(instance, &instance->A, &instance->B);
      break;
    case 19: // A-1
      set1(instance, &temp);
      ifge(instance, &instance->A, &temp);
      break;
    case 3:  // C-1
      set1(instance, &temp);
      ifge(instance, &instance->CX, &temp);
      break;
    /* === 5) complement === */
    case 5: // 0-C->C
      sub(instance, &zero, &instance->CX, &instance->CX);
      break;
    case 7: // 0-C-1->C
      sub(instance, &zero, &instance->CX, &instance->CX);
      set1(instance, &temp);
      sub(instance, &instance->CX, &temp, &instance->CX);
      break;
    /* === 6) increment === */
    case 31: // A+1->A
      set1(instance, &temp);
      add(instance, &instance->A, &temp, &instance->A);
      break;
    case 15: // C+1->C
      set1(instance, &temp);
      add(instance, &instance->CX, &temp, &instance->CX);
      break;
    /* === 7) decrement === */
    case 27: // A-1->A
      set1(instance, &temp);
      sub(instance, &instance->A, &temp, &instance->A);
      break;
    case 11: // C-1->C
      set1(instance, &temp);
      sub(instance, &instance->CX, &temp, &instance->CX);
      break;
    /* === 8) shift === */
    case 22: // shift A right
      shift(instance, &instance->A, 0);
      break;
    case 20: // shift B right
      shift(instance, &instance->B, 0);
      break;
    case 18: // shift C right
      shift(instance, &instance->CX, 0);
      break;
    case 8:  // shift A left
      shift(instance, &instance->A, 1);
      break;
  }

  return 0;
}

/**
  * @brief  Decode type 3 (status operation) instructions (with opcode of xxxx_xx01_00).
  * @param  instance: HP-45 memory object
  * @param  opcode: bit 2 to 9 of opcode
  * @retval int: negative value for undefined opcode.
  */
int opcode0100(hp45inst_t *instance, uint8_t opcode)
{
  const uint8_t N = opcode>>4;

  switch((opcode>>2) & 0x03){
    case 0: // set flag N
      if(N >= 12)
        return -2;
      instance->S |= 1<<N;
      break;
    case 1: // interrogate flag N
      if(N >= 12)
        return -4;
      if(instance->S & (1<<N)){
        instance->CY = 1;
      }
      break;
    case 2: // reset flag N
      if(N >= 12)
        return -7;
      instance->S &= ~(1<<N);
      break;
    case 3: // clear all flags (N=0000)
      if(N)return -10;
      instance->S = 0;
      break;
  }

  return 0;
}

/**
  * @brief  Decode type 4 (pointer operation) instructions (with opcode of xxxx_xx11_00).
  * @param  instance: HP-45 memory object
  * @param  opcode: bit 2 to 9 of opcode
  * @retval int: negative value for undefined opcode.
  */
int opcode1100(hp45inst_t *instance, uint8_t opcode)
{
  const uint8_t P = opcode>>4;

  switch((opcode>>2) & 0x03){
    case 0: // set pointer to P
      instance->P = P;
      break;
    case 2: // interrogate if pointer at P
      instance->CY = (instance->P == P);
      break;
    case 1: // decrement pointer (P=XXXX i.e. don't care)
      instance->P--;
      break;
    case 3: // increment pointer (P=XXXX i.e. don't care)
      instance->P++;
      break;
  }

  return 0;
}

/**
  * @brief  Decode type 5 (data entry/display) instructions (with opcode of xxxx_xx10_00).
  * @param  instance: HP-45 memory object
  * @param  opcode: bit 2 to 9 of opcode
  * @retval int: negative value for undefined opcode.
  */
int opcode1000(hp45inst_t *instance, uint8_t opcode)
{
  const uint8_t N = opcode>>4;

  switch((opcode>>2) & 0x03){
    case 0: // 16 available instructions
      return -1;
    case 1: // enter 4 bit code N into C at P (load constant)
      if(N >= 10)return -2;
      if(instance->P < 14)
        instance->CX.nibble[instance->P] = N;
      instance->P--;
      break;
    case 2:
    case 3:
      switch(N){
        case 0: // display toggle
          instance->DispOn = !instance->DispOn;
          break;
        case 2: // exchange memory, C->M->C
          memcpy(&temp, &instance->CX, sizeof(reg_t));
          memcpy(&instance->CX, &instance->M, sizeof(reg_t));
          memcpy(&instance->M, &temp, sizeof(reg_t));
          break;
        case 4: // up stack, C->C->D->E->F
          memcpy(&instance->FT, &instance->EZ, sizeof(reg_t));
          memcpy(&instance->EZ, &instance->DY, sizeof(reg_t));
          memcpy(&instance->DY, &instance->CX, sizeof(reg_t));
          break;
        case 6: // down stack, F->F->E->D->A
          memcpy(&instance->A, &instance->DY, sizeof(reg_t));
          memcpy(&instance->DY, &instance->EZ, sizeof(reg_t));
          memcpy(&instance->EZ, &instance->FT, sizeof(reg_t));
          break;
        case 8: // display off
          instance->DispOn = 0;
          break;
        case 10: // recall memory, M->M->C
          memcpy(&instance->CX, &instance->M, sizeof(reg_t));
          break;
        case 11: //
          if(instance->DataAddr < 10)
            memcpy(&instance->CX, &instance->RAM[instance->DataAddr], sizeof(reg_t));
          break;
        case 12: // rotate down, C->F->E->D->C
          memcpy(&temp, &instance->CX, sizeof(reg_t));
          memcpy(&instance->CX, &instance->DY, sizeof(reg_t));
          memcpy(&instance->DY, &instance->EZ, sizeof(reg_t));
          memcpy(&instance->EZ, &instance->FT, sizeof(reg_t));
          memcpy(&instance->FT, &temp, sizeof(reg_t));
          break;
        case 14: // clear all registers, 0->A, B, C, D, E, F, M
          memset(instance, 0, sizeof(reg_t)*7);
          break;
        case 1:
        case 5:
        case 9:
        case 13: return -3; // Is -> A
        case 3:
        case 7:
        case 15: return -4; // BCD -> C
      }
  }

  return 0;
}

/**
  * @brief  Decode type 6-10 (ROM select, misc, etc) instructions (with opcode of xxxx_xx00_00).
  * @param  instance: HP-45 memory object
  * @param  opcode: bit 2 to 9 of opcode
  * @retval int: negative value for undefined opcode.
  */
int opcode0000(hp45inst_t *instance, uint8_t opcode)
{
  const uint16_t N = opcode>>5;

  if(opcode & 0x04){// TYPE 6
    switch((opcode>>3) & 0x03){
      case 0: // ROM select, one of 8 as specified in bits 9-7
        instance->PC = (instance->PC & 0x0FF) | (N<<8);
        break;
      case 1: // subroutine return
        instance->PC = (instance->PC & 0xF00) | instance->LR;
        break;
      case 2:
        if(N & 1){ // keyboard entry
          instance->PC = (instance->PC & 0xF00) | instance->KeyCode;
        }else{ // external key code entry
          return -1;
        }
        break;
      case 3:
        if((N & 0x5) == 0x4){ // send address from C to data storage circuit
          instance->DataAddr = instance->CX.nibble[12];
        }else if(N == 0x5){ // send data from C into auxiliary data storage circuit
          if(instance->DataAddr < 10)
            memcpy(&instance->RAM[instance->DataAddr], &instance->CX, sizeof(reg_t));
        }else{
          return -2;
        }
        break;
    }
  }else if(opcode & 0x08){// TYPE 7
    return -3;
  }else if(opcode & 0x10){// TYPE 8
    return -4;
  }else{// TYPE 9 & 10
    if(opcode){
      return -5;
    }else{
      // NOP
    }
  }

  return 0;
}

/* Public functions  ---------------------------------------------------------*/
/**
  * @brief  Notice VM that a key is pressed.
  * @param  instance: HP-45 memory object
  * @param  keycode: HP-45 native key code.
            Undefined key code may result in unexpected behavior.
  * @retval None
  */
void key_down(hp45inst_t* instance, uint8_t keycode)
{
  instance->KeyCode = keycode;
  instance->keydown = 1;
}

/**
  * @brief  Notice VM that a key is released.
  * @param  instance: HP-45 memory object
  * @retval None
  */
void key_up(hp45inst_t* instance)
{
  instance->keydown = 0;
}

/**
  * @brief  Initialize HP-45 memory to initial state (all-zero).
  * @param  instance: HP-45 memory object
  * @retval None
  */
void hp45_init(hp45inst_t *instance)
{  
  memset(instance, 0, sizeof(hp45inst_t));
}

/**
  * @brief  Perform simulation for 1 word-cycle (i.e. run 1 step)
            To simulate the speed of a real HP-45 machine,
            call this function every 286us (35 cycles per 10ms).
  * @param  instance: HP-45 memory object
  * @retval None
  */
int hp45_run(hp45inst_t *instance)
{
  const uint16_t opcode = ROM[instance->PC];

  instance->PC = (instance->PC & 0xF00) | ((instance->PC+1) & 0xFF);
  instance->S = (instance->S & 0xFFFE) | (uint16_t)instance->keydown;
  switch(opcode&0x003){
    /* instruction type 3-10 */
    case 0: 
      instance->CY = 0;
      switch((opcode>>2) & 0x03){
        /* instruction type 6-10: ROM select, misc */
        case 0: return opcode0000(instance, opcode>>2);
        /* instruction type 3: status operations */
        case 1: return opcode0100(instance, opcode>>2);
        /* instruction type 5: data entry/display */
        case 2: return opcode1000(instance, opcode>>2);
        /* instruction type 4: pointer operations */
        case 3: return opcode1100(instance, opcode>>2);
      }
    /* instruction type 1: jump subroutine */
    case 1:
      instance->LR = instance->PC;
      instance->PC = (instance->PC & 0xF00) | (opcode>>2);
      instance->CY = 0;
      return 0;
    /* instruction type 2: arithmetic/register */
    case 2:
      instance->CY = 0;
      return opcode10(instance, opcode>>2);
    /* instruction type 1: conditional branch */
    case 3:
      if(!instance->CY){
        instance->PC = (instance->PC & 0xF00) | (opcode>>2);
      }
      instance->CY = 0;
      return 0;
  }

  return -1;
}
