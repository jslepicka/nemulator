module;

module pacman:psg;

namespace pacman
{

c_pacman_psg::c_pacman_psg()
{
    sound_rom = new uint8_t[512];

    /*
    lowpass elliptical, 20kHz
    d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 760320);
    Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = regexprep(num2str(reshape(Hd.ScaleValues(1 : 4), [1 4]), '%.16ff '), '\s+', ',')
    b2 = regexprep(num2str(Hd.sosMatrix(5 : 8), '%.16ff '), '\s+', ',')
    a2 = regexprep(num2str(Hd.sosMatrix(17 : 20), '%.16ff '), '\s+', ',')
    a3 = regexprep(num2str(Hd.sosMatrix(21 : 24), '%.16ff '), '\s+', ',')
    */
    /*
    post-filter is butterworth bandpass, 30Hz - 12kHz
    d = fdesign.bandpass('N,F3dB1,F3dB2', 2, 30, 12000, 48000);
    Hd = design(d,'butter', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = num2str(Hd.ScaleValues(1), '%.16ff ')
    b = regexprep(num2str(Hd.sosMatrix(1:3), '%.16ff '), '\s+', ',')
    a = regexprep(num2str(Hd.sosMatrix(4:6), '%.16ff '), '\s+', ',')
    */
    resampler = resampler_t::create(audio_rate / 48000.0);
    mixer_enabled = 0;
    reset();
}

c_pacman_psg::~c_pacman_psg()
{
    delete[] sound_rom;
}

void c_pacman_psg::set_audio_rate(double freq)
{
    double x = audio_rate / freq;
    resampler->set_m((float)x);
}

int c_pacman_psg::get_buffer(const float **buf)
{
    int num_samples = resampler->get_output_buf(0, buf);
    return num_samples;
}

void c_pacman_psg::clear_buffer()
{
    resampler->clear_buf();
}

void c_pacman_psg::write_byte(uint16_t address, uint8_t data)
{
    sound_ram[address - 0x40] = data & 0xF;
    int x = 1;
}

void c_pacman_psg::reset()
{
    std::memset(accumulator, 0, sizeof(uint32_t) * 3);
    muted = 1;
}

void c_pacman_psg::mute(int muted)
{
    this->muted = muted ? 1 : 0;
}

void c_pacman_psg::execute(int cycles)
{
    while (cycles-- > 0) {
        float sample = 0.0f;
        for (int channel = 0; channel < 3; channel++) {
            uint32_t channel_offset = channel * 5;
            uint32_t frequency = (channel == 0 ? sound_ram[0x10] << 0 : 0) | sound_ram[0x11 + channel_offset] << 4 |
                                 sound_ram[0x12 + channel_offset] << 8 | sound_ram[0x13 + channel_offset] << 12 |
                                 sound_ram[0x14 + channel_offset] << 16;
            uint8_t volume = sound_ram[0x15 + channel_offset];
            uint8_t waveform = sound_ram[0x05 + channel_offset] & 0x7;
            accumulator[channel] = (accumulator[channel] + frequency) & 0xF'FFFF;
            uint8_t wave_index = (accumulator[channel] >> 15) & 0x1F;
            sample += muted ? 0.0f : (float)((sound_rom[waveform * 32 + wave_index] & 0xF) * volume);
        }
        if (mixer_enabled) {
            sample /= (225.0f * 3.0f); //this scaling is arbitrary
            //8x oversampled to ~768kHz
            for (int i = 0; i < 8; i++) {
                resampler->process({sample});
            }
        }
    }
}

} //namespace pacman