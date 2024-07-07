#include "mapper69.h"

#include "..\cpu.h"

const float c_mapper69::vol_table[16] = {
    0.0f,
    1.412537545f,
    1.995262315f,
    2.818382931f,
    3.981071706f,
    5.623413252f,
    7.943282347f,
    11.22018454f,
    15.84893192f,
    22.38721139f,
    31.6227766f,
    44.66835922f,
    63.09573445f,
    89.12509381f,
    125.8925412f,
    177.827941f
};

c_mapper69::c_mapper69()
{
    //Batman Return of the Joker, Hebereke
    //Honoo no Doukyuuji - Dodge Danpei (J) [PRG-ROM]
    mapperName = "FME-7";
    expansion_audio = 1;
}

c_mapper69::~c_mapper69()
{
}

void c_mapper69::WriteByte(unsigned short address, unsigned char value)
{
    switch (address >> 12)
    {
    case 6:
    case 7:
        if (prg_mode & 0x40)
        {
            if (prg_mode & 0x80)
                c_mapper::WriteByte(address, value);
        }
        break;
    case 8:
    case 9:
        command = value & 0x0F;
        break;
    case 0x0A:
    case 0x0B:
        switch (command)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            SetChrBank1k(command, value);
            break;
        case 8:
            {
                prg_mode = value;
                prg_6k = pPrgRom + (((prg_mode & 0x3F) % prgRomPageCount8k) * 0x2000);
            }
            break;
        case 9:
        case 0x0A:
        case 0x0B:
            SetPrgBank8k(command - 9, value);
            break;
        case 0x0C:
            switch (value & 0x03)
            {
            case 0:
                set_mirroring(MIRRORING_VERTICAL);
                break;
            case 1:
                set_mirroring(MIRRORING_HORIZONTAL);
                break;
            case 2:
                set_mirroring(MIRRORING_ONESCREEN_LOW);
                break;
            case 3:
                set_mirroring(MIRRORING_ONESCREEN_HIGH);
                break;
            }
            break;
        case 0x0D:
            irq_control = value & 0x81;
            //if ((irq_control & 0x01) == 0)
            //All writes should ACK IRQ: https://forums.nesdev.com/viewtopic.php?f=2&t=12436
            cpu->clear_irq();
            break;
        case 0x0E:
            irq_counter = (irq_counter & 0xFF00) | value;
            break;
        case 0x0F:
            irq_counter = (irq_counter & 0x00FF) | (value << 8);
            break;
        }
        break;
    case 0xC:
    case 0xD:
        audio_register_select = value & 0xF;
        break;
    case 0xE:
    case 0xF:
        audio_register[audio_register_select] = value;
        switch (audio_register_select)
        {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
            squares[audio_register_select >> 1].target_period =
                audio_register[audio_register_select & 0x6] |
                ((audio_register[(audio_register_select & 0x6) + 1] & 0xF) << 8);
            break;
        case 0x7:
            squares_enabled = 0;
            for (int i = 0; i < 3; i++)
            {
                int enabled = !(value & (1 << i));
                squares[i].enabled = enabled;
                squares_enabled += enabled;
            }
            break;
        case 0x8:
        case 0x9:
        case 0xA:
            squares[audio_register_select - 0x8].volume = vol_table[value & 0xF];
            break;
        default:
            break;
        }
        break;
    default:
        c_mapper::WriteByte(address, value);
        break;
    }
}

void c_mapper69::clock(int cycles)
{
    tick += cycles;
    while (tick > 2)
    {
        int prev_counter = irq_counter;
        if (irq_control & 0x80)
        {
            irq_counter--;
            if ((irq_counter > prev_counter) && (irq_control & 0x01))
                cpu->execute_irq();
        }
        tick -= 3;
    }
    audio_tick += cycles;
    while (audio_tick > 5)
    {
        audio_tick -= 6;
        for (int i = 0; i < 3; i++)
        {
            if (squares[i].target_period > 0)
            {
                if (++squares[i].current_period >= squares[i].target_period)
                {
                    squares[i].current_period = 0;
                    squares[i].phase_count = (squares[i].phase_count + 1) & 0xF;
                    squares[i].phase = squares[i].phase_count >> 3;
                }
            }
        }
    }
}

float c_mapper69::mix_audio(float sample)
{
    if (squares_enabled)
    {
        float square_vol = 0.0f;
        for (int i = 0; i < 3; i++)
        {
            if (squares[i].enabled & squares[i].phase)
                square_vol += squares[i].volume;
        }
        return sample - (square_vol / (vol_table[15] * 3.0f));
    }
    else
    {
        return sample;
    }
}

unsigned char c_mapper69::ReadByte(unsigned short address)
{
    switch (address >> 12)
    {
    case 6:
    case 7:
        if (prg_mode & 0x40)
        {
            if (prg_mode & 0x80)
                return c_mapper::ReadByte(address);
            else
                return 0;
        }
        else
        {
            return *(prg_6k + (address & 0x1FFF));
        }
        break;
    default:
        break;
    }

    return c_mapper::ReadByte(address);
}

void c_mapper69::reset()
{
    command = 0;
    tick = 0;
    prg_mode = 0;
    irq_control = 0;
    irq_counter = 0;
    prg_6k = pPrgRom;
    SetPrgBank8k(PRG_8000, 0);
    SetPrgBank8k(PRG_A000, 0);
    SetPrgBank8k(PRG_C000, 1);
    SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
    for (int i = 0; i < 8; i++)
        SetChrBank1k(i, i);
    audio_register_select = 0;
    memset(&audio_register, 0, sizeof(audio_register));
    memset(&squares, 0, sizeof(squares));
    audio_register[0x7] = -1;
    audio_tick = 0;
    squares_enabled = 0;
}