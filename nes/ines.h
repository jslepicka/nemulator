#pragma once
struct iNesHeader
{
    unsigned char Signature[4];  //'N' 'E' 'S' $1A
    unsigned char PrgRomPageCount;
    unsigned char ChrRomPageCount;
    struct ROMCONTROLBLOCK1 {
        bool Mirroring:1;
        bool Sram:1;
        bool Trainer:1;
        bool Fourscreen:1;
        unsigned char mapper_lo:4;
    } Rcb1;
    struct ROMCONTROLBLOCK2 {
        unsigned char Unused:4;
        unsigned char mapper_hi:4;
    } Rcb2;
    unsigned char PAL;
    unsigned char Zeros[7];
};