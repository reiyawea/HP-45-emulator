typedef union{
  struct{
    uint8_t X[2];  // 2 digit exponent
    uint8_t XS;    // sign of exponent
    uint8_t M[10]; // 10 digit mantissa
    uint8_t S;     // sign of mantissa
  } digit;
  uint8_t nibble[14], padding[2];
} reg_t;

typedef struct{
  reg_t A, B;       // General purpose registers for math and scratchpad use
  reg_t CX;         // Like A and B but also dedicated to memory reads and writes andtransfers to M
                    // The C register also contained the value displayed in the X register.
  reg_t DY, EZ, FT; // These registers held the user stack levels Y, Z, and T.
                    // These registers were accessed by rotating them down or by using STACK -> reg style commands.
  reg_t M;          // A scratchpad register which only supported transfers to and from the Cregister. No math etc.
  reg_t RAM[10];    // auxiliary data storage circuit
  uint16_t PC;      // 11-bit program counter
  uint16_t S;       // 12 bits of programmable status  
  uint8_t LR;       // return address of subroutine call
  uint8_t KeyCode;  // Stores key code that keyboard scanning circuit generated.
  uint8_t P;        // a 4-bit "pointer" register allows subsets of registers to be accessed
  uint8_t DataAddr; // the address that register C could be read from or written to
  uint8_t ws;       // stores word-select field of current instruction. not an actual part in HP-45.    
  uint8_t CY;       // carry flag
  uint8_t keydown;  // Store key state that keyboard scanning circuit generated
  uint8_t DispOn;   // LED display ON/OFF control bit
} hp45inst_t;

void key_down(hp45inst_t*, uint8_t);
void key_up(hp45inst_t*);
void hp45_init(hp45inst_t*);
int hp45_run(hp45inst_t*);
