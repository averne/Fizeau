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

#define CLOCK_IO_BASE 0x60006000
#define CLOCK_IO_SIZE (0x1000)

#define CLK_RST_CONTROLLER_CLK_OUT_ENB_L 0x10
#   define CLK_ENB_ISPB                  (1 << 3)
#   define CLK_ENB_RTC                   (1 << 4)
#   define CLK_ENB_TMR                   (1 << 5)
#   define CLK_ENB_UARTA                 (1 << 6)
#   define CLK_ENB_UARTB                 (1 << 7)
#   define CLK_ENB_GPIO                  (1 << 8)
#   define CLK_ENB_SDMMC2                (1 << 9)
#   define CLK_ENB_SPDIF                 (1 << 10)
#   define CLK_ENB_I2S2                  (1 << 11)
#   define CLK_ENB_I2C1                  (1 << 12)
#   define CLK_ENB_SDMMC1                (1 << 14)
#   define CLK_ENB_SDMMC4                (1 << 15)
#   define CLK_ENB_PWM                   (1 << 17)
#   define CLK_ENB_I2S3                  (1 << 18)
#   define CLK_ENB_VI                    (1 << 20)
#   define CLK_ENB_USBD                  (1 << 22)
#   define CLK_ENB_ISP                   (1 << 23)
#   define CLK_ENB_DISP2                 (1 << 26)
#   define CLK_ENB_DISP1                 (1 << 27)
#   define CLK_ENB_HOST1X                (1 << 28)
#   define CLK_ENB_I2S                   (1 << 30)
#   define CLK_ENB_CACHE2                (1 << 31)

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
#define DC_COM_CMU_LUT1            0xcd8
#   define LUT1_ADDR(x)            ((x) & 0xff)
#   define LUT1_DATA(x)            (((x) & 0xfff) << 16)
#   define LUT1_READ_DATA(x)       (((x) >> 16) & 0xfff)
#define DC_COM_CMU_LUT2            0xcdc
#   define LUT2_ADDR(x)            ((x) & 0x3ff)
#   define LUT2_DATA(x)            (((x) & 0xff) << 16)
#   define LUT2_READ_DATA(x)       (((x) >> 16) & 0xff)
#define DC_COM_CMU_LUT1_READ       0xce0
#   define LUT1_READ_ADDR(x)       (((x) & 0xff) << 8)
#   define LUT1_READ_EN            (1 << 0)
#define DC_COM_CMU_LUT2_READ       0xce4
#   define LUT2_READ_ADDR(x)       (((x) & 0x3ff) << 8)
#   define LUT2_READ_EN            (1 << 0)
#define DC_DISP_DISP_COLOR_CONTROL 0x10c0
#   define DISP_COLOR_SWAP         (1 << 16)
#   define BLANK_COLOR             (1 << 17)
#   define NON_BASE_COLOR          (1 << 18)
#   define CMU_ENABLE              (1 << 20)

#define READ(off)       (*reinterpret_cast<volatile std::uint32_t *>(off))
#define WRITE(off, val) (*reinterpret_cast<volatile std::uint32_t *>(off) = val)

