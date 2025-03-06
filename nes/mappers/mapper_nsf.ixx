module;
#include <fstream>

export module nes:mapper.nsf;
import :mapper;
import class_registry;
import std;

namespace nes
{

class c_mapper_nsf : public c_mapper, register_class<nes_mapper_registry, c_mapper_nsf>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x102,
                .name = "NSF",
                .constructor = []() { return std::make_unique<c_mapper_nsf>(); },
            },
        };
    }

    c_mapper_nsf()
    {
        nsf_data = NULL;
        sram = new unsigned char[8 * 1024];
    }

    ~c_mapper_nsf()
    {
        if (nsf_data) {
            delete[] nsf_data;
        }
        if (sram) {
            delete[] sram;
        }
    }

    void write_byte(unsigned short address, unsigned char value) override
    {
        if (address == 0x54F2) {
            int x = 1;
        }
        switch (address >> 12) {
            case 5:
                if (address >= 0x5300 && address < 0x5500) {
                    player_rom[address - 0x5300] = value;
                }
                else if (address >= 0x5FF8 && address <= 0x5FFF) {
                    int x = 1;
                    banks[address - 0x5FF8] = nsf_data + value * 4096;
                }
                break;
            case 6:
            case 7:
                sram[address - 0x6000] = value;
                break;
            default:
                break;
        }
    }

    unsigned char read_byte(unsigned short address) override
    {
        if (address == 0xa000) {
            int x = 1;
        }
        switch (address >> 12) {
            case 5:
                if (address >= 0x5300 && address < 0x5500) {
                    return player_rom[address - 0x5300];
                }
                return 0;
            case 6:
            case 7:
                return sram[address - 0x6000];
            default:
                //NSF 'rom' is at 0x5300
                if (address >= 0xFFFA) {
                    switch (address) {
                        case 0xFFFA: //nmi low
                            return 0xE0;
                        case 0xFFFB: //nmi high
                            return 0x54;
                        case 0xFFFC: //reset low
                            return 0x00;
                        case 0xFFFD: //reset high
                            return 0x53;
                        case 0xFFFE: //irq low
                            return 0x00;
                        case 0xFFFF: //irq high
                            return 0x53;
                    }
                }
                if (address == 0xFFFC) {
                    return 0x00;
                }
                else if (address == 0xFFFD) {
                    return 0x53;
                }
                else {
                    return banks[(address >> 12) & 0x7][address & 0xFFF];
                }
        }
    }

    void reset() override
    {
        int x = 1;
        int bankswitching = 0;
        for (int i = 0; i < 8; i++) {
            if (header->bank_init[i] != 0) {
                bankswitching = 1;
                break;
            }
        }
        if (!bankswitching) {
            for (int i = 0; i < 8; i++) {
                banks[i] = nsf_data + (i * 4096);
            }
        }
        else {
            for (int i = 0; i < 8; i++) {
                banks[i] = nsf_data + header->bank_init[i] * 4096;
            }
        }
        load_player();

        write_byte(0x54F2, header->init_address & 0xFF);
        write_byte(0x54F3, (header->init_address >> 8) & 0xFF);

        write_byte(0x54F4, header->play_address & 0xFF);
        write_byte(0x54F5, (header->play_address >> 8) & 0xFF);

        write_byte(0x54F6, 1); //1 = play song
        write_byte(0x54F7, 0); //song number
        write_byte(0x54F8, header->song_count);

        set_mirroring(MIRRORING_HORIZONTAL);
    }

    int load_image() override
    {
        //TODO: support unaligned load address

        int x = 1;
        header = (NSF_HEADER *)image;
        int nsf_length = file_length - sizeof(NSF_HEADER);
        if (header->load_address < 0x8000) {
            return -1;
        }
        int padding = header->load_address - 0x8000;
        int total_length = padding + nsf_length;
        if (total_length < 32 * 1024) {
            nsf_data = new unsigned char[32 * 1024];
        }
        else if (total_length & 0xFFF) {
            //pad
            nsf_data = new unsigned char[(total_length + 4096) & (~0xFFF)];
        }
        else {
            nsf_data = new unsigned char[total_length];
        }
        memcpy(nsf_data + padding, image + sizeof(NSF_HEADER), nsf_length);
        return 0;
    }



  private:
    struct NSF_HEADER
    {
        char signature[5];
        unsigned char version;
        unsigned char song_count;
        unsigned char starting_song;
        unsigned short load_address;
        unsigned short init_address;
        unsigned short play_address;
        char name_song[32];
        char name_artist[32];
        char name_copyright[32];
        unsigned short ntsc_speed;
        unsigned char bank_init[8];
        unsigned short pal_speed;
        unsigned char region;
        unsigned char expansion;
        unsigned char nsf2_reserved;
        unsigned char length[3];
    } *header;
    unsigned char *banks[8];
    unsigned char *nsf_data;
    unsigned char *sram;
    static unsigned char PLAYER_ROM[512];
    unsigned char player_rom[512];
    unsigned char chr_ram[8192];

    void load_player()
    {
        memcpy(player_rom, PLAYER_ROM, sizeof(PLAYER_ROM));
        return;
        std::ifstream file;
        int m = -1;

        file.open("c:\\nesasm\\player2.nes", std::ios_base::in | std::ios_base::binary);
        file.seekg(0x1300, std::ios_base::beg);
        file.read((char *)player_rom, 512);
        file.close();
    }

    void write_chr(unsigned short address, unsigned char value) override
    {
        chr_ram[address & 0x1FFF] = value;
    }

    unsigned char read_chr(unsigned short address) override
    {
        if (address > 16) {
            int x = 1;
        }
        if (chr_ram[address & 0x1FFF] != 0) {
            int x = 1;
        }
        return chr_ram[address & 0x1FFF];
    }
};

} //namespace nes

#include "mapper_nsf_rom.h"