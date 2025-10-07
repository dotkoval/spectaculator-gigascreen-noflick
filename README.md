# Gigascreen No‑Flick (Spectaculator Render Plugin)
<p align="right">English | <a href="README_ru.md">Русский</a></p>

A small render plugin for **Spectaculator** (ZX Spectrum emulator) that reduces flicker for **Gigascreen** by blending consecutive frames using precomputed lookup tables (LUTs). It ships as multiple binaries because **Spectaculator does not expose runtime configuration** for render plugins.

> This is not a silver bullet. It works best for Gigascreen content, where two alternating frames intentionally encode color. In regular dynamic scenes (especially true 50 fps motion), blending adjacent frames can soften detail, introduce slight ghosting, and overall look less presentable. There are a couple of ideas in development on how this could be improved, but for now it stays as it is.

> Built quickly while prototyping my own Gigascreen game (**Project AZX**: [Telegram](https://t.me/project_azx), [Forum](https://spectrumcomputing.co.uk/forums/viewtopic.php?t=13101)). I do not use Spectaculator for day‑to‑day development, but it is very popular in the community — so here is a compatible render plugin. Sources are open; this is a practical, minimal project — no plans to turn it into a full “industrial” build system.

---

## What it does
- Blends two consecutive Gigascreen frames with a fixed weight to suppress/soften flicker.
- Uses precomputed 2D LUTs per channel (5‑bit/6‑bit for RGB565), so runtime cost is minimal.
- Variants differ only by **blend ratio** (50/50 vs 40/60) and **gamma**; there is **no runtime UI** — pick the `.rpi` you want.

### Some screenshots

[<img src="docs/images/screenshot-gbt269.png" width="400">](docs/images/screenshot-gbt269.png)
[<img src="docs/images/screenshot-across-the-edge.png" width="400">](docs/images/screenshot-across-the-edge.png)

[<img src="docs/images/screenshot-kpacku1.png" width="400">](docs/images/screenshot-kpacku1.png)
[<img src="docs/images/screenshot-kpacku2.png" width="400">](docs/images/screenshot-kpacku2.png)

---

## Variants
### Blend “intensity” (weight of the second frame)
- **No‑Flick 100%** → 50/50 (flicker completely gone).
- **No‑Flick 80%** → 40/60 (keeps some “retro vibe”: flicker is much softer, but not fully gone).

### Gamma
- **Recommendation:** `Gamma = 2.4` (visually closer to a CRT look, to my eye). Your taste may vary: try 2.5 or 2.2.
- Available: `G1.8`, `G2.0`, `G2.2`, `G2.4`, `G2.5`, and `Linear` (no gamma mapping).  
  *Note:* the numeric range looks wide, but the **visual difference is subtle** in this blending context — roughly: 1.8 a bit darker, 2.5 a bit brighter.

Here is an image where you can pick the gamma value for your monitor based on which color best matches the central stripe in the image (a black-and-white checkerboard pattern).

![Gamma Samples](docs/images/gamma-samples.png)
---

## Install
1. Download plugin binaries from [Releases section](https://github.com/dotkoval/spectaculator-gigascreen-noflick/releases)
2. Copy the chosen `.rpi` files to `RenderPlugin` folder:
   ```
   <Spectaculator installation dir>/RenderPlugin/
   ```
3. Start Spectaculator and enable the plugin in the render plugin menu (Options → Display → Render Plugins).

![Spectaculator Options Display](docs/images/screenshot-display-settings.png)


### Ready‑made binaries
I will attach prebuilt `.rpi` files to **GitHub Releases** for this repo. You can find and download it in [Releases section](https://github.com/dotkoval/spectaculator-gigascreen-noflick/releases)

---

## Notes and assumptions
- **Pixel format.** Spectaculator appears to deliver frames in **RGB565**. In practice, actual Gigascreen scenes often use a **very small subset** of the 65536 colors. This is one reason the LUTs are tiny and fast.
- **No runtime options.** Multiple binaries exist **only** because Spectaculator does not provide a config API for render plugins. Hence different `.rpi` files for different blends and gammas.
- **Performance.** Blending is a couple of table lookups per channel; overhead is negligible.
- **Platforms.** Developed and tested on Windows. **No macOS build** (I have nothing to test it on).

---

## Build (very optional)
Minimalistic, cl‑based build works fine. 
You can find `build.cmd` script in the root for building plugin variations on Windows machine.

Anyway, here is a small example (Windows, VS Build Tools):

```bat
rem Developer Command Prompt or call vcvars64.bat first
cl /nologo /LD /O2 /EHsc /MD /I. ^
  /DGAMMA=24 ^
  /DRETRO_VIBES ^
  src\Main_Gigascreen.cpp ^
  /link /OUT:"Gigascreen_100_g24.rpi"
```

- `GAMMA=18|20|22|24|25` or omit it for `Linear` color space.
- `RETRO_VIBES` → **No‑Flick 80%**, by default (if not set) → **No‑Flick 100%**.
- `PLUGIN_TITLE` is what Spectaculator shows in the list.

There is a small Python helper that generates LUT headers into `luts/<gammaXX|linear>/<noflickXX>/...`. Using that script is optional for consumers — release binaries already contain all the headers compiled into `.rpi`. But it is might be interesting for curious people who wants to customize blending options.

---

## References / background
- sRGB colorspace in Gigascreen: https://hype.retroscene.org/blog/graphics/808.html  
- sRGB transfer functions (linear↔sRGB): https://en.wikipedia.org/wiki/SRGB

---

## Disclaimer
This is an “as is” side tool I wrote while building **Project AZX** ([Telegram channel](https://t.me/project_azx), [Thread on Spectrum Computing](https://spectrumcomputing.co.uk/forums/viewtopic.php?t=13101)).
If you have ideas or improvements (or CRT comparison captures), feel free to open an Issue or PR :)

.koval'2025
