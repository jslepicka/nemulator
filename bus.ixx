export module bus;

import nemulator.std;

export template<typename T>
struct s_bus
{
    void *ctx;
    uint16_t (*read_word)(void *ctx, T address) = nullptr;
    uint8_t (*read_byte)(void *ctx, T address) = nullptr;
    void (*write_word)(void *ctx, T address, uint16_t data) = nullptr;
    void (*write_byte)(void *ctx, T address, uint8_t data) = nullptr;
    void (*int_ack)(void *ctx) = nullptr;
};