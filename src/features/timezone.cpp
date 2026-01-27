#include "timezone.h"

const TZEntry MENU_DATA[] = { // POSIX TABLE FOR POSIX STRINGS AND MENUS
  // POSIX Code                                               Label           Offset        City/Region
  {"AOE12",                                                   "  AOE",        "  -12",      "  Baker Island"},           // 0
  {"NUT11",                                                   "  NUT",        "  -11",      "  American Samoa"},         // 1
  {"HST10",                                                   "  HST",        "  -10",      "  Hawaii"},                 // 2
  {"MART9:30",                                                "  MART",       "  -9:30",    "  French Polynesia"},       // 3
  {"AKST9AKDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                "  AKST",       "  -9",       "  Alaska"},                 // 4
  {"PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  PDT",        "  -8",       "  Los Angeles"},            // 5
  {"MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  MST MDT",    "  -7",       "  Denver"},                 // 6
  {"MST7",                                                    "  MST",        "  -7",       "  Phoenix"},                // 7
  {"CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  CST",        "  -6",       "  Chicago"},                // 8
  {"EST5EDT,M3.2.0/2,M11.1.0/2",                              "  EST",        "  -5",       "  New York"},               // 9
  {"AST4ADT,M3.2.0/2:00:00,M11.1.0/2:00:00",                  "  AST",        "  -4",       "  Halifax"},                // 10
  {"BRT3",                                                    "  BRT",        "  -3",       "  São Paulo"},              // 11
  {"FKST3FKDT,M9.1.0/2:00:00,M4.3.0/2:00:00",                 "  FKST",       "  -3",       "  Falkland Islands"},       // 12
  {"GRNL2",                                                   "  GRL",        "  -2",       "  South Georgia"},          // 13
  {"AZOT1AZOST,M3.5.0/0:00:00,M10.5.0/1:00:00",               "  AZOT",       "  -1",       "  Azores"},                 // 14
  {"GMT0",                                                    "  GMT",        "  0",        "  London"},                 // 15
  {"GMT0BST,M3.5.0/1:00:00,M10.5.0/2:00:00",                  "  BST",        "  +1",       "  United Kingdom"},         // 16
  {"CET-1CEST,M3.5.0/2:00:00,M10.5.0/3:00:00",                "  CET",        "  +1",       "  Berlin"},                 // 17
  {"WAT-1",                                                   "  WAT",        "  +1",       "  Nigeria"},                // 18
  {"EET-2EEST,M3.5.0/3:00:00,M10.5.0/4:00:00",                "  EET",        "  +2",       "  Athens"},                 // 19
  {"CAT-2",                                                   "  CAT",        "  +2",       "  South Africa"},           // 20
  {"MSK-3",                                                   "  MSK",        "  +3",       "  Moscow"},                 // 21
  {"AST-3",                                                   "  AST",        "  +3",       "  Saudi Arabia"},           // 22
  {"GST-4",                                                   "  GST",        "  +4",       "  Dubai"},                  // 23
  {"AZT-4AZST,M3.5.0/4:00:00,M10.5.0/5:00:00",                "  AZT",        "  +4",       "  Azerbaijan"},             // 24
  {"AFT-4:30",                                                "  AFT",        "  +4:30",    "  Afghanistan"},            // 25
  {"PKT-5",                                                   "  PKT",        "  +5",       "  Pakistan"},               // 26
  {"IST-5:30",                                                "  IST",        "  +5:30",    "  India"},                  // 27
  {"NPT-5:45",                                                "  NPT",        "  +5:45",    "  Nepal"},                  // 28
  {"BST-6",                                                   "  BST",        "  +6",       "  Bangladesh"},             // 29
  {"MMT-6:30",                                                "  MMT",        "  +6:30",    "  Myanmar"},                // 30
  {"ICT-7",                                                   "  ICT",        "  +7",       "  Thailand"},               // 31
  {"WIB-7",                                                   "  WIB",        "  +7",       "  Indonesia West"},         // 32
  {"CST-8",                                                   "  CST CHINA",  "  +8",       "  China"},                  // 33
  {"AWST-8",                                                  "  AWST",       "  +8",       "  Perth"},                  // 34
  {"HKT-8",                                                   "  HKT",        "  +8",       "  Hong Kong"},              // 35
  {"WITA-8",                                                  "  WITA",       "  +8",       "  Indonesia Central"},      // 36
  {"JST-9",                                                   "  JST",        "  +9",       "  Japan"},                  // 37
  {"KST-9",                                                   "  KST",        "  +9",       "  South Korea"},            // 38
  {"ACST-9:30ACDT,M10.1.0/2:00:00,M4.1.0/3:00:00",            "  ACST",       "  +9:30",    "  Adelaide"},               // 39
  {"CST-9:30",                                                "  CST N TERR", "  +9:30",    "  Northern Territory"},     // 40
  {"AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/3:00:00",              "  AEST",       "  +10",      "  Sydney"},                 // 41
  {"CHUT-10",                                                 "  CHUT",       "  +10",      "  Chuuk"},                  // 42
  {"WIT-9",                                                   "  WIT",        "  +9",       "  Indonesia East"},         // 43
  {"SBT-11",                                                  "  SBT",        "  +11",      "  Solomon Islands"},        // 44
  {"NCT-11",                                                  "  NCT",        "  +11",      "  New Caledonia"},          // 45
  {"NZST-12NZDT,M9.5.0/2:00:00,M4.1.0/3:00:00",               "  NZST",       "  +12",      "  New Zealand"},            // 46
  {"FJT-12FJDT,M11.1.0/2:00:00,M1.2.0/3:00:00",               "  FJT",        "  +12",      "  Fiji"},                   // 47
  {"TOT-13",                                                  "  TOT",        "  +13",      "  Tonga"},                  // 48
  {"PHOT-13",                                                 "  PHOT",       "  +13",      "  Phoenix Islands"},        // 49
  {"LINT-14",                                                 "  LINT",       "  +14",      "  Line Islands"}            // 50
};

TimeZoneSetting::TimeZoneSetting() {
    currentIndex = 9;
}

int TimeZoneSetting::getCount() {
    return sizeof(MENU_DATA) / sizeof(MENU_DATA[0]);
}

void TimeZoneSetting::onKnobTurn(int direction) {
    currentIndex += direction;
    if (currentIndex < 0) currentIndex = getCount() - 1;
    if (currentIndex >= getCount()) currentIndex = 0;
}

String TimeZoneSetting::getDisplayString() {
    return String(MENU_DATA[currentIndex].tz);
}

const char* TimeZoneSetting::getSelectedPosix() {
    return MENU_DATA[currentIndex].posix;
}
