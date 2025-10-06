#!/usr/bin/env python3

import argparse
import math
from typing import List

def rint(x):
    return int(math.floor(x + 0.5))

def gen_forward(maxv, gamma, linear: bool):
    if linear:
        return list(range(maxv + 1))
    # linear -> sRGB: ((1+0.055)*pow(v/maxv, 1/gamma) - 0.055) * maxv
    out = []
    a = 0.055
    invg = 1.0 / gamma
    for v in range(maxv+1):
        lin = v / maxv
        corr = ((1.0 + a) * (lin ** invg) - a) * maxv
        out.append(min(max(rint(corr), 0), maxv))
    return out

def gen_inverse(maxv, gamma, linear: bool):
    if linear:
        return list(range(maxv + 1))
    # sRGB -> linear: pow((v/maxv + 0.055)/(1+0.055), gamma) * maxv
    out = []
    a = 0.055
    for v in range(maxv+1):
        srgb = v / maxv
        corr = ((srgb + a) / (1.0 + a)) ** gamma
        corr *= maxv
        out.append(min(max(rint(corr), 0), maxv))
    return out

def emit_table(name, arr, per_line=16):
    print(f"static const unsigned char {name}[{len(arr)}] = {{")
    for i in range(0, len(arr), per_line):
        chunk = ", ".join(f"0x{x:02X}" for x in arr[i:i+per_line])
        print(f"  {chunk}{',' if i+per_line < len(arr) else ''}")
    print("};")

def make_blend2d(maxv: int, lut_fwd: List[int], lut_inv: List[int], ratio: float) -> List[List[int]]:
    N = maxv + 1
    mat: List[List[int]] = []
    for a in range(N):
        ra = lut_inv[a]
        row = [0] * N
        for b in range(N):
            rb = lut_inv[b]
            # half-up average of two ints: (x + y ) // 2
            lin_mix = int(ra * (1 - ratio) + rb * ratio)
            row[b] = lut_fwd[lin_mix]
        mat.append(row)
    return mat

def emit_2d_uint8(name: str, mat: List[List[int]], per_line: int = 16) -> None:
    N = len(mat)
    print(f"static const unsigned char {name}[{N}][{N}] = {{")
    for r in range(N):
        row = mat[r]
        print("  {")
        for i in range(0, N, per_line):
            chunk = row[i:i+per_line]
            hexes = ",".join(f"0x{v:02X}" for v in chunk)
            trail = "," if (i + per_line) < N else ""
            print(f"    {hexes}{trail}")
        end_trail = "," if (r + 1) < N else ""
        print(f"  }}{end_trail}")
    print("};")

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Generate LUT headers (linear<->sRGB) in hex with 16 items per line.")
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--gamma", type=float, default=2.4, help="Gamma value (e.g. 2.4)")
    mode.add_argument("--linear", action="store_true", help="Use identity (no sRGB transform)")
    ap.add_argument("--ratio", type=float, default=0.5, help="Mix previous/current frame ratio (e.g. 0.5)")
    ap.add_argument("--bits", type=int, default=5, choices=[5,6,8], help="Bit depth: 5, 6 or 8")
    args = ap.parse_args()
    is_linear = bool(args.linear)

    maxv = (1 << args.bits) - 1

    lut_fwd = gen_forward(maxv, args.gamma, is_linear)
    lut_inv = gen_inverse(maxv, args.gamma, is_linear)
    mat = make_blend2d(maxv, lut_fwd, lut_inv, args.ratio)

    if is_linear:
        arr_name = f"linear_blend_{args.bits}b"
    else:
        g10 = int(round(args.gamma * 10))
        arr_name = f"srgb_blend_g{g10}_{args.bits}b"

    arr_name = f"lut_blend_{args.bits}b"

    print("// Auto-generated with tools/gen_lut.py")
    print("// 2D blend table: array[a][b] = fwd_avg(inv[a], inv[b])")
    print(f"// gamma = {args.gamma}, linear = {is_linear}, bits = {args.bits}, mix ratio = {args.ratio}\n")
    print("#pragma once")

    #emit_table(f"srgb_to_linear_{args.bits}b", lut_fwd)
    #emit_table(f"linear_to_srgb_{args.bits}b", lut_inv)

    emit_2d_uint8(arr_name, mat, per_line=16)
