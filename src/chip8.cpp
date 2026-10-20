#include "chip8.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{

    constexpr uint16_t kFontAddress = 0x50;
    constexpr uint16_t kProgramStart = 0x200;

    // Built-in font: the digits 0 to F, each 5 bytes tall. Each byte is one row of
    // pixels; the top 4 bits are the pixels. For example, "0" is:
    //   0xF0 = 1111....
    //   0x90 = 1..1....
    //   0x90 = 1..1....
    //   0x90 = 1..1....
    //   0xF0 = 1111....
    constexpr std::array<uint8_t, 80> kFont = {
        0xF0,
        0x90,
        0x90,
        0x90,
        0xF0, // 0
        0x20,
        0x60,
        0x20,
        0x20,
        0x70, // 1
        0xF0,
        0x10,
        0xF0,
        0x80,
        0xF0, // 2
        0xF0,
        0x10,
        0xF0,
        0x10,
        0xF0, // 3
        0x90,
        0x90,
        0xF0,
        0x10,
        0x10, // 4
        0xF0,
        0x80,
        0xF0,
        0x10,
        0xF0, // 5
        0xF0,
        0x80,
        0xF0,
        0x90,
        0xF0, // 6
        0xF0,
        0x10,
        0x20,
        0x40,
        0x40, // 7
        0xF0,
        0x90,
        0xF0,
        0x90,
        0xF0, // 8
        0xF0,
        0x90,
        0xF0,
        0x10,
        0xF0, // 9
        0xF0,
        0x90,
        0xF0,
        0x90,
        0x90, // A
        0xE0,
        0x90,
        0xE0,
        0x90,
        0xE0, // B
        0xF0,
        0x80,
        0x80,
        0x80,
        0xF0, // C
        0xE0,
        0x90,
        0x90,
        0x90,
        0xE0, // D
        0xF0,
        0x80,
        0xF0,
        0x80,
        0xF0, // E
        0xF0,
        0x80,
        0xF0,
        0x80,
        0x80, // F
    };

} // namespace

Chip8::Chip8()
{
    std::copy(kFont.begin(), kFont.end(), memory_.begin() + kFontAddress);
}

bool Chip8::load_rom(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::fprintf(stderr, "Could not open ROM: %s\n", path.c_str());
        return false;
    }
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (rom.size() > memory_.size() - kProgramStart)
    {
        std::fprintf(stderr, "ROM is too large (%zu bytes)\n", rom.size());
        return false;
    }
    std::copy(rom.begin(), rom.end(), memory_.begin() + kProgramStart);
    return true;
}

void Chip8::cycle()
{
    // FETCH: every instruction is 2 bytes, stored high byte first.
    uint16_t opcode = static_cast<uint16_t>((memory_[pc_] << 8) | memory_[pc_ + 1]);
    pc_ += 2;

    // DECODE + EXECUTE
    execute(opcode);
}

void Chip8::tick_timers()
{
    if (delay_timer_ > 0)
        --delay_timer_;
    if (sound_timer_ > 0)
        --sound_timer_;
}

void Chip8::set_key(int key, bool pressed)
{
    if (key >= 0 && key < 16)
        keys_[key] = pressed;
}

void Chip8::execute(uint16_t opcode)
{
    // Split the 16-bit opcode into the pieces instructions use.
    // Example: opcode 0x6A42 -> x = 0xA, nn = 0x42 ("set VA to 0x42").
    const uint8_t x = (opcode >> 8) & 0x0F; // second nibble: a register number
    const uint8_t y = (opcode >> 4) & 0x0F; // third nibble: a register number
    const uint8_t n = opcode & 0x000F;      // fourth nibble: a 4-bit number
    const uint8_t nn = opcode & 0x00FF;     // second byte: an 8-bit number
    const uint16_t nnn = opcode & 0x0FFF;   // last 12 bits: a memory address

    // Silence "unused variable" warnings until you use them.
    (void)x;
    (void)y;
    (void)n;
    (void)nn;

    // The first nibble says which kind of instruction this is.
    switch (opcode & 0xF000)
    {
    case 0x0000:
        if (opcode == 0x00E0)
        {
            // 00E0: clear the screen. (Done for you as an example.)
            display_.fill(0);
        }
        else if (opcode == 0x00EE)
        {
            // TODO 00EE: return from a subroutine.
            //   Pop the return address off the stack into pc_.
        }
        break;

    case 0x1000:
        // 1NNN: jump to address NNN. (Done for you as an example.)
        pc_ = nnn;
        break;

    case 0x6000:
        // 6XNN: set VX = NN.
        V_[x] = nn;
        break;

    case 0x7000:
        // 7XNN: add NN to VX (no carry flag).
        V_[x] += nn;
        break;

    case 0xA000:
        // ANNN: set I = NNN.
        I_ = nnn;
        break;

    case 0xD000:
    {
        // Starting position wraps around the screen
        int start_x = V_[x] % kWidth;
        int start_y = V_[y] % kHeight;

        V_[0xF] = 0; // no collision yet

        for (int row = 0; row < n; ++row)
        {
            int py = start_y + row;
            if (py >= kHeight)
                break; // clip at the bottom edge

            uint8_t sprite_byte = memory_[I_ + row]; // read row "row" of the sprite from memory, starting at I_

            for (int col = 0; col < 8; ++col)
            {
                int px = start_x + col;
                if (px >= kWidth)
                    break; // clip at the right edge

                bool bit_on = sprite_byte & (0x80 >> col); // check bits left to right
                if (!bit_on)
                    continue;

                uint8_t &pixel = display_[py * kWidth + px]; // index into the flat screen array: row py, column px

                if (pixel == 1)
                    V_[0xF] = 1; // turning a lit pixel off is a collision
                pixel ^= 1;      // flip the pixel
            }
        }
        break;
    }

    default:
        std::fprintf(stderr, "Unknown opcode: %04X at %03X\n", opcode, pc_ - 2);
        break;
    }
}
