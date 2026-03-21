module;

export module genesis;
import nemulator.std;
import nemulator.buttons;
import system;
import class_registry;
import z80;
import :vdp;
import sms;
import ym2612;
import bus;
import m68k;
import dsp;

namespace genesis
{

export class c_genesis : public c_system, register_class<system_registry, c_genesis>
{
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
    c_genesis();
    ~c_genesis();
    int emulate_frame();
    uint8_t read_byte(uint32_t address);
    uint16_t read_word(uint32_t address);
    void write_byte(uint32_t address, uint8_t value);
    void write_word(uint32_t address, uint16_t value);
    int reset();
    int *get_video();
    int get_sound_buf(const float **buf);
    int irq;
    int nmi;
    void set_audio_freq(double freq);
    int load();
    int is_loaded()
    {
        return loaded;
    }
    void enable_mixer();
    void disable_mixer();
    void set_input(int input);

  private:
    s_bus<uint32_t> bus;
    s_bus<uint16_t> z80_bus;
    s_bus<uint8_t> z80_io_bus;

    int loaded = 0;
    static const int CLOCKS_PER_MIX = 4;
    int mix_clock;
    std::unique_ptr<c_m68k> m68k;
    //std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> rom;
    std::unique_ptr<uint8_t[]> cart_ram;
    std::unique_ptr<c_vdp> vdp;
    std::unique_ptr<c_z80> z80;
    std::unique_ptr<c_ym2612> ym;
    int file_length;
    uint8_t ipl;
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
    uint32_t has_sram;
    int is_ps4;
    int ps4_ram_access;
    uint32_t bank_register;
    bool z80_was_reset;

    void open_sram();
    void close_sram();

    void write_bank_register(uint8_t value);
    

    void on_mode_switch(int x_res);
    uint8_t z80_read_byte(uint16_t address);
    void z80_write_byte(uint16_t address, uint8_t value);
    uint8_t z80_read_port(uint8_t port);
    void z80_write_port(uint8_t port, uint8_t value);

    std::unique_ptr<sms::c_psg> psg;

    uint32_t m68k_required;
    uint32_t z80_required;
    uint64_t current_cycle;
    int z80_comp;

    uint64_t next_events[8];

    int mixer_enabled;

    int line;
    bool frame_complete;

    //using lpf_t = dsp::c_biquad4_t<
    //    0.5068508387f, 0.3307863474f, 0.1168005615f, 0.0055816281f,
    //   -1.9496889114f, -1.9021773338f, -1.3770858049f, -1.9604763985f,
    //   -1.9442052841f, -1.9171522856f, -1.8950747252f, -1.9676681757f,
    //    0.9609073400f,  0.9271715879f,  0.8989855647f,  0.9881398082f>;


    /*
    d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 7671360/6/4);
    Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
    set(Hd, 'Arithmetic', 'single');
    g = regexprep(num2str(reshape(Hd.ScaleValues(1:4), [1 4]), '%.16ff '), '\s+', ',')
    b2 = regexprep(num2str(Hd.sosMatrix(5:8), '%.16ff '), '\s+', ',')
    a2 = regexprep(num2str(Hd.sosMatrix(17:20), '%.16ff '), '\s+', ',')
    a3 = regexprep(num2str(Hd.sosMatrix(21:24), '%.16ff '), '\s+', ',')
    */

    using lpf_t = dsp::c_biquad4_t<
        0.5179223418235779f, 0.3575305044651032f, 0.2226604372262955f, 0.0057023479603231f,
        -1.6284488439559937f, -1.3291022777557373f, 0.3889163136482239f, -1.7029347419738770f,
        -1.7699114084243774f, -1.7353214025497437f, -1.7109397649765015f, -1.8106834888458252f,
        0.8959513902664185f, 0.8095921277999878f, 0.7395310401916504f, 0.9678796529769898f>;

    using bpf_t = dsp::c_first_order_bandpass<0.1840657775125092f, 0.1840657775125092f, -0.6318684449749816f,
                                            0.9998691174378402f, -0.9998691174378402f, -0.9997382348756805f>;
    using resampler_t = dsp::c_resampler<2, lpf_t, bpf_t>;

    std::unique_ptr<resampler_t> resampler;

    static constexpr double BASE_AUDIO_FREQ = (488.0 * 262.0 * 60.0) / 6.0 / (double)CLOCKS_PER_MIX;

    enum CYCLE_EVENT
    {
        M68K_CLOCK,
        Z80_CLOCK,
        VDP_PHASE,
        YM_CLOCK,
        PSG_CLOCK,
        END_FRAME = 7 //must be last
    };
    void enable_z80();
    void disable_z80();
    bool z80_enabled;

    static const uint32_t CYCLES_PER_M68K_CLOCK = 7;
    static const uint32_t CYCLES_PER_Z80_CLOCK = 15;
    static const uint32_t CYCLES_PER_PSG_CLOCK = 15 * 16;
    static const uint32_t CYCLES_PER_YM_CLOCK = 7 * 6;
};

} //namespace genesis