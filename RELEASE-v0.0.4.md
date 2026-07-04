# Borato 224 v0.0.4

A reverb audio plugin built with JUCE. This release adds the **CLAP** format,
overhauls the macOS release pipeline (separate Apple Silicon / Intel builds with
real Audio Unit validation), centralizes the version into a single source of
truth, and ships a unified install guide.

> All artifacts are unsigned development builds. The macOS Audio Unit is ad-hoc
> signed and is validated in CI with `auval -v aufx B224 Bora`.

## Highlights

- 🎛️ **New CLAP format** — Borato 224 now builds and ships as CLAP alongside
  VST3, AU (macOS) and Standalone.
- 🍎 **Proper macOS AU validation** — the Audio Unit is ad-hoc signed, installed,
  and validated on a native runner; a failing `auval` now fails the build.
- 🖥️ **Separate Apple Silicon and Intel builds** — `macOS-arm64` and `macOS-intel`
  artifacts, each built on a native runner with a correct deployment target
  (11.0 for arm64, 10.15 for Intel).
- 🔢 **Single source of truth for the version** — set once in `CMakeLists.txt`;
  the CI derives it at runtime for every artifact name and release asset.
- 📄 **Unified install guide** — one `INSTALL.md` covering macOS, Windows and Linux.

## What's new

### Plugin formats
- Added **CLAP** support through the `clap-juce-extensions` submodule
  (`BORATO224_BUILD_CLAP`, on by default). The Projucer project also enables
  AU, LV2, VST3 and Standalone.

### Release automation (CI)
- New GitHub Actions workflow that builds and packages **macOS** and **Ubuntu**
  release artifacts.
- macOS now produces **separate arm64 and Intel** artifacts via a build matrix,
  running on native runners so `auval` can load each component.
- macOS AU step: `codesign` ad-hoc signing + `--verify --deep --strict`, install
  into `~/Library/Audio/Plug-Ins/Components/`, quarantine/AU-cache reset, and a
  **mandatory** `auval -v aufx B224 Bora` that fails the workflow on a bad
  component. The zipped AU is the exact signed/validated bundle.
- The release **version is centralized** in `CMakeLists.txt` and read at runtime,
  so cutting a release is a one-line change.

### Documentation
- Added an **MIT `LICENSE`** and a **`MANUAL.md`**.
- Clarified the README (CMake build generators, install/manual sections).
- Added a single **`INSTALL.md`** with install steps for macOS, Windows and Linux.

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

## Changelog since v0.0.3

- `feat: add CLAP and LV2 build support` (6cb5afc)
- `ci: build macos release artifacts` (ec27a37)
- `fix: build separate macOS arm64 and Intel release artifacts` (#7, 05265a5)
- `docs: add license and clarify install/manual` (ede42c3)
- `docs: clarify cmake build generators` (#4, 91ce094)
- `release: cut v0.0.4 and centralize version + AU validation` (8a87627)

**Full diff:** https://github.com/filipeborato/borato-224/compare/v0.0.3...v0.0.4
