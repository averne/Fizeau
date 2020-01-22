/**
 * Copyright (C) 2020 averne
 *
 * This file is part of Fizeau.
 *
 * Fizeau is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Fizeau is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fizeau.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <switch.h>

#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "fizeau.h"

typedef enum {
    FizeauCmdId_GetIsActive     = 0,
    FizeauCmdId_SetIsActive     = 1,
    FizeauCmdId_GetDuskTime     = 2,
    FizeauCmdId_SetDuskTime     = 3,
    FizeauCmdId_GetDawnTime     = 4,
    FizeauCmdId_SetDawnTime     = 5,
    FizeauCmdId_GetColor        = 6,
    FizeauCmdId_SetColor        = 7,
    FizeauCmdId_GetBrightness   = 8,
    FizeauCmdId_SetBrightness   = 9,
    FizeauCmdId_EasterEgg       = 10,
} FizeauCmdId;

static Service g_fizeau_srv;

NX_GENERATE_SERVICE_GUARD(fizeau);

Result _fizeauInitialize(void) {
    return smGetService(&g_fizeau_srv, "fizeau");
}

void _fizeauCleanup(void) {
    serviceClose(&g_fizeau_srv);
}

Service *fizeauGetServiceSession(void) {
    return &g_fizeau_srv;
}

Result fizeauGetIsActive(bool *is_active) {
    u8 tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetIsActive, tmp);
    if (R_SUCCEEDED(rc) && is_active)
        *is_active = !!tmp;
    return rc;
}

Result fizeauSetIsActive(bool is_active) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetIsActive, is_active);
}

Result fizeauGetDuskTime(Time *time) {
    Time tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetDuskTime, tmp);
    if (R_SUCCEEDED(rc) && time)
        *time = tmp;
    return rc;
}

Result fizeauSetDuskTime(Time time) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetDuskTime, time);
}

Result fizeauGetDawnTime(Time *time) {
    Time tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetDawnTime, tmp);
    if (R_SUCCEEDED(rc) && time)
        *time = tmp;
    return rc;
}

Result fizeauSetDawnTime(Time time) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetDawnTime, time);
}

Result fizeauGetColor(uint16_t *color) {
    uint16_t tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetColor, tmp);
    if (R_SUCCEEDED(rc) && color)
        *color = tmp;
    return rc;
}

Result fizeauSetColor(uint16_t color) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetColor, color);
}

Result fizeauGetBrightness(float *brightness) {
    float tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetBrightness, tmp);
    if (R_SUCCEEDED(rc) && brightness)
        *brightness = tmp;
    return rc;
}

Result fizeauSetBrightness(float brightness) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetBrightness, brightness);
}

Result fizeauEasterEgg(void) {
    return serviceDispatch(&g_fizeau_srv, FizeauCmdId_EasterEgg);
}
