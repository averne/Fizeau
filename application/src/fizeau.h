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

#ifndef _IPC_H
#define _IPC_H

#include <stdint.h>
#include <switch.h>
#include <common.hpp>

#ifdef __cplusplus
extern "C" {
#endif

Result fizeauInitialize();
void fizeauExit();

Service *fizeauGetServiceSession();

Result fizeauGetIsActive(bool *is_active);
Result fizeauSetIsActive(bool is_active);
Result fizeauGetDuskTime(Time *time);
Result fizeauSetDuskTime(Time time);
Result fizeauGetDawnTime(Time *time);
Result fizeauSetDawnTime(Time time);
Result fizeauGetColor(uint16_t *color);
Result fizeauSetColor(uint16_t color);
Result fizeauEasterEgg(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _IPC_H
