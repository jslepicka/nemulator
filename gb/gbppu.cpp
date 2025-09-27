module;
#include <assert.h>

module gb:ppu;
import gb;
import :apu;

namespace gb {

// clang-format off
const unsigned int c_gbppu::palettes[][4] = {
    {
        //0 - lime green
        //https://www.color-hex.com/color-palette/26401
        0xFF0FBC9B,
        0xFF0FAC8B,
        0xFF306230,
        0xFF0F380F
    },
    {
        //1 - green-yellow from libretro
        //https://docs.libretro.com/library/gambatte/
        0xFF10827B,
        0xFF42795A,
        0xFF4A5939,
        0xFF394129
    },
    {
        //2 - green
        //https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-Greenscale-Ver-A-classic-808011585
        0xFF0CBE9C,
        0xFF0A876E,
        0xFF34622C,
        0xFF0C360C
    },
    {
        //3 - brightened green-yellow libretro
        0xFF24968F,
        0xFF568D6E,
        0xFF5A6949,
        0xFF474F37
    },
    {
        //4 - gameboy pocket
        //https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-Pocket-Ver-808181843
        0xFFA1CFC4,
        0xFF6D958B,
        0xFF3C534D,
        0xFF1F1F1F
        
    },
    {
        //5 - yellow-green
        //https://lospec.com/palette-list/nostalgia
        0xFF58D0D0,
        0xFF40A8A0,
        0xFF288070,
        0xFF105040
    },
    {
        //6 - DMG
        //https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-DMG-Ver-808181265
        0xFF0F867F,
        0xFF447C57,
        0xFF485D36,
        0xFF3B452A
    },
    {
        //7 - black zero
        //http://www.emutalk.net/threads/55441-Game-Boy-Mono-quot-True-quot-colors
        0xFF0F867F,
        0xFF457C57,
        0xFF485D36,
        0xFF3B452A
    },
    {
        //8 - bgb lcd green
        0xFFD0F8E0,
        0xFF70C088,
        0xFF566834,
        0xFF201808
    },
    {
        //9 - shader
        0xFF02988B,
        0xFF027055,
        0xFF02532A,
        0xFF024816
    },
    {
        //10 - RokkumanX
        //https://github.com/libretro/gambatte-libretro/issues/130
        0xFF4FC084,
        0xFF68A66A,
        0xFF68864B,
        0xFF586636
    },
    {
        //11 - Greyscale, gamma adjusted
        0xFFF0F0F0,
        0xFF878787,
        0xFF373737,
        0xFF0C0C0C
    },
    {
        // 12 - Lime Midori
        //https://www.deviantart.com/thewolfbunny/art/Game-Boy-Palette-Lime-Midori-810574708
        0xFFAFEBE0,
        0xFF53CFAA,
        0xFF428D7B,
        0xFF505947,
    },
    {
        //13 - retroarch more contrast
        0xFF1D8F88,
        0xFF4C8364,
        0xFF4A5939,
        0xFF323A22
    },
    {
        //14 - photo
        0xFF08878F,
        0xFF317C63,
        0xFF3C5E35,
        0xFF25442E
    },
    {
        //15 - photo 3x3 sampling
        0xFF078990,
        0xFF337C64,
        0xFF376244,
        0xFF20462C

    }
};
// clang-format on

std::atomic<int> c_gbppu::color_lookup_built = 0;
uint32_t c_gbppu::color_lookup[32];

c_gbppu::c_gbppu(c_gb *gb)
{
    this->gb = gb;
    vram = std::make_unique_for_overwrite<uint8_t[]>(16384);
    vram1 = std::make_unique_for_overwrite<uint8_t[]>(16384);
    oam = std::make_unique_for_overwrite<uint8_t[]>(160);
    fb = std::make_unique_for_overwrite<uint32_t[]>(160 * 144);
    fb_back = std::make_unique_for_overwrite<uint32_t[]>(160 * 144);
    palette = palettes[15];
    generate_color_lookup();
}

c_gbppu::~c_gbppu()
{
    int x = 1;
}

void c_gbppu::generate_color_lookup()
{
    int expected = 0;
    if (color_lookup_built.compare_exchange_strong(expected, 1)) {
        double gamma = 1.0 / 1.2;
        for (int i = 0; i < 32; i++) {
            double x = i;
            //this curve is based off of sameboy's curve.  Dropped into excel and fit this polynomial to it.
            color_lookup[i] =
                std::clamp((uint32_t)(-.0148 * (x * x * x) + .6243 * (x * x) + 2.9731 * x), (uint32_t)0, (uint32_t)255);
        }
    }
    int x = 1;
}

void c_gbppu::reset()
{
    line = 0;
    current_cycle = 0;
    SCX = 0;
    SCY = 0;
    STAT = 0;
    LCDC = 0x91;
    BGP = 0xFC;
    OBP0 = 0xFF;
    OBP1 = 0xFF;
    LY = 0;
    LYC = 0;
    WY = 0;
    WX = 0;
    DMA = 0;

    cpu_divider = 0;
    mode = 2;
    first_tile = 1;
    fetch_phase = 0;
    obj_fetch_phase = 0;
    tile = 0;
    obj_p0 = 0;
    obj_p1 = 0;
    char_addr = 0;
    ybase = 0;
    p0_addr = 0;
    p1_addr = 0;
    current_pixel = 0;
    start_vblank = 0;
    fetch_x = 0;
    window_tile = 0;
    window_start_line = -1;

    dma_count = 0;
    sprite_count = 0;

    lcd_paused = 0;

    bg_latched = 0;

    stat_irq = 0;
    prev_stat_irq = 0;
    start_hblank = 0;
    SCX_latch = 0;

    cgb_vram_bank = 0;
    BCPS = 0;
    OBPS = 0;

    cgb_bg_attr = 0;
    KEY1 = 0;
    double_speed = 0;

    OPRI = 0;
    HDMA1 = 0;
    HDMA2 = 0;
    HDMA3 = 0;
    HDMA4 = 0;
    HDMA5 = 0;

    hdma_general_count = 0;
    hdma_hblank_count = 0;
    hdma_source = 0;
    hdma_dest = 0;
    hdma_length = 0;
    sprite_tile = 0;
    current_sprite = -1;
    bg_p0 = 0;
    bg_p1 = 0;
    fetching_sprites = 0;
    in_window = 0;
    done_drawing = 0;
    pixels_out = 0;
    sprites_here = 0;
    sprite_addr = 0;
    sprite_y_offset = 0;
    in_sprite_window = 0;
    bg_fifo_index = 0;
    obj_fifo_index = 0;

    std::memset(sprite_buffer, 0, sizeof(sprite_buffer));
    std::memset(vram.get(), 0, 16384);
    std::memset(vram1.get(), 0, 16384);
    std::memset(fb.get(), 0, 160 * 144 * sizeof(uint32_t));
    std::memset(fb_back.get(), 0, 160 * 144 * sizeof(uint32_t));
    std::memset(bg_fifo, 0, sizeof(bg_fifo));
    std::memset(obj_fifo, 0, sizeof(obj_fifo));
    std::memset(oam.get(), 0, sizeof(uint8_t) * 160);
    std::memset(cgb_bg_pal, 0xFF, sizeof(cgb_bg_pal));
}

void c_gbppu::eval_sprites(int y)
{
    int h = LCDC & 0x4 ? 16 : 8;
    if (h == 16) {
        int x = 1;
    }
    std::memset(sprite_buffer, 0, sizeof(sprite_buffer));
    s_sprite *s = (s_sprite *)oam.get();
    sprite_count = 0;
    for (int i = 0; i < 40; i++) {
        int sprite_y = s->y - 16;
        if (y >= sprite_y && y < (sprite_y + h)) {
            //sprite is in range, copy to sprite_buffer
            std::memcpy(&sprite_buffer[sprite_count], s, sizeof(s_sprite));
            if (h == 16) {
                sprite_buffer[sprite_count].tile &= ~0x1;
            }
            sprite_count++;
            if (sprite_count == 10)
                return;
        }
        s++;
    }
}

void c_gbppu::update_stat()
{
    if ((STAT & 0x44) == 0x44 || (mode == 0 && (STAT & 0x8)) || (mode == 1 && (STAT & 0x10)) ||
        (mode == 2 && (STAT & 0x20))) {
        stat_irq = 1;
        if (prev_stat_irq == 0) {
            gb->set_stat_irq(1);
        }
        prev_stat_irq = 1;
    }
    else {
        stat_irq = 0;
        if (prev_stat_irq == 1) {
            //gb->set_stat_irq(0);
        }
        prev_stat_irq = 0;
    }
}

void c_gbppu::set_ly(int line)
{
    if (LY == line)
        return;
    LY = line;
    if (LY == LYC) {
        STAT |= 0x4;
    }
    else {
        STAT &= ~0x4;
    }
    update_stat();
}

void c_gbppu::on_stop()
{
    if (KEY1 & 0x1) {
        double_speed ^= 0x1;
        KEY1 = double_speed ? 0x80 : 0;
    }
}

void c_gbppu::do_bg_fetch()
{
    switch (fetch_phase) {
        case 0: //sleep
            if (bg_fifo[bg_fifo_index].valid == 0) { //only proceed if bg_fifo is empty
                in_sprite_window = 0;
                if (bg_latched) {
                    bg_latched = 0;
                    //horizontal flip
                    int h_xor = cgb_bg_attr & 0x20 ? 7 : 0;
                    for (int i = 0; i < 8; i++) {
                        int shift = (7 - i) ^ h_xor;
                        uint8_t p = ((bg_p0 >> shift) & 0x1) | (((bg_p1 >> shift) & 0x1) << 1);
                        bg_fifo[bg_fifo_index].pattern = p;
                        bg_fifo[bg_fifo_index].priority = cgb_bg_attr & 0x80;
                        if (gb->get_model() == GB_MODEL::CGB) {
                            bg_fifo[bg_fifo_index].cgb_pal = cgb_bg_attr & 0x7;
                        }
                        bg_fifo[bg_fifo_index].valid = 1;
                        bg_fifo_index = (bg_fifo_index + 1) & 0x7;
                    }
                }
                fetch_phase++;
            }
            break;
        case 1: //fetch nt
            if (true || first_tile) {
                if (in_window) {
                    ybase = window_start_line - WY;
                    char_addr = 0x9800 | ((LCDC & 0x40) << 4) | ((ybase & 0xF8) << 2) | window_tile;
                }
                else {
                    ybase = (line + SCY) & 0xFF;
                    uint32_t xbase = ((SCX >> 3) + fetch_x) & 0x1F;
                    char_addr = 0x9800 | ((LCDC & 0x8) << 7) | ((ybase & 0xF8) << 2) | xbase;
                }
            }
            tile = vram[char_addr - 0x8000];
            if (gb->get_model() == GB_MODEL::CGB) {
                cgb_bg_attr = vram1[char_addr - 0x8000];
            }
            fetch_phase++;
            break;
        case 2: //sleep
            fetch_phase++;
            break;
        case 3: //fetch pt0
        {
            //vertical flip
            uint32_t v_xor = cgb_bg_attr & 0x40 ? 7 : 0;
            uint32_t y_adj = (ybase & 0x7) ^ v_xor;
            if (LCDC & 0x10) {
                p0_addr = (tile << 4) | (y_adj << 1);
            }
            else {
                p0_addr = 0x800 + (((int8_t)tile + 128) << 4) + (y_adj << 1);
            }
            if (gb->get_model() == GB_MODEL::CGB) {
                if (cgb_bg_attr & 0x8) {
                    bg_p0 = vram1[p0_addr];
                }
                else {
                    bg_p0 = vram[p0_addr];
                }
            }
            else {
                bg_p0 = vram[p0_addr];
            }
            fetch_phase++;
        } break;
        case 4: //sleep
            fetch_phase++;
            break;
        case 5: //fetch pt1
            if (gb->get_model() == GB_MODEL::CGB) {
                if (cgb_bg_attr & 0x8) {
                    bg_p1 = vram1[p0_addr + 1];
                }
                else {
                    bg_p1 = vram[p0_addr + 1];
                }
            }
            else {
                bg_p1 = vram[p0_addr + 1];
            }
            bg_latched = 1;
            if (!first_tile || in_window) {
                ;
                fetch_x++;
                window_tile++;
                in_sprite_window = 1;
                fetch_phase++;
            }
            else {
                fetch_phase = 0;
            }
            first_tile = 0;
            break;
        case 6:
            fetch_phase++;
            break;
        case 7:
            fetch_phase = 0;
            break;
        default:
            __assume(0);
    }
}

void c_gbppu::do_obj_fetch()
{
    if (sprites_here && in_sprite_window) {
        fetching_sprites = 1;
        int h = LCDC & 0x4 ? 16 : 8;
        switch (obj_fetch_phase) {
            case 0: //oam read
                for (int i = current_sprite + 1; i < sprite_count; i++) {
                    if (sprite_buffer[i].x == current_pixel - (SCX_latch & 0x7)) {
                        current_sprite = i;
                        sprite_tile = sprite_buffer[current_sprite].tile;
                        if (h == 16) {
                            sprite_tile &= ~0x1;
                        }
                        sprite_y_offset = (line - sprite_buffer[current_sprite].y) & (h - 1);
                        if (sprite_buffer[current_sprite].flags & 0x40) {
                            sprite_y_offset = (h - 1) - sprite_y_offset;
                        }
                        break;
                    }
                }
                obj_fetch_phase++;
                break;
            case 1: //oam read
                obj_fetch_phase++;
                break;
            case 2:
                obj_fetch_phase++;
                break;
            case 3: {
                sprite_addr = (sprite_tile << 4) | (sprite_y_offset << 1);
                if (sprite_buffer[current_sprite].tile == 0x60) {
                    int x = 1;
                }
                if (gb->get_model() == GB_MODEL::CGB) {
                    if (sprite_buffer[current_sprite].flags & 0x8) {
                        obj_p0 = vram1[sprite_addr];
                    }
                    else {
                        obj_p0 = vram[sprite_addr];
                    }
                }
                else {
                    obj_p0 = vram[sprite_addr];
                }
                obj_fetch_phase++;
            } break;
            case 4:
                obj_fetch_phase++;
                break;
            case 5:
                if (gb->get_model() == GB_MODEL::CGB) {
                    if (sprite_buffer[current_sprite].flags & 0x8) {
                        obj_p1 = vram1[sprite_addr + 1];
                    }
                    else {
                        obj_p1 = vram[sprite_addr + 1];
                    }
                }
                else {
                    obj_p1 = vram[sprite_addr + 1];
                }
                {
                    uint32_t obj_pri = sprite_buffer[current_sprite].flags & 0x80;
                    uint32_t obj_pal = sprite_buffer[current_sprite].flags & 0x10;
                    for (int i = 0; i < 8; i++) {
                        int shift = i;
                        if (!(sprite_buffer[current_sprite].flags & 0x20)) {
                            shift ^= 7;
                        }
                        uint8_t p = ((obj_p0 >> shift) & 0x1) | (((obj_p1 >> shift) & 0x1) << 1);

                        s_obj_fifo_entry *o = &obj_fifo[(obj_fifo_index + i) & 0x7];

                        //sprite here
                        uint32_t has_priority = !o->valid;

                        if (gb->get_model() == GB_MODEL::CGB && o->valid) {
                            if (current_sprite < o->oam_index) {
                                has_priority = 1;
                            }
                        }
                        if (p != 0 && has_priority) {
                            if (gb->get_model() == GB_MODEL::CGB) {
                                o->cgb_pal = sprite_buffer[current_sprite].flags & 0x7;
                            }
                            else {
                                o->dmg_pal = obj_pal;
                            }
                            o->pattern = p;
                            o->priority = obj_pri;
                            o->valid = 1;
                            o->oam_index = current_sprite;
                        }
                    }
                }
                obj_fetch_phase = 0;
                if (--sprites_here == 0) {
                    fetching_sprites = 0;
                    lcd_paused = 0;
                }
                break;
            default:
                __assume(0);
        }
    }
}

void c_gbppu::output_pixel()
{
    uint8_t pattern;
    pattern = bg_fifo[bg_fifo_index].pattern;

    int pal = BGP;
    uint32_t cgb_pal = 0;
    if (gb->get_model() == GB_MODEL::CGB) {
        pal = bg_fifo[bg_fifo_index].cgb_pal;
        uint8_t pal1 = cgb_bg_pal[pal * 8 + (pattern * 2)];
        uint8_t pal2 = cgb_bg_pal[pal * 8 + (pattern * 2) + 1];
        cgb_pal = pal1 | pal2 << 8;
    }
    if (!(LCDC & 0x1)) {
        pattern = 0;
    }

    //if there is a sprite for this pixel
    if (obj_fifo[obj_fifo_index].valid) {
        s_obj_fifo_entry *s = &obj_fifo[obj_fifo_index];

        //priority check
        if (gb->get_model() == GB_MODEL::CGB) {
            if (pattern == 0 || (LCDC & 0x1) == 0 || (s->priority | bg_fifo[bg_fifo_index].priority) == 0) {
                pattern = s->pattern;
                uint8_t pal1 = cgb_ob_pal[s->cgb_pal * 8 + (pattern * 2)];
                uint8_t pal2 = cgb_ob_pal[s->cgb_pal * 8 + (pattern * 2) + 1];
                cgb_pal = pal1 | pal2 << 8;
            }
        }
        else {
            if (s->priority == 0 || pattern == 0) {
                pattern = s->pattern;
                pal = s->dmg_pal ? OBP1 : OBP0;
            }
        }
        obj_fifo[obj_fifo_index] = {0};
    }
    obj_fifo_index = (obj_fifo_index + 1) & 0x7;
    bg_fifo[bg_fifo_index].valid = 0;
    bg_fifo_index = (bg_fifo_index + 1) & 0x7;

    if (current_pixel >= (8 + (SCX_latch & 0x7))) {
        if (gb->get_model() == GB_MODEL::CGB) {
            uint8_t r = cgb_pal & 0x1F;
            uint8_t g = (cgb_pal >> 5) & 0x1F;
            uint8_t b = (cgb_pal >> 10) & 0x1F;

            uint32_t col = color_lookup[r] | (color_lookup[g] << 8) | (color_lookup[b] << 16) | (0xFF << 24);
            *f++ = col;
            pixels_out++;
            if (pixels_out == 160) {
                done_drawing = 1;
            }
        }
        else {
            *f++ = palette[(pal >> (pattern * 2)) & 0x3];
            pixels_out++;
            if (pixels_out == 160) {
                done_drawing = 1;
            }
        }
    }
    current_pixel++;
}

//hblank
void c_gbppu::exec_mode0()
{
    if (start_hblank) {
        start_hblank = 0;
        update_stat();
        if (hdma_hblank_count) {
            hdma_length = 16;
            hdma_hblank_count -= 16;
        }
    }
}

//vblank
void c_gbppu::exec_mode1()
{
    if (start_vblank) {
        gb->set_vblank_irq(1);
        start_vblank = 0;
    }
}

//oam scan
void c_gbppu::exec_mode2()
{
    if (current_cycle == 79) {
        lcd_paused = 0;
        eval_sprites(line);
        mode = 3;
        update_stat();
        first_tile = 1;
        fetch_phase = 0;
        current_pixel = 0;
        f = fb_back.get() + (line * 160);
        if (!(LCDC & 0x80)) {
            for (int j = 0; j < 160; j++) {
                *(f + j) = 0xFFFFFFFF;
            }
        }
        bg_latched = 0;

        memset(obj_fifo, 0, sizeof(obj_fifo));
        memset(bg_fifo, 0, sizeof(bg_fifo));

        bg_fifo_index = 0;
        obj_fifo_index = 0;
        obj_fetch_phase = 0;
        in_sprite_window = 0;
        sprites_here = 0;
        pixels_out = 0;
        done_drawing = 0;
        start_hblank = 0;
        in_window = 0;
        fetching_sprites = 0;
        SCX_latch = SCX;
        fetch_x = 0;
    }
}

//rendering
void c_gbppu::exec_mode3()
{
    //if any sprites on current line, set fetch_sprite to number of sprites
    //and pause lcd
    if ((LCDC & 0x2) && !first_tile && current_pixel != 0 && sprites_here == 0) {
        for (int i = 0; i < sprite_count; i++) {
            if (sprite_buffer[i].x == current_pixel - (SCX_latch & 0x7)) {
                lcd_paused = 1;
                sprites_here++;
            }
        }
        current_sprite = -1;
    }
    if ((LCDC & 0x20) && line >= WY) {
        int x = 1;
    }
    if ((LCDC & 0x20) && !in_window && !fetching_sprites && line >= WY) {
        if (current_pixel - 1 - (SCX_latch & 0x7) == WX) {
            in_window = 1;
            memset(bg_fifo, 0, sizeof(bg_fifo));
            fetch_phase = 0;
            bg_latched = 0;
            first_tile = 1;
            window_tile = 0;
            if (window_start_line == -1) {
                window_start_line = line;
            }
        }
    }

    do_bg_fetch();
    do_obj_fetch();

    if (!lcd_paused && bg_fifo[bg_fifo_index].valid) {
        output_pixel();
    }

    if (current_cycle >= 251 && done_drawing) {
        mode = 0;
        start_hblank = 1;
    }
}

void c_gbppu::execute(int cycles)
{
    while (cycles-- > 0) {
        if (LCDC & 0x80) {

            if (current_cycle == 0) {
                set_ly(line);
            }
            else if (current_cycle == 1) {
                if (line == 153) {
                    set_ly(0);
                }
            }

            switch (mode) {
                case 0:
                    //mode 0 - hblank
                    //85 - 208 dots
                    //248 .. 455
                    exec_mode0();
                    break;
                case 1:
                    //mode 1 - vblank
                    //10 lines
                    exec_mode1();
                    break;
                case 2:
                    //mode 2 - scanning OAM
                    //80 dots
                    //0 .. 79
                    exec_mode2();
                    break;
                case 3:
                    //mode 3 - rendering
                    //168 - 291 dots
                    //80 .. 247
                    exec_mode3();
                    break;
                default:
                    __assume(0);
            }

            if (current_cycle++ == 455) { //end of line
                //end of line
                current_cycle = 0;
                line++;
                if (window_start_line != -1 && in_window) {
                    window_start_line++;
                }
                if (line == 144) {
                    //begin vblank
                    start_vblank = 1;
                    mode = 1;
                    update_stat();
                    std::swap(fb, fb_back);
                }
                else if (line == 154) {
                    //end vblank
                    //gb->set_vblank_irq(0);
                    line = 0;
                    mode = 2;
                    update_stat();
                    window_start_line = -1;
                }
                else if (line < 144) {
                    mode = 2;
                    update_stat();
                }
            }
        }
        cpu_divider = (cpu_divider + 1) & 0x3;
        if (cpu_divider == 0 || (double_speed && cpu_divider == 2)) {
            gb->clock_timer();
            if (hdma_length) {
                hdma_length--;
                assert(hdma_source < 0x8000 || (hdma_source >= 0xA000 && hdma_source < 0xE000));
                assert(hdma_dest >= 0x8000 && hdma_dest < 0xA000);
                write_byte(hdma_dest++, gb->read_byte(hdma_source++));
            }
            else {
                if (dma_count) {
                    if (dma_count < 161) {
                        int x = 160 - dma_count;
                        oam[x] = gb->read_byte(DMA + x);
                    }
                    dma_count--;
                }
                //cpu executes during normal dma but is halted during hdma
                gb->cpu->execute(4);
            }
        }
        if (cpu_divider == 0) {
            //does apu run during hdma?
            gb->apu->clock();
        }
    }
}

uint8_t c_gbppu::read_byte(uint16_t address)
{
    if (address < 0xA000) {
        if (gb->get_model() == GB_MODEL::CGB) {
            if (cgb_vram_bank & 0x1) {
                return vram1[address - 0x8000];
            }
            else {
                return vram[address - 0x8000];
            }
        }
        return vram[address - 0x8000];
    }
    else if (address >= 0xFE00 && address <= 0xFE9F) {
        return oam[address - 0xFE00];
    }
    else {
        switch (address) {
            case 0xFF40:
                return LCDC;
            case 0xFF41:
                return (STAT & ~0x3) | (LCDC & 0x80 ? (mode & 0x3) : 0);
            case 0xFF42:
                return SCY;
            case 0xFF43:
                return SCX;
            case 0xFF44:
                return LY;
            case 0xFF45:
                return LYC;
            case 0xFF46:
                return DMA;
            case 0xFF47:
                return BGP;
            case 0xFF48:
                return OBP0;
            case 0xFF49:
                return OBP1;
            case 0xFF4A:
                return WY;
            case 0xFF4B:
                return WX;
            case 0xFF4D:
                return KEY1;
            case 0xFF4F:
                return 0xFE | (cgb_vram_bank & 0x1);
            case 0xFF55:
                if (HDMA5 & 0x80 && hdma_hblank_count) {
                    int remaining_blocks = (hdma_hblank_count >> 4) - 1;
                    return remaining_blocks;
                }
                return 0xFF;
            case 0xFF68:
                return BCPS;
            case 0xFF69:
                return cgb_bg_pal[BCPS & 0x3F];
            case 0xFF6A:
                return OBPS;
            case 0xFF6B:
                return cgb_ob_pal[OBPS & 0x3F];
            case 0xFF6C:
                return OPRI;
            default:
                //printf("unhandled read from ppu\n");
                //exit(0);
                int x = 1;
                break;
        }
    }
    return 0;
}

void c_gbppu::write_byte(uint16_t address, uint8_t data)
{
    auto update_hdma_source = [&]() { hdma_source = ((HDMA1 << 8) | (HDMA2)) & 0xFFF0; };
    auto update_hdma_dest = [&]() { hdma_dest = 0x8000 + (((HDMA3 << 8) | (HDMA4)) & 0x1FF0); };

    if (address < 0xA000) {
        if (gb->get_model() == GB_MODEL::CGB) {
            if (cgb_vram_bank & 0x1) {
                vram1[address - 0x8000] = data;
            }
            else {
                vram[address - 0x8000] = data;
            }
        }
        else {
            vram[address - 0x8000] = data;
        }
    }
    else if (address >= 0xFE00 && address <= 0xFE9F) {
        oam[address - 0xFE00] = data;
    }
    else {
        switch (address) {
            case 0xFF40:
                if (LCDC & 0x80 && !(data & 0x80)) {
                    LY = 0;
                    line = 0;
                    current_cycle = 0;
                    update_stat();
                    uint32_t col = palette[0];
                    if (gb->get_model() == GB_MODEL::CGB) {
                        col = 0xFFFFFFFF;
                    }
                    std::fill_n(fb.get(), 160 * 144, col);
                }
                LCDC = data;
                break;
            case 0xFF41:
                STAT = data;
                update_stat();
                break;
            case 0xFF42:
                SCY = data;
                break;
            case 0xFF43:
                SCX = data;
                break;
            case 0xFF44:
                LY = data;
                break;
            case 0xFF45:
                LYC = data;
                if (LY == LYC) {
                    STAT |= 0x4;
                }
                else {
                    STAT &= ~0x4;
                }
                update_stat();
                break;
            case 0xFF46:
                DMA = data << 8;
                dma_count = 160;
                break;
            case 0xFF47:
                BGP = data;
                break;
            case 0xFF48:
                OBP0 = data;
                break;
            case 0xFF49:
                OBP1 = data;
                break;
            case 0xFF4A:
                WY = data;
                break;
            case 0xFF4B:
                WX = data;
                break;
            case 0xFF4D:
                KEY1 = (KEY1 & 0xFE) | data & 1;
                break;
            case 0xFF4F:
                cgb_vram_bank = 0xFE | (data & 0x1);
                break;
            case 0xFF51: //HDMA1
                HDMA1 = data;
                update_hdma_source();
                break;
            case 0xFF52: //HDMA2
                HDMA2 = data;
                update_hdma_source();
                break;
            case 0xFF53: //HDMA3
                HDMA3 = data;
                update_hdma_dest();
                break;
            case 0xFF54: //HDMA4
                HDMA4 = data;
                update_hdma_dest();
                break;
            case 0xFF55: //HDMA5
            {
                HDMA5 = data;
                int x = 1;
                int length = ((data & 0x7F) + 1) * 0x10;
                if (data & 0x80) {
                    if (mode == 0) {
                        hdma_length = 16;
                        hdma_hblank_count = length - 16;
                    }
                    else {
                        hdma_hblank_count = length;
                    }
                }
                else {
                    hdma_length = length;
                }

            } break;
            case 0xFF68:
                BCPS = data;
                break;
            case 0xFF69: {
                int i = BCPS & 0x3F;
                cgb_bg_pal[i] = data;
                if (BCPS & 0x80) {
                    i += 1;
                    BCPS = 0x80 | (i & 0x3F);
                }
            } break;
            case 0xFF6A:
                OBPS = data;
                break;
            case 0xFF6B: {
                int i = OBPS & 0x3F;
                cgb_ob_pal[i] = data;
                if (OBPS & 0x80) {
                    i += 1;
                    OBPS = 0x80 | (i & 0x3F);
                }
            } break;
            case 0xFF6C:
                OPRI = data & 1;
                break;

            default:
                //printf("unhandled write to ppu\n");
                //exit(0);
                int x = 1;
                break;
        }
    }
    return;
}

} //namespace gb