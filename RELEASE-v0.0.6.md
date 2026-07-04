# Borato 224 v0.0.6

A major architectural overhaul of the Borato 224 reverb plugin. This release
decomposes the monolithic editor into modular UI components, rewrites the DSP
engine with a proper FDN reverb topology, hardens state management with
thread-safe snapshot/compare/A-B logic, and adds automated state tests.

> All artifacts are unsigned development builds. The macOS Audio Unit is ad-hoc
> signed and validated in CI with `auval -v aufx B224 Bora`.

## Highlights

- 🧩 **Modular UI architecture** — the 800-line `PluginEditor.cpp` is decomposed
  into six focused components: `DisplayComponent`, `ProgramButtonsComponent`,
  `SystemButtonsComponent`, `FaderBayComponent`, `PcbComponent`, and a shared
  `Borato224LookAndFeel`.
- 🔊 **Rewritten DSP engine** — `SimpleReverb` replaced by `ReverbEngine`, an
  8-line FDN reverb with per-program early reflections, input diffusion, stereo
  modulation, DC blocking, and soft-limiting.
- 🔒 **Thread-safe state management** — snapshot, compare, and A/B operations now
  use `std::atomic<bool>` + `CriticalSection` for safe cross-thread access, with
  proper `compare_exchange` for compare toggle and `memory_order_relaxed` for
  lock-free reads.
- 🧪 **Automated state tests** — new `tests/StateTests.cpp` validates STORE/RECALL,
  A/B round-trip, COMPARE, full state serialization, and legacy state migration.

## What's new

### UI refactor

The editor is now composed of dedicated components, each owning its rendering
and layout:

| Component | Responsibility |
|---|---|
| `Borato224LookAndFeel` | Shared look-and-feel: buttons, faders, LEDs, screws, dividers. Replaces the old inner `HardwareButtonLookAndFeel` and `FaderLookAndFeel` classes. |
| `DisplayComponent` | VFD-style display with unit LEDs, mode/label/value, and status indicators (Program/Edit/Store). |
| `ProgramButtonsComponent` | 8 program preset buttons with active-state toggle. |
| `SystemButtonsComponent` | 8 system buttons (Store, Recall, A/B, Compare, Edit, Bypass, Value-, Value+) with callback-based wiring. |
| `FaderBayComponent` | 8 parameter faders with `SliderAttachment` wiring and scale labels. |
| `PcbComponent` | Decorative PCB graphic (traces, resistors, capacitors, DIP IC, toggles). |

The `PluginEditor` now delegates all rendering to these components, reducing
its role to layout, timer-driven display updates, and callback coordination.

### DSP engine (`ReverbEngine`)

The new engine replaces `SimpleReverb` with a more capable architecture:

- **8-line FDN tank** with Hadamard feedback matrix and per-line modulation.
- **4-stage input diffusion** (allpass chain) with per-channel phase offsets.
- **Per-program early reflections** — 6 taps with program-specific delay scaling
  and gain scaling tables.
- **Program shapes** — 8 programs (HALL, ROOM, PLATE, CHMBR, AMBI, SPACE,
  RANDOM, USER) each defining diffusion, density, modulation, size, early level,
  stereo width, and vintage darkening.
- **3-band damping** — one-pole crossover with bass/mid/treble gain controls.
- **Stereo modulation** — sine LFO with per-line phase offsets, plus random
  modulation for the RANDOM program.
- **DC blocking** and **soft limiting** on the output path.

### State management improvements

- **Intelligent USER Preset**: The `USER` program button now acts as a dedicated "Favorite Reverb" slot. Pressing `STORE` captures the global snapshot AND simultaneously copies the current reverb faders into the `USER` preset, allowing you to easily save and recall your favorite tweaks independently of the global RECALL snapshot.
- `abSlotB` and `comparing` changed from `bool` to `std::atomic<bool>`.
- All snapshot maps (`storedSnapshot`, `compareSnapshot`, `abA`, `abB`) protected
  by `juce::CriticalSection`.
- `beginCompare()` uses `compare_exchange_strong` to prevent double-entry.
- `setStateInformation` properly initializes snapshots when loading legacy state
  (pre-v0.0.6 presets that lack snapshot XML).
- `createPersistedStateXml` now uses `state.copyState()` instead of `state.state`
  for thread-safe serialization.
- `setParameterValue` validates `std::isfinite` before applying.
- `parseParameterMapXml` validates finite values before inserting into the map.

### Build system

- Version bumped to **0.0.6** in `CMakeLists.txt`.
- New `BORATO224_BUILD_TESTS` option (OFF by default) to build the state test
  console app.
- Source files updated: `Source/dsp/ReverbEngine.{cpp,h}` and `Source/ui/*`
  replace the old `Source/SimpleReverb.{cpp,h}`.
- `dist-win/` added to `.gitignore`.

### Tests

`tests/StateTests.cpp` provides 5 automated tests:

1. **STORE/RECALL** — verify snapshot capture and restore.
2. **A/B round-trip** — verify slot swapping preserves both slots.
3. **COMPARE** — verify begin/end compare restores the working state.
4. **State round-trip** — verify full serialization/deserialization of snapshot,
   A/B slots, and compare state.
5. **Legacy state migration** — verify that loading a pre-v0.0.6 state
   initializes snapshots from the current parameter values.

Run with:
```bash
cmake -B build -DBORATO224_BUILD_TESTS=ON
cmake --build build --target Borato224StateTests
ctest --test-dir build
```

## Downloads

| Platform | Files |
|---|---|
| macOS (Apple Silicon) | `...-macOS-arm64-VST3.zip`, `-AU.zip`, `-CLAP.zip`, `-Standalone.zip` |
| macOS (Intel) | `...-macOS-intel-VST3.zip`, `-AU.zip`, `-CLAP.zip`, `-Standalone.zip` |
| Windows (x64) | `...-Windows-x64-VST3.zip`, `-CLAP.zip`, `-Standalone.zip` |
| Linux (Ubuntu) | `...-Ubuntu-VST3.tar.gz`, `-CLAP.tar.gz`, `-Standalone.tar.gz` |
| All | `INSTALL.md` |

See **`INSTALL.md`** for per-platform install paths and the Gatekeeper /
SmartScreen notes.

## Changelog since v0.0.5

- `feat: intelligent USER preset mapping tied to STORE button`
- `refactor: decompose PluginEditor into modular UI components`
- `feat: rewrite DSP engine as ReverbEngine with FDN topology`
- `fix: thread-safe snapshot/compare/A-B with atomics and CriticalSection`
- `fix: validate finite values in state parsing and parameter setting`
- `fix: use copyState() for thread-safe APVTS serialization`
- `feat: add automated state management tests`
- `build: bump version to 0.0.6, add BORATO224_BUILD_TESTS option`

**Full diff:** https://github.com/filipeborato/borato-224/compare/v0.0.5...v0.0.6
