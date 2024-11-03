/**
 * Copyright (c) 2024 averne
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
    FizeauCommandId_GetIsActive,
    FizeauCommandId_SetIsActive,
    FizeauCommandId_GetProfile,
    FizeauCommandId_SetProfile,
    FizeauCommandId_GetActiveProfileId,
    FizeauCommandId_SetActiveProfileId,
} FizeauCommandId;

typedef enum {
    FizeauProfileId_Profile1,
    FizeauProfileId_Profile2,
    FizeauProfileId_Profile3,
    FizeauProfileId_Profile4,
    FizeauProfileId_Total,
    FizeauProfileId_Invalid = 0xffff,
} FizeauProfileId;

#define FIZEAU_RC_MODULE            R_MODULE(0xf12)
#define FIZEAU_RC_INVALID_PROFILEID 1

#define FIZEAU_MAKERESULT(r) MAKERESULT(FIZEAU_RC_MODULE, FIZEAU_RC_ ## r)

typedef struct {
    Temperature temperature;
    Saturation  saturation;
    Hue         hue;
    Contrast    contrast;
    Gamma       gamma;
    Luminance   luminance;
    ColorRange  range;
} FizeauSettings;

typedef struct {
    FizeauSettings day_settings, night_settings;
    Component components;
    Component filter;

    Time dusk_begin, dusk_end;
    Time dawn_begin, dawn_end;

    Time dimming_timeout;
} FizeauProfile;

Result fizeauIsServiceActive(bool *out);
Result fizeauInitialize();
void fizeauExit();

Service *fizeauGetServiceSession();

Result fizeauGetIsActive(bool *is_active);
Result fizeauSetIsActive(bool is_active);

Result fizeauGetProfile(FizeauProfileId id, FizeauProfile *profile);
Result fizeauSetProfile(FizeauProfileId id, FizeauProfile *profile);

Result fizeauGetActiveProfileId(bool is_external, FizeauProfileId *id);
Result fizeauSetActiveProfileId(bool is_external, FizeauProfileId id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _FZ_IPC_H
