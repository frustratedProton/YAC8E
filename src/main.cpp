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

  constexpr int scale{10};
  InitWindow(128 * scale, 64 * scale, "CHIP-8");
  SetTargetFPS(60);
  InitAudioDevice();

  constexpr int SAMPLE_RATE{44100};
  constexpr int BUFFER_SIZE{4096};

  AudioStream audio_stream{LoadAudioStream(SAMPLE_RATE, 16, 1)};
  PlayAudioStream(audio_stream);

  std::array<int16_t, BUFFER_SIZE> audio_buffer{};

  // generates audio buffer from XO-CHIP pattern and pitch
  auto generateAudio = [&](int16_t *buffer, int size) {
    const double freq{4000.0 * pow(2.0, (chip8.pitch - 64) / 48.0)};
    const double samples_per_pattern{SAMPLE_RATE / freq};

    for (int i{0}; i < size; ++i) {
      const int pattern_pos{
          static_cast<int>(i / (samples_per_pattern / 128.0)) % 128};
      const int byte_idx{pattern_pos / 8};
      const int bit_idx{7 - (pattern_pos % 8)};
      const bool bit{
          static_cast<bool>((chip8.audio_pattern[byte_idx] >> bit_idx) & 1)};
      buffer[i] = bit ? 16000 : -16000;
    }
  };

  constexpr Color COLOR_BG{0x99, 0x66, 0x00, 0xFF};
  constexpr Color COLOR_1{0xFF, 0xFF, 0xFF, 0xFF};
  constexpr Color COLOR_2{0x00, 0x00, 0x00, 0xFF};
  constexpr Color COLOR_3{0xFF, 0x00, 0x00, 0xFF};

  constexpr int CYCLES_PER_FRAME{10};

  while (!WindowShouldClose()) {
    handleInput(chip8);

    for (int i{0}; i < CYCLES_PER_FRAME; ++i)
      emulateCycle(chip8);

    if (chip8.hires && GetScreenWidth() != 128 * scale)
      SetWindowSize(128 * scale, 64 * scale);
    else if (!chip8.hires && GetScreenWidth() != 64 * scale)
      SetWindowSize(64 * scale, 32 * scale);

    if (chip8.delay_timer > 0)
      chip8.delay_timer--;

    if (chip8.sound_timer > 0) {
      chip8.sound_timer--;
      if (IsAudioStreamProcessed(audio_stream)) {
        generateAudio(audio_buffer.data(), BUFFER_SIZE);
        UpdateAudioStream(audio_stream, audio_buffer.data(), BUFFER_SIZE);
      }
    } else {
      if (IsAudioStreamProcessed(audio_stream)) {
        audio_buffer.fill(0);
        UpdateAudioStream(audio_stream, audio_buffer.data(), BUFFER_SIZE);
      }
    }

    const int display_width{chip8.hires ? 128 : 64};
    const int display_height{chip8.hires ? 64 : 32};
    constexpr int plane_size{128 * 64};

    BeginDrawing();
    ClearBackground(COLOR_BG);

    for (int y{0}; y < display_height; ++y) {
      for (int x{0}; x < display_width; ++x) {
        const int idx{y * display_width + x};

        const uint8_t p1{chip8.display[idx]};
        const uint8_t p2{chip8.display[plane_size + idx]};

        Color color{COLOR_BG};
        if (p1 && p2)
          color = COLOR_3;
        else if (p1)
          color = COLOR_1;
        else if (p2)
          color = COLOR_2;

        if (p1 || p2)
          DrawRectangle(x * scale, y * scale, scale, scale, color);
      }
    }

    EndDrawing();
    chip8.draw_flag = false;
  }

  UnloadAudioStream(audio_stream);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}