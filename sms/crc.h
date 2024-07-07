#pragma once
#include <stdint.h>
struct s_crc
{
    uint32_t crc;
    int has_sram;
} crc_table[] =
{
    { 0xC7DED988, 1 }, // Golden Axe Warrior (UE) [!].sms
    { 0xF424AD15, 1 }, // Golden Axe Warrior (UE) [T+Bra].sms
    { 0x68F58DF7, 1 }, // Golden Axe Warrior (UE) [T+Bra_ALVS].sms
    { 0x8872F23F, 1 }, // Golden Axe Warrior (UE) [T+Bra_Emuboarding].sms
    { 0x472D1CE4, 1 }, // Golden Axe Warrior (UE) [T+Fre_Haruney].sms
    { 0x21DB20AE, 1 }, // Golden Axe Warrior (UE) [T+Ger0.82_Taurus].sms
    { 0xA53677B3, 1 }, // Golden Axe Warrior (UE) [T+Spa100_pkt].sms
    { 0x48651325, 1 }, // Golf Mania (E) [!].sms
    { 0x5DABFDC3, 1 }, // Golf Mania (Prototype) [!].sms
    { 0x0E333B6E, 1 }, // Miracle Warriors - Seal of the Dark Lord (UE) [!].sms
    { 0xEBC2BFA1, 1 }, // Miracle Warriors - Seal of the Dark Lord (UE) [b1].sms
    { 0x302B5D2B, 1 }, // Miracle Warriors - Seal of the Dark Lord (UE) [b2].sms
    { 0x026D94A4, 1 }, // Monopoly (E) [!].sms
    { 0x69538469, 1 }, // Monopoly (U) [!].sms
    { 0xFFF835BD, 1 }, // Monopoly (U) [o1].sms
    { 0x2BCDB8FA, 1 }, // Penguin Land (J) [!].sms
    { 0xAC7F95B4, 1 }, // Penguin Land (SG-1000) [b1].sms
    { 0xF97E9875, 1 }, // Penguin Land (UE) [!].sms
    { 0xF6552DA8, 1 }, // Penguin Land (UE) [o1].sms
    { 0x75971BEF, 1 }, // Phantasy Star (B) [!].sms
    { 0x07301F83, 1 }, // Phantasy Star (J) (from Saturn Collection CD) [!].sms
    { 0x6605D36A, 1 }, // Phantasy Star (J) [!].sms
    { 0xEEFE22DE, 1 }, // Phantasy Star (J) [b1].sms
    { 0x747E83B5, 1 }, // Phantasy Star (K) [!].sms
    { 0x3CA83C04, 1 }, // Phantasy Star (Lutz Hack).sms
    { 0xE4A65E79, 1 }, // Phantasy Star (UE) (V1.2) [!].sms
    { 0x00BEF1D7, 1 }, // Phantasy Star (UE) (V1.3) [!].sms
    { 0x7F4F28C6, 1 }, // Phantasy Star (UE) (V1.3) [b1].sms
    { 0x57BD9ED5, 1 }, // Phantasy Star (UE) (V1.3) [b2].sms
    { 0x56BD28D8, 1 }, // Phantasy Star (UE) (V1.3) [T+Fre_floflo].sms
    { 0xB52D60C8, 1 }, // Ultima IV - Quest of the Avatar (E) [!].sms
    { 0x19D64680, 1 }, // Ultima IV - Quest of the Avatar (E) [T+Ger].sms
    { 0xDE9F8517, 1 }, // Ultima IV - Quest of the Avatar (Prototype) [!].sms
    { 0x32759751, 1 }, // Ys (J).sms
    { 0xB33E2827, 1 }, // Ys - The Vanished Omens (UE) [!].sms
    { 0xAFD29460, 1 }, // Ys - The Vanished Omens (UE) [T+Fre_Crispysix].sms
};