# NF Delay 42 -- notes for Claude

Digital delay processor plug-in (VST3 + AU, AAX later) by NF Audio Tools. Modelled on the **behaviour** described in the public owner's
manual of a classic 1U rack digital delay (the manual is in `Docs/`, local only, not pushed). It is NOT a circuit-level clone: no schematic or
unit measurements were available, so the sound is built from the manual's numbers (see "What is modelled"). The name, logo and trademarks of
the original maker are deliberately not used.

## Identity
- Product name **NF Delay 42** (the owner wrote "NF Delay-42"; the standing release model says no dash in plug-in names, so no dash; repo/folder `NF-Delay-42`).
- Bundle `com.nfaudiotools.nfdelay42`, plug-in code `Nfd4`, manufacturer `Nfat`, version in `CMakeLists.txt` line 5 (`project(NFDelay42 VERSION ...)`).
- Licence: NOT enforced yet (`NFDelay42_LICENSE=OFF`, no `Source/License/`). Owner adds it at the end with Paulo (see his release checklist).

## Layout
- `Source/DSP/DelayEngine.h` plain C++17 DSP core, no JUCE. `Tests/DelayEngineTests.cpp` tests it against the manual's numbers:
  `c++ -std=c++17 -O2 Tests/DelayEngineTests.cpp -o /tmp/nfd_tests && /tmp/nfd_tests` (must print ALL TESTS PASSED).
- `Source/PluginProcessor.*` parameters (APVTS) and the mapping to engine units. `Source/PresetManager.*` presets (12 factory + user XML).
- `Source/PluginEditor.*` + `Source/UI/PanelParts.h`: the front panel, drawn in the coordinate space of the reference photo of the hardware
  (panel = 1700 x 152 design units, ratio 11.2:1 -- a 1U rack ratio, so this plug-in does NOT use the 810 x 270 default; default 1500 x 134, resizable, `uiWidth` in state).
- Layers: chassis (plate, blue sections, printed labels, tick dots) = one cached static image; knobs/buttons = own components; knob pointers, LEDs and the 7-segment display are drawn live.
- Reference photo and manual live in `Assets/Reference/` and `Docs/` (git-ignored: third-party material). Measured colours are in `PanelParts.h` (`kBlue` ...).

## What is modelled (manual section numbers)
256 delay taps; 800 ms (16 kHz) / 1600 ms (DELAY X2, 6 kHz) at MANUAL = X1; MANUAL X0.5..X1.5 (3:1); VCO depth 0..full 3:1 and the manual/depth interaction (p. 6);
LFO 0.1-10 Hz, waveform sine <-> envelope follower <-> square; feedback taken after the D/A (before the output filter), HI CUT 6 dB/oct @ 4 kHz in the feedback path,
FB INV / DLY INV; OUTPUT MIX equal blend at centre; input stage 5:1 compression above -3 dB then soft limiter at 0 dB; HEADROOM LEDs; anti-alias + reconstruction
filters calibrated to -3 dB total at 16 / 6 kHz; infinite repeat captured at the first clock pulse, loop = whole memory (changes with MANUAL and X2); clock = memory x fraction (1,3,5,7,9 / 1..64, default 1/2);
SET-MODE DLY/CLK, UP/DOWN with hold-repeat (1 step/s, both buttons = fast); BYPASS; POWER (off = signal passes untouched, power-up resets memory).
Not modelled: converter noise/companding character, the optional memory expansion, rear-panel jacks (host automation replaces the pedals).
Plug-in conveniences not in the hardware: mouse wheel / vertical drag on the display changes the tap; 30 ms glide when the tap changes (avoids clicks).

## Build
Mac is Intel; JUCE is at `~/JUCE`. Dev build (VST3+AU, no copy step):
```bash
mkdir -p build && cd build && cmake .. -DCMAKE_OSX_ARCHITECTURES=x86_64 -DFETCHCONTENT_SOURCE_DIR_JUCE=$HOME/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build . --target NFDelay42_VST3 NFDelay42_AU NFDelay42DspTests -j8
```
UI snapshot (renders the editor to a PNG, never shipped): `-DNFDelay42_BUILD_SNAPSHOT=ON`, target `NFDelay42Snapshot`, usage `NFDelay42Snapshot out.png [width] [tap] [clockMode] [peakDb]`.
AAX (owner's Mac only): `-DNFDelay42_ENABLE_AAX=ON` with the SDK in `~/Documents/AAX_SDK`, then sign with PACE `wraptool` (no wrap GUID registered yet for this product).

## TODO
- Manuals (PT/EN PDF) and the matching 3-line menu entries; installers (macOS DMG script, Windows .iss); PACE product/wrap; licence (with Paulo); GitHub repo `nenofernando/NF-Delay-42` + Paulo (`creativepfb`) as collaborator.
- If the owner can record the real unit (impulse, noise, a sweep), the converter/filter character can be matched much closer.
