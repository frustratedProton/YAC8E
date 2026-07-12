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
  // opcode is 16 bits (2 bytes)
  // get 2 succeessive bytes from memory
  // and combine them into one 16bit instruction
  const uint16_t opcode{static_cast<uint16_t>((chip8.memory[chip8.PC] << 8) |
                                              chip8.memory[chip8.PC + 1])};

  // move PC by 2 for next opcode
  chip8.PC += 2;

  // decode and exec loop
  // the first nibble deterines the instruction family
  // remaining nibbles are interpreted as X, Y, N, NN or NNN
  // nibble or half-bytes are first hexadecimal numbers
  const uint16_t nnn{
      static_cast<uint16_t>(opcode & 0x0FFF)}; // lowest 12 bits (address)
  const uint8_t x{
      static_cast<uint8_t>((opcode & 0x0F00) >> 8)}; // 2nd nibble (register VX)
  const uint8_t y{
      static_cast<uint8_t>((opcode & 0x00F0) >> 4)}; // 3rd nibble (register VY)
  const uint8_t n{static_cast<uint8_t>(opcode & 0x000F)};  // last nibble
  const uint8_t nn{static_cast<uint8_t>(opcode & 0x00FF)}; // last byte

  switch (opcode & 0xF000) {
  case 0x0000:
    if (opcode == 0x00E0) {
      // 00E0 - clear screen
      // turn pixel off
      chip8.display.fill(0);
      std::cout << "CLS\n";
    } else if (opcode == 0x00EE) {
      // 00EE - return from subroutine
      chip8.SP--;
      chip8.PC = chip8.stack[chip8.SP];
      std::cout << "RET\n";
    }
    break;

  case 0x1000:
    // 1NNN - JUMP to address NNN
    chip8.PC = nnn;
    std::cout << "JP 0x" << std::hex << nnn << '\n';
    break;

  case 0x2000:
    // 2NNN - call subroutine at NNN
    // first push current PC to stack
    chip8.stack[chip8.SP] = chip8.PC;
    chip8.SP++;
    // set pc to nnn
    chip8.PC = nnn;
    std::cout << "CALL 0x" << std::hex << nnn << '\n';
    break;

  case 0x6000:
    // 6XNN - set VX to NN
    chip8.V[x] = nn;
    std::cout << "LD V" << std::hex << static_cast<int>(x) << ", 0x"
              << static_cast<int>(nn) << '\n';
    break;

  case 0x7000:
    // 7XNN - add NN to VX
    chip8.V[x] += nn;
    std::cout << "ADD V" << std::hex << static_cast<int>(x) << ", 0x"
              << static_cast<int>(nn) << '\n';
    break;

  case 0xA000:
    // ANNN - set I to NNN
    chip8.I = nnn;
    std::cout << "LD I, 0x" << std::hex << nnn << '\n';
    break;

  case 0xD000: {
    // DXYN - draw sprites at (VX, VY) with height N
    const uint8_t x_pos{static_cast<uint8_t>(chip8.V[x] & 63u)};
    const uint8_t y_pos{static_cast<uint8_t>(chip8.V[y] & 31u)};
    chip8.V[0xF] = 0;

    for (uint8_t row{0}; row < n; ++row) {
      const uint8_t sprite_byte{chip8.memory[chip8.I + row]};

      // stop if we reach bottom edge
      if (y_pos + row >= 32)
        break;

      for (uint8_t col{0}; col < 8; ++col) {

        if (x_pos + col >= 64)
          break;

        const uint8_t sprite_pixel{
            static_cast<uint8_t>(sprite_byte & (0x80u >> col))};

        if (sprite_pixel != 0) {
          const uint16_t pixel_index{
              static_cast<uint16_t>((y_pos + row) * 64 + (x_pos + col))};

          // if both sprite pixel and screen pixel are on
          // collision detected
          if (chip8.display[pixel_index] == 1)
            chip8.V[0xF] = 1;

          // XOR the pixel
          chip8.display[pixel_index] ^= 1;
        }
      }
    }

    std::cout << "DRW V" << std::hex << static_cast<int>(x) << ", V"
              << static_cast<int>(y) << ", " << static_cast<int>(n) << '\n';

    break;
  }

  default:
    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << '\n';
    break;
  }
}

void printDisplay(const Chip8 &chip8) {
  for (uint16_t y{0}; y < 32; ++y) {
    for (uint16_t x{0}; x < 64; ++x) {
      std::cout << (chip8.display[y * 64 + x] ? "█" : " ");
    }
    std::cout << '\n';
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

  for (int i{0}; i < 30; ++i)
    emulateCycle(chip8);

  printDisplay(chip8);

  return 0;
}