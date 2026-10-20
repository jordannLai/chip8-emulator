// The "frontend": opens a window, reads the keyboard, and draws the CHIP-8
// display. All the CPU logic lives in chip8.cpp.

#include <SDL.h>

#include <array>
#include <cstdio>

#include "chip8.h"

namespace {

constexpr int kScale = 12;               // each CHIP-8 pixel becomes 12x12 on screen
constexpr int kInstructionsPerFrame = 11; // ~660 instructions/sec at 60 frames/sec
constexpr uint32_t kOnColor = 0xFFE8E6E3;
constexpr uint32_t kOffColor = 0xFF14161C;

// The original CHIP-8 keypad (left) mapped onto a modern keyboard (right):
//   1 2 3 C        1 2 3 4
//   4 5 6 D   ->   Q W E R
//   7 8 9 E        A S D F
//   A 0 B F        Z X C V
int map_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_1: return 0x1; case SDLK_2: return 0x2; case SDLK_3: return 0x3; case SDLK_4: return 0xC;
        case SDLK_q: return 0x4; case SDLK_w: return 0x5; case SDLK_e: return 0x6; case SDLK_r: return 0xD;
        case SDLK_a: return 0x7; case SDLK_s: return 0x8; case SDLK_d: return 0x9; case SDLK_f: return 0xE;
        case SDLK_z: return 0xA; case SDLK_x: return 0x0; case SDLK_c: return 0xB; case SDLK_v: return 0xF;
        default: return -1;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <rom file>\n", argv[0]);
        return 1;
    }

    Chip8 chip8;
    if (!chip8.load_rom(argv[1])) return 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          Chip8::kWidth * kScale, Chip8::kHeight * kScale, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);  // fallback, e.g. in CI
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                             Chip8::kWidth, Chip8::kHeight);
    if (!window || !renderer || !texture) {
        std::fprintf(stderr, "SDL setup failed: %s\n", SDL_GetError());
        return 1;
    }

    std::array<uint32_t, Chip8::kWidth * Chip8::kHeight> pixels{};
    bool running = true;
    uint32_t last_frame = SDL_GetTicks();

    while (running) {
        // 1. Keyboard and window events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int key = map_key(event.key.keysym.sym);
                if (key >= 0) chip8.set_key(key, event.type == SDL_KEYDOWN);
            }
        }

        // 2. Run a batch of instructions, then tick the 60 Hz timers once
        for (int i = 0; i < kInstructionsPerFrame; ++i) chip8.cycle();
        chip8.tick_timers();

        // 3. Draw the display
        const auto& display = chip8.display();
        for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = display[i] ? kOnColor : kOffColor;
        SDL_UpdateTexture(texture, nullptr, pixels.data(), Chip8::kWidth * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // 4. Hold to 60 frames per second
        uint32_t elapsed = SDL_GetTicks() - last_frame;
        if (elapsed < 16) SDL_Delay(16 - elapsed);
        last_frame = SDL_GetTicks();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
