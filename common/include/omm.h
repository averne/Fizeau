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

#ifndef _OMM_IPC_H
#define _OMM_IPC_H

#include <stdint.h>
#include <switch.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

Result ommInitialize(void);
void ommExit();

Result ommGetOperationMode(AppletOperationMode *mode);
Result ommGetOperationModeChangeEvent(Event *out, bool autoclear);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _OMM_IPC_H
