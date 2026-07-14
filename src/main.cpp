#include <cstdlib>
#include <raylib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>

// add debug flag
#ifdef DEBUG
#define LOG(x) std::cout << x
#else
#define LOG(x)
#endif

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
      LOG("CLS\n");
    } else if (opcode == 0x00EE) {
      // 00EE - return from subroutine
      chip8.SP--;
      chip8.PC = chip8.stack[chip8.SP];
      LOG("RET\n");
    }
    break;

  case 0x1000:
    // 1NNN - JUMP to address NNN
    chip8.PC = nnn;
    LOG("JP 0x" << std::hex << nnn << '\n');
    break;

  case 0x2000:
    // 2NNN - call subroutine at NNN
    // first push current PC to stack
    chip8.stack[chip8.SP] = chip8.PC;
    chip8.SP++;
    // set pc to nnn
    chip8.PC = nnn;
    LOG("CALL 0x" << std::hex << nnn << '\n');
    break;

  case 0x3000:
    // 3XNN - skip if VX = NN
    if (chip8.V[x] == nn)
      chip8.PC += 2;
    break;

  case 0x4000:
    // 4XNN - skip if VX != NN
    if (chip8.V[x] != nn)
      chip8.PC += 2;
    break;

  case 0x5000:
    // 5XYO - skip if VX == VY
    if (chip8.V[x] == chip8.V[y])
      chip8.PC += 2;
    break;

  case 0x6000:
    // 6XNN - set VX to NN
    chip8.V[x] = nn;
    LOG("LD V" << std::hex << static_cast<int>(x) << ", 0x"
               << static_cast<int>(nn) << '\n');
    break;

  case 0x7000:
    // 7XNN - add NN to VX
    chip8.V[x] += nn;
    LOG("ADD V" << std::hex << static_cast<int>(x) << ", 0x"
                << static_cast<int>(nn) << '\n');
    break;

  case 0x8000:
    // another switch case since we are now
    // decode based on later nibbles
    switch (opcode & 0x000Fu) {
    case 0x0:
      // 8XY0 - set VX to VY
      chip8.V[x] = chip8.V[y];
      break;

    case 0x1:
      // 8XY1 - VX OR VY (VX |= VY)
      chip8.V[x] |= chip8.V[y];
      break;

    case 0x2:
      // 8XY2 - VX AND VY (VX &= VY)
      chip8.V[x] &= chip8.V[y];
      break;

    case 0x3:
      // 8XY3 - VX XOR VY
      chip8.V[x] ^= chip8.V[y];
      break;

    case 0x4: {
      // 8XY4 - ADD
      // unlike 7XNN this will affect the
      // carry flag (VF Register).
      const uint16_t sum{static_cast<uint16_t>(chip8.V[x] + chip8.V[y])};
      // if the result is larget than 255, and overflows the
      // 8bit VX, flag register is set to 1
      chip8.V[0xF] = (sum > 255u) ? 1 : 0;
      chip8.V[x] = static_cast<uint8_t>(sum & 0xFFu);
      break;
    }

    case 0x5: {
      // 8XY5 - VX - VY
      // VF is NOT borrow
      const uint8_t vx{chip8.V[x]};
      const uint8_t vy{chip8.V[y]};
      chip8.V[0xF] = (vx >= vy) ? 1 : 0;
      chip8.V[x] = vx - vy;
      break;
    }

    case 0x7: {
      // 8XY5 - VX = VY - VX
      // We doing a 0x7 here because both this and
      // 0x5 are subtraction instructions.
      // VF is NOT borrow
      const uint8_t vx{chip8.V[x]};
      const uint8_t vy{chip8.V[y]};
      chip8.V[0xF] = (vy >= vx) ? 1 : 0;
      chip8.V[x] = vy - vx;
      break;
    }

    case 0x6: {
      // 8XY6 - VX >> = 1
      // VF = shifted out bit
      // in modern behaviour, we ignore VY
      const uint8_t shifted{static_cast<uint8_t>(chip8.V[x] & 0x1u)};
      // oouuugh this might be first time ligratures
      // are making code less readable to me
      chip8.V[x] >>= 1;
      chip8.V[0xF] = shifted;
      break;
    }

    case 0xE: {
      // 8XYE - VX << = 1
      // VF = shifted out bit
      // in modern behaviour, we ignore VY
      const uint8_t shifted{static_cast<uint8_t>(
          (chip8.V[x] & 0x80u) >> 7)};
      chip8.V[x] <<= 1;
      chip8.V[0xF] = shifted;
      break;
    }

    default:
      std::cerr << "Unknown opcode: 0x" << std::hex << opcode << '\n';
      break;
    }

    break;
  case 0x9000:
    // 9XYO - skip if VX != VY
    if (chip8.V[x] != chip8.V[y])
      chip8.PC += 2;
    break;

  case 0xA000:
    // ANNN - set I to NNN
    chip8.I = nnn;
    LOG("LD I, 0x" << std::hex << nnn << '\n');
    break;

  case 0xB000:
    // BNNN - jump to differnt subroutines
    // this is another ambiguous instruction
    // so i decided with original behaviour
    chip8.PC = nnn + chip8.V[0];
    break;

  case 0xC000: {
    // CXNN - VX = random & nn
    // generates a random number, binary ANDs with
    // NN and puts its result in VX
    const uint8_t random{static_cast<uint8_t>(rand() % 256)};
    chip8.V[x] = random & nn;
    break;
  }

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

    LOG("DRW V" << std::hex << static_cast<int>(x) << ", V"
                << static_cast<int>(y) << ", " << static_cast<int>(n) << '\n');

    break;
  }

  case 0xE000:
    switch (opcode & 0x00FFu) {
    case 0x9E:
      if (chip8.key[chip8.V[x]])
        chip8.PC += 2;
      break;

    case 0xA1:
      if (!chip8.key[chip8.V[x]])
        chip8.PC += 2;
      break;

    default:
      std::cerr << "Unknown opcode: 0x" << std::hex << opcode << '\n';
      break;
    }
    break;

  case 0xF000:
    switch (opcode & 0x00FFu) {
    case 0x07:
      // FX07 - sets VX to delay timer
      chip8.V[x] = chip8.delay_timer;
      break;

    case 0x15:
      // FX15 - set delay timer to VX
      chip8.delay_timer = chip8.V[x];
      break;

    case 0x18:
      // FX18 - set sound timer to VX
      chip8.sound_timer = chip8.V[x];
      break;

    case 0x1E:
      // FX1E - add VX to I
      // set VF if I overflows past 0xFFF
      // (above is mostly applies to Amiga)
      chip8.I += chip8.V[x];
      chip8.V[0xF] = (chip8.I > 0xFFF) ? 1 : 0;
      break;

    case 0x0A: {
      // FX0A - block until a key is pressed
      // decrement PC so this instruction keep
      //  repeating until a key is pressed
      bool key_pressed{false};
      for (uint8_t i{0}; i < 16; ++i) {
        if (chip8.key[i]) {
          chip8.V[x] = i;
          key_pressed = true;
          break;
        }
      }

      if (!key_pressed)
        chip8.PC -= 2;
      break;
    }

    case 0x29:
      // FX29 - set I to font character in VX
      // each fond character is 5 bytes, stored starting in 0x000
      chip8.I = (chip8.V[x] & 0x0Fu) * 5;
      break;

    case 0x33: {
      // FX33 - BCD conversion (decimal to binary)
      // store hundreds, tens, ones digits of
      // VX in memory at I, I + 1, I + 2
      const uint8_t vx{chip8.V[x]};
      chip8.memory[chip8.I] = vx / 100;
      chip8.memory[chip8.I + 1] = (vx / 10) % 10;
      chip8.memory[chip8.I + 2] = vx % 10;
      break;
    }

    case 0x55:
      // FX55 - store V0 to in memory starting at I
      // I is not modified in modern behaviour
      for (uint8_t i{0}; i <= x; ++i)
        chip8.memory[chip8.I + i] = chip8.V[i];
      break;

    case 0x65:
      // FX65 - load V0 to VX from memory starting at I
      // again, I is not modified in modern behaviour
      for (uint8_t i{0}; i <= x; ++i)
        chip8.V[i] = chip8.memory[chip8.I + i];
      break;
    }

    break;

  default:
    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << '\n';
    break;
  }
}

void handleInput(Chip8 &chip8) {
  if (!IsKeyPressed(KEY_ESCAPE))
    CloseWindow();

  // map hex keys to qwerty
  chip8.key[0x0] = IsKeyDown(KEY_X);
  chip8.key[0x1] = IsKeyDown(KEY_ONE);
  chip8.key[0x2] = IsKeyDown(KEY_TWO);
  chip8.key[0x3] = IsKeyDown(KEY_THREE);
  chip8.key[0x4] = IsKeyDown(KEY_Q);
  chip8.key[0x5] = IsKeyDown(KEY_W);
  chip8.key[0x6] = IsKeyDown(KEY_E);
  chip8.key[0x7] = IsKeyDown(KEY_A);
  chip8.key[0x8] = IsKeyDown(KEY_S);
  chip8.key[0x9] = IsKeyDown(KEY_D);
  chip8.key[0xA] = IsKeyDown(KEY_Z);
  chip8.key[0xB] = IsKeyDown(KEY_C);
  chip8.key[0xC] = IsKeyDown(KEY_FOUR);
  chip8.key[0xD] = IsKeyDown(KEY_R);
  chip8.key[0xE] = IsKeyDown(KEY_F);
  chip8.key[0xF] = IsKeyDown(KEY_V);
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

  //   if (!loadRom(chip8, "roms/bc_test.ch8"))
  //     return 1;

//   if (!loadRom(chip8, "roms/Pong (1 player).ch8"))
//     return 1;

  //   if (!loadRom(chip8, "roms/test_opcode.ch8"))
  //     return 1;

  constexpr int scale{15};
  InitWindow(64 * scale, 32 * scale, "CHIP-8");
  SetTargetFPS(60);

  constexpr int CYCLES_PER_FRAME{10};

  while (!WindowShouldClose()) {
    handleInput(chip8);

    for (int i{0}; i < CYCLES_PER_FRAME; ++i)
      emulateCycle(chip8);

    if (chip8.delay_timer > 0)
      chip8.delay_timer--;
    if (chip8.sound_timer > 0)
      chip8.sound_timer--;

    BeginDrawing();
    ClearBackground(BLACK);

    for (int y{0}; y < 32; ++y) {
      for (int x{0}; x < 64; ++x) {
        if (chip8.display[y * 64 + x]) {
          DrawRectangle(x * scale, y * scale, scale, scale, WHITE);
        }
      }
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}