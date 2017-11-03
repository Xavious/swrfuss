/***************************************************************************
*                           STAR WARS REALITY 1.0                          *
*--------------------------------------------------------------------------*
* Star Wars Reality Code Additions and changes from the Smaug Code         *
* copyright (c) 1997 by Sean Cooper                                        *
* -------------------------------------------------------------------------*
* Starwars and Starwars Names copyright(c) Lucas Film Ltd.                 *
*--------------------------------------------------------------------------*
* SMAUG 1.0 (C) 1994, 1995, 1996 by Derek Snider                           *
* SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,                    *
* Scryn, Rennard, Swordbearer, Gorog, Grishnakh and Tricops                *
* ------------------------------------------------------------------------ *
* Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
* Chastain, Michael Quan, and Mitchell Tse.                                *
* Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
* Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
* ------------------------------------------------------------------------ *
*			     Mud constants module			   *
****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "mud.h"

/* undef these at EOF */
#define AM 95
#define AC 95
#define AT 85
#define AW 85
#define AV 95
#define AD 95
#define AR 90
#define AA 95

/*
 * Race table.
 */
const struct race_type race_table[MAX_RACE] = {
   /*
    * race name     DEF_AFF      st dx ws in cn ch lk fc hp mn re su   RESTRICTION  LANGUAGE 
    */
   {
    "Human", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, LANG_COMMON},
   {
    "Wookiee", 0, 8, -1, -3, 0, 2, -2, 0, 0, 3, 0, 0, 0, 0, LANG_WOOKIEE},
   {
    "Twi'lek", 0, 0, 2, 2, 2, -1, -1, 0, 0, 0, 0, 0, 0, 0, LANG_TWI_LEK},
   {
    "Rodian", 0, 0, 3, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, LANG_RODIAN},
   {
    "Hutt", 0, -3, -9, -3, 3, 5, -6, 0, 0, 3, 0, 0, 0, 0, LANG_HUTT},
   {
    "Mon Calamari", AFF_AQUA_BREATH, 0, -1, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, LANG_MON_CALAMARI},
   {
    "Noghri", AFF_SNEAK, 0, 8, -2, -1, 0, -3, 0, 0, 0, 0, 0, 0, 0, LANG_NOGHRI},
   {
    "Gamorrean", 0, 6, 0, -5, -5, 5, -2, 0, 0, 3, 0, 0, 0, 0, LANG_GAMORREAN},
   {
    "Jawa", 0, -3, 3, 1, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, LANG_JAWA},
   {
    "Adarian", 0, -2, -1, +2, +2, -1, +2, 0, 0, 0, 0, 0, 0, 0, LANG_ADARIAN},
   {
    "Ewok", 0, -2, -1, -5, -5, -2, +8, 0, 0, 0, 0, 0, 0, 0, LANG_EWOK},
   {
    "Verpine", 0, -1, 0, +1, +6, -1, 0, 0, 0, 0, 0, 0, 0, 0, LANG_VERPINE},
   {
    "Defel", AFF_INVISIBLE, +1, +3, -3, -3, +1, 0, 0, 0, 0, 0, 0, 0, 0, LANG_DEFEL},
   {
    "Trandoshan", AFF_INFRARED, +2, 0, 0, 0, +6, -1, 0, 0, 0, 0, 0, 0, 0, LANG_TRANDOSHAN},
   {
    "Chadra-Fan", AFF_INFRARED, -3, +3, 0, +2, -1, 0, 0, 0, 0, 0, 0, 0, 0, LANG_CHADRA_FAN},
   {
    "Quarren", AFF_AQUA_BREATH + AFF_INFRARED, -1, +1, 0, +1, -1, 0, 0, 0, 0, 0, 0, 0, 0, LANG_MON_CALAMARI},
   {
    "Duinuogwuin", AFF_FLYING, 0, -1, +3, 0, +8, +1, 0, 0, 0, +10, 0, 0, 0, LANG_DUINUOGWUIN}

};


const char *const npc_race[MAX_NPC_RACE] = {
   "Human", "Wookiee", "Twi'lek", "Rodian", "Hutt", "Mon Calamari", "Noghri",
   "Gamorrean", "Jawa", "Adarian", "Ewok", "Verpine", "Defel", "Trandoshan", "Chadra-Fan", "Quarren",
   "Duinuogwuin", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25",
   "r26", "r27", "r28", "r29", "r30", "r31", "r32", "r33",
   "r34", "r35", "r36", "r37", "r38", "r39", "r40",
   "r41", "r42", "r43", "r44", "r45", "r46",
   "r47", "r48", "r49", "r50", "r51", "r52", "r53",
   "r54", "r55", "r56", "r57", "r58", "r59",
   "r60", "r61", "r62", "r63", "r64", "r65", "r66",
   "r67", "r68", "r69", "r70", "r71", "r72", "r73", "r74",
   "r75", "r76", "r77", "r78", "r79", "r80", "r81", "r82",
   "r83", "r84", "r85", "r86", "r87", "r88", "r89", "r90"
};


const char *const ability_name[MAX_ABILITY] = {
   "combat", "piloting", "engineering", "bounty hunting", "smuggling", "diplomacy",
   "leadership", "force"
};


/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
   {-5, -4, 0, 0},   /* 0  */
   {-5, -4, 3, 1},   /* 1  */
   {-3, -2, 3, 2},
   {-3, -1, 10, 3},  /* 3  */
   {-2, -1, 25, 4},
   {-2, -1, 55, 5},  /* 5  */
   {-1, 0, 80, 6},
   {-1, 0, 90, 8},
   {0, 0, 100, 10},
   {0, 0, 100, 12},
   {0, 0, 115, 14},  /* 10  */
   {0, 0, 115, 15},
   {0, 0, 140, 16},
   {0, 0, 140, 17},  /* 13  */
   {0, 1, 170, 18},
   {1, 1, 170, 19},  /* 15  */
   {1, 2, 195, 20},
   {2, 3, 220, 22},
   {2, 4, 250, 25},  /* 18  */
   {3, 5, 400, 30},
   {3, 6, 500, 35},  /* 20  */
   {4, 7, 600, 40},
   {5, 7, 700, 45},
   {6, 8, 800, 50},
   {8, 10, 900, 55},
   {10, 12, 999, 60} /* 25   */
};



const struct int_app_type int_app[26] = {
   {3},  /*  0 */
   {5},  /*  1 */
   {7},
   {8},  /*  3 */
   {9},
   {10}, /*  5 */
   {11},
   {12},
   {13},
   {15},
   {17}, /* 10 */
   {19},
   {22},
   {25},
   {28},
   {31}, /* 15 */
   {34},
   {37},
   {40}, /* 18 */
   {44},
   {49}, /* 20 */
   {55},
   {60},
   {70},
   {85},
   {99}  /* 25 */
};



const struct wis_app_type wis_app[26] = {
   {0},  /*  0 */
   {0},  /*  1 */
   {0},
   {0},  /*  3 */
   {0},
   {1},  /*  5 */
   {1},
   {1},
   {1},
   {2},
   {2},  /* 10 */
   {2},
   {2},
   {2},
   {2},
   {3},  /* 15 */
   {3},
   {4},
   {5},  /* 18 */
   {5},
   {5},  /* 20 */
   {6},
   {6},
   {6},
   {6},
   {7}   /* 25 */
};



const struct dex_app_type dex_app[26] = {
   {60}, /* 0 */
   {50}, /* 1 */
   {50},
   {40},
   {30},
   {20}, /* 5 */
   {10},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {-10},   /* 15 */
   {-15},
   {-20},
   {-30},
   {-40},
   {-50},   /* 20 */
   {-60},
   {-75},
   {-90},
   {-105},
   {-120}   /* 25 */
};



const struct con_app_type con_app[26] = {
   {-4, 20},   /*  0 */
   {-3, 25},   /*  1 */
   {-2, 30},
   {-2, 35},   /*  3 */
   {-1, 40},
   {-1, 45},   /*  5 */
   {-1, 50},
   {0, 55},
   {0, 60},
   {0, 65},
   {0, 70}, /* 10 */
   {0, 75},
   {0, 80},
   {0, 85},
   {0, 88},
   {1, 90}, /* 15 */
   {2, 95},
   {2, 97},
   {3, 99}, /* 18 */
   {3, 99},
   {4, 99}, /* 20 */
   {4, 99},
   {5, 99},
   {6, 99},
   {7, 99},
   {8, 99}  /* 25 */
};


const struct cha_app_type cha_app[26] = {
   {-60},   /* 0 */
   {-50},   /* 1 */
   {-50},
   {-40},
   {-30},
   {-20},   /* 5 */
   {-10},
   {-5},
   {-1},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {1},
   {5},  /* 15 */
   {10},
   {20},
   {30},
   {40},
   {50}, /* 20 */
   {60},
   {70},
   {80},
   {90},
   {99}  /* 25 */
};

/* Have to fix this up - not exactly sure how it works (Scryn) */
const struct lck_app_type lck_app[26] = {
   {60}, /* 0 */
   {50}, /* 1 */
   {50},
   {40},
   {30},
   {20}, /* 5 */
   {10},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {-10},   /* 15 */
   {-15},
   {-20},
   {-30},
   {-40},
   {-50},   /* 20 */
   {-60},
   {-75},
   {-90},
   {-105},
   {-120}   /* 25 */
};

const struct frc_app_type frc_app[26] = {
   {0},  /* 0 */
   {0},  /* 1 */
   {0},
   {0},
   {0},
   {0},  /* 5 */
   {0},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {0},  /* 15 */
   {0},
   {0},
   {0},
   {0},
   {0},  /* 20 */
   {0},
   {0},
   {0},
   {0},
   {0}   /* 25 */
};



/*
 * Liquid properties.
 * Used in #OBJECT section of area file.
 */
const struct liq_type liq_table[LIQ_MAX] = {
   {"water", "clear", {0, 1, 10}},  /*  0 */
   {"beer", "amber", {3, 2, 5}},
   {"wine", "rose", {5, 2, 5}},
   {"ale", "brown", {2, 2, 5}},
   {"dark ale", "dark", {1, 2, 5}},

   {"whisky", "golden", {6, 1, 4}}, /*  5 */
   {"lemonade", "pink", {0, 1, 8}},
   {"firebreather", "boiling", {10, 0, 0}},
   {"local specialty", "everclear", {3, 3, 3}},
   {"slime mold juice", "green", {0, 4, -8}},

   {"milk", "white", {0, 3, 6}}, /* 10 */
   {"tea", "tan", {0, 1, 6}},
   {"coffee", "black", {0, 1, 6}},
   {"blood", "red", {0, 2, -1}},
   {"salt water", "clear", {0, 1, -2}},

   {"cola", "cherry", {0, 1, 5}},   /* 15 */
   {"mead", "honey color", {4, 2, 5}}, /* 16 */
   {"grog", "thick brown", {3, 2, 5}}, /* 17 */
   {"milkshake", "creamy", {0, 8, 5}}  /* 18 */
};

const char *const attack_table[13] = {
   "hit",
   "slice", "stab", "slash", "whip", "claw",
   "blast", "pound", "crush", "shot", "bite",
   "pierce", "suction"
};



/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n)	n
#define LI LEVEL_IMMORTAL

#undef AM
#undef AC
#undef AT
#undef AW
#undef AV
#undef AD
#undef AR
#undef AA

#undef LI
