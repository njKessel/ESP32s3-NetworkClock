// display_font.h

#pragma once
#include <Arduino.h>
#include <stdint.h>

// Manually define the type if the headers are failing to pull it into global scope
typedef unsigned int size_t;

void initFontTable();

enum segmentBit : uint32_t {
    SEG_A = 1u << 0,
    SEG_B = 1u << 1,
    SEG_C = 1u << 2,
    SEG_D = 1u << 3,
    SEG_E = 1u << 4,
    SEG_F = 1u << 5,
    SEG_G = 1u << 6,
    SEG_H = 1u << 7,
    SEG_K = 1u << 8,
    SEG_M = 1u << 9,
    SEG_N = 1u << 10,
    SEG_P = 1u << 11,
    SEG_R = 1u << 12,
    SEG_S = 1u << 13,
    SEG_T = 1u << 14,
    SEG_U = 1u << 15,
    SEG_DP = 1u << 16,
};

constexpr uint32_t SEG_ALL =
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G | SEG_H |
    SEG_K | SEG_M | SEG_N | SEG_P | SEG_R | SEG_S | SEG_T | SEG_U | SEG_DP;

uint32_t getSegmentPattern(char c, bool DP);

void displayBuilder(const char* str, uint64_t* displayWords, bool navDelay);
