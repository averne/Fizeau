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

#ifndef _FZ_IPC_H
#define _FZ_IPC_H

#include <stdint.h>
#include <switch.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FizeauProfileId_Profile1,
    FizeauProfileId_Profile2,
    FizeauProfileId_Profile3,
    FizeauProfileId_Profile4,
    FizeauProfileId_Total,
    FizeauProfileId_Invalid = 0xffff,
} FizeauProfileId;

typedef struct {
    Service s;
} FizeauProfile;

Result fizeauIsServiceActive(bool *out);
Result fizeauInitialize();
void fizeauExit();

Service *fizeauGetServiceSession();

Result fizeauGetIsActive(bool *is_active);
Result fizeauSetIsActive(bool is_active);
Result fizeauOpenProfile(FizeauProfile *out, FizeauProfileId id);
Result fizeauGetActiveInternalProfileId(FizeauProfileId *id);
Result fizeauSetActiveInternalProfileId(FizeauProfileId id);
Result fizeauGetActiveExternalProfileId(FizeauProfileId *id);
Result fizeauSetActiveExternalProfileId(FizeauProfileId id);

void fizeauProfileClose(FizeauProfile *p);
Result fizeauProfileGetDawnTime(FizeauProfile *p, Time *time_begin, Time *time_end);
Result fizeauProfileSetDawnTime(FizeauProfile *p, Time time_begin, Time time_end);
Result fizeauProfileGetDuskTime(FizeauProfile *p, Time *time_begin, Time *time_end);
Result fizeauProfileSetDuskTime(FizeauProfile *p, Time time_begin, Time time_end);
Result fizeauProfileGetCmuTemperature(FizeauProfile *p, Temperature *temp_day, Temperature *temp_night);
Result fizeauProfileSetCmuTemperature(FizeauProfile *p, Temperature temp_day, Temperature temp_night);
Result fizeauProfileGetCmuColorFilter(FizeauProfile *p, ColorFilter *filter_day, ColorFilter *filter_night);
Result fizeauProfileSetCmuColorFilter(FizeauProfile *p, ColorFilter filter_day, ColorFilter filter_night);
Result fizeauProfileGetCmuGamma(FizeauProfile *p, Gamma *gamma_day, Gamma *gamma_night);
Result fizeauProfileSetCmuGamma(FizeauProfile *p, Gamma gamma_day, Gamma gamma_night);
Result fizeauProfileGetCmuSaturation(FizeauProfile *p, Saturation *sat_day, Saturation *sat_night);
Result fizeauProfileSetCmuSaturation(FizeauProfile *p, Saturation sat_day, Saturation sat_night);
Result fizeauProfileGetCmuLuminance(FizeauProfile *p, Luminance *luma_day, Luminance *luma_night);
Result fizeauProfileSetCmuLuminance(FizeauProfile *p, Luminance luma_day, Luminance luma_night);
Result fizeauProfileGetCmuColorRange(FizeauProfile *p, ColorRange *range_day, ColorRange *range_night);
Result fizeauProfileSetCmuColorRange(FizeauProfile *p, ColorRange range_day, ColorRange range_night);
Result fizeauProfileGetScreenBrightness(FizeauProfile *p, Brightness *brightness_day, Brightness *brightness_night);
Result fizeauProfileSetScreenBrightness(FizeauProfile *p, Brightness brightness_day, Brightness brightness_night);
Result fizeauProfileGetDimmingTimeout(FizeauProfile *p, Time *timeout);
Result fizeauProfileSetDimmingTimeout(FizeauProfile *p, Time timeout);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _FZ_IPC_H
