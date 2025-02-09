#include "invaders.h"
#include <fstream>
#include <vector>
#include <string.h>

import z80;
import crc32;
import dsp;
import interpolate;

c_invaders::c_invaders()
{
    system_name = "Arcade";

    display_info.fb_width = FB_WIDTH;
    display_info.fb_height = FB_HEIGHT;
    display_info.aspect_ratio = 3.0 / 4.0;
    display_info.rotation = 270;

    //real hardware uses an i8080, but z80, being (mostly) compatible, seems to work just fine
    z80 = std::make_unique<c_z80>([this](uint16_t address) { return this->read_byte(address); }, //read_byte
                [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
                [this](uint8_t port) { return this->read_port(port); }, //read_port
                [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
                [this]() { this->int_ack(); }, //int_ack callback
                &nmi, &irq, &data_bus);
    loaded = 0;
    sample_channel[0].loop = 1;
    /*
    lowpass elliptical, 20kHz
    d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 1996800/2);
    Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = regexprep(num2str(reshape(Hd.ScaleValues(1 : 4), [1 4]), '%.16ff '), '\s+', ',')
    b2 = regexprep(num2str(Hd.sosMatrix(5 : 8), '%.16ff '), '\s+', ',')
    a2 = regexprep(num2str(Hd.sosMatrix(17 : 20), '%.16ff '), '\s+', ',')
    a3 = regexprep(num2str(Hd.sosMatrix(21 : 24), '%.16ff '), '\s+', ',')
    */

    /*
    lowpass elliptical, 20kHz
    d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 1996800/4);
    Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = regexprep(num2str(reshape(Hd.ScaleValues(1 : 4), [1 4]), '%.16ff '), '\s+', ',')
    b2 = regexprep(num2str(Hd.sosMatrix(5 : 8), '%.16ff '), '\s+', ',')
    a2 = regexprep(num2str(Hd.sosMatrix(17 : 20), '%.16ff '), '\s+', ',')
    a3 = regexprep(num2str(Hd.sosMatrix(21 : 24), '%.16ff '), '\s+', ',')
    */
    switch (audio_divider) {
        case 2:
            lpf = new dsp::c_biquad4(
                {0.5070392489433289f, 0.3306076824665070f, 0.1138657182455063f, 0.0055798427201807f},
                {-1.9594025611877441f, -1.9208804368972778f, -1.4828784465789795f, -1.9681241512298584f},
                {-1.9513448476791382f, -1.9262821674346924f, -1.9057153463363647f, -1.9728368520736694f},
                {0.9648271203041077f, 0.9343814253807068f, 0.9088804125785828f, 0.9893420338630676f});
            break;
        case 4:
            lpf = new dsp::c_biquad4(
                {0.5084333419799805f, 0.3372127115726471f, 0.1511920541524887f, 0.0056142648681998f},
                {-1.8411995172500610f, -1.6990419626235962f, -0.5021169781684876f, -1.8745096921920776f},
                {-1.8784115314483643f, -1.8417872190475464f, -1.8132950067520142f, -1.9135959148406982f},
                {0.9312939047813416f, 0.8732110261917114f, 0.8254680037498474f, 0.9789751172065735f});
            break;
        default:
            break;
    }


    /*
    post-filter is butterworth bandpass, 30Hz - 12kHz
    d = fdesign.bandpass('N,F3dB1,F3dB2', 2, 30, 12000, 48000);
    Hd = design(d,'butter', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = num2str(Hd.ScaleValues(1), '%.16ff ')
    b = regexprep(num2str(Hd.sosMatrix(1:3), '%.16ff '), '\s+', ',')
    a = regexprep(num2str(Hd.sosMatrix(4:6), '%.16ff '), '\s+', ',')
    */
    post_filter = new dsp::c_biquad(0.4990182518959045f, {1.0000000000000000f, 0.0000000000000000f, -1.0000000000000000f},
                          {1.0000000000000000f, -0.9980365037918091f, 0.0019634978380054f});
    resampler = new dsp::c_resampler((double)audio_freq / 48000.0, lpf, post_filter);
    mixer_enabled = 0;
    reset();
}

c_invaders::~c_invaders()
{
    delete post_filter;
    delete lpf;
    delete resampler;
}

int c_invaders::load_romset(std::vector<s_roms> &romset)
{
    for (auto &r : romset) {
        std::ifstream file;
        std::string fn = (std::string)path + (std::string) "/" + r.filename;
        file.open(fn, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.read((char *)(r.loc + r.offset), r.length);
            file.close();
        }
        else {
            return 0;
        }
        int crc = get_crc32((unsigned char *)(r.loc + r.offset), r.length);
        if (crc != r.crc32) {
            return 0;
        }
        int x = 1;
    }
    return 1;
}

int c_invaders::load_samples(std::vector<s_sample_load_info> &sample_load_info)
{
    char *file_buf;
    for (auto &li : sample_load_info) {
        std::ifstream file;
        std::string fn = (std::string)path + (std::string) "/" + li.filename;
        file.open(fn, std::ios_base::in | std::ios_base::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            int file_len = file.tellg();
            file.seekg(0, std::ios::beg);

            file_buf = new char[file_len];
            file.read(file_buf, file_len);
            file.close();

            uint32_t crc = get_crc32((unsigned char *)file_buf, file_len);
            if (crc != li.crc32) {
                continue;
            }
            li.channel->load_wav(file_buf);
            delete[] file_buf;
        }
    }
    return 1;
}

void c_invaders::c_sample_channel::trigger()
{
    if (len) {
        clock = 0;
        active = 1;
    }
}

void c_invaders::c_sample_channel::load_wav(char *buf)
{
    struct s_fmt
    {
        uint32_t block_id;
        uint32_t block_size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t freq;
        uint32_t bytes_per_second;
        uint16_t bytes_per_block;
        uint16_t bits_per_sample;
    } *fmt;
    int p = 0;

    //skip over RIFF chunk
    p += 12;

    //read in fmt chunk
    fmt = (s_fmt *)&buf[12];
    p += 8 + fmt->block_size;

    //skip over fact chunk if it exists
    char fact[4] = {'f', 'a', 'c', 't'};
    if (memcmp(&buf[p], fact, 4) == 0) {
        int chunk_size = *(int *)&buf[p + 4];
        p += 8 + chunk_size;
    }

    char data_id[4] = {'d', 'a', 't', 'a'};
    if (memcmp(&buf[p], data_id, 4) == 0) {
        int chunk_size = *(int *)&buf[p + 4];
        int num_samples = chunk_size / 2;
        data = std::make_unique<int16_t[]>(num_samples);
        memcpy(data.get(), &buf[p + 8], chunk_size);
        len = num_samples;
        freq = fmt->freq;
        divisor = audio_freq / fmt->freq;
    }
}

int c_invaders::load()
{
    // clang-format off
    std::vector<s_roms> invaders_roms = {
        {"invaders.e", 0x14e538b0, 2048, 0x1800, rom},
        {"invaders.f", 0x0ccead96, 2048, 0x1000, rom},
        {"invaders.g", 0x6bfaca4a, 2048, 0x0800, rom},
        {"invaders.h", 0x734f5ad8, 2048, 0x0000, rom},
    };

    std::vector<s_sample_load_info> invaders_sample_load_info = {
        {"0.wav", 0x429b7860, &sample_channel[0]},
        {"1.wav", 0x442d7ae7, &sample_channel[1]},
        {"2.wav", 0xd4d796f5, &sample_channel[2]},
        {"3.wav", 0xae5c3f0d, &sample_channel[3]},
        {"4.wav", 0x9df13868, &sample_channel[4]},
        {"5.wav", 0x9d68ed05, &sample_channel[5]},
        {"6.wav", 0x190b84dd, &sample_channel[6]},
        {"7.wav", 0x2c2e8e44, &sample_channel[7]},
        {"8.wav", 0x4fc91e86, &sample_channel[8]},
        {"9.wav", 0x9e14d606, &sample_channel[9]},
    };

    // clang-format on

    if (!load_romset(invaders_roms))
        return 0;

    load_samples(invaders_sample_load_info);

    loaded = 1;
    return 0;
}

int c_invaders::is_loaded()
{
    return loaded;
}

void c_invaders::int_ack()
{
    //Interrupt flip-flop is cleared when SYNC and D0 of status word are high during
    //i8080 interrupt acknowledge cycle.
    //
    //On Z80, the equivalent is when M1 and IORQ go low during interrupt acknowledge
    //cycle.
    irq = 0;
}

int c_invaders::emulate_frame()
{
    resampler->clear_buf();
    for (int line = 0; line < 262; line++) {
        switch (line) {
            case 96:
                data_bus = 0xCF; //RST 8
                irq = 1;
                break;
            case 224:
                data_bus = 0xD7; //RST 10
                irq = 1;
                break;
            default:
                break;
        }
        //19.968MHz clock divided by 10 / 262 / 59.541985Hz refresh
        z80->execute(128);
        clock_sound(128 / audio_divider);
    }
    return 0;
}

void c_invaders::clock_sound(int cycles)
{
    //generating sound at 1996800 / audio_divisor Hz
    for (int i = 0; i < cycles; i++) {
        float sample = 0.0f;
        for (auto &channel : sample_channel) {
            if (channel.active) {
                double f_pos = (double)channel.clock / channel.divisor;
                int pos = (int)f_pos;
                if (channel.loop) {
                    pos %= channel.len;
                }
                if (pos > channel.len - 1) {
                    channel.active = 0;
                }
                else {
                    static const float vol = .5f;

                    if (false) {
                        //zero-order hold
                        float zero_order_sample = (float)channel.data[pos] / 32768.0f;
                        sample += zero_order_sample * vol;
                    }
                    else {
                        //linear interpolation
                        double frac = f_pos - pos;
                        float left_sample = (float)channel.data[pos] / 32768.0f;
                        if (frac == 0.0) {
                            sample += left_sample * vol;
                        }
                        else {
                            float right_sample = (float)channel.data[pos + 1] / 32768.0f;
                            sample += interpolate::lerp(left_sample, right_sample, frac) * vol;
                        }
                    }
                    channel.clock += 1;
                }
            }
        }
        if (mixer_enabled) {
            resampler->process(sample);
        }
    }
}

int c_invaders::reset()
{
    memset(fb, 0, sizeof(fb));
    memset(ram, 0, sizeof(ram));
    memset(vram, 0, sizeof(vram));
    shift_register = 0;
    shift_amount = 0;
    INP1.value = 0x8;
    irq = 0;
    nmi = 0;
    data_bus = 0;
    prev_sound1 = 0;
    prev_sound2 = 0;
    z80->reset();

    // clang-format off
    INP2 = {
        {
        .ships0 = 1,
        .ships1 = 1,
        .extra_ship = 1,
        }
    };
    // clang-format on
    return 0;
}

int c_invaders::get_crc()
{
    return 0;
}

void c_invaders::set_input(int input)
{
    INP1.value = input;
}

int *c_invaders::get_video()
{
    return (int *)fb;
}

uint8_t c_invaders::read_byte(uint16_t address)
{
    if (address > 0x4000) {
        int check_mirroring = 1;
    }
    if (address < 0x2000) {
        return rom[address];
    }
    else if (address < 0x2400) {
        return ram[address - 0x2000];
    }
    else if (address < 0x4000) {
        return vram[address - 0x2400];
    }
    else {
        int x = 1;
    }
    return 0;
}

void c_invaders::write_byte(uint16_t address, uint8_t data)
{
    if (address < 0x2000) {
        int rom = 1;
    }
    else if (address < 0x2400) {
        ram[address - 0x2000] = data;
    }
    else if (address < 0x4000) {

        enum screen_loc
        {
            status_line_y = 16,  //line separating playfield from ships/credits info
            base_top_y = 71,     //the top line of the base section
            ufo_bottom_y = 192,  //bottom line of ufo section, just above aliens
            ufo_top_y = 223,     //top line of ufo section, just below scores
            ships_left_x = 25,   //left edge of remaining ships icons
            credits_left_x = 136 //left edge of credits
        };
        int loc = address - 0x2400;
        vram[loc] = data;
        uint32_t *f = &fb[loc * 8];
        int x = loc >> 5; //divide by 32 (256 width / 8 bits/byte)
        int y_base = (loc & 0x1F) * 8;
        const int white = 0xFFFFF0F0;
        const int green = 0xFFB9E000;
        const int orange = 0xFF5A97F1;
        for (int i = 0; i < 8; i++) {
            int color = white;
            int y = y_base + i;
            if (y < screen_loc::status_line_y) {
                if (x >= screen_loc::ships_left_x && x <= screen_loc::credits_left_x) {
                    color = green;
                }
            }
            else if (y > screen_loc::status_line_y && y <= screen_loc::base_top_y) {
                color = green;
            }
            else if (y >= screen_loc::ufo_bottom_y && y <= screen_loc::ufo_top_y) {
                color = orange;
            }
            *f++ = (data & 0x1) * color;
            data >>= 1;
        }
    }
    else {
        int x = 1;
    }
}

uint8_t c_invaders::read_port(uint8_t port)
{
    switch (port) {
        case 0: //INP0
            break;
        case 1: //INP1
            return INP1.value;
        case 2: //INP2
            return INP2.value;
        case 3: //SHFT_IN
            return ((shift_register << shift_amount) & 0xFF00) >> 8;
        default:
            break;
    }
    return 0;
}

void c_invaders::write_port(uint8_t port, uint8_t data)
{
    // clang-format off
    int sounds1[5] = {
        0, //ufo repeating
        1, //shot
        2, //flash/player die
        3, //invader die
        9, //extended play
     };
    int sounds2[5] = {
        4, //fleet movement 1
        5, //fleet movement 2
        6, //fleet movement 3
        7, //fleet movement 4
        8, //ufo hit
    };


    // clang-format on
    int x = 0;
    switch (port) {
        case 2: //SHFTAMNT
            shift_amount = data & 0x7;
            break;
        case 3: //SOUND1
            //ufo sound
            if (data & 1 && !(prev_sound1 & 1)) {
                sample_channel[0].trigger();
            }
            else if (!(data & 1)) {
                sample_channel[0].active = 0;
            }

            for (int i = 1; i < 5; i++) {
                int mask = 1 << i;
                if (data & mask && !(prev_sound1 & mask)) {
                    sample_channel[sounds1[i]].trigger();
                }
            }
            prev_sound1 = data;
            break;
        case 4: //SHFT_DATA
            shift_register = (shift_register >> 8) | ((uint16_t)data << 8);
            break;
        case 5: //SOUND2
            for (int i = 0; i < 5; i++) {
                int mask = 1 << i;
                if (data & mask && !(prev_sound2 & mask)) {
                    sample_channel[sounds2[i]].trigger();
                }
            }
            prev_sound2 = data;
            break;
        case 6: //WATCHDOG
            break;
        default:
            break;
    }
}

int c_invaders::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    int num_samples = resampler->get_output_buf(buf_l);
    *buf_r = NULL;
    return num_samples;
}

void c_invaders::set_audio_freq(double freq)
{
    double x = (double)audio_freq / freq;
    resampler->set_m(x);
}

void c_invaders::enable_mixer()
{
    mixer_enabled = 1;
}

void c_invaders::disable_mixer()
{
    mixer_enabled = 0;
}