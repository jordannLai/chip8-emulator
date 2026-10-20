#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>

// The CHIP-8 virtual machine: memory, registers, timers, display, and keypad.
// This class knows nothing about windows or keyboards, so it can be unit
// tested on its own. main.cpp connects it to the screen and keyboard.
class Chip8 {
public:
    static constexpr int kWidth = 64;
    static constexpr int kHeight = 32;

    Chip8();

    // Copies a program into memory starting at 0x200. Returns false on error.
    bool load_rom(const std::string& path);

    // Runs one fetch-decode-execute step.
    void cycle();

    // Counts the delay and sound timers down. Call this 60 times per second.
    void tick_timers();

    // Pressed or released key 0x0 to 0xF.
    void set_key(int key, bool pressed);

    // One byte per pixel: 1 = on, 0 = off.
    const std::array<uint8_t, kWidth * kHeight>& display() const { return display_; }

    // True while the sound timer is running (the speaker should beep).
    bool sound_active() const { return sound_timer_ > 0; }

private:
    // Decodes and runs one instruction. This is the CPU.
    void execute(uint16_t opcode);

    std::array<uint8_t, 4096> memory_{};   // 4 KB of RAM
    std::array<uint8_t, 16> V_{};          // registers V0 to VF (VF doubles as a flag)
    uint16_t I_ = 0;                       // index register, holds memory addresses
    uint16_t pc_ = 0x200;                  // program counter, programs start at 0x200
    std::array<uint16_t, 16> stack_{};     // return addresses for subroutine calls
    uint8_t sp_ = 0;                       // stack pointer
    uint8_t delay_timer_ = 0;
    uint8_t sound_timer_ = 0;
    std::array<uint8_t, kWidth * kHeight> display_{};
    std::array<bool, 16> keys_{};
    std::mt19937 rng_{std::random_device{}()};
};
