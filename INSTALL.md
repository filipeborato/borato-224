# Borato 224 — Install guide

Borato 224 ships as **VST3**, **CLAP** and a **Standalone** app on every
platform, plus an **Audio Unit (AU)** on macOS. All artifacts are unsigned
development builds (the macOS AU is ad-hoc signed and validated in CI).

Replace `0.0.4` below with the version of the release you downloaded.

---

## macOS

Artifacts come in two architecture flavours — pick the one for your Mac:

- Apple Silicon (M1/M2/M3/...): `...-macOS-arm64-...`
- Intel: `...-macOS-intel-...`

### VST3

Unzip `Borato224-v0.0.4-macOS-<arch>-VST3.zip` and copy `Borato 224.vst3` to:

```text
~/Library/Audio/Plug-Ins/VST3/
```

### Audio Unit (AU)

Unzip `Borato224-v0.0.4-macOS-<arch>-AU.zip` and copy `Borato 224.component` to:

```text
~/Library/Audio/Plug-Ins/Components/
```

You can validate it locally with:

```bash
auval -v aufx B224 Bora
```

### CLAP

Unzip `Borato224-v0.0.4-macOS-<arch>-CLAP.zip` and copy `Borato 224.clap` to:

```text
~/Library/Audio/Plug-Ins/CLAP/
```

### Standalone

Unzip `Borato224-v0.0.4-macOS-<arch>-Standalone.zip` and move `Borato 224.app`
to `/Applications` or any local folder.

### Gatekeeper note

Because these builds are not notarized, macOS may block them the first time. If
needed, remove the quarantine attribute after unzipping:

```bash
xattr -dr com.apple.quarantine "Borato 224.vst3"
xattr -dr com.apple.quarantine "Borato 224.clap"
xattr -dr com.apple.quarantine "Borato 224.component"
xattr -dr com.apple.quarantine "Borato 224.app"
```

---

## Windows (x64)

### VST3

Unzip `Borato224-v0.0.4-Windows-x64-VST3.zip` and copy the `Borato 224.vst3`
folder to:

```text
C:\Program Files\Common Files\VST3\
```

### CLAP

Unzip `Borato224-v0.0.4-Windows-x64-CLAP.zip` and copy `Borato 224.clap` to:

```text
C:\Program Files\Common Files\CLAP\
```

### Standalone

Unzip `Borato224-v0.0.4-Windows-x64-Standalone.zip` and run `Borato 224.exe`
directly.

### SmartScreen note

Because these builds are not code-signed, Windows SmartScreen may warn the first
time you launch the standalone. Choose **More info → Run anyway** to continue.

After copying the plugins, rescan in your DAW.

---

## Linux (Ubuntu)

### VST3

Extract `Borato224-v0.0.4-Ubuntu-VST3.tar.gz` and copy `Borato 224.vst3` to:

```text
~/.vst3/
```

Example:

```bash
mkdir -p ~/.vst3
tar -xzf Borato224-v0.0.4-Ubuntu-VST3.tar.gz
cp -R "Borato 224.vst3" ~/.vst3/
```

### CLAP

Extract `Borato224-v0.0.4-Ubuntu-CLAP.tar.gz` and copy `Borato 224.clap` to:

```text
~/.clap/
```

Example:

```bash
mkdir -p ~/.clap
tar -xzf Borato224-v0.0.4-Ubuntu-CLAP.tar.gz
cp "Borato 224.clap" ~/.clap/
```

### Standalone

Extract `Borato224-v0.0.4-Ubuntu-Standalone.tar.gz`, mark the app executable,
and run it:

```bash
tar -xzf Borato224-v0.0.4-Ubuntu-Standalone.tar.gz
chmod +x "Borato 224"
./"Borato 224"
```

If the standalone app fails to start, install the JUCE runtime dependencies
listed in the README Linux section. After copying the plugins, rescan in your
DAW.
