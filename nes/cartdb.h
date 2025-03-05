#pragma once
#include <map>
import nes_mapper;
namespace nes
{

struct s_cartdb
{
    int mapper;
    int submapper;
    int mirroring;
};

const std::map<unsigned int, s_cartdb> cartdb = {
    {0x9b506a48, {0, -1, -1}}, //Wrecking Crew - bad header
    {0x889129CB, {256, -1, -1}}, //Startropics (U) [!].nes
    {0xD054FFB0, {256, -1, -1}}, //Startropics II - Zoda's Revenge (U) [!].nes
    {0x96ce586e, {189, -1, -1}}, //???
    {0x02CC3973, {3, -1, -1}}, //Ninja Kid - bad header
    {0x9BDE3267, {3, -1, MIRRORING_VERTICAL}}, //Adventures of Dino Riki - bad header
    {0x404B2E8B, {-1, -1, MIRRORING_FOURSCREEN}}, //Rad Racer 2 - bad header
    {0x90C773C1, {118, -1, -1}}, //Goal! Two - bad header
    {0xA80A0F01, {257, -1, -1}}, //Incredible Crash Dummies
    {0x982DFB38, {257, -1, -1}}, //Mickey's Safari in Letterland
    {0x6BC65D7E, {140, -1, -1}}, //Youkai Kuraba
    {0x5B4C6146, {-1, -1, MIRRORING_VERTICAL}}, //Family Boxing (J) - Ring King - bad header
    {0x4F2F1846, {-1, -1, MIRRORING_VERTICAL}}, //Damista '89 - Kaimaku Han!! - bad header
    {0x93991433, {-1, 1, -1}}, //Low G Man
    {0x404B2E8B, {-1, -1, MIRRORING_FOURSCREEN}}, //Rad Racer 2
    {0xA49B48B8, {-1, 1, -1}}, //Dragon Quest III - Soshite Densetsu e... (J) (PRG0) [!].nes
    {0x9C654F15, {-1, 1, -1}}, //Dragon Quest III - Soshite Densetsu e... (J) (PRG0) [o1].nes
    {0x9F830358, {-1, 1, -1}}, //Dragon Quest III - Soshite Densetsu e... (J) (PRG0) [T+Eng0.0111_Spinner_8].nes
    {0x869501CA, {-1, 1, -1}}, //Dragon Quest III - Soshite Densetsu e... (J) (PRG1) [!].nes
    {0x0794F2A5, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [!].nes
    {0xAC413EB0, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b1].nes
    {0x579EEE6B, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b2].nes
    {0x545D027B, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b3].nes
    {0x3EBCE9D3, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [b4].nes
    {0xB4CAFFFB, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [o1].nes
    {0x1613A3E8, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [o2].nes
    {0xB7BDE3FC, {-1, 1, -1}}, //Dragon Quest IV - Michibikareshi Monotachi (J) (PRG1) [o3].nes
    {0x9CAD70D7, {-1, 1, -1}}, //Dragon Quest IV Mayuge V1.0 (DQ4 Hack).nes
    {0xA86A5318, {-1, 1, -1}}, //Dragon Warrior III (U) [!].nes
    {0x759EE933, {-1, 1, -1}}, //Dragon Warrior III (U) [b1].nes
    {0x1ED7F280, {-1, 1, -1}}, //Dragon Warrior III (U) [b1][o1].nes
    {0xA48352DD, {-1, 1, -1}}, //Dragon Warrior III (U) [b2].nes
    {0xE1D847F8, {-1, 1, -1}}, //Dragon Warrior III (U) [b2][o1].nes
    {0xE02BD69D, {-1, 1, -1}}, //Dragon Warrior III (U) [b3].nes
    {0x74597343, {-1, 1, -1}}, //Dragon Warrior III (U) [b4].nes
    {0x26733857, {-1, 1, -1}}, //Dragon Warrior III (U) [b5].nes
    {0xA6BE0EA7, {-1, 1, -1}}, //Dragon Warrior III (U) [b6].nes
    {0x29BD4B11, {-1, 1, -1}}, //Dragon Warrior III (U) [o1].nes
    {0x9898CA71, {-1, 1, -1}}, //Dragon Warrior III (U) [o2].nes
    {0xA44021DB, {-1, 1, -1}}, //Dragon Warrior III (U) [T+Fre1.0_Generation IX].nes
    {0x9549652C, {-1, 1, -1}}, //Dragon Warrior III (U) [T+Por1.1_CBT].nes 9549652C
    {0xAABF628C, {-1, 1, -1}}, //Dragon Warrior III (U) [T-FreBeta_Generation IX].nes
    {0x9B9D3893, {-1, 1, -1}}, //Dragon Warrior III (U) [T-Por_CBT].nes
    {0x39EEEE89, {-1, 1, -1}}, //Dragon Warrior III Special Ed. V0.5 (Hack).nes
    {0x506E259D, {-1, 1, -1}}, //Dragon Warrior IV (U) [!].nes
    {0x41413B06, {-1, 1, -1}}, //Dragon Warrior IV (U) [b1].nes
    {0x2E91EB15, {-1, 1, -1}}, //Dragon Warrior IV (U) [o1].nes
    {0x06390812, {-1, 1, -1}}, //Dragon Warrior IV (U) [o2].nes
    {0xFC2B6281, {-1, 1, -1}}, //Dragon Warrior IV (U) [o3].nes
    {0x030AB0B2, {-1, 1, -1}}, //Dragon Warrior IV (U) [o4].nes
    {0xAB43AA55, {-1, 1, -1}}, //Dragon Warrior IV (U) [o5].nes
    {0x97F8C475, {-1, 1, -1}}, //Dragon Warrior IV (U) [o6].nes
    {0x44544B8A, {-1, 1, -1}}, //Final Fantasy 99 (DQ3 Hack) [a1].nes
    {0xB8F8D911, {-1, 1, -1}}, //Final Fantasy 99 (DQ3 Hack) [a1][o1].nes
    {0x70FA1880, {-1, 1, -1}}, //Final Fantasy 99 (DQ3 Hack).nes
    {0xCEE5857B, {-1, 1, -1}}, //Ninjara Hoi! (J) [!].nes
    {0xCB8F9273, {-1, 1, -1}}, //Ninjara Hoi! (J) [b1].nes
    {0xFFBC61C5, {-1, 1, -1}}, //Ninjara Hoi! (J) [b2].nes
    {0xB061F6E2, {-1, 1, -1}}, //Ninjara Hoi! (J) [o1].nes
    {0xC247CC80, {-1, 1, -1}}, //Family Circuit '91
    {0x419461d0, {-1, -1, MIRRORING_VERTICAL}}, //???
    {0x9ea1dc76, {-1, -1, MIRRORING_HORIZONTAL}}, //Rainbow Islands - bad header
    {0x6997F5E1, {-1, -1, MIRRORING_VERTICAL}}, //???
    {0x97B6CB19, {-1, 2, -1}}, //Aladdin (SuperGame) (Mapper 4)
    {0xD26EFD78, {-1, -1, MIRRORING_VERTICAL}}, //SMB + Duck Hunt - bad header
    {0x1bc686a8, {-1, 1, -1}}, //Fire Hawk
    {0xBA51AC6F, {-1, 1, -1}}, //Holy Diver (J).nes
    {0x3D1C3137, {-1, 2, -1}}, //Uchuusen - Cosmo Carrier (J).nes
    {0xD1691028, {-1, 1, -1}}, //Devil Man (J)
};

} //namespace nes