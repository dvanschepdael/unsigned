/**
 * @file bios_state_internal.h
 * @brief Typed internal representation of BIOS USER requests and USER_MODE values consumed by runtime.c.
 */

#ifndef UNSIGNED_SYSTEM_BIOS_STATE_INTERNAL_H
#define UNSIGNED_SYSTEM_BIOS_STATE_INTERNAL_H

typedef enum UNeoGeoBiosRequest {
    U_NEO_GEO_BIOS_REQUEST_INIT = 0,
    U_NEO_GEO_BIOS_REQUEST_EYE_CATCHER = 1,
    U_NEO_GEO_BIOS_REQUEST_DEMO = 2,
    U_NEO_GEO_BIOS_REQUEST_TITLE = 3,
    U_NEO_GEO_BIOS_REQUEST_INVALID = 0xff,
} UNeoGeoBiosRequest;

typedef enum UNeoGeoMode {
    U_NEO_GEO_MODE_BOOT = 0,
    U_NEO_GEO_MODE_DEMO = 1,
    U_NEO_GEO_MODE_GAME = 2,
} UNeoGeoMode;

#endif
