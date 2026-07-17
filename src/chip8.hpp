#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>

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
  //   std::array<uint8_t, 64 * 32> display{}; // 64*32 resolution, monochrome
  std::array<uint8_t, 16> key{};           // hex keypad with 16 keys
  std::array<uint8_t, 128 * 64> display{}; // super-chip has display of 128 * 64

  // SUPER-CHIP stuff
  bool hires{false}; // switch between hires and og chip-8 display
  std::array<uint8_t, 8> rpl_flags{}; // RPL user flags
};

void init(Chip8 &chip8);
bool loadRom(Chip8 &chip8, const std::string &filename);
void scrollDown(Chip8 &chip8, uint8_t pixels);
void scrollRight(Chip8 &chip8);
void scrollLeft(Chip8 &chip8);
void emulateCycle(Chip8 &chip8);