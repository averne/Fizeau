// Copyright (c) 2024 averne
//
// This file is part of Fizeau.
//
// Fizeau is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Fizeau is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Fizeau.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdint>

#define DISP_IO_BASE 0x54200000
#define DISP_IO_SIZE (0x80000)

#define DC_CMD_STATE_CONTROL       0x104
#   define GENERAL_ACT_REQ         (1 <<  0)
#   define WIN_A_ACT_REQ           (1 <<  1)
#   define WIN_B_ACT_REQ           (1 <<  2)
#   define WIN_C_ACT_REQ           (1 <<  3)
#   define WIN_D_ACT_REQ           (1 <<  4)
#   define CURSOR_ACT_REQ          (1 <<  7)
#   define GENERAL_UPDATE          (1 <<  8)
#   define WIN_A_UPDATE            (1 <<  9)
#   define WIN_B_UPDATE            (1 << 10)
#   define WIN_C_UPDATE            (1 << 11)
#   define WIN_D_UPDATE            (1 << 12)
#   define CURSOR_UPDATE           (1 << 15)
#   define NC_HOST_TRIG_ENABLE     (1 << 24)
#define DC_COM_CMU_CSC_KRR         0xca8
#define DC_COM_CMU_CSC_KGR         0xcac
#define DC_COM_CMU_CSC_KBR         0xcb0
#define DC_COM_CMU_CSC_KRG         0xcb4
#define DC_COM_CMU_CSC_KGG         0xcb8
#define DC_COM_CMU_CSC_KBG         0xcbc
#define DC_COM_CMU_CSC_KRB         0xcc0
#define DC_COM_CMU_CSC_KGB         0xcc4
#define DC_COM_CMU_CSC_KBB         0xcc8
#define DC_DISP_DISP_COLOR_CONTROL 0x10c0
#   define DISP_COLOR_SWAP         (1 << 16)
#   define BLANK_COLOR             (1 << 17)
#   define NON_BASE_COLOR          (1 << 18)
#   define CMU_ENABLE              (1 << 20)

#define READ(off)       (*reinterpret_cast<volatile std::uint32_t *>(off))
#define WRITE(off, val) (*reinterpret_cast<volatile std::uint32_t *>(off) = val)

