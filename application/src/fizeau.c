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

#include <common.hpp>

typedef enum {
    FizeauCmdId_GetIsActive                = 0,
    FizeauCmdId_SetIsActive                = 1,
    FizeauCmdId_OpenProfile                = 2,
    FizeauCmdId_GetActiveInternalProfileId = 3,
    FizeauCmdId_SetActiveInternalProfileId = 4,
    FizeauCmdId_GetActiveExternalProfileId = 5,
    FizeauCmdId_SetActiveExternalProfileId = 6,
} FizeauCmdId;

typedef enum {
    FizeauProfileCmdId_GetDuskTime         = 0,
    FizeauProfileCmdId_SetDuskTime         = 1,
    FizeauProfileCmdId_GetDawnTime         = 2,
    FizeauProfileCmdId_SetDawnTime         = 3,
    FizeauProfileCmdId_GetCmuTemperature   = 4,
    FizeauProfileCmdId_SetCmuTemperature   = 5,
    FizeauProfileCmdId_GetCmuGamma         = 6,
    FizeauProfileCmdId_SetCmuGamma         = 7,
    FizeauProfileCmdId_GetCmuLuminance     = 8,
    FizeauProfileCmdId_SetCmuLuminance     = 9,
    FizeauProfileCmdId_GetCmuColorRange    = 10,
    FizeauProfileCmdId_SetCmuColorRange    = 11,
    FizeauProfileCmdId_GetScreenBrightness = 12,
    FizeauProfileCmdId_SetScreenBrightness = 13,
} FizeauProfileCmdId;

static Service g_fizeau_srv;

NX_GENERATE_SERVICE_GUARD(fizeau);

Result fizeauIsServiceActive(bool *out) {
    bool tmp;
    SmServiceName name = smEncodeName("fizeau");
    Result rc = serviceDispatchInOut(smGetServiceSession(), 65100, name, tmp); // AMS extension
    if (R_SUCCEEDED(rc) && out)
        *out = tmp;
    return rc;
}

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

Result fizeauOpenProfile(FizeauProfile *out, FizeauProfileId id) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_OpenProfile, id,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fizeauGetActiveInternalProfileId(FizeauProfileId *id) {
    u32 tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetActiveInternalProfileId, tmp);

    if (R_SUCCEEDED(rc) && id)
        *id = tmp;
    return rc;
}

Result fizeauSetActiveInternalProfileId(FizeauProfileId id) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetActiveInternalProfileId, id);
}

Result fizeauGetActiveExternalProfileId(FizeauProfileId *id) {
    u32 tmp;
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCmdId_GetActiveExternalProfileId, tmp);

    if (R_SUCCEEDED(rc) && id)
        *id = tmp;
    return rc;
}

Result fizeauSetActiveExternalProfileId(FizeauProfileId id) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCmdId_SetActiveExternalProfileId, id);
}

void fizeauProfileClose(FizeauProfile *p) {
    serviceClose(&p->s);
}

Result fizeauProfileGetDuskTime(FizeauProfile *p, Time *time_begin, Time *time_end) {
    struct {
        Time begin, end;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetDuskTime, tmp);

    if (R_SUCCEEDED(rc)) {
        if (time_begin)
            *time_begin = tmp.begin;
        if (time_end)
            *time_end   = tmp.end;
    }
    return rc;
}

Result fizeauProfileSetDuskTime(FizeauProfile *p, Time time_begin, Time time_end) {
    struct {
        Time begin, end;
    } in = { time_begin, time_end };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetDuskTime, in);
}

Result fizeauProfileGetDawnTime(FizeauProfile *p, Time *time_begin, Time *time_end) {
    struct {
        Time begin, end;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetDawnTime, tmp);

    if (R_SUCCEEDED(rc)) {
        if (time_begin)
            *time_begin = tmp.begin;
        if (time_end)
            *time_end   = tmp.end;
    }
    return rc;
}

Result fizeauProfileSetDawnTime(FizeauProfile *p, Time time_begin, Time time_end) {
    struct {
        Time begin, end;
    } in = { time_begin, time_end };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetDawnTime, in);
}

Result fizeauProfileGetCmuTemperature(FizeauProfile *p, Temperature *temp_day, Temperature *temp_night) {
    struct {
        Temperature temp_day, temp_night;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetCmuTemperature, tmp);

    if (R_SUCCEEDED(rc)) {
        if (temp_day)
            *temp_day = tmp.temp_day;
        if (temp_night)
            *temp_night = tmp.temp_night;
    }
    return rc;
}

Result fizeauProfileSetCmuTemperature(FizeauProfile *p, Temperature temp_day, Temperature temp_night) {
    struct {
        Temperature temp_day, temp_night;
    } in = { temp_day, temp_night };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetCmuTemperature, in);
}

Result fizeauProfileGetCmuGamma(FizeauProfile *p, Gamma *gamma_day, Gamma *gamma_night) {
    struct {
        Gamma gamma_day, gamma_night;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetCmuGamma, tmp);

    if (R_SUCCEEDED(rc)) {
        if (gamma_day)
            *gamma_day = tmp.gamma_day;
        if (gamma_night)
            *gamma_night = tmp.gamma_night;
    }
    return rc;
}

Result fizeauProfileSetCmuGamma(FizeauProfile *p, Gamma gamma_day, Gamma gamma_night) {
    struct {
        Gamma gamma_day, gamma_night;
    } in = { gamma_day, gamma_night };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetCmuGamma, in);
}

Result fizeauProfileGetCmuLuminance(FizeauProfile *p, Luminance *luma_day, Luminance *luma_night) {
    struct {
        Luminance luma_day, luma_night;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetCmuLuminance, tmp);

    if (R_SUCCEEDED(rc)) {
        if (luma_day)
            *luma_day = tmp.luma_day;
        if (luma_night)
            *luma_night = tmp.luma_night;
    }
    return rc;
}

Result fizeauProfileSetCmuLuminance(FizeauProfile *p, Luminance luma_day, Luminance luma_night) {
    struct {
        Luminance luma_day, luma_night;
    } in = { luma_day, luma_night };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetCmuLuminance, in);
}

Result fizeauProfileGetCmuColorRange(FizeauProfile *p, ColorRange *range_day, ColorRange *range_night) {
    struct {
        ColorRange range_day, range_night;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetCmuColorRange, tmp);

    if (R_SUCCEEDED(rc)) {
        if (range_day)
            *range_day = tmp.range_day;
        if (range_night)
            *range_night = tmp.range_night;
    }
    return rc;
}

Result fizeauProfileSetCmuColorRange(FizeauProfile *p, ColorRange range_day, ColorRange range_night) {
    struct {
        ColorRange range_day, range_night;
    } in = { range_day, range_night };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetCmuColorRange, in);
}

Result fizeauProfileGetScreenBrightness(FizeauProfile *p, Brightness *brightness_day, Brightness *brightness_night) {
    struct {
        Brightness brightness_day, brightness_night;
    } tmp;
    Result rc = serviceDispatchOut(&p->s, FizeauProfileCmdId_GetScreenBrightness, tmp);

    if (R_SUCCEEDED(rc)) {
        if (brightness_day)
            *brightness_day = tmp.brightness_day;
        if (brightness_night)
            *brightness_night = tmp.brightness_night;
    }
    return rc;
}

Result fizeauProfileSetScreenBrightness(FizeauProfile *p, Brightness brightness_day, Brightness brightness_night) {
    struct {
        Brightness brightness_day, brightness_night;
    } in = { brightness_day, brightness_night };
    return serviceDispatchIn(&p->s, FizeauProfileCmdId_SetScreenBrightness, in);
}
