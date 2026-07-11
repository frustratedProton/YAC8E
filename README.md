# Yet Another Chip-8 Emulator [WIP]

A simple CHIP-8 emulator written in `C++`. This project was built to learn about emulator development and computer architecture by implementing the CHIP-8 virtual machine from scratch.

## Architecture

### Memory

1. Had 4096 (`0x1000`) memory locations, all of which are 8 bits  
2. Chip-8 interpreter occupies the first 512 bytes of memory on these machines  

### Registers

1. Had 16 8-bit registers named V0 to VF.  
2. VF is also used as flag (carry/collision)  
3. Address register `I` is 12 bits wide and used with several opcodes.  
4. One program counter PC  
5. One stack pointer SP

### Stack

1. ONLY used to return addresses when subroutines are called.  
2. Size: 16 levels  
3. Used by:  
- `2NNN` (CALL)  
- `00EE` (RETURN)

### Timers

1. 2 8 bit timers  
2. Delay Timer (DT)  
    - Counts down at 60HZ  
    - Used for timing  
3. Sound Timer (ST)  
    - Counts down at 60HZ  
    - Plays a beep _while_ greater than zero

### Input

1. CHIP-8 has a hexadecimal keypad with 16 keys  
     
   1 2 3 C  
   4 5 6 D  
   7 8 9 E  
   A 0 B F  
2. 8, 4, 6 and 2 keys are typically used for directional input.  
3. Opcodes are used to detect inputs

### Display

1. Original Display:  
   - Resolution: 64x32  
   - Monochrome  
2. Sprites:  
1. 8 pixels wide  
2. 1-15 pixel tall  
3. Drawn using XOR (what??)  
4. If drawing erases a pixel, VF \= 1

### Opcodes

1. Has 35 opcodes which are 2 bytes long and stored in big-endian  
     
### REFERENCES
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
https://en.wikipedia.org/wiki/CHIP-8
https://tobiasvl.github.io/blog/write-a-chip-8-emulator/

