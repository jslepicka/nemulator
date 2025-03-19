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
    }

    void reset()
    {
        ticks = 0;
        external_connector = 0;
        timer_irq_reload_low = 0;
        timer_irq_reload_hi = 0;
        timer_irq_control = 0;
        master_io_enable = 0;
        write_data_register = 0;
        read_data_register = 0;
        fds_control.value = 0;
        fds_control.drive_motor_control = 1;
        transfer = 0;
        transfer_delay = 0;
        transfer_offset = 0;
        drive_state = DRIVE_STATE::IDLE;
        drive_status.value = 0;
        drive_status.ready_flag = 1;
        disk_status.value = 0;
        disk_status.rw_enable = 1;
        irq_asserted = 0;
        in_gap = 1;
        motor_on = 0;
        scanning_disk = 0;
        reset_transfer = 0;
        end_of_head = 0;
    }

    void clock(int cycles) override
    {
        if (++ticks == 3) {
            ticks = 0;
            if (!motor_on) {
                scanning_disk = 0;
                end_of_head = 1;
                return;
            }

            if (reset_transfer && !scanning_disk) {
                return;
            }

            if (end_of_head) {
                transfer_delay = 4000;
                end_of_head = 0;
                transfer_offset = 0;
                in_gap = 1;
                //drive_status.ready_flag = 0;
                return;
            }

            if (transfer_delay) {
                transfer_delay--;
            }
            else {
                scanning_disk = 1;

                if (fds_control.transfer_mode == TRANSFER_MODE::READ) {
                    uint8_t d = fds_image[transfer_offset++];
                    if (in_gap) {
                        if (d == 0x80) {
                            in_gap = 0;
                        }
                    }
                    else {
                        read_data_register = d;
                        disk_status.byte_transfer_flag = 1;
                        if (irq_asserted) {
                            int x = 1;
                        }
                        if (fds_control.irq_enabled && !irq_asserted) {
                            execute_irq();
                            irq_asserted = 1;
                        }
                        //transfer_delay = 8 * 18;
                        if (transfer_offset >= fds_image.size()) {
                            motor_on = 0;
                            disk_status.end_of_head = 1;
                            drive_status.ready_flag = 1;
                        }
                    }
                    transfer_delay = 150;
                }
                else {
                    int x = 1;
                }
            }
        }
    }

  private:
    int ticks;
    std::unique_ptr<uint8_t[]> ram;
    std::unique_ptr<uint8_t[]> chr_ram;
    uint8_t external_connector;
    uint8_t timer_irq_reload_low;
    uint8_t timer_irq_reload_hi;
    uint8_t timer_irq_control;
    uint8_t master_io_enable;
    uint8_t write_data_register;
    uint8_t read_data_register;
    int transfer;
    int transfer_offset;
    int transfer_delay;
    int drive_state;
    std::vector<uint8_t> fds_image;
    int irq_asserted;
    int in_gap;
    int motor_on;
    int scanning_disk;
    int reset_transfer;
    int end_of_head;

    union {
        struct
        {
            uint8_t transfer_reset : 1;
            uint8_t drive_motor_control : 1;
            uint8_t transfer_mode : 1;
            uint8_t mirroring : 1;
            uint8_t crc_transfer_control : 1;
            uint8_t : 1;
            uint8_t crc_enabled : 1;
            uint8_t irq_enabled : 1;
        };
        uint8_t value;
    } fds_control;

    union {
        struct
        {
            uint8_t timer_interrupt : 1;
            uint8_t byte_transfer_flag : 1;
            uint8_t : 1;
            uint8_t mirroring : 1;
            uint8_t crc_control : 1;
            uint8_t : 1;
            uint8_t end_of_head : 1;
            uint8_t rw_enable : 1;
        };
        uint8_t value;
    } disk_status;

    union {
        struct
        {
            uint8_t disk_flag : 1;
            uint8_t ready_flag : 1;
            uint8_t protect_flag : 1;
            uint8_t : 5;
        };
        uint8_t value;
    } drive_status;

    enum DRIVE_MOTOR
    {
        START = 0,
        STOP = 1
    };

    enum TRANSFER_MODE
    {
        WRITE = 0,
        READ = 1,
    };

    enum DRIVE_STATE
    {
        IDLE,
        SPIN_UP,
        TRANSFER,

    };

    unsigned char read_byte(unsigned short address) override
    {
        if (address < 0x6000) {
            char buf[64];
            sprintf(buf, "fds read: %04x\n", address);
            OutputDebugString(buf);
            int x = 1;
            switch (address) {
                case 0x4030:
                    disk_status.byte_transfer_flag = 0;
                    irq_asserted = 0;
                    clear_irq();
                    return disk_status.value;
                case 0x4031:
                    disk_status.byte_transfer_flag = 0;
                    irq_asserted = 0;
                    clear_irq();
                    //{
                    //    char buf[64];
                    //    sprintf(buf, "disk read: %02x %c\n", read_data_register, read_data_register);
                    //    OutputDebugString(buf);
                    //}
                    return read_data_register;
                case 0x4032: //drive status
                    irq_asserted = 0;
                    clear_irq();
                    drive_status.ready_flag = !scanning_disk;
                    return drive_status.value | 0x40;
                case 0x4033: //expansion
                    return (!fds_control.drive_motor_control << 7) | (external_connector & 0x7F); //battery ok
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
        if (address < 0x6000) {
            char buf[64];
            sprintf(buf, "fds write: %04x = %02x\n", address, value);
            OutputDebugString(buf);
            int x = 1;
            switch (address) {
                case 0x4020:
                    timer_irq_reload_low = value;
                    break;
                case 0x4021:
                    timer_irq_reload_hi = value;
                    break;
                case 0x4022:
                    timer_irq_control = value;
                    break;
                case 0x4023:
                    master_io_enable = value;
                    break;
                case 0x4024:
                    write_data_register = value;
                    disk_status.byte_transfer_flag = 0;
                    break;
                case 0x4025: {

                    fds_control.value = value | 0x20;
                    set_mirroring(fds_control.mirroring ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
                    ////if (!fds_control.transfer_reset) {
                    ////    if (start_transfer && !fds_control.drive_motor_control) {
                    ////        transfer = 0;
                    ////        transfer_delay = 14354 / 8;
                    ////        transfer_offset = 0;
                    ////        drive_state = DRIVE_STATE::SPIN_UP; //should this be start of disk instead?
                    ////        drive_status.ready_flag = 0;
                    ////        in_gap = 1;
                    ////    }
                    ////}
                    ////else {
                    ////    int x = 1;
                    ////    drive_status.ready_flag = 1;
                    ////    //transfer_delay = 18 * 28300;
                    ////}

                    //if (!fds_control.transfer_reset) {
                    //    transfer_delay = 14354 / 8;
                    //    transfer_offset = 0;
                    //    in_gap = 1;
                    //}



                    ////if (
                    motor_on = (value & 0x02);
                    reset_transfer = (value & 0x01);
                    disk_status.mirroring = fds_control.mirroring;
                    irq_asserted = 0;
                    clear_irq();
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
        int x = 1;
        std::ifstream rom;
        rom.open("c:\\roms\\fds\\disksys.rom", std::ios_base::in | std::ios_base::binary);
        if (rom.is_open()) {
            rom.read((char *)ram.get() + 32768, 8192);
            rom.close();
        }
        if (fds_image.size() == 0) {
            int src_offset = 0;
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
        }


        return 1;
    }
};
} //namespace nes
