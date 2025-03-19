module;

module nes;
import :cartdb;
import :ines;
import crc32;

namespace nes
{

c_nes::c_nes()
{
    ram = 0;
    sram = 0;
    cpu = 0;
    ppu = 0;
    mapper = 0;
    image = 0;
    joypad = 0;
    loaded = false;
    limit_sprites = false;
    crc32 = 0;
    game_genie = std::make_unique<c_game_genie>();
    mapper_info = 0;
}

c_nes::~c_nes()
{
}

void c_nes::enable_mixer()
{
    apu->enable_mixer();
}

void c_nes::disable_mixer()
{
    apu->disable_mixer();
}

void c_nes::set_sprite_limit(bool limit_sprites)
{
    this->limit_sprites = limit_sprites;
    ppu->set_sprite_limit(limit_sprites);
}

bool c_nes::get_sprite_limit()
{
    return ppu->get_sprite_limit();
}

unsigned char c_nes::dmc_read(unsigned short address)
{
    //cpu->availableCycles -= 12;
    cpu->execute_apu_dma();
    return read_byte(address);
}

unsigned char c_nes::read_byte(unsigned short address)
{
    //unlimited health in holy diver
    //if (address == 0x0440) {
    //    return 6;
    //}
    //start battletoads on level 2
    //if (address == 0x8320) {
    //    return 0x2;
    //}
    //journey to silius unlimited energy
    //   if (address == 0x00B0) {
    //       return 0x0F;
    //   }
    ////journey to silius unlimited gun power
    //   if (address == 0x00B1) {
    //       return 0x3F;
    //   }
    switch (address >> 12) {
        case 0:
        case 1:
            return ram[address & 0x7FF];
            break;
        case 2:
        case 3:
        //if (address <= 0x2007)
            return ppu->read_byte(address & 0x2007);
            break;
        case 4:
            if (address >= 0x4020) {
                return mapper->read_byte(address);
            }
            switch (address) {
                case 0x4015:
                    return apu->read_byte(address);
                    break;
                case 0x4016:
                case 0x4017:
                    return joypad->read_byte(address);
                    break;
                default:
                    break;
            }
            break;
        default:
            unsigned char val = mapper->read_byte(address);
            if (game_genie->count > 0) {
                val = game_genie->filter_read(address, val);
            }
            return val;
            break;
    }
    return 0;
}

void c_nes::write_byte(unsigned short address, unsigned char value)
{
    switch (address >> 12) {
        case 0:
        case 1:
            ram[address & 0x7FF] = value;
            break;
        case 2:
        case 3:
            ppu->write_byte(address & 0x2007, value);
            break;
        case 4:
            if (address == 0x4014) {
                cpu->do_sprite_dma(ppu->get_sprite_memory(), (value & 0xFF) << 8);
            }
            else if (address == 0x4016) {
                joypad->write_byte(address, value);
            }
            else if (address >= 0x4000 && address <= 0x4017) {
                apu->write_byte(address, value);
            }
            else
                mapper->write_byte(address, value);
            break;
        default:
            mapper->write_byte(address, value);
            break;
    }
}

int c_nes::LoadImage(std::string &pathFile)
{
    std::ifstream file;
    int m = -1;

    file.open(pathFile, std::ios_base::in | std::ios_base::binary);
    if (file.fail())
        return -1;
    file.seekg(0, std::ios_base::end);
    file_length = (int)file.tellg();
    file.seekg(0, std::ios_base::beg);
    image = std::make_unique_for_overwrite<unsigned char[]>(file_length);
    file.read((char *)image.get(), file_length);
    file.close();
    if (file_length < sizeof(iNesHeader)) {
        return -1;
    }
    header = (iNesHeader *)image.get();

    char ines_signature[] = {'N', 'E', 'S', 0x1a};
    char fds_signature[] = "*NINTENDO-HVC*";
    char nsf_signature[] = {'N', 'E', 'S', 'M', 0x1A};

    if (std::memcmp(header->Signature, ines_signature, 4) == 0) {

        //if expected file size doesn't match real file size, try to fix chr rom page count
        //fixes fire emblem
        int expected_file_size =
            (header->PrgRomPageCount * 16384) + (header->ChrRomPageCount * 8192) + sizeof(iNesHeader);
        int actual_chr_size = file_length - (header->PrgRomPageCount * 16384) - sizeof(iNesHeader);

        if (file_length != expected_file_size && header->ChrRomPageCount != 0) {
            header->ChrRomPageCount = (actual_chr_size / 8192);
        }

        unsigned char *h = (unsigned char *)&header->Rcb2;

        if (*h & 0x0C)
            *h = 0;
        m = (header->Rcb1.mapper_lo) | (header->Rcb2.mapper_hi << 4);
    }
    else if (std::memcmp(image.get() + 1, fds_signature, 14) == 0) {
        m = 0x103;
        //m = -1;
    }
    else if (std::memcmp(image.get(), nsf_signature, 5) == 0) {
        m = 0x102;
    }

    if (m != -1 && m != 0x102) {
        crc32 = get_crc32((unsigned char *)image.get() + sizeof(iNesHeader), file_length - sizeof(iNesHeader));
    }
    return m;
}

int c_nes::load()
{
    int submapper = -1;
    cpu = std::make_unique<c_cpu>();
    ppu = std::make_unique<c_ppu>();
    joypad = std::make_unique<c_joypad>();
    apu = std::make_unique<c_apu>();

    mapperNumber = LoadImage(path_file);

    auto cartdb_entry = cartdb.find(crc32);
    if (cartdb_entry != cartdb.end()) {
        s_cartdb c = cartdb_entry->second;
        if (c.mapper != -1) {
            mapperNumber = c.mapper;
        }
        if (c.submapper != -1) {
            submapper = c.submapper;
        }
        if (c.mirroring != -1) {
            switch (c.mirroring) {
                case 0:
                case 1:
                    header->Rcb1.Mirroring = c.mirroring;
                    break;
                case 4:
                    header->Rcb1.Fourscreen = 1;
            }
        }
    }

    auto &r = nes_mapper_registry::get_registry();
    auto m = r.find(mapperNumber);
    if (m == r.end())
        return 0;
    mapper = (m->second).constructor();
    mapper_info = &(m->second);
    apu->set_nes(this);
    if (submapper != -1) {
        mapper->set_submapper(submapper);
    }

    mapper->sramFilename = sram_path_file;
    mapper->crc32 = crc32;
    mapper->file_length = file_length;
    reset();
    return 1;
}

int c_nes::reset()
{
    if (!ram) {
        ram = std::make_unique_for_overwrite<unsigned char[]>(2048);
    }
    std::memset(ram.get(), 0xFF, 2048);

    mapper->close_sram();
    cpu->nes = this;
    apu->reset();
    ppu->cpu = cpu.get();
    ppu->reset();
    ppu->mapper = mapper.get();

    ppu->apu = apu.get();
    ppu->set_sprite_limit(limit_sprites);
    mapper->execute_irq = [&]() { cpu.get()->execute_irq(); };
    mapper->clear_irq = [&]() { cpu.get()->clear_irq(); };
    mapper->ppu_get_drawing_bg = [&]() { return ppu.get()->drawing_bg; };
    mapper->ppu_get_sprite_size = [&]() { return ppu.get()->get_sprite_size(); };
    mapper->header = header;
    mapper->image = image.get();
    if (mapper->load_image() == -1) {
        return 1;
    }
    mapper->reset();
    cpu->reset();
    joypad->reset();
    loaded = true;

    //game_genie->add_code("IPVGZGZE");
    //game_genie->add_code("LEIIXZ");
    //game_genie->add_code("GXVAAASA");

    return 1;
}

void c_nes::set_submapper(int submapper)
{
    mapper->set_submapper(submapper);
}

int c_nes::get_nwc_time()
{
    return mapper->get_nwc_time();
}

int c_nes::emulate_frame()
{
    if (!loaded)
        return 1;
    apu->clear_buffer();
    for (int scanline = 0; scanline < 262; scanline++) {
        if (scanline == 261 || (scanline >= 0 && scanline <= 239)) {
            ppu->eval_sprites();
        }
        ppu->run_ppu_line();
    }
    return 0;
}

int *c_nes::get_video()
{
    return ppu->get_frame_buffer();
}

void c_nes::set_audio_freq(double freq)
{
    apu->set_audio_rate(freq);
}

int c_nes::get_sound_bufs(const short **buf_l, const short **buf_r)
{
    int num_samples = apu->get_buffer(buf_l);
    *buf_r = nullptr;
    return num_samples;
}

int c_nes::get_mapper_number()
{
    return mapperNumber;
}

int c_nes::get_mirroring_mode()
{
    if (mapper)
        return mapper->get_mirroring();
    else
        return 0;
}

std::string &c_nes::get_mapper_name()
{
    static std::string unknown = "Unknown";
    if (mapper_info) {
        return mapper_info->name;
    }
    else {
        return unknown;
    }
}

void c_nes::set_input(int input)
{
    joypad->joy1 = input & 0xFF;
    joypad->joy2 = (input >> 8) & 0xFF;
}

} //namespace nes