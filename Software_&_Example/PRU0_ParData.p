// PRU0_ParData.p program to read the high byte
// of the data.
// 
// Name:        Read Register:  Pin used:   Mode:   0 Value 1 Value
// Bit0         R31.t0          P9.31       6
// Bit1         R31.t1          P9.29       6
// Bit2         R31.t2          P9.30       6
// Bit3         R31.t3          P9.28       6
// Bit4         R31.t4          P9.42       6
// Bit5         R31.t5          P9.27       6
// Bit6         R31.t6          P9.41       6
// Bit7         R31.t7          P9.25       6

.origin 0                           // Start of program in PRU0 memory.
.entrypoint START                   // Entrypoint for the debugger.

MOV r0, 0x00000000                   // r0 is the register offset, leave at 0

#define PRU1_R31_VEC_VALID 32       // Allows notification of program completion
#define PRU_EVTOUT_0    3           // The event number that is sent - complete

START:
    
    MOV r0, 0x00000000              // r0 is the register offset, leave at 0
    MOV r1, 0x00000000
    MOV r20, 0x00000000             // Init control register
    MOV r21, 0x00000000             // Init data register
   
CHECK_DATA:
    XIN 10, r20, 4                  // Get read or exit
    MOV r21.b0, r31.b0              // Read the high byte
    XOUT 10, r21, 4                 // Load the high byte
    QBBS END, r20.t1                // Exit if r20.t1 set
    JMP CHECK_DATA
    
END:
    MOV	 R31.b0, PRU1_R31_VEC_VALID | PRU_EVTOUT_0   // Notify Halt
    HALT                                             // Halt the program