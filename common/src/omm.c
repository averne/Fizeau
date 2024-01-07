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
#include "omm.h"

static Service g_ommSrv;

NX_GENERATE_SERVICE_GUARD(omm);

Result _ommInitialize(void) {
    return smGetService(&g_ommSrv, "omm");
}

void _ommCleanup(void) {
    serviceClose(&g_ommSrv);
}

Service* ommGetServiceSession(void) {
    return &g_ommSrv;
}

Result ommGetOperationMode(AppletOperationMode *mode) {
    u8 tmp;

    Result rc = serviceDispatchOut(&g_ommSrv, 0, tmp);
    if (R_SUCCEEDED(rc) && mode)
        *mode = tmp;

    return rc;
}

Result ommGetOperationModeChangeEvent(Event *out, bool autoclear) {
    Handle evt_handle = INVALID_HANDLE;
    Result rc = serviceDispatch(&g_ommSrv, 1,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &evt_handle,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out, evt_handle, autoclear);

    return rc;
}
