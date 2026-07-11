Firmware of single board OBD1 ECU

https://wiki.rusefi.com/uaEFI-Honda-OBD1

This repository is a rusEFI **custom board** definition: it holds only the board-specific
customization on top of the rusEFI core, which lives in the `ext/rusefi` git submodule.
This board is itself derived from the uaEFI121 board
(`ext/rusefi/firmware/config/boards/hellen/uaefi121/`) and reuses its `mega-uaefi` code.

# Documentation & Schematics

* Board schematics PDF is in the [docs](docs/) folder: `docs/uaefi-Honda-OBD1-e-schematic.pdf`
* [docs/schematic-notes.md](docs/schematic-notes.md) — text extraction of the schematic:
  which pages matter for firmware, full pin-mapping tables, populate-options
* `docs/vato/CurrentTune.msq` — a known-good reference tune
* Wiki page: https://wiki.rusefi.com/uaEFI-Honda-OBD1

# Custom Board Customization

A custom board customizes rusEFI at three levels: build system (`.mk`/`.env` files),
C++ code (pin defaults, validation, knock hardware), and code generation inputs
(connector `.yaml`, TunerStudio `.txt`/`.ini` templating files).

## Build system

* `meta-info.env` — top-level board identity: `SHORT_BOARD_NAME=uaefi-obd1` (used in bundle
  and signature names), `PROJECT_CPU=ARCH_STM32F4`, `USE_OPENBLT=yes` (OpenBLT bootloader).
* `board.mk` — makefile fragment included by the rusEFI build:
  * adds `board_configuration.cpp` and the parent board's `mega-uaefi.cpp` to the build (`BOARDCPPSRC`)
  * adds include paths for `generated/` headers and the parent uaefi121 board (`BOARDINC`)
  * sets the default engine type: `-DDEFAULT_ENGINE_TYPE=engine_type_e::HONDA_OBD1`
  * chains into the parent board's `mega-uaefi.mk`
* `board_unit_tests.mk` — adds board-specific unit tests (`tests/test_example.cpp`) to the
  rusEFI unit test suite.

## C++ files

* `board_configuration.cpp` — the heart of board customization. Its entry point
  `setup_custom_board_overrides()` installs three hooks into the rusEFI core:
  * `custom_board_DefaultConfiguration` → `customBoardDefaultConfiguration()`: starts from the
    parent board defaults (`setUaefiBoardDefaultConfiguration()`), then assigns Honda-OBD1
    pin mapping — injector pins, ICM ignition output, fuel pump, IACV, tach, VR crank/cam
    trigger inputs, and TPS/MAP/CLT/IAT analog channels, all via `Gpio::MM100_*` /
    `MM100_IN_*` symbols from `hellen_mm100_meta.h`.
  * `custom_board_ConfigOverrides` → parent board's `setMegaUaefiBoardConfigOverrides()`.
  * `custom_board_validateConfig` → `customBoardValidateConfig()`: board-specific sanity
    checks; here it rejects a known-bad 24/0 toothed-wheel trigger configuration with a
    critical error.

  The file also defines the `OUTPUTS[]` table (`getBoardMetaOutputs()` /
  `getBoardMetaOutputsCount()` / `getBoardMetaLowSideOutputsCount()`) used by hardware QC /
  board self-test to enumerate every physical output with its connector pin.
* `default_tune.cpp` — `boardTuneDefaults()` sets tune values baked into the default
  calibration (e.g. `displacement = 1.6` for the D16 engine).
* `knock_config.h` — knock sensing hardware description: which ADC (`ADCD3`), which
  pin/channel (PA3 / `ADC_CHANNEL_IN3`), and sample rate/time for the STM32F4.

## Connector YAML (`connectors/`)

* `connectors/honda-obd1.yaml` — the machine-readable pinout definition, the single source
  of truth mapping physical connector pins to MCU functions. Top-level keys:
  * `meta:` — points at the header (`config/boards/hellen_mm100_meta.h`) whose symbols
    (e.g. `MM100_INJ4`, `MM100_IN_CLT_ANALOG`) are referenced by pin entries.
  * `pins:` — one entry per physical pin across the Honda OBD1 A/B/D ECU connectors plus
    the board's own J1–J5 adapter connectors and the C AUX plug. Per-pin fields:
    * `pin:` — connector pin name (e.g. `A7`, `B15`, `J4_8`)
    * `class:` — role: `outputs`, `analog_inputs`, `event_inputs`, `switch_inputs`
      (a list plus `id:` list when one pin serves two roles)
    * `meta:` — the MCU signal symbol from the meta header; generates the `PIN_xxx` defines
    * `ts_name:` — pin display name in TunerStudio drop-downs
    * `function:`, `type:` (inj/ign/ls/hs/av/at/gnd/5v/12v/can/hall/etb), `color:` —
      documentation for humans and the interactive pinout
  * `info:` — interactive pinout metadata: board title, wiki URL, connector photo
    (`76pin-178780.jpg`) and x/y coordinates of every pin on that image.

  From this YAML, rusEFI automation (PinoutLogic.java) generates the files that sit next to
  it: `generated_board_pin_names.h`, `generated_outputs.h`, `generated_ts_name_by_pin.cpp`.
  **Never edit the `generated_*` files manually** — edit the YAML and regenerate.
* `connectors/readme.md` — how to activate custom pin naming for the interactive pinout.

## TunerStudio .ini templating & config generation

* `prepend.txt` — `#define` flags prepended during .ini generation to tailor the
  TunerStudio dialogs to this board: hides irrelevant UI (ETB pins, bank-2 cams, cylinders
  5–12, rotary/HD options...), shows VR threshold settings, and overrides table sizes
  (`IGN_TRIM_SIZE 8`).
* `extra.txt` — board-specific data points added to the configuration definition
  (`struct_no_prefix extra_s` — currently empty).
* `board_config.txt` — board-specific calibrations hook (see comments inside).
* `board_indicators.ini` — board-specific TunerStudio indicators (see `gen_config_common.sh`).
* `generated/` — output of config generation: TunerStudio .ini, console live-data files,
  simulator tunes, canned tunes. Auto-generated; normally refreshed by GitHub Actions or
  `manual-generate-meta-data.sh`.

## Build & maintenance scripts

* `compile_firmware.sh` — build the firmware locally (wraps `ext/rusefi/firmware/bin/compile.sh`
  with this board's `meta-info.env`).
* `_compile_bundle.sh` / `_compile_bootloader.sh` / `_compile_simulator.sh` /
  `_compile_unit_tests.sh` — build the full bundle, OpenBLT bootloader, simulator, and run
  board unit tests respectively.
* `manual-generate-meta-data.sh` — run config/code generation locally (GitHub Actions is the
  recommended path).
* `bin/update-rusefi.sh` — reset and re-init the `ext/rusefi` submodule to a clean state.
* `bin/git_clean.sh` — deep-clean repo and submodule.
* `extra_files_to_copy_on_image/` — extra files (Discord/nightly bundle links) copied into
  the firmware bundle image.

## Tests

* `tests/test_example.cpp` — board-specific GoogleTest cases, wired in via
  `board_unit_tests.mk` and run by `_compile_unit_tests.sh`.
