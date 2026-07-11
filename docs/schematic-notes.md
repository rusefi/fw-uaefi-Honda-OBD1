# uaefi-Honda-OBD1 schematic — text extraction & firmware relevance notes

Source: `docs/uaefi-Honda-OBD1-e-schematic.pdf` (28 pages, A2, KiCad-generated).

## How this file was produced (repeatable process)

### Toolset

Only **poppler-utils** is required (`pdftotext`, `pdfinfo`) plus standard shell tools
(`grep`, `tr`, `sed`, `wc`):

```bash
sudo apt-get install poppler-utils     # Debian/Ubuntu/WSL
# brew install poppler                 # macOS
```

The PDF is vector KiCad output, so all labels are real text — **no OCR, no page
rendering, no Python PDF libraries needed**. If a future schematic PDF is a scanned
image instead, this recipe does not apply (check with `pdffonts`: no fonts listed =
image-only PDF).

### Step 1 — survey the document

```bash
cd <repo-root>
PDF=docs/uaefi-Honda-OBD1-e-schematic.pdf
pdfinfo $PDF                                   # page count, page size, producer
pdftotext -layout $PDF /tmp/schematic.txt      # full text, preserving 2D layout
wc -l /tmp/schematic.txt
```

`-layout` keeps the spatial arrangement, which is essential for schematics: a net label
and its connector pin number stay on the same output line.

### Step 2 — split per page and classify by title block

```bash
for i in $(seq 1 $(pdfinfo $PDF | awk '/^Pages/{print $2}')); do
  pdftotext -layout -f $i -l $i $PDF /tmp/page$i.txt
done

# KiCad title blocks identify most sheets
for i in /tmp/page*.txt; do
  echo "=== $i ==="
  grep -iE "Sheet: /|Title:|Module:|Id: [0-9]+/" $i | head -5
done
```

Pages whose title block doesn't extract (dense pages, image-heavy corners) are
classified by distinctive part numbers and net-name patterns instead:

```bash
# fingerprint ambiguous pages by known markers
grep -oiE "STM32[A-Z0-9]*|MAX99[0-9]+|LIS[0-9A-Z]+|TJA[0-9]+|MPX[H0-9]+|TLE[0-9]+|Module:[^ ]+" \
  /tmp/pageN.txt | sort | uniq -c
# condensed human-readable view of a page
tr -s ' ' < /tmp/pageN.txt | grep -viE '^\s*$' | head -40
```

### Step 3 — extract the pin mapping from the top-level page

On this board, page 1 carries everything firmware needs: connector-pin → signal (rows
like `A7 OUT_FUEL_PUMP_RELAY`) and signal → MCU pin (labels like `OUT_INJ1_(PD0)`,
`IN_TPS_(PA4)`). Useful filters:

```bash
tr -s ' ' < /tmp/page1.txt | grep -viE '^\s*$' | less        # read the whole page
grep -oE "[A-Z0-9_]+_\(P[A-E][0-9]+[^)]*\)" /tmp/page1.txt    # all signal→STM32-pin labels
grep -nE "^\s+[ABD][0-9]+ " /tmp/page1.txt                    # OBD1 connector pin rows
```

### Step 4 — cross-check (do not trust layout text blindly)

`pdftotext -layout` interleaves columns on dense pages, so every net↔pin association
in this document was verified against the firmware sources of truth:

```bash
# pin naming source of truth
less connectors/honda-obd1.yaml
# firmware pin assignments
less board_configuration.cpp
# platform symbol → STM32 pin definitions
grep <SYMBOL> ext/rusefi/firmware/config/boards/hellen/hellen_mm100_meta.h
# compare local vs parent-board hardware headers (this is how the PA2/PA3 knock
# discrepancy below was found)
diff knock_config.h ext/rusefi/firmware/config/boards/hellen/uaefi121/knock_config.h
```

Anything where the schematic and the repo disagree gets flagged (see the discrepancy
section below) rather than silently picked from one side.

## Page map and firmware relevance

The board is a stack: **top board (Honda OBD1 adapter) → mega-uaefi module → mcu100 CPU
module + Hellen-One function modules**. Firmware only cares about the mapping
connector-pin ↔ signal ↔ STM32 pin, plus analog conditioning and populate-options that
change firmware configuration. Module internals are fixed by the Hellen platform.

| Page | Sheet | Firmware relevance |
|---|---|---|
| **1** | **Top level `uaefiOBD1honda`** (uaefi-Honda-OBD1.kicad_sch) | **ESSENTIAL — the only page needed for pin mapping.** OBD1 A/B/C/D connector → signal, adapter connectors J1–J5, and signal → STM32 pin for the whole mcu100 module. All tables below come from here. |
| **2** | mega-uaefi module top (uaefi.kicad_sch) | High — routes module signals to submodules; shows which LS/IGN/INJ channel goes through which driver. |
| 3 | /LS/ — low-side drivers | Low — VNLD5160 smart low-side driver internals; relevant only for understanding output behavior/diagnostics. |
| 4 | /LS HOT/ — always-hot low side | Low — same driver, for main-relay/fuel-pump class outputs (OUT_LS5_HOT, OUT_LS6_HOT). |
| 5 | /MAP/ — onboard MAP option | Medium — MPX4/MPXH6400 onboard MAP sensor populate-option (affects MAP sensor type calibration). |
| 6 | /Ignition/ | **Medium-high** — ignition outputs are **logic-level (smart coil) by default**; option: remove R870–R872/R876–R878 and populate Q841–Q846 (ISL9V3040 IGBTs) for dumb coils. Changes ignition mode / dwell settings. |
| 7 | /EGT/ | Low — MAX31855K thermocouple interface (SPI), populate option. |
| 8 | /Injector/ | Low — 3× VNLD5160 = 6 injector channels, driver internals only. |
| 9 | WBO (wideband) module | **Not main-firmware** — has its own STM32F042K6 running separate rusEFI WBO firmware; talks to main ECU over CAN. (Ip_sense = 10 × (LSU_Ip − LSU_Rtrim).) |
| 10 | Power module (12 V/5 V, main relay, VBAT) | Not relevant (only IN_VIGN key-voltage sense reaches firmware). |
| 11 | Hellen-One Knock module | Medium — LMV321 op-amp conditioning feeding IN_KNOCK ADC; background for knock threshold/frequency settings. |
| 12 | VR-discrete module ("outputs a pulse on rising edge") | Medium — conditions the **cam (CYP)** VR signal into a discrete pulse. |
| 13 | VR MAX9924 module | Medium — conditions the **crank (CKP 24-tooth)** VR signal; R843 populate-option turns the VR− input into a Hall input. |
| 14 | Motor-driver module (TLE9201SG) | Medium — ETB H-bridge, DIR+PWM control (2 channels: DC1, DC2). |
| 15–27 | mcu100 CPU-module internals (op-amp buffers, 680k/4.7k TPS-PPS dividers, SD card, level shifters, **STM32F429VGT6** pinout (p18), USB, LIS2DH12 accelerometer (p27), fuses) | **Not needed** — fixed Hellen mcu100 platform; already encoded in `hellen_mm100_meta.h`. Reference only. |
| 28 | Hellen-One CAN module (TJA1051T) | Not relevant beyond "CAN1 has an onboard transceiver". |

**TL;DR: page 1 (and to a lesser degree pages 2, 6, 13) is what firmware work needs;
pages 15–28 are platform module internals you can ignore.**

## Honda OBD1 ECU connector (J6, TE 178780-1) → board signal

Connector A — power & outputs:

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| A1 | OUT_INJ1 | A14 | OEM Fuel Injection Air Control — UNUSED |
| A2 | OUT_INJ4 | A15 | OUT_AC_RELAY |
| A3 | OUT_INJ2 | A16 | OUT_LS1 (aux low side 1) |
| A4 | OUT_HS_VTC (VTEC solenoid, high side) | A17 | OUT_IAB_SOLENOID |
| A5 | OUT_INJ3 | A18/A19 | (WBO option pins) |
| A6 | OEM O2 heater — UNUSED | A20 | OUT_LS2 (purge solenoid / aux low side 2) |
| A7 | OUT_FUEL_PUMP_RELAY | A21 | OUT_IGNITION_CONTROL_MODULE (ICM) |
| A8 | (WBO option pin) | A22 | — |
| A9 | OUT_IDLE (IACV) | A23/A24 | Power ground |
| A10 | OEM engine mount ctrl — UNUSED | A25 | 12V_KEY (ignition switch / vbatt sense) |
| A11 | OUT_EGR_SOLENOID | A26 | Logic ground |
| A12 | OUT_FAN_CONTROL | | |
| A13 | OUT_MIL | | |

Connector B — power & triggers:

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| B1 | 12V_KEY (= A25) | B9 | IN_HS_START (+12 V when cranking) |
| B2 | Logic ground | B10 | IN_VSS (vehicle speed) |
| B3/B4 | TCU — UNUSED | B11/B12 | CYP_1+ / CYP_1− (cam, 1 pulse/rev, VR-discrete module) |
| B5 | IN_AC_PRESSURE_SW | B13/B14 | TDC signal — UNUSED |
| B7 | AT gear pos — UNUSED | B15/B16 | CKP_24+ / CKP_24− (crank, 24 pulses, MAX9924 module) |
| B8 | Power steering sw — UNUSED | | |

Connector C — AUX plug (WBO + spares):

| Pin | Signal |
|---|---|
| C1 | +12V_PROT (WBO heater power) |
| C2 | WBO_HTR− (heater control) |
| C3 | WBO_Vm |
| C4 | WBO_Ip |
| C5 | WBO_Rtrim |
| C6 | WBO_Vs (Un) |
| C7 | IN_FLEX |
| C8/C9/C11 | Aux P17 / P18 / P20 |
| C10 | NC |

Connector D — sensors:

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| D1 | Constant +12 V — UNUSED | D13 | IN_CLT |
| D2 | IN_HS_BRAKE (+12 V when braking) | D14 | IN_O2/AUX2 (OEM O2 — spare analog) |
| D3 | IN_KNOCK | D15 | IN_IAT |
| D4 | IN_DIAG (MIL diag switch) | D17 | IN_MAP |
| D6 | VTEC pressure switch — UNUSED | D18–D20 | +5 V outs (TCM/MAP/TPS) |
| D7 | K-Line — UNUSED | D21/D22 | Sensor ground |
| D11 | IN_TPS1 | | |
| D12 | IN_EGR/AUX1 (EGR lift — spare analog) | | |

## Adapter connectors (on-board Molex Micro-Fit)

| Conn | Type | Pinout |
|---|---|---|
| J1 | 43045-0612 | 1 ETB_P, 2 IN_TPS1, 3 +5VP, 4 ETB_N, 5 IN_TPS2, 6 GNDA — "ETB upgrade option" |
| J2 | 43045-0612 | 2 IN_PPS1, 5 IN_PPS2, +5VP / GNDA on 1,3,4,6 |
| J3 | 43045-0612 | 3 CANH, 6 CANL (+ LED green/yellow, UART2 Rx/Tx per YAML) |
| J4 | 43045-0812 | 8 OUT_IGN1, 7 OUT_IGN2, 6 OUT_IGN3, 5 OUT_IGN4, 4 OUT_TACH, 2 +12V_PROT, 1 GND |
| J5 | 43045-0612 | 2 IN_CRANK (aux hall), 5 IN_CAM (aux hall), +5VP / GNDA |
| J7 | 43045-0212 | 1 EGT+, 2 EGT− (MAX31855 option) |
| J8 | E10T-FT3-PPF | USB (VBUS, D+, D−, CC) |

## Board signal → STM32 pin (mcu100 module edge, from page 1)

Outputs:

| Signal | STM32 | Signal | STM32 |
|---|---|---|---|
| IGN1 | PC13 | INJ1 | PD0 |
| IGN2 | PE5 | INJ2 | PA9 |
| IGN3 | PE4 | INJ3 | PD11 |
| IGN4 | PE3 | INJ4 | PD10 |
| IGN5 (tach) | PE2 | INJ5 | PD2 |
| IGN6 (VTEC) | PB8 | INJ6 | PA8 |
| IGN7 (ICM) | PB9 | INJ7 | PD15 |
| IGN8 (fan) | PE6 | INJ8 | PD12 |
| PWM1 (A/C) | PD13 | PWM4 | PC8 |
| PWM2 (fuel pump) | PC6 | PWM5 | PC9 |
| PWM3 | PC7 | PWM6 | PD14 |

Inputs & comms:

| Signal | STM32 | Signal | STM32 |
|---|---|---|---|
| UART8_TX = crank VR (CKP 24) | PE1 | IN_TPS | PA4 |
| UART8_RX = cam VR (CYP 1) | PE0 | IN_PPS | PA3 |
| IN_CRANK (aux hall, J5) | PB1 | IN_KNOCK | PA2 |
| IN_CAM (aux hall, J5) | PA6 | IN_CLT | PC2 |
| IN_VSS (flex on this board) | PE11 | IN_IAT | PC3 |
| IN_D1 | PE12 | IN_MAP1 | PC0 |
| IN_D2 (VSS on this board) | PE13 | IN_MAP2 | PC1 |
| IN_D3 | PE14 | IN_O2S / CAN wakeup | PA0 |
| IN_D4 | PE15 | IN_O2S2 | PA1 |
| UART2 TX / RX | PD5 / PD6 | IN_AUX1 | PB0 |
| I2C SCL / SDA | PB10 / PB11 | IN_AUX2 | PC4/PE9 |
| OUT_PWR_EN | PE10 | IN_AUX3 (digital only) | PD1 |
| IN_VIGN (key voltage) | PA5 | IN_AUX4 | PC5 |

Note on page 1: "F40x does not have UART8" — on STM32F4 those pins are plain GPIO/EXTI,
which is exactly how the firmware uses them (`triggerInputPins[0] = MM100_UART8_TX`,
`camInputs[0] = MM100_UART8_RX` in `board_configuration.cpp`).

## Populate-options and notes that affect firmware configuration

* **Ignition outputs are logic-level (smart coil) by default.** For dumb coils: remove
  R870–R872, R876–R878 and populate Q841–Q846 (ISL9V3040 IGBTs, page 6). Firmware dwell
  settings must match the populated variant.
* **Crank VR input (MAX9924):** install 4.7 kΩ R843 to use VR− as a Hall input; install
  R770/R775 to dampen very hot VR signals (pages 1, 13).
* **CAN wakeup:** move resistor R143 → R145 to enable CAN wakeup on PA0; AUX1 analog
  input becomes unavailable (page 1).
* **Onboard MAP option** MPX4/MPXH6400 (page 5) — if populated, MAP sensor type in tune
  must match.
* **WBO controller is autonomous** (STM32F042K6, own firmware, CAN-attached) — main
  firmware sees it as a CAN wideband, not analog input (page 9).
* **Knock conditioning** (page 11): LMV321 stage, ~high-pass/gain network into IN_KNOCK.
* **ETB**: two TLE9201SG H-bridges (DC1/DC2), DIR + PWM control lines (page 14).

## ⚠ Known discrepancy found during extraction

This repo's local `knock_config.h` says knock is **PA3 / ADC_CHANNEL_IN3**, but the
schematic (page 1: `IN_KNOCK_(PA2)`) and the parent board file
`ext/rusefi/firmware/config/boards/hellen/uaefi121/knock_config.h` both say
**PA2 / ADC_CHANNEL_IN2** (PA3 is IN_PPS). The local copy looks stale and would sample
the PPS pin instead of knock if it wins the include-path race.

## Key ICs (for reference)

| IC | Function | Page |
|---|---|---|
| STM32F429VGT6 | Main MCU (mcu100 module) | 18 |
| VNLD5160TR-E | Dual smart low-side drivers (injectors, LS outputs) | 3, 4, 8 |
| ISL9V3040D3ST | Ignition IGBTs (dumb-coil option, DNP by default) | 6 |
| MAX9924UAUB+ | VR conditioner (crank) | 13 |
| TLE9201SG | DC motor H-bridge (ETB) ×2 | 14 |
| MPX4/MPXH6400 | Onboard MAP option | 5 |
| MAX31855KASA | EGT thermocouple interface | 7 |
| STM32F042K6 | Standalone WBO controller | 9 |
| TJA1051T/3 | CAN transceiver | 23, 28 |
| LIS2DH12TR | Accelerometer (SPI1) | 27 |
| BTS6143D | High-side driver (VTEC) | 1 |
