# IM-BCI8 Rev.A — Manufacturing Files

This directory contains the manufacturing-oriented files required to reproduce the **IM-BCI8 Rev.A PCB**.

These files correspond to the same hardware revision documented in:

`hardware/kicad/IM-BCI8_RevA.*`

If you intend to modify the design, use the KiCad source files instead.

If you only want to reproduce the tested Rev.A PCB, the files in this directory are the recommended starting point.

---

## Hardware revision

**Board:** IM-BCI8  
**Revision:** Rev.A  
**EDA software:** KiCad 10  
**Layer count:** 4  
**Nominal PCB thickness:** approximately 1.6 mm

The KiCad project defines the following stack:

```text
F.Cu       35 µm
Prepreg
In1.Cu     15.2 µm
FR4 core
In2.Cu     15.2 µm
Prepreg
B.Cu       35 µm
```

The exact dielectric thicknesses and materials may depend on the selected PCB manufacturer.

If controlled impedance or stackup-sensitive modifications are made, the manufacturer's actual production stackup should be checked rather than assuming the KiCad stackup is exact.

---

## Directory contents

```text
manufacturing/
├── README.md
├── gerbers/
│   └── IM-BCI8_RevA_Gerbers.zip
├── BOM/
│   └── IM-BCI8_RevA_BOM.csv
└── assembly/
    └── IM-BCI8_RevA_positions.csv
```

### Gerbers

The Gerber package contains the PCB fabrication layers and drill data required by a PCB manufacturer.

It should include at minimum:

```text
F.Cu
In1.Cu
In2.Cu
B.Cu

F.Mask
B.Mask

F.SilkS
B.SilkS

Edge.Cuts

PTH drill file
NPTH drill file, if applicable
```

A Gerber job file may also be included.

---

## BOM

`IM-BCI8_RevA_BOM.csv` contains the electronic components used on the tested Rev.A board.

The BOM should be considered the reference component list for this revision.

Component availability and pricing change over time, so equivalent substitutions may be possible, but substitutions involving the following components should be reviewed carefully:

- ADS1299
- ESP32-S3 module
- TPS61099
- AP2112K
- LP5907
- MCP73831
- USB protection device
- analog filtering and reference components

Passive substitutions should preserve the required:

- resistance / capacitance value;
- tolerance where relevant;
- voltage rating;
- package size;
- dielectric type where relevant.

---

## Assembly position file

The component position file is provided primarily for users who want to prepare automated or semi-automated assembly.

The original Rev.A prototype was assembled manually.

For manual assembly, this file is not required.

---

## Recommended fabrication approach

Rev.A was designed as a standard 4-layer PCB and does not require exotic fabrication processes.

The board uses standard SMD components, with the ADS1299 in TQFP packaging and the ESP32-S3 as a preassembled module.

The most demanding component to assemble is the TPS61099 package because of its small package and exposed thermal pad.

Users assembling the board manually should therefore be comfortable with:

- fine-pitch SMD soldering;
- solder paste or controlled hot-air reflow;
- visual inspection under magnification;
- continuity testing before power-up.

---

## Before powering a newly assembled board

Do **not** immediately connect the battery and USB after assembly.

The recommended bring-up sequence is documented separately, but the minimum checks should include:

```text
1. Visual inspection
2. Check for solder bridges
3. Check VBAT ↔ GND for shorts
4. Verify regulator outputs progressively
5. Verify ESP32 operation
6. Verify ADS1299 communication
7. Run ADS1299 internal tests
8. Validate physical channels
9. Only then connect the headset
```

Detailed procedures will be available in the project bring-up and troubleshooting documentation.

---

## Battery polarity warning

Rev.A does **not** include reverse-polarity protection on the battery input.

Incorrect battery polarity may permanently damage the board.

Verify the battery connector polarity before first power-up.

---

## Human-use warning

IM-BCI8 Rev.A does not provide medical-grade galvanic isolation.

When electrodes are attached to a person:

**battery operation and wireless communication only.**

Do not simultaneously connect:

- USB;
- charger;
- laboratory power supply;
- grounded oscilloscope;
- function generator;
- other grounded test equipment.

See the project safety documentation before human EEG acquisition.

---

## Prototype cost

The Rev.A PCB and its electronic components cost approximately:

**~$150 USD in one-off prototype quantities**

This figure is only intended as a practical estimate.

It excludes:

- assembly labor;
- tools;
- shipping;
- import taxes;
- spare components;
- development time.

The cost can vary significantly depending on supplier, quantity and region.

---

## Manufacturing reproducibility

The goal of this directory is to preserve a known manufacturing state.

If a future hardware revision changes the schematic or PCB, it should receive a new revision identifier rather than silently replacing these files.

For example:

```text
Rev.A  → first functional prototype
Rev.B  → future corrected / improved hardware
```

The Rev.A manufacturing package should remain available even after later revisions are released so measurements and reported behavior can always be traced back to the corresponding hardware.

---

## Important note

The presence of manufacturing files does not imply that IM-BCI8 is a certified or production-ready commercial product.

Rev.A is experimental open hardware intended for engineering, education and BCI experimentation.

Anyone manufacturing or modifying the board is responsible for verifying the resulting hardware before use.
