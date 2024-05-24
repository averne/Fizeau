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

#include <switch.h>

#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "fizeau.h"

#include <common.hpp>

static Service g_fizeau_srv;

NX_GENERATE_SERVICE_GUARD(fizeau);

Result fizeauIsServiceActive(bool *out) {
    bool tmp = false;
    SmServiceName name = smEncodeName("fizeau");
    Result rc = tipcDispatchInOut(smGetServiceSessionTipc(), 65100, name, tmp); // AMS extension
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
    Result rc = serviceDispatchOut(&g_fizeau_srv, FizeauCommandId_GetIsActive, tmp);

    if (R_SUCCEEDED(rc) && is_active)
        *is_active = !!tmp;

    return rc;
}

Result fizeauSetIsActive(bool is_active) {
    return serviceDispatchIn(&g_fizeau_srv, FizeauCommandId_SetIsActive, is_active);
}

Result fizeauGetProfile(FizeauProfileId id, FizeauProfile *profile) {
    FizeauProfile tmp;
    Result rc = serviceDispatchInOut(&g_fizeau_srv, FizeauCommandId_GetProfile, id, tmp);

    if (R_SUCCEEDED(rc) && profile)
        *profile = tmp;

    return rc;
}

Result fizeauSetProfile(FizeauProfileId id, FizeauProfile *profile) {
    struct {
        FizeauProfileId id;
        FizeauProfile profile;
    } tmp = { id, *profile };
    return serviceDispatchIn(&g_fizeau_srv, FizeauCommandId_SetProfile, tmp);
}

Result fizeauGetActiveProfileId(bool is_external, FizeauProfileId *id) {
    FizeauProfileId tmp;
    Result rc = serviceDispatchInOut(&g_fizeau_srv, FizeauCommandId_GetActiveProfileId, is_external, tmp);

    if (R_SUCCEEDED(rc))
        *id = tmp;

    return rc;
}

Result fizeauSetActiveProfileId(bool is_external, FizeauProfileId id) {
    struct {
        bool is_external;
        FizeauProfileId id;
    } tmp = { is_external, id };
    return serviceDispatchIn(&g_fizeau_srv, FizeauCommandId_SetActiveProfileId, tmp);
}
