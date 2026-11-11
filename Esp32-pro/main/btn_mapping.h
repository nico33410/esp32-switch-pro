/*
 * btn_mapping.h — Profils : tableau physique → logique + axes sticks
 *
 * Chaque entrée physique (A, B, X…) pointe vers une touche Switch.
 * Par défaut : identité (physique A → logique A).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#define MAPPING_MAGIC       0x4D415032  // "MAP2"
#define MAPPING_NAMESPACE   "mapping"
#define MAX_PROFILES        4
#define PROFILE_NAME_LEN    16
#define MAP_BTN_COUNT       18
#define MAP_NONE            0xFF

// Index touche (physique et logique partagent les mêmes noms)
#define MAP_A       0
#define MAP_B       1
#define MAP_X       2
#define MAP_Y       3
#define MAP_L       4
#define MAP_R       5
#define MAP_ZL      6
#define MAP_ZR      7
#define MAP_MINUS   8
#define MAP_PLUS    9
#define MAP_HOME    10
#define MAP_CAPTURE 11
#define MAP_DUP     12
#define MAP_DDOWN   13
#define MAP_DLEFT   14
#define MAP_DRIGHT  15
#define MAP_L3      16
#define MAP_R3      17

// Flags axes joystick (dans BtnProfile.flags)
#define STICK_SWAP_L   (1 << 0)
#define STICK_INV_LX   (1 << 1)
#define STICK_INV_LY   (1 << 2)
#define STICK_SWAP_R   (1 << 3)
#define STICK_INV_RX   (1 << 4)
#define STICK_INV_RY   (1 << 5)

// Turbo : 2 impulsions/s → bascule toutes les 250 ms
#define TURBO_INTERVAL_MS  250

typedef struct {
    uint8_t  target[MAP_BTN_COUNT];
    uint8_t  flags;
    uint32_t turbo_mask;  // bit i = turbo actif sur touche physique i
} BtnProfile;

typedef struct {
    uint32_t magic;
    uint8_t  active_profile;
    uint8_t  profile_count;
    char     names[MAX_PROFILES][PROFILE_NAME_LEN];
    BtnProfile profiles[MAX_PROFILES];
} BtnMappingStore;

typedef void (*btn_phys_read_fn)(uint16_t *mcp, bool *l3, bool *r3);

void btn_mapping_set_phys_reader(btn_phys_read_fn fn);
void btn_mapping_read_phys(uint16_t *mcp, bool *l3, bool *r3);

bool mapping_load(void);
bool mapping_save(void);
void mapping_reset_defaults(void);

const BtnMappingStore *mapping_store(void);
const BtnProfile *mapping_active_profile(void);
uint8_t mapping_active_index(void);

bool mapping_set_active(uint8_t idx);
bool mapping_create_profile(const char *name);
bool mapping_delete_profile(uint8_t idx);
bool mapping_rename_profile(uint8_t idx, const char *name);
bool mapping_set_target(uint8_t profile, uint8_t phys, uint8_t logic);
bool mapping_set_stick_flags(uint8_t profile, uint8_t flags);
bool mapping_set_turbo_mask(uint8_t profile, uint32_t mask);

void mapping_apply(uint16_t mcp_state, bool l3, bool r3);
void mapping_apply_sticks(uint16_t *lsx, uint16_t *lsy, uint16_t *rsx, uint16_t *rsy);

bool mapping_phys_pressed(uint8_t phys, uint16_t mcp, bool l3, bool r3);
const char *mapping_btn_name(uint8_t idx);
