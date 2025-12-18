#include "display_font.h"
#include <cstddef>  

inline uint32_t blank = SEG_ALL; // ALL SEGMENTS BLANK (ACTIVE LOW)

static uint32_t fontTable[128] = {}; // zero-initialized for all ASCII

void initFontTable() {
    fontTable['0'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_H;
    fontTable['1'] =                 SEG_C | SEG_D;
    fontTable['2'] = SEG_A | SEG_B | SEG_C |         SEG_E | SEG_F | SEG_G |                 SEG_P |         SEG_U;
    fontTable['3'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F |                         SEG_P |         SEG_U;
    fontTable['4'] =                 SEG_C | SEG_D |                 SEG_H |                 SEG_P |         SEG_U;
    fontTable['5'] = SEG_A | SEG_B |         SEG_D | SEG_E | SEG_F | SEG_H |                 SEG_P |         SEG_U;
    fontTable['6'] = SEG_A | SEG_B |         SEG_D | SEG_E | SEG_F | SEG_G | SEG_H |         SEG_P |         SEG_U;
    fontTable['7'] = SEG_A | SEG_B | SEG_C | SEG_D;
    fontTable['8'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_H |         SEG_P |         SEG_U;
    fontTable['9'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F |         SEG_H |         SEG_P |         SEG_U;

    fontTable['<'] =                                                 SEG_N |         SEG_R;
    fontTable['>'] =                         SEG_K |                                         SEG_T;

    fontTable['A'] = SEG_A | SEG_B | SEG_C | SEG_D |         SEG_G | SEG_H |                 SEG_P |         SEG_U;
    fontTable['B'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F |         SEG_M | SEG_P |         SEG_S;
    fontTable['C'] = SEG_A | SEG_B |                 SEG_E | SEG_F | SEG_G | SEG_H;
    fontTable['D'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F |         SEG_M |         SEG_S;
    fontTable['E'] = SEG_A | SEG_B |                 SEG_E | SEG_F | SEG_G | SEG_H |         SEG_P |         SEG_U;
    fontTable['F'] = SEG_A | SEG_B |                                 SEG_G | SEG_H |         SEG_P |         SEG_U;
    fontTable['G'] = SEG_A | SEG_B |         SEG_D | SEG_E | SEG_F | SEG_G | SEG_H |         SEG_P;
    fontTable['H'] =                 SEG_C | SEG_D |         SEG_G | SEG_H |                 SEG_P |         SEG_U;
    fontTable['I'] = SEG_A | SEG_B |                 SEG_E | SEG_F |         SEG_M |         SEG_S;
    fontTable['J'] = SEG_A | SEG_B |                         SEG_F | SEG_G |         SEG_M |         SEG_S;
    fontTable['K'] =                                 SEG_G | SEG_H |         SEG_N | SEG_R |         SEG_U;
    fontTable['L'] =                                 SEG_E | SEG_F | SEG_G | SEG_H;
    fontTable['M'] =                 SEG_C | SEG_D |         SEG_G | SEG_H | SEG_K |         SEG_N;
    fontTable['N'] =                 SEG_C | SEG_D |         SEG_G | SEG_H | SEG_K |         SEG_R;
    fontTable['O'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_H;
    fontTable['P'] = SEG_A | SEG_B | SEG_C |         SEG_G | SEG_H |                 SEG_P |         SEG_U;
    fontTable['Q'] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_H |         SEG_R;
    fontTable['R'] = SEG_A | SEG_B | SEG_C |         SEG_G | SEG_H |                 SEG_P | SEG_R |         SEG_U;
    fontTable['S'] = SEG_A | SEG_B |         SEG_D | SEG_E | SEG_F | SEG_H |                 SEG_P |         SEG_U;
    fontTable['T'] = SEG_A | SEG_B |                         SEG_M |         SEG_S;
    fontTable['U'] =                 SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_H;
    fontTable['V'] =                                 SEG_G | SEG_H |         SEG_M |                 SEG_T;
    fontTable['W'] =                 SEG_C | SEG_D |                                         SEG_R |         SEG_T;
    fontTable['X'] =                                         SEG_K |         SEG_N |         SEG_R |         SEG_T;
    fontTable['Y'] =                                         SEG_K |         SEG_N |                 SEG_S;
    fontTable['Z'] = SEG_A | SEG_B |                                 SEG_G | SEG_H |         SEG_N |                 SEG_T;
}

enum anodeBit : uint64_t {
  ANODE0 = 1ull << 25,  // 74HC595 U4 pin ANODE1 on 12C-LTP-587HR_rev1
  ANODE1 = 1ull << 26,
  ANODE2 = 1ull << 27,
  ANODE3 = 1ull << 28,
  ANODE4 = 1ull << 29,
  ANODE5 = 1ull << 30,
  ANODE6 = 1ull << 31,
  ANODE7 = 1ull << 32,
  ANODE8 = 1ull << 33,
  ANODE9 = 1ull << 34,
  ANODE10 = 1ull << 35,
  ANODE11 = 1ull << 36, // 74HC595 U5 pin ANODE12 on 12C-LTP-587HR_rev1
};

uint32_t getSegmentPattern(char c, bool DP) {
    if ((unsigned char)c < sizeof(fontTable)/sizeof(fontTable[0])) {
        if (DP) {
            return fontTable[(unsigned char)c] | SEG_DP;
        } else {
            return fontTable[(unsigned char)c];
        }
    } else {
        return 0;
    }
}

void displayBuilder(const char* str, std::size_t len, uint64_t* displayWords) {
    for (std::size_t i = 0, displayIndex = 0; i < len && displayIndex < 12; i++) {
        bool decimalPoint = (i + 1 < len && str[i + 1] == '.');
        if (decimalPoint) i++;

        uint64_t segmentPattern = static_cast<uint64_t>(SEG_ALL & ~getSegmentPattern(str[i], decimalPoint));
        uint64_t anodeBit = 1ull << (25 + displayIndex);

        displayWords[displayIndex] = segmentPattern | anodeBit;
        displayIndex++;
    }
}
