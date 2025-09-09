module;

module invaders;
import crc32;
import interpolate;

namespace invaders
{

std::vector<c_invaders::s_system_info> c_invaders::get_registry_info()
{
    return {
        {
            .is_arcade = 1,
            .name = "Arcade",
            .identifier = "invaders",
            .title = "Space Invaders",
            .display_info =
                {
                    .fb_width = FB_WIDTH,
                    .fb_height = FB_HEIGHT,
                    .rotation = 270,
                    .aspect_ratio = 3.0 / 4.0,
                },
            .button_map =
                {
                    {BUTTON_1SELECT, 0x01},
                    {BUTTON_1START, 0x04},
                    {BUTTON_1A, 0x10},
                    {BUTTON_1LEFT, 0x20},
                    {BUTTON_1RIGHT, 0x40},
                },
            .constructor = []() { return std::make_unique<c_invaders>(); },
        },
    };
}

c_invaders::c_invaders()
{
    //real hardware uses an i8080, but z80, being (mostly) compatible, seems to work just fine
    z80 = std::make_unique<c_z80>(
        [this](uint16_t address) { return this->read_byte(address); }, //read_byte
        [this](uint16_t address, uint8_t data) { this->write_byte(address, data); }, //write_byte
        [this](uint8_t port) { return this->read_port(port); }, //read_port
        [this](uint8_t port, uint8_t data) { this->write_port(port, data); }, //write_port
        [this]() { this->int_ack(); }, //int_ack callback
        &nmi, &irq, &data_bus);
    loaded = 0;
    sample_channels[0].loop = 1;

    // 30Hz high pass
    // d = fdesign.highpass('N,F3dB', 2, 30, 48000);
    // Hd = design(d, 'butter', 'FilterStructure', 'df2tsos');
    // set(Hd, 'Arithmetic', 'single');
    // g = num2str(Hd.ScaleValues(1), '%.16ff ')
    // b = regexprep(num2str(Hd.sosMatrix(1 : 3), '%.16ff '), '\s+', ',')
    // a = regexprep(num2str(Hd.sosMatrix(4 : 6), '%.16ff '), '\s+', ',')

    post_filter =
        new dsp::c_biquad(0.9972270727157593f, {1.0000000000000000f, -2.0000000000000000f, 1.0000000000000000f},
                          {1.0000000000000000f, -1.9944463968276978f, 0.9944617748260498f});

    //no need for low pass filter since we're playing back samples
    null_filter = new dsp::c_null_filter();
    resampler = new dsp::c_resampler((double)audio_freq / 48000.0, null_filter, post_filter);
    mixer_enabled = 0;
}

c_invaders::~c_invaders()
{
    delete null_filter;
    delete post_filter;
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
            std::streamoff file_len = file.tellg();
            file.seekg(0, std::ios::beg);

            file_buf = new char[file_len];
            file.read(file_buf, file_len);
            file.close();

            uint32_t crc = get_crc32((unsigned char *)file_buf, (unsigned int)file_len);
            if (crc != li.crc32) {
                continue;
            }
            li.channel->load_wav(file_buf);
            li.channel->set_playback_rate(audio_freq);
            delete[] file_buf;
        }
    }
    return 1;
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
        {"0.wav", 0x429b7860, &sample_channels[0]},
        {"1.wav", 0x442d7ae7, &sample_channels[1]},
        {"2.wav", 0xd4d796f5, &sample_channels[2]},
        {"3.wav", 0xae5c3f0d, &sample_channels[3]},
        {"4.wav", 0x9df13868, &sample_channels[4]},
        {"5.wav", 0x9d68ed05, &sample_channels[5]},
        {"6.wav", 0x190b84dd, &sample_channels[6]},
        {"7.wav", 0x2c2e8e44, &sample_channels[7]},
        {"8.wav", 0x4fc91e86, &sample_channels[8]},
        {"9.wav", 0x9e14d606, &sample_channels[9]},
    };

    // clang-format on

    if (!load_romset(invaders_roms))
        return 0;

    load_samples(invaders_sample_load_info);

    loaded = 1;
    reset();
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
    static const float vol = .4f;
    //generating sound at 1996800 / audio_divisor Hz
    for (int i = 0; i < cycles; i++) {
        float sample = 0.0f;
        for (auto &channel : sample_channels) {
            if (channel.is_active()) {
                sample += channel.get_sample() * vol;
            }
        }
        if (mixer_enabled) {
            sample = std::min(std::max(sample, -1.0f), 1.0f);
            resampler->process(sample);
        }
    }
}

int c_invaders::reset()
{
    std::memset(fb, 0, sizeof(fb));
    std::memset(ram, 0, sizeof(ram));
    std::memset(vram, 0, sizeof(vram));
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

    for (auto &c : sample_channels) {
        c.reset();
    }
    return 0;
}

void c_invaders::set_input(int input)
{
    INP1.value = input | 0x8;
}

int *c_invaders::get_video()
{
    return (int *)fb;
}

uint8_t c_invaders::read_byte(uint16_t address)
{
    address &= 0x3FFF;
    if (address < 0x2000) {
        return rom[address];
    }
    else if (address < 0x2400) {
        return ram[address - 0x2000];
    }
    else if (address < 0x4000) {
        return vram[address - 0x2400];
    }
    return 0;
}

void c_invaders::write_byte(uint16_t address, uint8_t data)
{
    address &= 0x3FFF;
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
            return (shift_register >> (8 - shift_amount)) & 0xFF;
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
                sample_channels[0].trigger();
            }
            else if (!(data & 1)) {
                sample_channels[0].stop();
            }

            for (int i = 1; i < 5; i++) {
                int mask = 1 << i;
                if (data & mask && !(prev_sound1 & mask)) {
                    sample_channels[sounds1[i]].trigger();
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
                    sample_channels[sounds2[i]].trigger();
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

int c_invaders::get_sound_bufs(const float **buf_l, const float **buf_r)
{
    int num_samples = resampler->get_output_buf(buf_l);
    *buf_r = nullptr;
    return num_samples;
}

void c_invaders::set_audio_freq(double freq)
{
    double x = (double)audio_freq / freq;
    resampler->set_m((float)x);
}

void c_invaders::enable_mixer()
{
    mixer_enabled = 1;
}

void c_invaders::disable_mixer()
{
    mixer_enabled = 0;
}
} //namespace invaders