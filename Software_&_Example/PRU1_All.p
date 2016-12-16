// Data_PRU1.p program to read DR (data ready), OU (over/under flow), 
// and read the low byte of the data.
// Does the analysis for peak points. Peak >= 5 points and
// peak < 255 points
// Also reads the Power Off bit for shutdown.
// 
// Name:        Read Register:  Pin used:   Mode:   0 Value 1 Value
// Bit0         R31.t0          P8.45       6
// Bit1         R31.t1          P8.46       6
// Bit2         R31.t2          P8.43       6
// Bit3         R31.t3          P8.44       6
// Bit4         R31.t4          P8.41       6
// Bit5         R31.t5          P8.42       6
// Bit6         R31.t6          P8.39       6
// Bit7         R31.t7          P8.40       6
// DR           R31.t8          P8.27       6       NRDY     RDY
// OU           R31.t9          P8.29       6       OU       NOU
// SPARE        R30.t10         P8.28       6       (PULLED HIGH)     
// Stop         R31.t11         p8.30       6       STOP     RUN
//
//  Data: 16 bit number 0xnnnn 
//  Point data: Maximum, Position of maximum, width and number of saturated points.
// 
// Memory usage
//
// Baseline+Threshold Address @ 0x0000 0400
// Point Pointer Address @ 0x0000 0404
// STOP Address @ 0x0000408
//
// Rolling Baseline 1K buffer:
//  PRU1 DRAM 1K = 0x0400
//  Start address = 0x0000 0000
//  End address = 0x0000 03FF
//
// Rolling Data Buffer
//  Shared RAM 12K = 0x3000
//  Start address = 0x0001 0000
//  End Address= 0x0001 2FFF
//
// Raw Data Buffer
//  PRU0 RAM 1K = 0x0400
//  Start address = 0x0000 2000
//  End address = 0x0000 23FE
//


.origin 0                           // Start of program in PRU0 memory.
.entrypoint START                   // Entrypoint for the debugger.

#define MAXV 0xFFFF                 // Saturated 16 bit value.
#define MINV 0x0000                 // Zero value

#define PRU1_R31_VEC_VALID 32       // Allows notification of program completion
#define PRU_EVTOUT_1    4           // The event number that is sent - complete
#define CONST_PRUCFG C4
#define SPP 0x34

START: 
    MOV r1, 0x00026034              // Enable the scratch pad with PRU1 priority
    MOV r2, 0x00000003
    SBBO r2, r1, 0, 4
    
    MOV r0, 0x00000000              // This is the register shift and must stay 0
    MOV r1, 0x00000000              // Current Baseline value - r1.w0 = BL, r1.w2 = BLTH
    MOV r2, 0x00000000              // Current Peak Value -     r2.w0 = MAX, r2.w2 = POS
    MOV r3, 0x00000000              // Current Peak Value -     r3.w0 = SATD, r3.w2 = CT
    MOV r4, 0x00000000              // Current Point -          r4.w0 = PT
    MOV r5, 0x00000000              // Control value -          r5.w0 = type, r5.w2 = STOP
                                    // r5.t0 = 0 baseline =1 point
    MOV r6, 0x00010000              // Point pointer - init
    MOV r7, 0x00000000              // Baseline pointer - init
    MOV r8, 0x00000400              // Baseline + threshold addr - const
    MOV r9, 0x00000404              // Baseline addr - const
    MOV r10, 0x00000408             // Point pointer addr - const
    MOV r11, 0x0000040C             // STOP address - const
    MOV r12, 0x00000000             // Start of BL buffer - const
    MOV r13, 0x000003FE             // End of Baseline buffer - const
    MOV r14, 0x00010000             // Start of Point buffer - const
    MOV r15, 0x00012FF8             // End of Point buffer - const
    MOV r16, 0x00002000             // Start of raw data buffer on PRU0 mem
    MOV r17, 0x000023FE             // End of raw data buffer on PRU 0 mem
    MOV r18, 0x00002000             // Raw data pointer
    MOV r19, 0x00000000             // Stop value 0x0000 run, 0xFFFF Stop
    MOV r20, 0x00000000             // Take data CMD register .t0 take data, .t1 STOP
    MOV r21, 0x00000000             // Data register for high byte from PRU0
    MOV r22, 0x0000                 // Zero value for output
    
LOAD_NOSTOP:
    SBBO r19, r11, 0, 2             // Load a zero into the STOP location
    
SETPEAKPTR:
    SBBO r6, r10, 0, 4              // Write the pointer address out  
    
                                    // START of data loop 
READ_BLTH:                          // Get the latest Baseline + threshold value
    LBBO r1.w2, r8, 0, 2            // BLTH
    LBBO r1.w0, r9, 0, 2            // BL

WAIT_NDR1:                          // Wait for data not ready (LOW)
    QBBS WAIT_NDR1, R31.t8
    
WAIT_DR: 
    QBBC WAIT_DR, r31.t8            // Wait for data ready (low -> high)
    QBBC END, R31.t11               // Check STOP
    QBBS OU_FLOW, R31.t9            // Handle OU flow
    MOV r4.b0, r31.b0               // Read the low byte
    XOUT 10, r20, 4                 // Tell PRU0 to take data
    XIN 10, r21, 4                  // Read high byte
    MOV r4.b1, r21.b0               // put in data point 
    SBBO r4.w0, r18, 0, 2           // Write raw to buffer
    ADD r18, r18, 2                 // Increment pointer
    QBLE CHECK_PT, r17, r18         // Is the raw buffer full?
    MOV r18, r16                    // Restart buffer
    JMP CHECK_PT                    // Check for point or baseline
    
OU_FLOW:    
    QBBC READ_BLTH, r5.t0           // Last point BL, do nothing and start over
    MOV r4.w0, MAXV                 // Current value is saturated
    JMP CHECK_MAX                   // Check the max, don't subtract the baseline

CHECK_PT:                           // Determine if >= BL+TH
    QBGE PEAK_PT, r1.w2, r4.w0

    QBBS WRITE_PK, r5.t0            // Write the last peak out, miss this baseline point
    
STORE_BL:                           // Store as baseline
    SBBO r4.w0, r7, 0, 2            // Write BL to buffer
    ADD r7, r7, 2                   // Increment pointer
    QBLE READ_BLTH, r13, r7         // Is the BL buffer full?
    MOV r7, r12                     // Restart buffer
    JMP READ_BLTH                   // Go to start of loop
    
PEAK_PT: 
    SUB r4.w0, r4.w0, r1.w0         // Subtract the Baseline
    QBBS CHECK_MAX, r5.t0           // If the last point was a peak, Check Max
    SET r5.t0                       // It is a peak point
    MOV r2.w0, r4.w0                // put peak in max
    MOV r2.w2, 1                    // Start count
    MOV r3.w0, 0                    // Start saturated at 0
    MOV r3.w2, 1                    // Start POS at 1
    JMP READ_BLTH                   // LOOP Again.
    
CHECK_MAX:
    ADD r3.w2, r3.w2,1              // Increment the count
    QBGT SET_MAX, r2.w0, r4.w0      // Other values stay the same if the new 
                                    // is not greater
    JMP READ_BLTH                   // LOOP Again.
    
SET_MAX:
    MOV r2.w0, r4.w0                // Move the new peak into max
    MOV r2.w2, r3.w2                // move the count into the position
    JMP READ_BLTH
    
WRITE_PK: 
    CLR r5.t0                       // end of peak
    QBGT CLEAR, r3.w2, 5            // not enough points
    QBLT CLEAR, r3.w2, 255          // too many points
    SBBO r2, r6, 0, 8               // write out r2 and r3
    ADD r6, r6, 8                   // Increment the pointer
    SBBO r6, r10, 0, 4              // Write the pointer address out 
    QBLE CLEAR, r15, r6             // Is the PT buffer full?
    MOV r6, r14                     // Restart buffer
    
CLEAR:    
    MOV r2, 0x00000000              // restart point
    MOV r3, 0x00000000
    MOV r4, 0x00000000
    JMP READ_BLTH                   // LOOP Again.
    
END:
    MOV r19, MAXV                   // 0xFFFF
    SBBO r19, r11, 0, 2             // Stop program
    SET r20.t1                      //
    XOUT 14, r20, 4                 // Tell PRU0 to stop
	MOV	R31.b0, PRU1_R31_VEC_VALID | PRU_EVTOUT_1   // Notify Halt
    HALT                                            // Halt the program
    