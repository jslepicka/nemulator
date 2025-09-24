module;
#include <cassert>
#include <stdio.h>
#include <immintrin.h>
#include <Windows.h>
#undef min
module genesis;
import m68k;
import crc32;
import random;

namespace genesis
{

c_genesis::c_genesis()
{
    m68k = std::make_unique<c_m68k>([this](uint32_t address) { return this->read_word(address); },
                                    [this](uint32_t address, uint16_t value) { this->write_word(address, value); },
                                    [this](uint32_t address) { return this->read_byte(address); },
                                    [this](uint32_t address, uint8_t value) { this->write_byte(address, value); },
                                    [this]() { this->vdp->ack_irq(); }, &ipl, &stalled);
    vdp = std::make_unique<c_vdp>(
        &ipl, [this](uint32_t address) { return this->read_word(address); },
        [this](int x_res) { this->on_mode_switch(x_res); }, &stalled);
    z80 =
        std::make_unique<c_z80>([this](uint16_t address) { return this->z80_read_byte(address); },
                                [this](uint16_t address, uint8_t value) { this->z80_write_byte(address, value); },
                                [this](uint8_t port) { return this->z80_read_port(port); }, //read port
                                [this](uint8_t port, uint8_t value) { this->z80_write_byte(port, value); }, //write port
                                nullptr, //int ack
                                &z80_nmi, //nmi
                                &z80_irq, //irq
                                nullptr //data bus
        );
    ym = std::make_unique<c_ym2612>();
    cart_ram_start = 0;
    cart_ram_end = 0;
    z80_has_bus = 1;
    z80_reset = 1;
    has_sram = 0;
    is_ps4 = 0;
    ps4_ram_access = 0;
    bank_register = 0;
    psg = std::make_unique<sms::c_psg>();
    mixer_enabled = 0;

    /*
    lowpass 20kHz
    d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 7671360/6);
    Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = regexprep(num2str(reshape(Hd.ScaleValues(1:4), [1 4]), '%.16ff '), '\s+', ',')
    b2 = regexprep(num2str(Hd.sosMatrix(5:8), '%.16ff '), '\s+', ',')
    a2 = regexprep(num2str(Hd.sosMatrix(17:20), '%.16ff '), '\s+', ',')
    a3 = regexprep(num2str(Hd.sosMatrix(21:24), '%.16ff '), '\s+', ',')
    */

    /*
    1st order bandpass 2Hz - 3390Hz
    fs=48000;
    fc_l=3390;
    fc_h=2;
    [b_l, a_l] = butter(1, fc_l/(fs/2), 'low');
    [b_h, a_h] = butter(1, fc_h/(fs/2), 'high');
    b0_l = num2str(b_l(1), '%.16ff');
    b1_l = num2str(b_l(2), '%.16ff');
    a1_l = num2str(a_l(2), '%.16ff');
    b0_h = num2str(b_h(1), '%.16ff');
    b1_h = num2str(b_h(2), '%.16ff');
    a1_h = num2str(a_h(2), '%.16ff');
    fprintf('%s, %s, %s, %s, %s, %s\n', b0_l, b1_l, a1_l, b0_h, b1_h, a1_h);

    */

    struct
    {
        std::unique_ptr<dsp::c_biquad4> *lpf;
        std::unique_ptr<dsp::c_first_order_bandpass> *post_filter;
        std::unique_ptr<dsp::c_resampler> *resampler;
    } filters[] = {{&lpf_l, &post_filter_l, &resampler_l}, {&lpf_r, &post_filter_r, &resampler_r}};

    for (auto &f : filters) {
        *f.lpf = std::make_unique<dsp::c_biquad4>(
            std::array{0.5068508386611939f, 0.3307863473892212f, 0.1168005615472794f, 0.0055816280655563f},
            std::array{-1.9496889114379883f, -1.9021773338317871f, -1.3770858049392700f, -1.9604763984680176f},
            std::array{-1.9442052841186523f, -1.9171522855758667f, -1.8950747251510620f, -1.9676681756973267f},
            std::array{0.9609073400497437f, 0.9271715879440308f, 0.8989855647087097f, 0.9881398081779480f});

        //2Hz - 3390Hz
        *f.post_filter = std::make_unique<dsp::c_first_order_bandpass>(0.1840657775125092f, 0.1840657775125092f,
                                                                       -0.6318684449749816f, 0.9998691174378402f,
                                                                       -0.9998691174378402f, -0.9997382348756805f);

        *f.resampler =
            std::make_unique<dsp::c_resampler>((float)(BASE_AUDIO_FREQ / 48000.0), f.lpf->get(), f.post_filter->get());
    }
}

c_genesis::~c_genesis()
{
    close_sram();
}

uint8_t c_genesis::z80_read_port(uint8_t port)
{
    return 0;
}

void c_genesis::z80_write_port(uint8_t port, uint8_t value)
{
}

void c_genesis::on_mode_switch(int x_res)
{
    crop_right = 320 - x_res;
}

int c_genesis::load()
{
    std::ifstream file;
    file.open(path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail())
        return 0;
    file.seekg(0, std::ios_base::end);
    file_length = (int)file.tellg();
    rom_size = file_length;

    //round up rom_size to nearest power of 2
    if ((file_length & (file_length - 1)) != 0) {
        int v = 1;
        while (v <= file_length) {
            v <<= 1;
        }
        rom_size = v;
    }
    file.seekg(0, std::ios_base::beg);
    
    rom = std::make_unique_for_overwrite<uint8_t[]>(rom_size);
    
    file.read((char *)rom.get(), file_length);
    file.close();
    crc32 = get_crc32(rom.get(), file_length);
    rom_mask = rom_size - 1;

    rom_end = std::byteswap(*(uint32_t *)(rom.get() + 0x1A4));

    if (rom[0x1B0] == 'R' && rom[0x1B1] == 'A') {
        cart_ram_start = std::byteswap(*(uint32_t *)(rom.get() + 0x1B4));
        cart_ram_start &= ~1;
        cart_ram_end = std::byteswap(*(uint32_t *)(rom.get() + 0x1B8));
        cart_ram_end |= 1;
        cart_ram_size = cart_ram_end - cart_ram_start + 1;
        cart_ram = std::make_unique<uint8_t[]>(cart_ram_size);
        if (rom[0x1B2] & 0x40) {
            has_sram = 1;
            open_sram();
        }
    }
    const char ps4_title[] = "PHANTASY STAR The end of the millennium";
    if (std::memcmp(&rom[0x120], ps4_title, sizeof(ps4_title) - 1) == 0) {
        is_ps4 = 1;
    }
    if (cart_ram_start && cart_ram_start < rom_size) {
        is_ps4 = 1;
    }

    reset();
    loaded = 1;
    return 1;
}

void c_genesis::open_sram()
{
    std::ifstream file;
    file.open(sram_path_file, std::ios_base::in | std::ios_base::binary);
    if (file.fail()) {
        return;
    }
    file.seekg(0, std::ios_base::end);
    int len = (int)file.tellg();
    if (len != cart_ram_size) {
        return;
    }
    file.seekg(0, std::ios_base::beg);
    file.read((char *)cart_ram.get(), len);
    file.close();
}

void c_genesis::close_sram()
{
    if (has_sram) {
        std::ofstream file;
        file.open(sram_path_file, std::ios_base::out | std::ios_base::binary);
        if (file.fail()) {
            return;
        }
        file.write((char *)cart_ram.get(), cart_ram_size);
        file.close();
    }
}

int c_genesis::reset()
{
    ipl = 0;
    z80_irq = 0;
    m68k->reset();
    vdp->reset();
    z80->reset();
    psg->reset();
    ym->reset();
    last_bus_request = 0;
    stalled = 0;
    th1 = 0;
    th2 = 0;
    joy1 = -1;
    joy2 = -1;
    std::memset(ram, 0, sizeof(ram));
    std::memset(z80_ram, 0, sizeof(z80_ram));

    crop_right = 0;
    z80_reset = 0;
    z80_has_bus = 0;
    z80_enabled = false;
    z80_busreq = 0;
    z80_nmi = 0;
    z80_irq = 0;
    bank_register = 0;
    m68k_required = 1;
    z80_required = 1;
    current_cycle = 0;
    z80_comp = 0;

    for (auto &n : next_events) {
        n = (uint64_t)-1;
    }

    next_events[CYCLE_EVENT::M68K_CLOCK] = (CYCLES_PER_M68K_CLOCK << 3) | CYCLE_EVENT::M68K_CLOCK;
    next_events[CYCLE_EVENT::VDP_PHASE] = (1 << 3) | CYCLE_EVENT::VDP_PHASE;
    next_events[CYCLE_EVENT::END_FRAME] = ((3420 * 262) << 3) | CYCLE_EVENT::END_FRAME;
    next_events[CYCLE_EVENT::YM_CLOCK] = (CYCLES_PER_YM_CLOCK << 3) | CYCLE_EVENT::YM_CLOCK;
    next_events[CYCLE_EVENT::Z80_CLOCK] = ((uint64_t)-1 << 3) | CYCLE_EVENT::Z80_CLOCK;
    next_events[CYCLE_EVENT::PSG_CLOCK] = (CYCLES_PER_PSG_CLOCK << 3) | CYCLE_EVENT::PSG_CLOCK;

    //prep the z80 -- this is just for instruction cycle alignment purposes.  probably not necessary.
    z80->execute(0);
    z80_required = z80->get_required_cycles();

    z80_was_reset = false;

    return 0;
}

void c_genesis::enable_z80()
{
    if (z80_enabled) {
        return;
    }
    z80_enabled = true;
    z80_required = z80->get_required_cycles();
    if (z80_was_reset) {
        //z80_required should only be 0 following reset
        assert(z80_comp == 0);
        z80->execute(0);
        z80_required = z80->get_required_cycles();
        z80_was_reset = false;
    }

    uint64_t next;
    if (z80_comp == 0) {
        next = current_cycle + (z80_required * CYCLES_PER_Z80_CLOCK);
    }
    else {
        next = current_cycle + z80_comp;
    }
    next_events[CYCLE_EVENT::Z80_CLOCK] = (next << 3) | CYCLE_EVENT::Z80_CLOCK;
}

void c_genesis::disable_z80()
{
    if (!z80_enabled) {
        return;
    }
    else {
        z80_enabled = false;
        if (!z80_was_reset) {
            uint64_t current_z80 = next_events[CYCLE_EVENT::Z80_CLOCK] >> 3;
            uint64_t diff = current_z80 - current_cycle;
            z80_comp = diff;
            if (z80_comp == 0) {
                // if diff is equal to zero, the z80 is scheduled to run on this cycle
                // however it won't because this code is being executed by the
                // m68k which has a higher priority.
                // so run it now before disabling the event so that when it's resumed
                // it's scheduled correctly.
                assert(z80_required == z80->get_required_cycles());
                z80->execute(z80_required);
                z80_comp = 0;
            }
        }
        next_events[CYCLE_EVENT::Z80_CLOCK] = ((uint64_t)-1 << 3) | CYCLE_EVENT::Z80_CLOCK;
    }
}

int c_genesis::emulate_frame()
{
    psg->clear_buffer();
    resampler_l->clear_buf();
    resampler_r->clear_buf();

    auto get_next_event = [&]() {
        uint64_t a = next_events[M68K_CLOCK];
        uint64_t b = next_events[Z80_CLOCK];
        uint64_t c = next_events[VDP_PHASE];
        uint64_t d = next_events[YM_CLOCK];
        uint64_t e = next_events[PSG_CLOCK];
        uint64_t f = next_events[END_FRAME];

        uint64_t m1 = std::min(a, b);
        uint64_t m2 = std::min(c, d);
        uint64_t m3 = std::min(e, f);

        uint64_t m12 = std::min(m1, m2);

        return std::min(m12, m3);
    };

    while (true) {
        uint64_t next_event = get_next_event();
        current_cycle = next_event >> 3;
        int event = next_event & 7;
        if (event == CYCLE_EVENT::YM_CLOCK) {
            next_events[CYCLE_EVENT::YM_CLOCK] += CYCLES_PER_YM_CLOCK << 3;
            ym->clock(1);
            if (mixer_enabled) {
                resampler_l->process(ym->out_l * (1.0f - 1.0f / 6.0f) + psg->out * (1.0f / 6.0f * 2.0f));
                resampler_r->process(ym->out_r * (1.0f - 1.0f / 6.0f) + psg->out * (1.0f / 6.0f * 2.0f));
            }
        }
        else if (event == CYCLE_EVENT::M68K_CLOCK) {
            m68k->execute(m68k_required);
            m68k_required = m68k->get_required_cycles();
            next_events[CYCLE_EVENT::M68K_CLOCK] += (m68k_required * CYCLES_PER_M68K_CLOCK) << 3;
        }
        else if (event == CYCLE_EVENT::Z80_CLOCK) {
            assert(!z80_was_reset);
            assert(z80_enabled);
            assert(z80_has_bus && z80_reset);
            z80->execute(z80_required);
            z80_required = z80->get_required_cycles();
            assert(z80_required != 0);
            next_events[CYCLE_EVENT::Z80_CLOCK] += (z80_required * CYCLES_PER_Z80_CLOCK) << 3;
        }
        else if (event == CYCLE_EVENT::PSG_CLOCK) {
            psg->clock(16);
            next_events[CYCLE_EVENT::PSG_CLOCK] += CYCLES_PER_PSG_CLOCK << 3;
        }
        else if (event == CYCLE_EVENT::VDP_PHASE) {
            uint32_t next = vdp->do_event();
            next_events[CYCLE_EVENT::VDP_PHASE] += (next << 3);
            z80_irq = vdp->line == 224;
        }
        else if (event == CYCLE_EVENT::END_FRAME) {
            next_events[CYCLE_EVENT::END_FRAME] += (3420 * 262) << 3;
            break;
        }
    }

    return 0;
}

uint8_t c_genesis::z80_read_byte(uint16_t address)
{
    //char buf[256];
    //sprintf(buf, "z80 read from %x\n", address);
    //OutputDebugString(buf);
    if (address < 0x4000) {
        return z80_ram[address & 0x1FFF];
    }
    else if (address < 0x6000) {
        //ym2162
        return ym->read(address);
    }
    else if (address < 0x6100) {
        //bank register
        return bank_register;
    }
    else if (address < 0x7F00) {
        //unused
        return 0;
    }
    else if (address < 0x8000) {
        //vdp
        return 0;
    }
    else {
        uint32_t a = (bank_register << 15) | (address & 0x7FFF);
        return read_byte(a);
    }
}

void c_genesis::z80_write_byte(uint16_t address, uint8_t value)
{
    //char buf[256];
    //sprintf(buf, "z80 write %x to %x\n", value, address);
    //OutputDebugString(buf);
    if (address < 0x4000) {
        z80_ram[address & 0x1FFF] = value;
    }
    else if (address < 0x6000) {
        //ym2162
        ym->write(address & 0x3, value);
    }
    else if (address < 0x6100) {
        //bank register
        write_bank_register(value);
    }
    else if (address < 0x7F00) {
        //unused
    }
    else if (address < 0x8000) {
        //vdp
        if (address == 0x7F11) {
            psg->write(value);
        }
    }
}

void c_genesis::write_bank_register(uint8_t value)
{
    bank_register >>= 1;
    bank_register |= ((value & 0x1) << 8);
}

uint8_t c_genesis::read_byte(uint32_t address)
{
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                return cart_ram[address - cart_ram_start];
            }
            else {
                return rom[address & rom_mask];
            }
        }
        //probably not correct.  how is on-cart ram in this address space
        //handled?  is that even a thing?
        if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
            return cart_ram[address - cart_ram_start];
        }
        else {
            return rom[address & rom_mask];
        }
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        return vdp->read_byte(address);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        //return 0;
        address &= 0x7FFF;
        if (address < 0x4000) {
            return z80_ram[address & 0x1FFF];
        }
        else if (address < 0x6000) {
            return ym->read(address & 0x3);
        }
        else {
            return 0;
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0xA10001:
                return 0xA0;
            case 0xA10003:
                if (th1 & 0x40) {
                    uint16_t ret = 0x40 | (joy1 & 0x3F);

                    return ret;
                }
                else {
                    uint16_t ret = (joy1 & 0x3) | ((joy1 >> 8) & 0x30);
                    return ret;
                }
                return 0x00;
            case 0xA10005:
                if (th2 & 0x40) {
                    uint16_t ret = 0x40 | (0xFF & 0x3F);

                    return ret;
                }
                else {
                    uint16_t ret = (0xFF & 0x3) | ((0xFFFF >> 8) & 0x30);
                    return ret;
                }
                return 0x00;
            case 0xA11100:
            case 0xA11101:
                //68k bus access? incomplete
                //return last_bus_request;
                return (z80_has_bus && z80_reset);
            case 0xA11200:
            case 0xA11201:
                return z80_reset;
            default:
                return 0x00;
        }
    }
    else {
        return ram[address & 0xFFFF];
    }
    return 0;
}

uint16_t c_genesis::read_word(uint32_t address)
{
    assert(!(address & 1));
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                return std::byteswap(*(uint16_t *)(cart_ram.get() + address - cart_ram_start));
            }
            else {
                address &= rom_mask;
                return std::byteswap(*((uint16_t *)(rom.get() + address)));
            }
        }
        if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
            //assert(0);
            return std::byteswap(*(uint16_t *)(cart_ram.get() + address - cart_ram_start));
        }
        else {
            address &= rom_mask;
            return std::byteswap(*((uint16_t *)(rom.get() + address)));
        }
    }
    else if (address >= 0x00C00000 && address <= 0xC0001E) {
        return vdp->read_word(address);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        //return 0;
        address &= 0x7FFF;
        if (address < 0x4000) {
            return (z80_ram[address & 0x1FFF] << 8) | (z80_ram[(address + 1) & 0x1FFF] & 0xFF);
        }
        else if (address < 0x6000) {
            //this probably shouldn't be allowed
            assert(0);
            return (ym->read(address & 0x3) << 8) | (ym->read((address + 1) & 0x3));
        }
        else {
            return 0;
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0xA11100:
            case 0xA11101:
                return (z80_has_bus && z80_reset);
                break;
            case 0xA11200:
            case 0xA11201:
                return z80_reset;
                break;
        }
    }
    else if (address >= 0xE00000) {
        return std::byteswap(*(uint16_t *)&ram[address & 0xFFFF]);
    }
    return 0;
}

void c_genesis::write_byte(uint32_t address, uint8_t value)
{
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                cart_ram[address - cart_ram_start] = value;
            }
        }
        else if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end)
        {
            cart_ram[address - cart_ram_start] = value;
        }
    }
    else if (address >= 0x00C00000 && address < 0xC00011) {
        vdp->write_byte(address, value);
    }
    else if (address == 0xC00011) {
        psg->write(value);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        address &= 0x7FFF;
        if (address < 0x4000) {
            z80_ram[address & 0x1FFF] = value;
        }
        else if (address < 0x6000) {
            ym->write(address & 0x3, value);
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0xA10003:
                th1 = value;
                break;
            case 0xA10005:
                th2 = value;
                break;
            case 0xA10009:
                
                break;
            case 0xA11100:
            case 0xA11101:
                z80_busreq = value;
                z80_has_bus = !value;
                if (z80_has_bus && z80_reset) {
                    enable_z80();
                }
                else {
                    disable_z80();
                }
                break;
            case 0xA11200:
            case 0xA11201:
                z80_reset = value;
                if (!z80_reset) {
                    z80->reset();
                    z80_comp = 0;
                    z80_required = 0;
                    z80_was_reset = true;
                }
                if (z80_has_bus && z80_reset) {
                    enable_z80();
                }
                else {
                    disable_z80();
                }
                if (!z80_reset) {
                    z80->reset();
                    z80_comp = 0;
                    z80_required = 0;
                }
                break;
            case 0xA130F1:
                ps4_ram_access = value & 0x1;
                break;
            default:
                break;
        }
    }
    else {
        ram[address & 0xFFFF] = value;
    }
}

void c_genesis::write_word(uint32_t address, uint16_t value)
{
    assert(!(address & 1));
    if (address < 0x400000) {
        if (is_ps4) {
            if (ps4_ram_access && address >= cart_ram_start && address <= cart_ram_end) {
                //assert(0);
                cart_ram[address - cart_ram_start] = value >> 8;
                cart_ram[address - cart_ram_start + 1] = value & 0xFF;
            }
        }
        else if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end)
        {
            //assert(0);
            cart_ram[address - cart_ram_start] = value >> 8;
            cart_ram[address - cart_ram_start + 1] = value & 0xFF;
        }
    }
    else if (address >= 0x00C00000 && address < 0xC00011) {
        vdp->write_word(address, value);
    }
    else if (address == 0xC00011) {
        assert(0);
    }
    else if (address >= 0xA00000 && address < 0xA10000) {
        address &= 0x7FFF;
        if (address < 0x4000) {
            z80_ram[address & 0x1FFF] = value >> 8;
            z80_ram[(address + 1) & 0x1FFF] = value & 0xFF;
        }
        else if (address < 0x6000) {
            //assert(0);
            //this probably shouldn't be allowed
            ym->write(address & 0x3, value >> 8);
            ym->write((address + 1) & 0x3, value >> 8);
        }
    }
    else if (address < 0xE00000) {
        switch (address) {
            case 0x00A11100:
                z80_busreq = value;
                z80_has_bus = !value;
                if (z80_has_bus && z80_reset) {
                    enable_z80();
                }
                else {
                    disable_z80();
                }
                break;
            case 0xA11200:
                z80_reset = value;
                if (!z80_reset) {
                    z80->reset();
                    z80_comp = 0;
                    z80_required = 0;
                    z80_was_reset = true;
                }
                if (z80_has_bus && z80_reset) {
                    enable_z80();
                }
                else {
                    disable_z80();
                }
                if (!z80_reset) {
                    z80->reset();
                    z80_comp = 0;
                    z80_required = 0;
                }
                break;
        }
    }
    else {
        ram[address & 0xFFFF] = value >> 8;
        ram[(address + 1) & 0xFFFF] = value & 0xFF;
    }
}


void c_genesis::set_input(int input)
{
    joy1 = input;
    joy1 = ~joy1;
}

int *c_genesis::get_video()
{
    return (int*)vdp->frame_buffer;
}

int c_genesis::get_sound_bufs(const float **buf_l, const float **buf_r)
{
    int num_samples = resampler_l->get_output_buf(buf_l);
    resampler_r->get_output_buf(buf_r);
    return num_samples;

}
void c_genesis::set_audio_freq(double freq)
{
    double m = BASE_AUDIO_FREQ / freq;
    resampler_l->set_m(m);
    resampler_r->set_m(m);
}

void c_genesis::enable_mixer()
{
    psg->enable_mixer();
    mixer_enabled = 1;
}

void c_genesis::disable_mixer()
{
    psg->disable_mixer();
    mixer_enabled = 0;
}

} //namespace genesis