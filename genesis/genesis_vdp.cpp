module;
#include <cassert>
#include <Windows.h>
module genesis:vdp;

namespace genesis
{

c_vdp::c_vdp(uint8_t *ipl)
{
    this->ipl = ipl;
}

c_vdp::~c_vdp()
{
}

void c_vdp::reset()
{
    //status.value = 0x3400;
    status.value = 0;
    status.fifo_empty = 1;
    line = 0;
    std::memset(reg, 0, sizeof(reg));
    address_reg = 0;
    address_write = 0;
    memset(vram, 0, sizeof(vram));
    memset(cram, 0, sizeof(cram));
    memset(vsram, 0, sizeof(vsram));
    memset(frame_buffer, 0, sizeof(frame_buffer));
    line = 0;
}

uint16_t c_vdp::read_word(uint32_t address)
{
    uint16_t ret;
    switch (address) {
        case 0x00C00004:
        case 0x00C00006:
            ret = status.value;
            status.vint = 0;
            status.dma = 0;
            *ipl &= ~0x6;
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
            x = 1;
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
                    //assert(0);
                    break;
            }
            _address += reg[0x0F];
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
                    address_reg |= value;
                    address_type = (ADDRESS_TYPE)((((address_reg & 0xF0) | (address_reg >> 28)) >> 2) & 0xF);
                    _address = (address_reg << 14) | ((address_reg >> 16) & 0x3FFF);
                    vram_to_vram_copy = address_reg & 0x40;
                    dma_copy = address_reg & 0x80;
                    if (dma_copy) {
                        OutputDebugString("DMA requested\n");
                        status.dma = 1;
                    }
                    x = 1;
                }
                else {
                    address_reg = value << 16;
                }
                address_write ^= 1;
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
    uint32_t color = ((entry >> 0) & 0xF) * 16;
    color |= (((entry >> 4) & 0xF) * 16) << 8;
    color |= (((entry >> 8) & 0xF) * 16) << 16;
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
            if (a_tile_number) {
                int x = 1;
            }
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
                
                //uint32_t a_color = cram[(a_pal * 32) + (pixel * 2)];
                *fb = lookup_color(a_pal, a_pixel);

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


} //namespace genesis