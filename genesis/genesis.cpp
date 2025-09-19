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

    //Analog screen sections in relation to HCounter (H32 mode):
    //-----------------------------------------------------------------
    //| Screen section | HCounter  |Pixel| Pixel |Serial|Serial |MCLK |
    //| (PAL/NTSC H32) |  value    |clock| clock |clock |clock  |ticks|
    //|                |           |ticks|divider|ticks |divider|     |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|Left border     |0x00B-0x017|  13 |SCLK/2 |   26 |MCLK/5 | 130 |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|Active display  |0x018-0x117| 256 |SCLK/2 |  512 |MCLK/5 |2560 |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|Right border    |0x118-0x125|  14 |SCLK/2 |   28 |MCLK/5 | 140 |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|Back porch      |0x126-0x127|   9 |SCLK/2 |   18 |MCLK/5 |  90 |
    //|(Right Blanking)|0x1D2-0x1D8|     |       |      |       |     |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|Horizontal sync |0x1D9-0x1F2|  26 |SCLK/2 |   52 |MCLK/5 | 260 |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|Front porch     |0x1F3-0x00A|  24 |SCLK/2 |   48 |MCLK/5 | 240 |
    //|(Left Blanking) |           |     |       |      |       |     |
    //|----------------|-----------|-----|-------|------|-------|-----|
    //|TOTALS          |           | 342 |       |  684 |       |3420 |
    //-----------------------------------------------------------------

    //Analog screen sections in relation to HCounter (H40 mode):
    //--------------------------------------------------------------------
    //| Screen section |   HCounter    |Pixel| Pixel |EDCLK| EDCLK |MCLK |
    //| (PAL/NTSC H40) |    value      |clock| clock |ticks|divider|ticks|
    //|                |               |ticks|divider|     |       |     |
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|Left border     |0x00D-0x019    |  13 |EDCLK/2|  26 |MCLK/4 | 104 |
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|Active display  |0x01A-0x159    | 320 |EDCLK/2| 640 |MCLK/4 |2560 |
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|Right border    |0x15A-0x167    |  14 |EDCLK/2|  28 |MCLK/4 | 112 |
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|Back porch      |0x168-0x16C    |   9 |EDCLK/2|  18 |MCLK/4 |  72 |
    //|(Right Blanking)|0x1C9-0x1CC    |     |       |     |       |     |
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|Horizontal sync |0x1CD.0-0x1D4.5| 7.5 |EDCLK/2|  15 |MCLK/5 |  75 |
    //|                |0x1D4.5-0x1D5.5|   1 |EDCLK/2|   2 |MCLK/4 |   8 |
    //|                |0x1D5.5-0x1DC.0| 7.5 |EDCLK/2|  15 |MCLK/5 |  75 |
    //|                |0x1DD.0        |   1 |EDCLK/2|   2 |MCLK/4 |   8 |
    //|                |0x1DE.0-0x1E5.5| 7.5 |EDCLK/2|  15 |MCLK/5 |  75 |
    //|                |0x1E5.5-0x1E6.5|   1 |EDCLK/2|   2 |MCLK/4 |   8 |
    //|                |0x1E6.5-0x1EC.0| 6.5 |EDCLK/2|  13 |MCLK/5 |  65 |
    //|                |===============|=====|=======|=====|=======|=====|
    //|        Subtotal|0x1CD-0x1EC    | (32)|       | (64)|       |(314)|
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|Front porch     |0x1ED          |   1 |EDCLK/2|   2 |MCLK/5 |  10 |
    //|(Left Blanking) |0x1EE-0x00C    |  31 |EDCLK/2|  62 |MCLK/4 | 248 |
    //|                |===============|=====|=======|=====|=======|=====|
    //|        Subtotal|0x1ED-0x00C    | (32)|       | (64)|       |(258)|
    //|----------------|---------------|-----|-------|-----|-------|-----|
    //|TOTALS          |               | 420 |       | 840 |       |3420 |
    //--------------------------------------------------------------------

    //Digital render events in relation to HCounter:
    //----------------------------------------------------
    //|        Video |PAL/NTSC         |PAL/NTSC         |
    //|         Mode |H32     (RSx=00) |H40     (RSx=11) |
    //|              |V28/V30 (M2=*)   |V28/V30 (M2=*)   |
    //| Event        |Int any (LSMx=**)|Int any (LSMx=**)|
    //|--------------------------------------------------|
    //|HCounter      |[1]0x000-0x127   |[1]0x000-0x16C   |
    //|progression   |[2]0x1D2-0x1FF   |[2]0x1C9-0x1FF   |
    //|9-bit internal|                 |                 |
    //|--------------------------------------------------|
    //|VCounter      |HCounter changes |HCounter changes |
    //|increment     |from 0x109 to    |from 0x149 to    |
    //|              |0x10A in [1].    |0x14A in [1].    |
    //|--------------------------------------------------| //Logic analyzer tests conducted on 2012-11-03 confirm 18 SC
    //|HBlank set    |HCounter changes |HCounter changes | //cycles between HBlank set in status register and HSYNC
    //|              |from 0x125 to    |from 0x165 to    | //asserted in H32 mode, and 21 SC cycles in H40 mode.
    //|              |0x126 in [1].    |0x166 in [1].    | //Note this actually means in H40 mode, HBlank is set at
    //0x166.5.
    //|--------------------------------------------------| //Logic analyzer tests conducted on 2012-11-03 confirm 46 SC
    //|HBlank cleared|HCounter changes |HCounter changes | //cycles between HSYNC cleared and HBlank cleared in status
    //|              |from 0x009 to    |from 0x00A to    | //register in H32 mode, and 61 SC cycles in H40 mode.
    //|              |0x00A in [1].    |0x00B in [1].    | //Note this actually means in H40 mode, HBlank is cleared at
    //0x00B.5.
    //|--------------------------------------------------|
    //|F flag set    |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-03 confirm 28 SC
    //|              |from 0x000 to    |from 0x000 to    | //cycles between HSYNC cleared and odd flag toggled in status
    //|              |0x001 in [1]     |0x001 in [1]     | //register in H32 mode, and 40 SC cycles in H40 mode.
    //|--------------------------------------------------|
    //|ODD flag      |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-03 confirm 30 SC
    //|toggled       |from 0x001 to    |from 0x001 to    | //cycles between HSYNC cleared and odd flag toggled in status
    //|              |0x002 in [1]     |0x002 in [1]     | //register in H32 mode, and 42 SC cycles in H40 mode.
    //|--------------------------------------------------|
    //|HINT flagged  |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-02 confirm 74 SC
    //|via IPL lines |from 0x109 to    |from 0x149 to    | //cycles between HINT flagged in IPL lines and HSYNC
    //|              |0x10A in [1].    |0x14A in [1].    | //asserted in H32 mode, and 78 SC cycles in H40 mode.
    //|--------------------------------------------------|
    //|VINT flagged  |HCounter changes |HCounter changes | //Logic analyzer tests conducted on 2012-11-02 confirm 28 SC
    //|via IPL lines |from 0x000 to    |from 0x000 to    | //cycles between HSYNC cleared and VINT flagged in IPL lines
    //|              |0x001 in [1].    |0x001 in [1].    | //in H32 mode, and 40 SC cycles in H40 mode.
    //|--------------------------------------------------|
    //|HSYNC asserted|HCounter changes |HCounter changes |
    //|              |from 0x1D8 to    |from 0x1CC to    |
    //|              |0x1D9 in [2].    |0x1CD in [2].    |
    //|--------------------------------------------------|
    //|HSYNC negated |HCounter changes |HCounter changes |
    //|              |from 0x1F2 to    |from 0x1EC to    |
    //|              |0x1F3 in [2].    |0x1ED in [2].    |
    //----------------------------------------------------


	//Analog screen sections in relation to VCounter:
    //-------------------------------------------------------------------------------------------
    //|           Video |NTSC             |NTSC             |PAL              |PAL              |
    //|            Mode |H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|
    //|                 |V28     (M2=0)   |V30     (M2=1)   |V28     (M2=0)   |V30     (M2=1)   |
    //|                 |Int none(LSMx=*0)|Int none(LSMx=*0)|Int none(LSMx=*0)|Int none(LSMx=*0)|
    //|                 |------------------------------------------------------------------------
    //|                 | VCounter  |Line | VCounter  |Line | VCounter  |Line | VCounter  |Line |
    //| Screen section  |  value    |count|  value    |count|  value    |count|  value    |count|
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|Active display   |0x000-0x0DF| 224 |0x000-0x1FF| 240*|0x000-0x0DF| 224 |0x000-0x0EF| 240 |
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|Bottom border    |0x0E0-0x0E7|   8 |           |   0 |0x0E0-0x0FF|  32 |0x0F0-0x107|  24 |
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|Bottom blanking  |0x0E8-0x0EA|   3 |           |   0 |0x100-0x102|   3 |0x108-0x10A|   3 |
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|Vertical sync    |0x1E5-0x1E7|   3 |           |   0 |0x1CA-0x1CC|   3 |0x1D2-0x1D4|   3 |
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|Top blanking     |0x1E8-0x1F4|  13 |           |   0 |0x1CD-0x1D9|  13 |0x1D5-0x1E1|  13 |
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|Top border       |0x1F5-0x1FF|  11 |           |   0 |0x1DA-0x1FF|  38 |0x1E2-0x1FF|  30 |
    //|-----------------|-----------|-----|-----------|-----|-----------|-----|-----------|-----|
    //|TOTALS           |           | 262 |           | 240*|           | 313 |           | 313 |
    //-------------------------------------------------------------------------------------------
    //*When V30 mode and NTSC mode are both active, no border, blanking, or retrace
    //occurs. A 30-row display is setup and rendered, however, immediately following the
    //end of the 30th row, the 1st row starts again. In addition, the VCounter is never
    //reset, which usually happens at the beginning of vertical blanking. Instead, the
    //VCounter continuously counts from 0x000-0x1FF, then wraps around back to 0x000 and
    //begins again. Since there are only 240 lines output as part of the display, this
    //means the actual line being rendered is desynchronized from the VCounter. Digital
    //events such as vblank flags being set/cleared, VInt being triggered, the odd flag
    //being toggled, and so forth, still occur at the correct VCounter positions they
    //would occur in (IE, the same as PAL mode V30), however, since the VCounter has 512
    //lines per cycle, this means VInt is triggered at a slower rate than normal.
    //##TODO## Confirm on the hardware that the rendering row is desynchronized from the
    //VCounter. This would seem unlikely, since a separate render line counter would have
    //to be maintained apart from VCounter for this to occur.

    //Digital render events in relation to VCounter under NTSC mode:
    //#ODD - Runs only when the ODD flag is set
    //----------------------------------------------------------------------------------------
    //|        Video |NTSC             |NTSC             |NTSC             |NTSC             |
    //|         Mode |H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|H32/H40(RSx00/11)|
    //|              |V28     (M2=0)   |V28     (M2=0)   |V30     (M2=1)   |V30     (M2=1)   |
    //| Event        |Int none(LSMx=*0)|Int both(LSMx=*1)|Int none(LSMx=*0)|Int both(LSMx=*1)|
    //|--------------------------------------------------------------------------------------|
    //|VCounter      |[1]0x000-0x0EA   |[1]0x000-0x0EA   |[1]0x000-0x1FF   |[1]0x000-0x1FF   |
    //|progression   |[2]0x1E5-0x1FF   |[2]0x1E4(#ODD)   |                 |                 |
    //|9-bit internal|                 |[3]0x1E5-0x1FF   |                 |                 |
    //|--------------------------------------------------------------------------------------|
    //|VBlank set    |VCounter changes |                 |VCounter changes |                 |
    //|              |from 0x0DF to    |     <Same>      |from 0x0EF to    |     <Same>      |
    //|              |0x0E0 in [1].    |                 |0x0F0 in [1].    |                 |
    //|--------------------------------------------------------------------------------------|
    //|VBlank cleared|VCounter changes |                 |VCounter changes |                 |
    //|              |from 0x1FE to    |     <Same>      |from 0x1FE to    |     <Same>      |
    //|              |0x1FF in [2].    |                 |0x1FF in [1].    |                 |
    //|--------------------------------------------------------------------------------------|
    //|F flag set    |At indicated     |                 |At indicated     |                 |
    //|              |HCounter position|                 |HCounter position|                 |
    //|              |while VCounter is|     <Same>      |while VCounter is|     <Same>      |
    //|              |set to 0x0E0 in  |                 |set to 0x0F0 in  |                 |
    //|              |[1].             |                 |[1].             |                 |
    //|--------------------------------------------------------------------------------------|
    //|VSYNC asserted|VCounter changes |                 |      Never      |                 |
    //|              |from 0x0E7 to    |     <Same>      |                 |     <Same>      |
    //|              |0x0E8 in [1].    |                 |                 |                 |
    //|--------------------------------------------------------------------------------------|
    //|VSYNC cleared |VCounter changes |                 |      Never      |                 |
    //|              |from 0x1F4 to    |     <Same>      |                 |     <Same>      |
    //|              |0x1F5 in [2].    |                 |                 |                 |
    //|--------------------------------------------------------------------------------------|
    //|ODD flag      |At indicated     |                 |At indicated     |                 |
    //|toggled       |HCounter position|                 |HCounter position|                 |
    //|              |while VCounter is|     <Same>      |while VCounter is|     <Same>      |
    //|              |set to 0x0E0 in  |                 |set to 0x0F0 in  |                 |
    //|              |[1].             |                 |[1].             |                 |
    //----------------------------------------------------------------------------------------


    next_events[CYCLE_EVENT::M68K_CLOCK] = (7 << 3) | CYCLE_EVENT::M68K_CLOCK;
    next_events[CYCLE_EVENT::VDP_END_LINE] = (3420 << 3) | CYCLE_EVENT::VDP_END_LINE;
    //vdp timing is almost certainly not accurate
    next_events[CYCLE_EVENT::VDP_HBLANK] = (2328 << 3) | CYCLE_EVENT::VDP_HBLANK;
    next_events[CYCLE_EVENT::END_FRAME] = ((3420 * 262) << 3) | CYCLE_EVENT::END_FRAME;
    next_events[CYCLE_EVENT::YM_CLOCK] = ((7 * 6) << 3) | CYCLE_EVENT::YM_CLOCK;
    next_events[CYCLE_EVENT::Z80_CLOCK] = ((uint64_t)-1 << 3) | CYCLE_EVENT::Z80_CLOCK;
    next_events[CYCLE_EVENT::PSG_CLOCK] = ((15 * 16) << 3) | CYCLE_EVENT::PSG_CLOCK;

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
        next = current_cycle + (z80_required * 15);
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
    uint32_t hblank_len = 50;
    int line = 0;

    auto get_next_event = [&]() {
        uint64_t a = next_events[M68K_CLOCK];
        uint64_t b = next_events[Z80_CLOCK];
        uint64_t c = next_events[VDP_HBLANK];
        uint64_t d = next_events[VDP_END_LINE];
        uint64_t e = next_events[YM_CLOCK];
        uint64_t f = next_events[PSG_CLOCK];
        uint64_t g = next_events[END_FRAME];

        uint64_t m1 = std::min(a, b);
        uint64_t m2 = std::min(c, d);
        uint64_t m3 = std::min(e, f);

        uint64_t m12 = std::min(m1, m2);
        uint64_t m3g = std::min(m3, g);

        return std::min(m12, m3g);
    };

    int line_count = 0;
    int loops = 0;
    while (true) {
        uint64_t next_event = get_next_event();
        current_cycle = next_event >> 3;
        int event = next_event & 7;
        if (event == CYCLE_EVENT::YM_CLOCK) {
            next_events[CYCLE_EVENT::YM_CLOCK] += (7 * 6) << 3;
            ym->clock(1);
            if (mixer_enabled) {
                resampler_l->process(ym->out_l * (1.0f - 1.0f / 6.0f) + psg->out * (1.0f / 6.0f * 2.0f));
                resampler_r->process(ym->out_r * (1.0f - 1.0f / 6.0f) + psg->out * (1.0f / 6.0f * 2.0f));
            }
        }
        else if (event == CYCLE_EVENT::M68K_CLOCK) {
            m68k->execute(m68k_required);
            m68k_required = m68k->get_required_cycles();
            next_events[CYCLE_EVENT::M68K_CLOCK] += (m68k_required * 7) << 3;
        }
        else if (event == CYCLE_EVENT::Z80_CLOCK) {
            assert(!z80_was_reset);
            assert(z80_enabled);
            assert(z80_has_bus && z80_reset);
            z80->execute(z80_required);
            z80_required = z80->get_required_cycles();
            assert(z80_required != 0);
            next_events[CYCLE_EVENT::Z80_CLOCK] += (z80_required * 15) << 3;
        }
        else if (event == CYCLE_EVENT::PSG_CLOCK) {
            psg->clock(16);
            next_events[CYCLE_EVENT::PSG_CLOCK] += (15 * 16) << 3;
        }
        else if (event == CYCLE_EVENT::VDP_HBLANK) {
            vdp->draw_scanline();
            next_events[CYCLE_EVENT::VDP_HBLANK] += 3420 << 3;
        }
        else if (event == CYCLE_EVENT::VDP_END_LINE) {
            vdp->end_line();
            next_events[CYCLE_EVENT::VDP_END_LINE] += 3420 << 3;
            line++;
            z80_irq = line == 224;
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