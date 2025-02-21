module;
#include "..\mapper.h"
export module nes_mapper.mapper24;

namespace nes
{

//Akumajou Densetsu (J)

class c_mapper24 : public c_mapper, register_class<nes_mapper_registry, c_mapper24>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 24,
                .name = "VRC6",
                .constructor = []() { return std::make_unique<c_mapper24>(); },
            },
            {
                .number = 26,
                .name = "VRC6",
                .constructor = []() { return std::make_unique<c_mapper24>(1); },
            },
        };
    }

    c_mapper24(int submapper = 0)
    {
        this->submapper = submapper;
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address >= 0x8000) {
            if (submapper == 1) {
            //mapper 26
                switch (address & 0x3) {
                    case 0x1:
                        address += 0x1;
                        break;
                    case 0x2:
                        address -= 0x1;
                        break;
                    default:
                        break;
                }
            }

            switch (address /*& 0xF003*/) {
                case 0x8000:
                case 0x8001:
                case 0x8002:
                case 0x8003:
                    SetPrgBank16k(PRG_8000, value);
                    break;
                case 0xB003:
                    switch ((value >> 2) & 0x3) {
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
                case 0xC000:
                case 0xC001:
                case 0xC002:
                case 0xC003:
                    SetPrgBank8k(PRG_C000, value);
                    break;
                case 0xD000:
                case 0xD001:
                case 0xD002:
                case 0xD003:
                    SetChrBank1k(address & 0x3, value);
                    break;
                case 0xE000:
                case 0xE001:
                case 0xE002:
                case 0xE003:
                    SetChrBank1k(0x4 | (address & 0x3), value);
                    break;
                case 0xF000:
                    irq_reload = value;
                    break;
                case 0xF001:
                    irq_control = value & 0x7;
                    if (irq_control & 0x2) {
                        irq_scaler = 0;
                        irq_counter = irq_reload;
                    }
                    if (irq_asserted) {
                        clear_irq();
                        irq_asserted = 0;
                    }
                    break;
                case 0xF002:
                    if (irq_asserted) {
                        clear_irq();
                        irq_asserted = 0;
                    }
                    if (irq_control & 0x1)
                        irq_control |= 0x2;
                    else
                        irq_control &= ~0x2;
                    break;

                case 0x9000:
                    pulse1.set_volume(value);
                    ;
                    break;
                case 0x9001:
                    pulse1.set_timer_low(value);
                    break;
                case 0x9002:
                    pulse1.set_timer_high(value);
                    break;
                case 0x9003:
                    freq_control = value;
                    break;

                case 0xA000:
                    pulse2.set_volume(value);
                    break;
                case 0xA001:
                    pulse2.set_timer_low(value);
                    break;
                case 0xA002:
                    pulse2.set_timer_high(value);
                    break;

                case 0xB000:
                    saw.set_volume(value & 0x3F);
                    break;
                case 0xB001:
                    saw.set_timer_low(value);
                    break;
                case 0xB002:
                    saw.set_timer_high(value);
                    break;
            }
        }
        else {
            c_mapper::write_byte(address, value);
        }
    }

    void clock(int cycles) override
    {
        ticks += cycles;
        while (ticks > 2) {
            pulse1.clock();
            pulse2.clock();
            saw.clock();
            ticks -= 3;
        }
        if ((irq_control & 0x02) && ((irq_control & 0x04) || ((irq_scaler += cycles) >= 341))) {
            if (!(irq_control & 0x04)) {
                irq_scaler -= 341;
            }
            if (irq_counter == 0xFF) {
                irq_counter = irq_reload;
                if (!irq_asserted) {
                    execute_irq();
                    irq_asserted = 1;
                }
            }
            else
                irq_counter++;
        }
    }

    void reset() override
    {
        SetPrgBank8k(PRG_E000, prgRomPageCount8k - 1);
        irq_counter = 0;
        irq_asserted = 0;
        irq_control = 0;
        irq_reload = 0;
        irq_scaler = 0;
        audio_out = 0.0;
        ticks = 0;
        freq_control = 0;
        pulse1.reset();
        pulse2.reset();
        saw.reset();
    }

    float mix_audio(float sample) override
    {
        //audio_out = 63.0f/(pulse1.get_output() + pulse2.get_output() + saw.get_output());
        //return pulse1.get_output()/63.0f;
        float a = (pulse1.get_output() + pulse2.get_output() + saw.get_output()) / 63.0f;
        //return (a * .5f) + (sample * .5f);
        return sample + -a;
    }

  private:
    int ticks;
    int irq_counter;
    int irq_asserted;
    int irq_control;
    int irq_reload;
    int irq_scaler;
    float audio_out;

    int freq_control;

    class c_channel
    {
      public:
        virtual int get_output()
        {
            return output;
        }

        virtual void clock()
        {
            if (--timer == 0) {
                clock_channel();
                timer = (timer_low | (timer_high & 0xF) << 8) + 1;
            }
        }

        virtual void reset()
        {
            volume = 0;
            timer_low = 0;
            timer_high = 0;
            timer = 1;
            output = 0;
        }

        virtual void set_volume(int val)
        {
            volume = val;
        }

        virtual void set_timer_low(int val)
        {
            timer_low = val;
        }

        virtual void set_timer_high(int val)
        {
            timer_high = val;
        }

      protected:
        int volume;
        int timer_low;
        int timer_high;
        virtual void clock_channel() = 0;
        int timer;
        int output;
    };

    class c_pulse : public c_channel
    {
      public:
        void reset() override
        {
            c_channel::reset();
            step = 15;
        }

        void set_timer_high(int val) override
        {
            timer_high = val;
            if (!(val & 0x80)) {
                step = 15;
                output = 0;
            }
        }

      private:
        int step;
        void clock_channel() override
        {
            if (timer_high & 0x80) {
                if (volume & 0x80) {
                    output = (volume & 0xF);
                }
                else if (step <= ((volume >> 4) & 0x7)) {
                    output = (volume & 0xF);
                }
                else {
                    output = 0;
                }
                step--;
                if (step < 0)
                    step = 15;
            }
        }
    } pulse1, pulse2;

    class c_saw : public c_channel
    {
      public:
        void reset() override
        {
            c_channel::reset();
            step = 0;
            accum = 0;
        }

        void set_timer_high(int val) override
        {
            timer_high = val;
            if (!(val & 0x80)) {
                accum = 0;
                output = 0;
            }
        }

      private:
        int step;
        int accum;
        virtual void clock_channel()
        {
            if (timer_high & 0x80) {
                if (!(++step & 0x1)) //only do stuff on even cycles
                {
                    if (step == 14) {
                        step = 0;
                        accum = 0;
                    }
                    else {
                        accum = (accum + volume) & 0xFF;
                    }
                    output = accum >> 3;
                }
            }
        }
    } saw;
};

} //namespace nes
