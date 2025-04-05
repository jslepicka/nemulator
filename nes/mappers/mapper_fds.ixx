module;
#include <fstream>
#include <Windows.h>

export module nes:mapper_fds;
import :mapper;
import class_registry;
import nemulator.std;

namespace nes
{
class c_mapper_fds : public c_mapper, register_class<nes_mapper_registry, c_mapper_fds>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {
            {
                .number = 0x103,
                .name = "FDS",
                .constructor = []() { return std::make_unique<c_mapper_fds>(); },
            },
        };
    }

    c_mapper_fds()
    {
        ram = std::make_unique<uint8_t[]>(40 * 1024);
        chr_ram = std::make_unique<uint8_t[]>(8 * 1024);
        num_sides = 0;
        side_number = 0;
        bios_loaded = 0;
    }

    ~c_mapper_fds()
    {
        save_overlay();
    }

    int switch_disk()
    {
        if (num_sides == 1) {
            return -1;
        }
        switching_disk = 1790000 * 1; //wait approximately 1 second
        disk_not_inserted = 1;

        side_number = (side_number + 1) % num_sides;

        return side_number;
    }

    void reset()
    {
        ticks = 0;
        disk_not_inserted = 0;
        scanning = 0;
        gap_covered = 0;
        previous_crc_flag = 0;
        do_irq = 0;
        shift_register = 0;
        crc_accumulator = 0;
        disk_position = 0;
        delay = 0;

        dont_stop_motor = 0;
        dont_scan_media = 0;
        dont_write = 0;
        mirroring = 0;
        transmit_crc = 0;
        in_data = 0;
        disk_irq_enable = 0;
        timer_irq = 0;
        data_ready = 0;
        crc = 0;
        end_of_disk = 0;

        enable_disk_io = 0;
        enable_sound_io = 0;

        external_connector = 0;

        timer_reload = 0;
        timer_control = 0;

        switching_disk = 0;
    }

    void clock(int cycles) override
    {
        if (++ticks == 3) {
            ticks = 0;

            if (timer_control & 0x2) {
                if (timer_counter) {
                    timer_counter--;
                }
                else {
                    timer_irq = 1;
                    execute_irq();
                    timer_counter = timer_reload;
                    if (!(timer_control & 0x1)) {
                        timer_control &= ~0x2;
                    }
                }
            }

            if (switching_disk) {
                if (--switching_disk == 0) {
                    disk_not_inserted = 0;
                }
                else {
                    return;
                }
            }

            if (dont_stop_motor == 0 || disk_not_inserted == 1) {
                scanning = 0;
                end_of_disk = 1;
                return;
            }

            if (dont_scan_media == 1 && scanning == 0) {
                return;
            }

            if (end_of_disk == 1) {
                delay = 50000;
                end_of_disk = 0;
                disk_position = 0;
                gap_covered = 0;
            }

            if (delay) {
                delay--;
                return;
            }

            scanning = 1;

            if (dont_write == 1) {
                shift_register = disk_sides[side_number][disk_position];
                do_irq = disk_irq_enable;

                if (in_data == 0) {
                    gap_covered = 0;
                }
                else if (!gap_covered && shift_register != 0x00) {
                    do_irq = 0;
                    gap_covered = 1;
                }
                if (gap_covered == 1) {
                    data_ready = 1;
                    read_data = shift_register;
                    if (do_irq) {
                        execute_irq();
                    }
                }
            }
            else {
                //write
                if (transmit_crc == 0) {
                    data_ready = 1;
                    shift_register = write_data;
                    do_irq = disk_irq_enable;
                    if (do_irq) {
                        execute_irq();
                    }
                }

                if (in_data == 0) {
                    shift_register = 0x00;
                }
                else {
                    int x = 1;
                }
                if (transmit_crc == 0) {
                    //crc accumulator
                }
                else {
                    //if (last_crc_flag == 0) {
                    //    //finish crc calculation
                    //}
                    shift_register = 0x00; //should be crc calculation lower 8 bits
                }
                disk_sides[side_number][disk_position] = shift_register;
                dirty_blocks[(side_number << 24) | disk_position] = shift_register;
                gap_covered = 0;
            }
            disk_position++;
            if (disk_position >= disk_sides[side_number].size()) {
                dont_stop_motor = 0;
            }
            else {
                delay = 149;
            }
        }
    }

  private:
    int ticks;
    std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> chr_ram;
    using disk_side_t = std::vector<uint8_t>;
    std::vector<disk_side_t> disk_sides;

    int disk_not_inserted;
    int scanning;
    int gap_covered;
    int previous_crc_flag;
    int do_irq;
    int shift_register;
    int crc_accumulator;
    int disk_position;
    int delay;

    int dont_stop_motor;
    int dont_scan_media;
    int dont_write;
    int mirroring;
    int transmit_crc;
    int in_data;
    int disk_irq_enable;
    int timer_irq;
    int data_ready;
    int crc;
    int end_of_disk;

    int enable_disk_io;
    int enable_sound_io;

    uint8_t read_data;
    uint8_t write_data;
    uint8_t external_connector;

    int timer_counter;
    int timer_reload;
    uint8_t timer_control;

    int bios_loaded;
    int side_number;
    int num_sides;

    int switching_disk;

    std::map<uint32_t, uint8_t> dirty_blocks;
    std::map<uint32_t, uint8_t> dirty_blocks2;

    unsigned char read_byte(unsigned short address) override
    {
        uint8_t out = 0;
        if (address < 0x6000) {
            switch (address) {
                case 0x4030:
                    out |= (timer_irq ? 1 : 0) << 0;
                    out |= (data_ready ? 1 : 0) << 1;
                    out |= 1 << 2;
                    out |= (mirroring ? 1 : 0) << 3;
                    out |= 0 << 4; //crc 0=good
                    out |= 1 << 5; //unusued, unsure of value
                    out |= (end_of_disk ? 1 : 0) << 6;
                    out |= 1 << 7; //write protect disabled
                    clear_irq();
                    timer_irq = 0;
                    data_ready = 0;
                    return out;
                case 0x4031:
                    clear_irq();
                    data_ready = 0;
                    return read_data;
                case 0x4032:
                    out |= (disk_not_inserted ? 1 : 0) << 0;
                    out |= (disk_not_inserted || !scanning ? 1 : 0) << 1;
                    out |= (disk_not_inserted ? 1 : 0) << 2;
                    clear_irq();
                    return out;
                case 0x4033:
                    return 0x80 | (external_connector & 0x7F);
                default:
                    return 0;
            }
        }
        else {
            return ram[address - 0x6000];
        }
        return 0;
    }
    void write_byte(unsigned short address, unsigned char value) override
    {
        int x;
        if (address < 0x6000) {
            switch (address) {
                case 0x4020:
                    x = 1;
                    timer_reload = (timer_reload & 0xFF00) | value;
                    break;
                case 0x4021:
                    x = 1;
                    timer_reload = (timer_reload & 0x00FF) | ((int)value << 8);
                    break;
                case 0x4022:
                    if (!enable_disk_io) {
                        return;
                    }
                    x = 1;
                    timer_control = value;
                    if (timer_control & 0x2) {
                        timer_counter = timer_reload;
                        if (timer_reload == 0) {
                            execute_irq();
                        }
                    }
                    else {
                        timer_irq = 0;
                        clear_irq();
                    }
                    break;
                case 0x4023:
                    enable_disk_io = (value >> 0) & 0x01;
                    enable_sound_io = (value >> 1) & 0x01;
                    break;
                case 0x4024:
                    write_data = value;
                    clear_irq();
                    data_ready = 0;
                    break;
                case 0x4025:
                    dont_stop_motor = (value >> 0) & 0x01;
                    dont_scan_media = (value >> 1) & 0x01;
                    dont_write = (value >> 2) & 0x01;
                    transmit_crc = (value >> 4) & 0x01;
                    in_data = (value >> 6) & 0x01;
                    disk_irq_enable = (value >> 7) & 0x01;
                    mirroring = (value >> 3) & 0x01;
                    set_mirroring(mirroring ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
                    clear_irq();
                    if (dont_stop_motor == 0) {
                        timer_irq = 0;
                    }
                    break;
                case 0x4026:
                    external_connector = value;
                    break;
                default:
                    break;
            }
        }
        else {
            ram[address - 0x6000] = value;
        }
    }
    unsigned char read_chr(unsigned short address) override
    {
        return chr_ram[address];
    }
    void write_chr(unsigned short address, unsigned char value) override
    {
        chr_ram[address] = value;
    }
    int load_image() override
    {
        if (!bios_loaded) {
            std::ifstream rom;
            rom.open(image_path + "\\" + "disksys.rom", std::ios_base::in | std::ios_base::binary);
            if (rom.is_open()) {
                rom.read((char *)ram.get() + 32768, 8192);
                rom.close();
            }
            else {
                return -1;
            }
            bios_loaded = 1;
        }

        if (!num_sides) {
            const char fwnes_header[] = "FDS\x1A";
            int header_adjustment = (memcmp(image, fwnes_header, sizeof(fwnes_header) - 1) == 0) * 16;
            
            num_sides = (file_length - header_adjustment) / 65500;
            for (int i = 0; i < num_sides; i++) {
                disk_sides.emplace_back(disk_side_t());

                int src_offset = i * 65500 + header_adjustment;

                auto &fds_image = disk_sides[i];

                auto add_block = [&](int len) {
                    fds_image.push_back(0x80);
                    fds_image.insert(fds_image.end(), image + src_offset, image + src_offset + len);
                    src_offset += len;
                    return len;
                };
                auto add_gap = [&](int len) {
                    for (int i = 0; i < len; i++) {
                        fds_image.push_back(0x00);
                    }
                };

                auto add_crc = [&]() {
                    for (int i = 0; i < 2; i++) {
                        fds_image.push_back(0xCD);
                    }
                };

                add_gap(28300 / 8);

                add_block(56);
                add_crc();
                add_gap(976 / 8);

                //block 2

                add_block(2);
                add_crc();
                add_gap(976 / 8);

                while (*(image + src_offset) == 0x3) {
                    unsigned char *f = image + src_offset + 13;
                    int file_size = *f | (*(f + 1) << 8);
                    file_size += 1;
                    add_block(16);
                    add_crc();
                    add_gap(976 / 8);

                    add_block(file_size);
                    add_crc();
                    add_gap(976 / 8);
                }
                if (fds_image.size() < 65500) {
                    fds_image.resize(65500);
                }
            }
            if (load_overlay() == -1) {
                return -1;
            }
        }
        return 1;
    }

    int load_overlay()
    {
        std::ifstream f;
        f.open(sramFilename, std::ios_base::in | std::ios_base::binary);
        if (f.is_open()) {
            uint32_t start;
            uint32_t count;
            uint8_t v = 0;
            while (f.peek() != EOF) {
                f.read((char *)&start, sizeof(start));
                f.read((char *)&count, sizeof(count));
                for (int i = 0; i < count; i++) {
                    f.read((char *)&v, 1);
                    uint32_t k = start + i;
                    uint32_t side = k >> 24;
                    uint32_t offset = k & 0x00FFFFFF;
                    if (side < disk_sides.size() && offset < disk_sides[side].size()) {
                        dirty_blocks[k] = v;
                        disk_sides[side][offset] = v;
                    }
                    else {
                        return -1;
                    }
                }
            }
            f.close();
        }
        return 0;
    }

    void save_overlay()
    {
        if (dirty_blocks.size() > 0) {
            std::ofstream f;
            f.open(sramFilename, std::ios_base::out | std::ios_base::binary);
            if (f.is_open()) {
                uint32_t start = -1;
                uint32_t last_k = -1;
                std::vector<uint8_t> bytes;

                auto write = [&]() {
                    uint32_t count = bytes.size();
                    if (count == 0) {
                        return;
                    }
                    f.write((const char *)&start, sizeof(start));
                    f.write((const char *)&count, sizeof(count));
                    f.write((const char *)bytes.data(), count);
                    bytes.clear();
                };

                for (auto [k, v] : dirty_blocks) {
                    if (start == -1) {
                        start = k;
                    }
                    else if (k != last_k + 1) {
                        write();
                        start = k;
                    }
                    bytes.push_back(v);
                    last_k = k;
                }
                write();
                f.close();
            }
        }
    }
};
} //namespace nes
