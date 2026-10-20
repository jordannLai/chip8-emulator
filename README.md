# CHIP-8 Emulator

A CHIP-8 emulator written in C++20 with SDL2.

CHIP-8 is a simple virtual machine from the 1970s with 35 instructions, 4 KB of memory, 16 registers, and a 64x32 monochrome display. Writing an emulator for it means implementing a CPU's full fetch-decode-execute cycle in software.

## Build and run (macOS)

```bash
brew install cmake sdl2
cmake -S . -B build
cmake --build build

# Download the test ROMs (not included in this repo)
for rom in 1-chip8-logo 2-ibm-logo 3-corax+ 4-flags 5-quirks 6-keypad 7-beep; do
  curl -sL -o "roms/$rom.ch8" "https://github.com/Timendus/chip8-test-suite/raw/main/bin/$rom.ch8"
done

./build/chip8 roms/2-ibm-logo.ch8
```

Press **Esc** to quit. The CHIP-8 keypad is mapped to `1 2 3 4 / Q W E R / A S D F / Z X C V`.

## Project layout

```
src/chip8.h, chip8.cpp   The virtual machine: memory, registers, timers, and the CPU (execute)
src/main.cpp             SDL2 front end: window, keyboard, drawing, 60 Hz timing
roms/                    Test ROMs and games
```

The core has no graphics dependency, so it can be unit tested on its own.

## Roadmap

- [x] Project setup: CMake, SDL2 window, keyboard, 60 Hz loop
- [ ] Milestone 1: IBM logo test (`00E0`, `1NNN`, `6XNN`, `7XNN`, `ANNN`, `DXYN`)
- [ ] Milestone 2: all 35 instructions, passing the corax+ and flags test ROMs
- [ ] Milestone 3: timers, input, and sound; playable games
- [ ] Milestone 4: configurable quirks for different CHIP-8 variants
- [ ] Milestone 5: unit tests, GitHub Actions CI, and a browser build with WebAssembly

## References

- [Tobias V. Langhoff, *Guide to making a CHIP-8 emulator*](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)
- [Timendus, CHIP-8 test suite](https://github.com/Timendus/chip8-test-suite)
