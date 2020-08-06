#!/bin/python3

import os, sys
from ctypes import Structure as struct, c_uint8 as uint8_t, c_uint16 as uint16_t
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import hsv_to_rgb
from scipy.optimize import curve_fit


def clamp(x, lo, hi):
    return max(min(x, hi), lo)


class Cmu(struct):
    _fields_ = [
        ("enable",   uint16_t),         # __nv_in
        ("_csc",     uint16_t * 9),     # __nv_in
        ("_lut_1",   uint16_t * 256),   # __nv_in
        ("_lut_2",   uint16_t * 960),   # __nv_in
        ("enabled",  uint16_t),         # __nv_out
        ("response", uint8_t * 4),      # __nv_out
    ]


    @property
    def csc(self):
        return np.array([QS18(nb).to_float() for nb in self._csc]).reshape((3, 3))

    @csc.setter
    def csc(self, csc):
        for i in range(len(csc)):
            self._csc[i] = uint16_t(csc[i])


    @property
    def lut_1(self):
        return np.array(self._lut_1)

    @lut_1.setter
    def lut_1(self, lut):
        for i in range(len(lut)):
            self._lut_1[i] = uint16_t(lut[i])


    @property
    def lut_2(self):
        return np.array(self._lut_2)

    @lut_2.setter
    def lut_2(self, lut):
        for i in range(len(lut)):
            self._lut_2[i] = uint16_t(lut[i])


# Fixed-point fractional number, M = number of integer bits, N = number of fractional bits
class QMN:
    M = N = 0

    def __init__(self, x):
        if type(x) is float:
            self.nb = self.float_to_int(x)
        else:
            self.nb = int(x)


    @classmethod
    def int_to_float(cls, nb):
        return nb / (1 << cls.N)


    @classmethod
    def float_to_int(cls, nb):
        return int(nb * (1 << cls.N)) & 0xffff


    def to_float(self):
        return self.int_to_float(self.nb)


    def to_int(self):
        return self.nb


class QS18(QMN):
    M = 1
    N = 8


class QS84(QMN):
    M = 8
    N = 4


def degamma_ramp(array, bits):
    def degamma(x, bits):
        if x <= 0.040045:
            x /= 12.92
        else:
            x = ((x + 0.055) / (1 + 0.055)) ** 2.4
        shift = (1 << bits) - 1; mask = (1 << (bits + 1)) - 1
        return (round(x * shift) & mask) * 256 / shift
    return np.array([degamma(nb, bits) for nb in array])


def regamma_ramp(array, bits):
    def regamma(x, bits):
        if x <= 0.0031308:
            x *= 12.92
        else:
            x = (1 + 0.055) * x ** (1 / 2.4) - 0.055
        shift = (1 << bits) - 1; mask = (1 << (bits + 1)) - 1
        return (round(x * shift) & mask) * 256 / shift
    return np.array([regamma(nb, bits) for nb in array])


def gamma_ramp(array, gamma, bits):
    def gamma_eq(x, gamma, bits):
        mask = (1 << bits) - 1
        return (int(x ** gamma * mask) & mask) * 256 / mask
    return np.array([gamma_eq(nb, gamma, bits) for nb in array])


def apply_luma(array, luma=0.0):
    luma = (clamp(luma, -1, 1) * 255 + 255) / 255
    return np.array([clamp(round(col * luma), 0, 0xff) for col in array])


def apply_range(array, min=16, max=235):
    return np.array([round(col * (max - min) / 255 + min) for col in array])


def plot_rgb(ax, mat=None):
    # Generate RGB plot from HSV values
    v, h = np.mgrid[0:1:100j, 0:1:300j]
    s = np.ones_like(v)
    rgb = hsv_to_rgb(np.dstack((h, s,v)))

    # Multiply by matrix
    if mat is not None:
        shape = rgb.shape
        rgb = rgb.reshape(-1, 3)
        rgb = np.dot(rgb, mat.T)
        rgb = rgb.reshape(shape)

    # Plot
    ax.imshow(rgb, origin="lower", aspect="auto")
    ax.axis("off")


def plot_cmu(cmu, name="", show_generated=True):
    fig, axes = plt.subplots(2, 2)
    ax1, ax2, ax3, ax4 = axes.flat

    ax1.plot([0, 256], [0, 256], label="Linear")
    ax1.plot(np.linspace(0, 256, 256), [QS84(nb).to_float() for nb in cmu.lut_1], label="LUT1")
    if show_generated:
        ax1.plot(np.linspace(0, 256, 256), gamma_ramp(np.linspace(0, 1, 256), 2.2, 12), color="r",   linestyle=":", label="Calculated v1")
        ax1.plot(np.linspace(0, 256, 256), degamma_ramp(np.linspace(0, 1, 256), 12), color="purple", linestyle=":", label="Calculated v2")
    ax1.set_title(r"LUT1 (sRGB \textrightarrow{} linear)")
    ax1.grid(linestyle=":")
    ax1.legend()

    ax2.plot([0, 256], [0, 256], label="Linear")
    ax2.plot(np.linspace(0, 32, 512), cmu.lut_2[:512], label="LUT2.1")
    ax2.plot(np.linspace(32, 256, 448), cmu.lut_2[512:], label="LUT2.2")
    if show_generated:
        ax2.plot(np.linspace(0, 32, 512),   gamma_ramp(np.linspace(0, 0.125, 512), 1 / 2.2, 8), color="r",    linestyle=":", label="Calculated1 v1")
        ax2.plot(np.linspace(32, 256, 448), gamma_ramp(np.linspace(0.125, 1, 448), 1 / 2.2, 8), color="pink", linestyle=":", label="Calculated2 v1")
        ax2.plot(np.linspace(0, 32, 512),   regamma_ramp(np.linspace(0, 0.125, 512), 8), color="purple",  linestyle=":", label="Calculated1 v2")
        ax2.plot(np.linspace(32, 256, 448), regamma_ramp(np.linspace(0.125, 1, 448), 8), color="magenta", linestyle=":", label="Calculated2 v2")
    ax2.set_title(r"LUT2 (linear \textrightarrow{} sRGB)")
    ax2.grid(linestyle=":")
    ax2.legend()

    plot_rgb(ax3)
    ax3.set_title("Colors before CSC pass")

    plot_rgb(ax4, cmu.csc)
    ax4.set_title("Colors after CSC pass")

    fig.tight_layout()
    fig.subplots_adjust(top=0.9)
    fig.suptitle(f"Cmu representation for {name.replace('_', '-')}")
    # fig.savefig("cmu.png", dpi=196, bbox_inches="tight")


def main(argc, argv):
    if (argc < 2):
        print(f"Usage {argv[0]} cmu.bin ...")

    applied_transformation = False
    for f in argv[1:]:
        with open(f, "rb") as fp:
            cmu = Cmu()
            fp.readinto(cmu)

            # if not applied_transformation:
            #     cmu.lut_2 = apply_luma(cmu.lut_2, -0.3)
            #     applied_transformation = True

            # if not applied_transformation:
            #     cmu.lut_2 = apply_range(cmu.lut_2)
            #     applied_transformation = True

            plot_cmu(cmu, os.path.basename(f))
    plt.show()


if __name__ == "__main__":
    sys.exit(main(len(sys.argv), sys.argv))
