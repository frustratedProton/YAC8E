#include "chip8.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <raylib.h>

void handleInput(Chip8 &chip8) {
  if (IsKeyPressed(KEY_ESCAPE))
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

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: ./main <rom path>\n";
    return 1;
  }

  Chip8 chip8;
  init(chip8);

  if (!loadRom(chip8, argv[1]))
    return 1;

  constexpr int scale{20}; 
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

    for (int y{0}; y < 32; ++y)
      for (int x{0}; x < 64; ++x)
        if (chip8.display[y * 64 + x])
          DrawRectangle(x * scale, y * scale, scale, scale, WHITE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}