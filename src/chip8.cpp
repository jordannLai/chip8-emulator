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
            // 00EE: return from a subroutine
            sp_--;
            pc_ = stack_[sp_];
        }

        break;

    case 0x1000:
        // 1NNN: jump to address NNN. (Done for you as an example.)
        pc_ = nnn;
        break;

    case 0x2000:
        // 2NNN: call subroutine at NNN
        stack_[sp_] = pc_;
        sp_++;
        pc_ = nnn;
        break;

    case 0x3000:
        // 3XNN: skip the next instruction if VX == NN
        if (V_[x] == nn)
        {
            pc_ += 2;
        }
        break;

    case 0x4000:
        // 4XNN: skip the next instruction if VX != NN
        if (V_[x] != nn)
        {
            pc_ += 2;
        }
        break;

    case 0x5000:
        // 5XY0: skip the next instruction if VX == VY
        if (V_[x] == V_[y])
        {
            pc_ += 2;
        }
        break;

    case 0x6000:
        // 6XNN: set VX = NN.
        V_[x] = nn;
        break;

    case 0x7000:
        // 7XNN: add NN to VX (no carry flag).
        V_[x] += nn;
        break;

    case 0x8000:
        switch (n)
        {
        case 0x0:
            // 8XY0: set VX = VY
            V_[x] = V_[y];
            break;
        case 0x1:
            // 8XY1: set VX = VX | VY
            V_[x] |= V_[y];
            V_[0xF] = 0;
            break;
        case 0x2:
            // 8XY2: set VX = VX & VY
            V_[x] &= V_[y];
            V_[0xF] = 0;
            break;
        case 0x3:
            // 8XY3: set VX = VX ^ VY
            V_[x] ^= V_[y];
            V_[0xF] = 0;
            break;
        case 0x4:
        {
            // 8XY4: VX = VX + VY, VF = 1 if the result overflowed 255
            int sum = V_[x] + V_[y]; // int, so it can hold more than 255
            uint8_t flag = (sum > 255) ? 1 : 0;
            V_[x] = sum & 0xFF; // keep only the low 8 bits
            V_[0xF] = flag;     // set VF LAST
            break;
        }
        case 0x5:
        {
            uint8_t flag = (V_[x] >= V_[y]) ? 1 : 0;
            V_[x] = V_[x] - V_[y];
            V_[0xF] = flag;
            break;
        }
        case 0x6:
        {
            uint8_t flag = V_[y] & 0x01;
            V_[x] = V_[y] >> 1;
            V_[0xF] = flag;
            break;
        }
        case 0x7:
        {
            uint8_t flag = (V_[y] >= V_[x]) ? 1 : 0;
            V_[x] = V_[y] - V_[x];
            V_[0xF] = flag;
            break;
        }
        case 0xE: // 8XYE: VX = VY << 1, VF = bit shifted out
        {
            uint8_t flag = (V_[y] >> 7) & 0x01;
            V_[x] = V_[y] << 1;
            V_[0xF] = flag;
            break;
        }

        default:
            std::fprintf(stderr, "Unknown opcode: %04X at %03X\n", opcode, pc_ - 2);
            break;
        }
        break;

    case 0x9000:
        // 9XY0: skip the next instruction if VX != VY
        if (V_[x] != V_[y])
        {
            pc_ += 2;
        }
        break;
    case 0xA000:
        // ANNN: set I = NNN.
        I_ = nnn;
        break;
    case 0xB000:
        // BNNN: jump to address NNN + V0
        pc_ = nnn + V_[0];
        break;

    case 0xC000:
    {
        std::uniform_int_distribution<int> dist(0, 255);
        V_[x] = dist(rng_) & nn;
        break;
    }

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
    case 0xE000:
        switch (nn) // EX__ instructions are told apart by the last two digits
        {
        case 0x9E: // EX9E: skip next instruction if key with the value of VX is pressed
            if (keys_[V_[x] & 0xF])
            {
                pc_ += 2;
            }
            break;

        case 0xA1: // EXA1: skip next instruction if key with the value of VX is not pressed
            if (!(keys_[V_[x] & 0xF]))
            {
                pc_ += 2;
            }
            break;

        default:
            std::fprintf(stderr, "Unknown opcode: %04X at %03X\n", opcode, pc_ - 2);
            break;
        }
        break;

    case 0xF000:
        switch (nn) // FX__ instructions are told apart by the last two digits
        {
        case 0x07: // FX07: VX = delay timer
            V_[x] = delay_timer_;
            break;

        case 0x0A: // FX0A: wait for a key press, store it in VX
        {
            bool found = false;
            for (int key = 0; key < 16; ++key)
            {
                if (keys_[key])
                {
                    V_[x] = key;
                    found = true;
                    break; // exits the for loop only, not the switch
                }
            }
            if (!found)
            {
                pc_ -= 2; // no key yet: rewind so this instruction runs again
            }
            break;
        }
        case 0x15: // FX15: delay timer = VX
            delay_timer_ = V_[x];
            break;

        case 0x18: // FX18: sound timer = VX
            sound_timer_ = V_[x];
            break;

        case 0x1E: // FX1E: I += VX
            I_ += V_[x];
            break;

        case 0x29: // FX29: I = address of the font character for digit VX
            I_ = kFontAddress + (V_[x] & 0xF) * 5;
            break;

        case 0x33:                               // FX33: store VX as 3 decimal digits at I, I+1, I+2
            memory_[I_] = V_[x] / 100;           // hundreds
            memory_[I_ + 1] = (V_[x] / 10) % 10; // tens
            memory_[I_ + 2] = V_[x] % 10;        // ones
            break;

        case 0x55: // FX55: store V0..VX in memory starting at I, then I += x + 1
            for (int i = 0; i <= x; ++i)
            {
                memory_[I_ + i] = V_[i];
            }
            I_ += x + 1; // original CHIP-8 leaves I pointing just past the data
            break;

        case 0x65: // FX65: load V0..VX from memory starting at I, then I += x + 1
            for (int i = 0; i <= x; ++i)
            {
                V_[i] = memory_[I_ + i];
            }
            I_ += x + 1;
            break;

        default:
            std::fprintf(stderr, "Unknown opcode: %04X at %03X\n", opcode, pc_ - 2);
            break;
        }
        break; // ends case 0xF000

    default:
        std::fprintf(stderr, "Unknown opcode: %04X at %03X\n", opcode, pc_ - 2);
        break;
    }
}
