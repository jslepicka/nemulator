module;
#include <cassert>
export module genesis;
import nemulator.std;
import nemulator.buttons;
import system;
import class_registry;
import z80;
import :vdp;
import sms;
import ym2612;
import m68k;
import dsp;
import crc32;
import :db;

namespace genesis
{

export class c_genesis : public c_system,
                         register_class<system_registry, c_genesis>
{
    enum CYCLE_EVENT
    {
        M68K_CLOCK,
        Z80_CLOCK,
        VDP_PHASE,
        YM_CLOCK,
        PSG_CLOCK,
        END_FRAME = 7,
        ALL = -1
    };
  public:
    static std::vector<s_system_info> get_registry_info()
    {
        // clang-format off
        static const std::vector<s_button_map> button_map = {
            {BUTTON_1UP,             0x01},
            {BUTTON_1DOWN,           0x02},
            {BUTTON_1LEFT,           0x04},
            {BUTTON_1RIGHT,          0x08},
            {BUTTON_1A,              0x10}, //B button 1
            {BUTTON_1C,              0x20}, //C button 3
            {BUTTON_1B,            0x1000}, //A button 2
            {BUTTON_1START,        0x2000}
        };
        // clang-format on

        return {
            {
                .name = "Sega Genesis",
                .identifier = "genesis",
                .extension = "bin",
                .display_info =
                    {
                        .fb_width = 320,
                        .fb_height = 224,
                        .crop_right = -1,
                    },
                .button_map = button_map,
                .num_sound_channels = 2,
                .volume = pow(10.0f, 4.0f / 20.0f), //boost by 4dB
                .constructor = []() { return std::make_unique<c_genesis>(); },
            }
        };
    }

    c_genesis()
    {
        m68k = std::make_unique<c_m68k<c_genesis>>(
            *this,
            [this]() { this->vdp->ack_irq(); },
            &ipl,
            &stalled
        );
        vdp = std::make_unique<c_vdp>(
            &ipl,
            [this](uint32_t address) { return this->read_word(address); },
            [this](int x_res) { this->on_mode_switch(x_res); },
            &stalled
        );
        z80 = std::make_unique<c_z80<c_genesis>>(
            *this,
            &z80_nmi, //nmi
            &z80_irq, //irq
            nullptr //data bus
        );
        vdp->global_cycle = &current_cycle;
        ym = std::make_unique<c_ym2612>();
        cart_ram_start = 0;
        cart_ram_end = 0;
        z80_has_bus = 1;
        z80_reset = 1;
        has_sram = false;
        cart_ram_enabled = false;
        bank_register = 0;
        psg = std::make_unique<sms::c_psg>();
        mixer_enabled = 0;
        region = REGION::US;

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

        resampler = resampler_t::create((float)(BASE_AUDIO_FREQ / 48000.0));
    }

    ~c_genesis()
    {
        close_sram();
    }

    int emulate_frame()
    {
        psg->clear_buffer();
        resampler->clear_buf();

        uint64_t next_event = update_event<CYCLE_EVENT::ALL>();
        while (true) {
            current_cycle = next_event >> 3;
            int event = next_event & 7;
            if (event == CYCLE_EVENT::YM_CLOCK) {
                next_events[CYCLE_EVENT::YM_CLOCK] += CYCLES_PER_YM_CLOCK << 3;
                next_event = update_event<CYCLE_EVENT::YM_CLOCK>();
                ym->clock(1);
                if (++mix_clock == CLOCKS_PER_MIX) {
                    mix_clock = 0;
                    if (mixer_enabled) {
                        resampler->process({ym->out_l * (1.0f - 1.0f / 6.0f) + psg->out * (1.0f / 6.0f * 2.0f),
                                            ym->out_r * (1.0f - 1.0f / 6.0f) + psg->out * (1.0f / 6.0f * 2.0f)});
                    }
                }
            }
            else if (event == CYCLE_EVENT::M68K_CLOCK) {
                m68k->execute(m68k_required);
                m68k_required = m68k->get_required_cycles();
                next_events[CYCLE_EVENT::M68K_CLOCK] += (m68k_required * CYCLES_PER_M68K_CLOCK) << 3;
                next_event = update_event<CYCLE_EVENT::M68K_CLOCK>();
            }
            else if (event == CYCLE_EVENT::Z80_CLOCK) {
                assert(!z80_was_reset);
                assert(z80_enabled);
                assert(z80_has_bus && z80_reset);
                z80->execute(z80_required);
                z80_required = z80->get_required_cycles();
                assert(z80_required != 0);
                next_events[CYCLE_EVENT::Z80_CLOCK] += (z80_required * CYCLES_PER_Z80_CLOCK) << 3;
                next_event = update_event<CYCLE_EVENT::Z80_CLOCK>();
            }
            else if (event == CYCLE_EVENT::PSG_CLOCK) {
                psg->clock(16);
                next_events[CYCLE_EVENT::PSG_CLOCK] += CYCLES_PER_PSG_CLOCK << 3;
                next_event = update_event<CYCLE_EVENT::PSG_CLOCK>();
            }
            else if (event == CYCLE_EVENT::VDP_PHASE) {
                uint32_t next = vdp->do_event();
                next_events[CYCLE_EVENT::VDP_PHASE] += (next << 3);
                next_event = update_event<CYCLE_EVENT::VDP_PHASE>();
                z80_irq = vdp->line == 224;
            }
            else if (event == CYCLE_EVENT::END_FRAME) {
                next_events[CYCLE_EVENT::END_FRAME] += (3420 * 262) << 3;
                next_event = update_event<CYCLE_EVENT::END_FRAME>();
                break;
            }
        }

        return 0;
    }

    uint8_t read_byte(uint32_t address)
    {
        if (address < 0x400000) {
            if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
                if (rom_size <= 0x200000 || (rom_size > 0x200000 && cart_ram_enabled)) {
                    return cart_ram[address - cart_ram_start];
                }
            }
            return rom[address & rom_mask];
        }
        else if (address >= 0x00C00000 && address <= 0xC0001E) {
            return vdp->read_byte(address);
        }
        else if (address >= 0xA00000 && address < 0xA10000) {
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
                    if (region == REGION::JAPAN) {
                        return 0x20;
                    }
                    else {
                        return 0xA0;
                    }
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

    uint16_t read_word(uint32_t address)
    {
        assert(!(address & 1));
        if (address < 0x400000) {
            if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
                if (rom_size <= 0x200000 || (rom_size > 0x200000 && cart_ram_enabled)) {
                    return std::byteswap(*(uint16_t *)(cart_ram.get() + address - cart_ram_start));
                }
            }
            return std::byteswap(*((uint16_t *)(rom.get() + (address & rom_mask))));
        }
        else if (address >= 0x00C00000 && address <= 0xC0001E) {
            return vdp->read_word(address);
        }
        else if (address >= 0xA00000 && address < 0xA10000) {
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

    void write_byte(uint32_t address, uint8_t value)
    {
        if (address < 0x400000) {
            if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
                if (rom_size <= 0x200000 || (rom_size > 0x200000 && cart_ram_enabled)) {
                    cart_ram[address - cart_ram_start] = value;
                }
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
                    cart_ram_enabled = value & 0x1;
                    break;
                default:
                    break;
            }
        }
        else {
            ram[address & 0xFFFF] = value;
        }
    }

    void write_word(uint32_t address, uint16_t value)
    {
        assert(!(address & 1));
        if (address < 0x400000) {
            if (cart_ram_start && address >= cart_ram_start && address <= cart_ram_end) {
                if (rom_size <= 0x200000 || (rom_size > 0x200000 && cart_ram_enabled)) {
                    cart_ram[address - cart_ram_start] = value >> 8;
                    cart_ram[address - cart_ram_start + 1] = value & 0xFF;
                }
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

    int reset()
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

        mix_clock = 0;

        for (auto &n : next_events) {
            n = (uint64_t)-1;
        }

        next_events[CYCLE_EVENT::M68K_CLOCK] = (CYCLES_PER_M68K_CLOCK << 3) | CYCLE_EVENT::M68K_CLOCK;
        next_events[CYCLE_EVENT::VDP_PHASE] = (1 << 3) | CYCLE_EVENT::VDP_PHASE;
        next_events[CYCLE_EVENT::END_FRAME] = ((3420 * 262) << 3) | CYCLE_EVENT::END_FRAME;
        next_events[CYCLE_EVENT::YM_CLOCK] = (CYCLES_PER_YM_CLOCK << 3) | CYCLE_EVENT::YM_CLOCK;
        next_events[CYCLE_EVENT::Z80_CLOCK] = ((uint64_t)-1 << 3) | CYCLE_EVENT::Z80_CLOCK;
        next_events[CYCLE_EVENT::PSG_CLOCK] = (CYCLES_PER_PSG_CLOCK << 3) | CYCLE_EVENT::PSG_CLOCK;
        update_event<CYCLE_EVENT::ALL>();

        //prep the z80 -- this is just for instruction cycle alignment purposes.  probably not necessary.
        z80->execute(0);
        z80_required = z80->get_required_cycles();

        z80_was_reset = false;

        return 0;
    }

    int *get_video()
    {
        return (int *)vdp->frame_buffer;
    }

    int get_sound_buf(const float **buf)
    {
        return resampler->get_output_buf(buf);
    }

    void set_audio_freq(double freq)
    {
        double m = BASE_AUDIO_FREQ / freq;
        resampler->set_m(m);
    }

    int load()
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
                has_sram = true;
                open_sram();
            }
        }

        if (db.contains(crc32)) {
            auto entry = db.at(crc32);
            region = entry.region;
        }

        reset();
        loaded = 1;
        return 1;
    }

    int is_loaded()
    {
        return loaded;
    }

    void enable_mixer()
    {
        mixer_enabled = 1;
    }

    void disable_mixer()
    {
        mixer_enabled = 0;
    }

    void set_input(int input)
    {
        joy1 = input;
        joy1 = ~joy1;
    }

        uint8_t z80_read_byte(uint16_t address)
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

    void z80_write_byte(uint16_t address, uint8_t value)
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

    uint8_t z80_read_port(uint8_t port)
    {
        return 0;
    }

    void z80_write_port(uint8_t port, uint8_t value)
    {
    }

    void z80_int_ack()
    {
    }

    uint8_t m68k_read_byte(uint32_t address)
    {
        return read_byte(address);
    }

    uint16_t m68k_read_word(uint32_t address)
    {
        return read_word(address);
    }

    void m68k_write_byte(uint32_t address, uint8_t data)
    {
        write_byte(address, data);
    }

    void m68k_write_word(uint32_t address, uint16_t data)
    {
        write_word(address, data);
    }

  private:
    void open_sram()
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

    void close_sram()
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

    void write_bank_register(uint8_t value)
    {
        bank_register >>= 1;
        bank_register |= ((value & 0x1) << 8);
    }

    void on_mode_switch(int x_res)
    {
        crop_right = 320 - x_res;
    }

    void enable_z80()
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
        update_event<CYCLE_EVENT::Z80_CLOCK>();
    }

    void disable_z80()
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
            update_event<CYCLE_EVENT::Z80_CLOCK>();
        }
    }

    template <c_genesis::CYCLE_EVENT event> uint64_t update_event()
    {
        uint64_t &a = next_events[END_FRAME];
        uint64_t &b = next_events[Z80_CLOCK];
        uint64_t &c = next_events[VDP_PHASE];
        uint64_t &d = next_events[PSG_CLOCK];
        uint64_t &e = next_events[YM_CLOCK];
        uint64_t &f = next_events[M68K_CLOCK];

        //  a, b -> m1 -+
        //              |-> m12 -+
        //  c, d -> m2 -+        |-> result
        //                       |
        //  e, f -> m3 ----------+

        if constexpr (event == CYCLE_EVENT::PSG_CLOCK) {
            m2 = std::min(c, d);
            m12 = std::min(m1, m2);
            return std::min(m12, m3);
        }
        else if constexpr (event == CYCLE_EVENT::END_FRAME) {
            m1 = std::min(a, b);
            m12 = std::min(m1, m2);
            return std::min(m12, m3);
        }
        else if constexpr (event == CYCLE_EVENT::Z80_CLOCK) {
            m1 = std::min(a, b);
            m12 = std::min(m1, m2);
            return std::min(m12, m3);
        }
        else if constexpr (event == CYCLE_EVENT::YM_CLOCK) {
            m3 = std::min(e, f);
            return std::min(m12, m3);
        }
        else if constexpr (event == CYCLE_EVENT::VDP_PHASE) {
            m2 = std::min(c, d);
            m12 = std::min(m1, m2);
            return std::min(m12, m3);
        }
        else if constexpr (event == CYCLE_EVENT::M68K_CLOCK) {
            m3 = std::min(e, f);
            return std::min(m12, m3);
        }
        else if constexpr (event == CYCLE_EVENT::ALL) {
            m1 = std::min(a, b);
            m2 = std::min(c, d);
            m3 = std::min(e, f);
            m12 = std::min(m1, m2);
            return std::min(m12, m3);
        }
    }

  public:
    int irq;
    int nmi;

  private:
    REGION region;
    int loaded = 0;
    static const int CLOCKS_PER_MIX = 4;
    int mix_clock;
    std::unique_ptr<c_m68k<c_genesis>> m68k;
    std::unique_ptr<uint8_t[]> rom;
    std::unique_ptr<uint8_t[]> cart_ram;
    std::unique_ptr<c_vdp> vdp;
    std::unique_ptr<c_z80<c_genesis>> z80;
    std::unique_ptr<c_ym2612> ym;
    int file_length;
    uint8_t ipl;
    bool z80_was_reset;
    bool frame_complete;
    bool z80_enabled;
    bool has_sram;
    bool cart_ram_enabled;
    int last_bus_request;
    int z80_reset;
    int z80_busreq;
    int z80_irq;
    int z80_nmi;
    int z80_running;
    int z80_has_bus;
    uint32_t stalled;
    uint32_t th1;
    uint32_t th2;
    uint32_t joy1;
    uint32_t joy2;
    uint8_t ram[64 * 1024];
    uint8_t z80_ram[8 * 1024];
    uint32_t rom_size;
    uint32_t rom_mask;
    uint32_t rom_end;
    uint32_t cart_ram_start;
    uint32_t cart_ram_end;
    uint32_t cart_ram_size;
    uint32_t bank_register;
    std::unique_ptr<sms::c_psg> psg;
    uint32_t m68k_required;
    uint32_t z80_required;
    uint64_t current_cycle;
    int z80_comp;

    uint64_t next_events[8];

    int mixer_enabled;
    int line;

    /*
    d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 7671360/6/4);
    Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = regexprep(num2str(reshape(Hd.ScaleValues(1:4), [1 4]), '%.16ff '), '\s+', ',')
    b2 = regexprep(num2str(Hd.sosMatrix(5:8), '%.16ff '), '\s+', ',')
    a2 = regexprep(num2str(Hd.sosMatrix(17:20), '%.16ff '), '\s+', ',')
    a3 = regexprep(num2str(Hd.sosMatrix(21:24), '%.16ff '), '\s+', ',')
    */

    using lpf_t =
        dsp::c_biquad4_t<0.5179223418235779f, 0.3575305044651032f, 0.2226604372262955f, 0.0057023479603231f,
                         -1.6284488439559937f, -1.3291022777557373f, 0.3889163136482239f, -1.7029347419738770f,
                         -1.7699114084243774f, -1.7353214025497437f, -1.7109397649765015f, -1.8106834888458252f,
                         0.8959513902664185f, 0.8095921277999878f, 0.7395310401916504f, 0.9678796529769898f>;

    using bpf_t = dsp::c_first_order_bandpass<0.1840657775125092f, 0.1840657775125092f, -0.6318684449749816f,
                                              0.9998691174378402f, -0.9998691174378402f, -0.9997382348756805f>;
    using resampler_t = dsp::c_resampler<2, lpf_t, bpf_t>;

    std::unique_ptr<resampler_t> resampler;

    static constexpr double BASE_AUDIO_FREQ = (488.0 * 262.0 * 60.0) / 6.0 / (double)CLOCKS_PER_MIX;

    static const uint32_t CYCLES_PER_M68K_CLOCK = 7;
    static const uint32_t CYCLES_PER_Z80_CLOCK = 15;
    static const uint32_t CYCLES_PER_PSG_CLOCK = 15 * 16;
    static const uint32_t CYCLES_PER_YM_CLOCK = 7 * 6;

    uint64_t m1, m2, m3, m12;
};

} //namespace genesis