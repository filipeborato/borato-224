# Borato 224

![Borato 224 plugin](assets/224-Borato-vst.png)

Borato 224 is a vintage digital reverb plugin inspired by the workflow and hardware look of classic units such as the Lexicon 224. The project is built with JUCE, uses CMake as the main build path, and also keeps a functional `.jucer` file for Projucer-based workflows.

## Building the Project

### Requirements

- CMake 3.22 or newer.
- A local JUCE checkout. The default expected path on Windows is `C:\JUCE`.
- A C++20 compiler.
- VST3 support through JUCE.

Recommended toolchains:

- Windows: Visual Studio 2022 or 2026 with the C++ desktop workload.
- macOS: Xcode command line tools or full Xcode.
- Linux: GCC or Clang, plus the system packages required by JUCE.

### Windows

Configure with Visual Studio 2022:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBORATO224_JUCE_SOURCE_DIR=C:\JUCE
```

Build Debug:

```powershell
cmake --build build --config Debug
```

Build Release:

```powershell
cmake --build build --config Release
```

The VST3 output is usually generated under:

```text
build/Borato224_artefacts/Debug/VST3/Borato 224.vst3
build/Borato224_artefacts/Release/VST3/Borato 224.vst3
```

If CMake reconfiguration fails because it points to a missing MSVC toolset, remove the stale build folder or configure a new one:

```powershell
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64 -DBORATO224_JUCE_SOURCE_DIR=C:\JUCE
cmake --build build-vs2022 --config Debug
```

### macOS

Install Xcode command line tools:

```bash
xcode-select --install
```

Configure with a local JUCE checkout:

```bash
cmake -S . -B build -DBORATO224_JUCE_SOURCE_DIR=/path/to/JUCE
```

Build Debug:

```bash
cmake --build build --config Debug
```

Build Release:

```bash
cmake --build build --config Release
```

The build can produce `VST3`, `AU`, and `Standalone` targets on macOS, depending on the local JUCE and SDK setup.

### Linux

Install common JUCE build dependencies. Package names vary by distribution; on Debian/Ubuntu this is a typical starting point:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libasound2-dev libjack-jackd2-dev \
  libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev \
  libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.1-dev
```

Configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBORATO224_JUCE_SOURCE_DIR=/path/to/JUCE
```

Build:

```bash
cmake --build build
```

For Release:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBORATO224_JUCE_SOURCE_DIR=/path/to/JUCE
cmake --build build-release
```

Linux builds normally produce `VST3` and `Standalone`. AU is a macOS-only format.

### Projucer Workflow

You can also open:

```text
Borato224.jucer
```

Confirm that the JUCE module paths point to your local JUCE checkout, save the project, then build through the generated IDE project under:

```text
Builds/VisualStudio2022/
Builds/VisualStudio2026/
```

On macOS or Linux, add the matching exporter in Projucer before saving.

## Working on the Project

### Structure

```text
Source/
  PluginProcessor.*    JUCE processor, APVTS, presets, A/B, compare and bypass
  PluginEditor.*       Canvas-drawn JUCE interface
  PluginParameters.*   Parameter IDs, ranges and program presets
  SimpleReverb.*       Main reverb DSP

assets/
  224-Borato-vst.png   Current plugin screenshot for documentation
  lexicon_224.svg      Main visual reference
  push_224.svg         Push button reference
  faders_224.svg       Fader reference
  leds_224.svg         LED reference
```

### Development Guidelines

- Keep GUI and DSP responsibilities separate.
- Use `AudioProcessorValueTreeState` for automatable parameters.
- Do not allocate, lock, log, or perform I/O on the audio thread.
- Change dynamic buffers and larger state only in setup code, constructors, or non-audio-thread paths.
- After DSP changes, test silence, impulses, fast automation, different buffer sizes and different sample rates.
- After GUI changes, test resize behavior, fader alignment, button states and display readability.

## Plugin Quick Manual

### Program

The `PROGRAM` buttons select the base reverb behavior:

- `HALL`: large musical space with a dense tail.
- `ROOM`: smaller room with shorter decay and stronger early reflections.
- `PLATE`: brighter, smoother plate-style tail.
- `CHMBR`: chamber behavior between room and hall.
- `AMBI`: short ambience for space without an obvious long tail.
- `SPACE`: large, expansive and diffuse reverb.
- `RANDOM`: generates a new preset variation and adds a less static modulation character.
- `USER`: neutral slot for manual settings.

### System

- `STORE`: saves the current state as an internal snapshot.
- `RECALL`: restores the saved snapshot.
- `A/B`: switches between two working states.
- `COMPARE`: compares the edited state against the stored state.
- `EDIT`: shows the selected parameter on the LED display.
- `BYPASS`: turns the effect on/off with a short fade to avoid clicks.
- `VALUE -`: decreases the selected parameter.
- `VALUE +`: increases the selected parameter.

### Faders

- `DECAY`: reverb time.
- `BASS`: low-frequency weight in the tail.
- `MID`: midrange tone control.
- `CROSSOVER`: split point between low and high behavior in the reverb tank.
- `TREBLE DECAY`: brightness and high-frequency decay.
- `DEPTH`: modulation depth.
- `PRE-DELAY`: delay before the reverb starts.
- `MIX`: dry/wet balance.

### Display

The LED display is contextual:

- In normal mode it shows the active program.
- When a fader is moved or `EDIT` is active, it shows parameter name, value and unit.
- Unit LEDs indicate `sec`, `ms`, `Hz` or `dB`.
- Status LEDs indicate `Program`, `Edit` and `Store`.

### Starting Point

Start with `HALL`, `PLATE` or `SPACE`, adjust `DECAY` and `PRE-DELAY`, then set `MIX` last. Use `CROSSOVER` together with `BASS` and `TREBLE DECAY` to make the tail darker, fuller or more open.
