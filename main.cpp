#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>

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

constexpr std::array FONT_SET{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void init(Chip8 &chip8) {
  chip8 = {};
  chip8.PC = 0x200; // init program counter to correct memory block
  std::copy(FONT_SET.begin(), FONT_SET.end(), chip8.memory.begin() + 0x000);
}

bool loadRom(Chip8 &chip8, const std::string &filename) {
  std::ifstream ifs{filename, std::ios::binary | std::ios::ate};

  if (!ifs.is_open()) {
    std::cerr << "Failed to open ROM file: " << filename << '\n';
    return false;
  }

  const auto size{ifs.tellg()};
  ifs.seekg(0, std::ios::beg);

  // check if ROM wether ROM can fit in memory
  constexpr auto max_rom_size{4096 - 0x200};
  if (size > max_rom_size) {
    std::cerr << "ROM too larget to fit in memory (max" << max_rom_size
              << " bytes)\n";
    return false;
  }

  // read file int buffer
  std::vector<uint8_t> buf(size);
  if (!ifs.read(reinterpret_cast<char *>(buf.data()), size)) {
    std::cerr << "Failed to read ROM\n";
    return false;
  }

  std::copy(buf.begin(), buf.end(), chip8.memory.begin() + 0x200);
  std::cout << "Loaded " << size << " bytes into memory\n";

  return true;
}

// fetch/decode/execute loop
void emulateCycle(Chip8 &chip8) {
  // fetch operation
  // an instruction is 2 bytes
  // get 2 succeessive bytes from memory
  // and combine them into one 16bit instruction
  const uint16_t opcode{static_cast<uint16_t>((chip8.memory[chip8.PC] << 8) |
                                              chip8.memory[chip8.PC + 1])};

  // move PC by 2 for next opcode
  chip8.PC += 2;

  // decode and exec loop
  switch (opcode & 0xF000) {
  case 0x0000:
    // TODO: implement 0x000
    break;
  case 0x1000:
    // TODO: implement jump
    break;
  case 0x6000:
    // TODO: implement set register
    break;

  default:
    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << '\n';
    break;
  }
}

int main() {

  Chip8 chip8;

  init(chip8);

  if (!loadRom(chip8, "roms/IBM Logo.ch8"))
    return 1;

  std::cout << "First opcode: " << std::hex << std::uppercase
            << static_cast<int>(chip8.memory[0x200])
            << static_cast<int>(chip8.memory[0x200 + 1]) << '\n';

  return 0;
}