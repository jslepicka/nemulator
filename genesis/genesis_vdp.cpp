module;
#include <cassert>
#include <Windows.h>
module genesis:vdp;

namespace genesis
{

c_vdp::c_vdp(uint8_t *ipl, read_word_t read_word_68k, uint32_t *stalled)
{
    this->stalled = stalled;
    this->ipl = ipl;
    this->read_word_68k = read_word_68k;
}

c_vdp::~c_vdp()
{
}

void c_vdp::reset()
{
    //status.value = 0x3400;
    status.value = 0;
    line = 0;
    std::memset(reg, 0, sizeof(reg));
    address_reg = 0;
    address_write = 0;
    memset(vram, 0, sizeof(vram));
    memset(cram, 0, sizeof(cram));
    memset(vsram, 0, sizeof(vsram));
    memset(frame_buffer, 0, sizeof(frame_buffer));
    line = 0;
    freeze_cpu = 0;
    pending_fill = 0;
}

uint16_t c_vdp::read_word(uint32_t address)
{
    uint16_t ret;
    switch (address) {
        case 0x00C00000:
        case 0x00C00002:
            address_write = 0;
            break;
        case 0x00C00004:
        case 0x00C00006:
            ret = status.value;
            status.vint = 0;
            status.dma = 0;
            *ipl &= ~0x6;
            address_write = 0;
            return ret;
        default:
            return 0;
    }
    return 0;
}

void c_vdp::write_word(uint32_t address, uint16_t value)
{
    int x = 0;
    uint16_t a = _address;
    switch (address) {
        case 0x0C00000:
        case 0x0C00002:
            //data
            if (pending_fill) {
                pending_fill = 0;
                uint16_t len = reg[0x13] | (reg[0x14] << 8);
                vram[_address] = value & 0xFF;
                do {
                    vram[_address ^ 0x1] = (value >> 8) & 0xFF;
                    //vram[_address] = (value >> 8) & 0xFF;
                    _address += reg[0x0F];
                } while (--len);
                return;
            }
            switch (address_type) {
                case ADDRESS_TYPE::VRAM_WRITE:
                    assert(_address < 64 * 1024);
                    vram[_address] = value >> 8;
                    vram[_address + 1] = value & 0xFF;
                    break;
                case ADDRESS_TYPE::CRAM_WRITE:
                    a &= 0x7F;
                    cram[a] = value >> 8;
                    cram[a + 1] = value & 0xFF;
                    break;
                case ADDRESS_TYPE::VSRAM_WRITE:
                    a &= 0x7F;
                    if (a < 0x50) {
                        vsram[a] = value >> 8;
                        vsram[a + 1] = value & 0xFF;
                    }
                    break;
                default:
                    x = 1;
                    //assert(0);
                    break;
            }
            _address += reg[0x0F];
            if (reg[0x0f] != 2) {
                int x = 1;
            }
            address_write = 0;
            break;
        case 0x0C00004:
        case 0x0C00006:
            //control
            if (value >> 14 == 0x02) 
            {
                uint8_t reg_number = (value >> 8) & 0x1F;
                uint8_t reg_value = value & 0xFF;
                reg[reg_number] = reg_value;
                int x = 1;
            }
            else {
                if (address_write) {
                    //second word
                    //address_reg |= value;
                    address_reg = (address_reg & 0xFFFF0000) | value;
                    address_type = (ADDRESS_TYPE)((((address_reg & 0xF0) | (address_reg >> 28)) >> 2) & 0xF);
                    _address = ((address_reg & 0x3) << 14) | ((address_reg >> 16) & 0x3FFF);
                    vram_to_vram_copy = address_reg & 0x40;
                    dma_copy = address_reg & 0x80;
                    if (dma_copy) {
                        OutputDebugString("DMA requested\n");
                        if (reg[0x01] & 0x10) {
                            status.dma = 1;
                            if (!(reg[0x17] & 0x80)) {
                                do_68k_dma();
                            }
                            else if ((reg[0x17] & 0xC0) == 0xC0) {
                                OutputDebugString("VRAM to VRAM DMA\n");
                            }
                            else if ((reg[0x17] & 0xC0) == 0x80) {
                                OutputDebugString("DMA Fill\n");
                                pending_fill = 1;
                            }
                        }
                    }
                    x = 1;
                    address_write = 0;
                }
                else {
                    //address_reg = value << 16;
                    address_reg = (address_reg & 0x0000FFFF) | (value << 16);
                    _address = ((address_reg & 0x3) << 14) | ((address_reg >> 16) & 0x3FFF);
                    address_write = 1;
                }
                //address_write ^= 1;
            }
            break;
        default: {
            int x = 1;
        }
            break;
    }
}

uint8_t c_vdp::read_byte(uint32_t address)
{
    uint8_t ret;
    switch (address) {
        case 0x00C00005:
        case 0x00C00007:
            ret = status.value & 0xFF;
            status.vint = 0;
            status.dma = 0;
            *ipl &= ~0x6;
            return ret;
        default:
            return 0;
    }
    return 0;
}

void c_vdp::write_byte(uint32_t address, uint8_t value)
{
    if (value != 0) {
        int x = 1;
    }
    uint16_t v = (value << 8) | value;
    switch (address) {
        case 0x0C00000:
        case 0x0C00001:
        case 0x0C00002:
        case 0x0C00003:
        case 0x0C00004:
        case 0x0C00005:
        case 0x0C00006:
        case 0x0C00007:
            //not sure if this is correct
            write_word(address & ~1, v);
            break;
        default: {
            int x = 1;
        }
            break;
    }
}

uint32_t c_vdp::lookup_color(uint32_t pal, uint32_t index)
{
    int loc = pal * 32 + index * 2;
    uint16_t entry = (cram[loc] << 8) | cram[loc + 1];
    uint32_t color = ((entry >> 0) & 0xE) * 16;
    color |= (((entry >> 4) & 0xE) * 16) << 8;
    color |= (((entry >> 8) & 0xE) * 16) << 16;
    color |= 0xFF000000;
    return color;
}

void c_vdp::draw_scanline()
{
    int y = line;
    //int plane_height = (reg[0x10] >> 4) & 0x3;
    //int plane_width = reg[0x10] & 0x3;

    int plane_width = 32;
    int plane_height = 32;

    switch ((reg[0x10] >> 4) & 0x3) {
        case 1:
            plane_height = 64;
            break;
        case 3:
            plane_height = 128;
            break;
        default:
            break;
    }
    switch (reg[0x10] & 0x3) {
        case 1:
            plane_width = 64;
            break;
        case 3:
            plane_width = 128;
            break;
        default:
            break;
    }


    uint32_t a_nt = (reg[0x02] & 0x38) << 10;
    uint32_t b_nt = (reg[0x04] & 0x7) << 13;
    
    int num_tiles = plane_width;
    int y_address = ((y / 8) % plane_height) * num_tiles * 2;
    
    if (line < 224) {
        uint32_t *fb = &frame_buffer[y * 320];
        for (int column = 0; column < 40; column++) {
            unsigned int nt_column = column % plane_width;
            uint16_t a_nt_address = a_nt + y_address + (nt_column * 2);
            uint16_t b_nt_address = b_nt + y_address + (nt_column * 2);
            uint32_t a_tile = (vram[a_nt_address] << 8) | vram[a_nt_address + 1];
            uint32_t b_tile = (vram[b_nt_address] << 8) | vram[b_nt_address + 1];

            uint32_t a_tile_number = a_tile & 0x7FF;
            uint32_t b_tile_number = b_tile & 0x7FF;
            uint32_t a_priority = a_tile >> 15;
            uint32_t b_priority = b_tile >> 15;
            uint32_t a_pal = (a_tile >> 13) & 0x7;
            uint32_t b_pal = (b_tile >> 13) & 0x7;
            uint32_t a_pattern_address = a_tile_number * 32 + ((line & 7) * 4);
            uint32_t b_pattern_address = b_tile_number * 32 + ((line & 7) * 4);
            uint32_t a_pattern = *((uint32_t *)&vram[a_pattern_address]);
            a_pattern = std::byteswap(a_pattern);
            uint32_t b_pattern = *((uint32_t *)&vram[b_pattern_address]);
            b_pattern = std::byteswap(b_pattern);
            
            
            
            for (int x = 0; x < 8; x++) {
                uint8_t a_pixel = (a_pattern >> ((7-x) * 4)) & 0xF;
                uint8_t b_pixel = (b_pattern >> ((7-x) * 4)) & 0xF;
                uint8_t pixel = 0;
                uint8_t palette = 0;

                //to-do: background color

                if (!b_priority && b_pixel) {
                    pixel = b_pixel;
                    palette = b_pal;
                }
                if (!a_priority && a_pixel) {
                    pixel = a_pixel;
                    palette = a_pal;
                }
                //to-do: sprites
                if (b_priority && b_pixel) {
                    pixel = b_pixel;
                    palette = b_pal;
                }
                if (a_priority && a_pixel) {
                    pixel = a_pixel;
                    palette = a_pal;
                }

                //uint32_t a_color = cram[(a_pal * 32) + (pixel * 2)];
                *fb = lookup_color(palette, pixel);
                //*fb = lookup_color(a_pal, a_pixel);

                fb++;
            }
            int x = 1;
        }
    }

    if (line == 224) {
        status.vblank = 1;
        if (reg[0x01] & 0x20) {
            status.vint = 1;
            *ipl |= 0x6;
        }
    }
    else if (line == 255) {
        status.vblank = 0;
        //does interrupt get cleared automatically?
        status.vint = 0;
        *ipl &= ~0x6;
    }

    if (line == 261) {
        line = 0;
    }
    else {
        line++;
    }
}

void c_vdp::do_68k_dma()
{
    uint16_t len = reg[0x13] | (reg[0x14] << 8);
    if (len == 0) {
        len = 0xFFFF;
    }
    *stalled = len * 3;
    uint32_t src = reg[0x15] | (reg[0x16] << 8) | ((reg[0x17] & 0x7F) << 16);
    if (src & 0x1) {
        int x = 1;
    }
    src <<= 1;
    src &= ~1;
    if (address_type == ADDRESS_TYPE::CRAM_WRITE) {
        int x = 1;
    }
    do {
        uint16_t data = read_word_68k(src);
        src += 2;
        if (src == 0x01000000) {
            src = 0xFF0000;
        }
        write_word(0x0C00000, data);

        len--;
    } while (len != 0);
    int x = 1;
}


} //namespace genesis