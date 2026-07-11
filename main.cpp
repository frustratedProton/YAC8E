#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

// emulating the chip8 architecture
struct Chip8 {
  std::array<uint8_t, 4096> memory{}; // 4KB RAM
  std::array<uint8_t, 16> V{};        // 16 registers from V0-VF
  uint16_t I{};                       // Index register
  uint16_t PC{};                      // Program Counter
  std::array<uint16_t, 16> stack{};   // 16 levels of stack
  uint8_t SP{};                       // Stack Pointer
  uint8_t delay_timer{};              // used for timing (duh)
  uint8_t sound_timer{};              // plays a beep while greater than zero
  std::array<uint8_t, 64 * 32> display{}; // 64*32 resolution, monochrome
  std::array<uint8_t, 16> key{};          // hex keypad with 16 keys
};

void init(Chip8& chip8) {
    chip8 = {};
    chip8.PC = 0x200;   // init program counter to correct memory block
}


int main() {

  // setup graphics
  // setup input

  // init chip8

  // emulation loop

  return 0;
}