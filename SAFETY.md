# IM-BCI8 Safety Information

IM-BCI8 is an **experimental open-hardware EEG / BCI platform**.

It is intended for engineering, education, research and experimentation.

**IM-BCI8 is not a medical device and must not be used for diagnosis, treatment, patient monitoring or other clinical purposes.**

Rev.A was designed as a low-cost experimental prototype. It does not provide the protection, isolation, redundancy or certification expected from commercial medical electrical equipment.

Anyone reproducing, modifying or using the project is responsible for understanding the electrical and biological risks involved.

---

# Core safety rule

When IM-BCI8 electrodes are attached to a person:

> **The board must operate from its internal battery only, with all wired external equipment disconnected.**

Human EEG acquisition should use:

```text
LiPo battery
    │
    ▼
 IM-BCI8
    │
    ├── EEG electrodes
    └── Wi-Fi
```

and **nothing else connected electrically to the system**.

---

# Never connect these during human EEG acquisition

While any EEG, REF or BIAS electrode is attached to a person, do **not** connect the IM-BCI8 board to:

- USB;
- a USB charger;
- a desktop computer;
- a laptop through USB;
- a laboratory power supply;
- a bench power supply;
- a grounded oscilloscope;
- a function generator;
- a logic analyzer;
- a debugger;
- another wired electronic system;
- mains-powered test equipment.

This rule applies even if the external equipment would normally be considered safe for electronics development.

Rev.A has **no medical-grade galvanic isolation** between its USB interface, board ground and electrode-connected electronics.

Do not assume that a USB connection is safe because a laptop is running from its own battery.

For human use:

**disconnect USB completely.**

---

# Development mode vs human-use mode

IM-BCI8 should be treated as having two separate operating modes.

## Development / bench mode

Used for:

- firmware flashing;
- USB debugging;
- oscilloscope measurements;
- function-generator testing;
- power-rail measurements;
- electrical characterization;
- ADS1299 internal test modes.

In this mode:

> **No person must be connected to the electrodes.**

---

## Human EEG mode

Used for actual EEG acquisition.

In this mode:

```text
USB                 DISCONNECTED
charger             DISCONNECTED
bench PSU           DISCONNECTED
oscilloscope        DISCONNECTED
function generator  DISCONNECTED
debugger             DISCONNECTED

battery             CONNECTED
Wi-Fi               ENABLED
```

Communication with the acquisition device should be wireless only.

---

# Before the first human test

A newly assembled or modified board should be validated electrically **before connecting it to a person**.

At minimum:

1. visually inspect the PCB;
2. inspect the ADS1299 and power components for solder bridges;
3. verify that VBAT is not shorted to GND;
4. verify the digital 3.3 V rail;
5. verify the AFE 3.3 V rail;
6. verify the boosted analog rail;
7. verify ESP32 operation;
8. verify ADS1299 SPI communication;
9. run the ADS1299 internal short test;
10. run the ADS1299 internal test signal;
11. verify all eight physical input channels;
12. verify REF operation;
13. verify BIAS behavior;
14. disconnect all external test equipment;
15. switch to battery-only operation;
16. only then connect electrodes to a person.

A board that has not successfully completed bench validation should not be used for human EEG acquisition.

---

# EEG inputs

Rev.A uses a relatively minimal analog input network.

Each signal input includes a series resistor and RC filtering, but this circuitry must **not** be considered equivalent to certified patient-protection circuitry.

The input network primarily supports:

- input filtering;
- reduction of high-frequency interference;
- some current limiting during normal operation.

It is not designed to protect against arbitrary external voltages.

Never intentionally apply an external voltage to an electrode input while the system is attached to a person.

---

# REF and BIAS electrodes

The current system uses:

- eight EEG signal electrodes;
- one common reference electrode;
- one BIAS electrode.

The **REF electrode is an acquisition reference**.

The **BIAS electrode is actively driven by the ADS1299 BIAS amplifier** to reduce common-mode interference.

BIAS must not be confused with a passive ground connection.

Do not:

- connect BIAS directly to an external ground;
- connect REF or BIAS to laboratory equipment while worn;
- inject a signal into REF or BIAS while worn;
- repurpose the electrode connections for electrical stimulation.

IM-BCI8 is an EEG acquisition system, **not an electrical stimulation device**.

---

# Electrode handling

Before use:

- inspect electrodes and cables for mechanical damage;
- make sure conductive parts cannot accidentally touch exposed electronics;
- verify that electrode wiring matches the intended connector pinout;
- ensure cables have adequate strain relief;
- avoid using damaged or heavily oxidized contacts.

Do not place electrodes over broken, irritated or otherwise damaged skin.

If an electrode causes pain, persistent irritation or unusual discomfort, remove it.

The mechanical pressure of dry electrodes should be sufficient for reliable contact but should not cause pain.

---

# Battery safety

Rev.A is designed for a **single-cell LiPo battery**.

The battery connector has **no reverse-polarity protection**.

Reversing the battery connector may permanently damage the board and may create a hazardous failure condition.

Before connecting a battery:

> **Verify connector polarity with the PCB markings, schematic or a multimeter.**

Do not assume that two batteries using physically compatible connectors have the same polarity.

---

# LiPo condition

Do not use a LiPo battery that is:

- swollen;
- punctured;
- mechanically damaged;
- leaking;
- unusually hot;
- showing damaged insulation or exposed conductors.

A damaged LiPo battery should be removed from service and handled according to appropriate local battery-disposal procedures.

Do not mechanically compress, puncture or modify the battery.

---

# Charging

Rev.A uses an MCP73831 single-cell LiPo charger.

The current hardware uses:

```text
RPROG = 2 kΩ
```

which configures a nominal charging current of approximately:

```text
500 mA
```

The selected battery must be rated to accept the configured charging current.

Charging should preferably be performed with the main IM-BCI8 electronics switched off.

Recommended sequence:

```text
Remove headset / electrodes from person
        ↓
Switch IM-BCI8 OFF
        ↓
Connect USB charger
        ↓
Charge battery
        ↓
Disconnect USB
        ↓
Switch IM-BCI8 ON
        ↓
Reconnect headset
```

**Never charge the battery while the electrodes are attached to a person.**

---

# USB

The USB-C port is intended for:

- firmware development;
- firmware flashing;
- debugging;
- battery charging.

It is **not** intended to provide an isolated data connection during human EEG acquisition.

Wi-Fi should be used instead.

---

# Exposed PCB and ESD

Rev.A contains exposed electronic components and sensitive analog circuitry.

Handle the PCB carefully.

Electrostatic discharge may damage:

- ADS1299 inputs;
- ESP32-S3;
- USB protection circuitry;
- power-management components;
- other semiconductor devices.

During assembly and bench work, appropriate ESD precautions are recommended.

Never assume that a board is safe for human use simply because it powers on.

---

# Moisture and contamination

Do not use the board when it is wet.

Keep liquids, conductive gels and cleaning products away from exposed electronics and connectors.

If the PCB becomes wet or contaminated:

1. disconnect power;
2. disconnect the battery;
3. do not attach the device to a person;
4. inspect and clean the board appropriately;
5. fully dry and electrically revalidate the system before reuse.

---

# Stop immediately if abnormal behavior occurs

Disconnect the system from the person immediately if any of the following occurs:

- unexpected heating;
- burning smell;
- visible smoke;
- repeated resets or unstable power;
- battery swelling;
- damaged wiring;
- exposed conductor touching the user;
- unexpected electrical sensation;
- pain or persistent skin irritation;
- unexpected behavior following a hardware modification.

Do not continue testing on a person to diagnose an electrical fault.

Return the device to bench-testing mode first.

---

# Hardware modifications

The safety assumptions described here apply only to the documented IM-BCI8 Rev.A architecture.

Modifications to:

- the power system;
- EEG input network;
- REF circuitry;
- BIAS circuitry;
- battery charger;
- USB interface;
- grounding;
- PCB layout;
- electrode system;

may introduce new hazards.

A modified board should be treated as a new experimental design and should repeat the complete electrical bring-up procedure before human use.

---

# Firmware modifications

Firmware cannot replace missing hardware protection.

Software errors may still:

- configure the ADS1299 incorrectly;
- enable unexpected acquisition modes;
- alter BIAS behavior;
- change power-management behavior;
- cause repeated resets or unstable operation.

After significant firmware changes, bench validation should be repeated before human testing.

---

# Measurement equipment

Oscilloscopes, function generators and laboratory power supplies are extremely useful during development, but they should only be used while **no person is connected to the electrode system**.

Many laboratory instruments have their signal ground connected to protective earth.

Connecting such equipment to an electrode-connected system can create current paths that do not exist during battery-only operation.

Therefore:

> **Bench instrumentation and human electrode connection must never overlap on Rev.A.**

---

# Intended use limitations

IM-BCI8 should not be relied upon for:

- medical diagnosis;
- neurological diagnosis;
- detection of epilepsy or seizures;
- monitoring of medical conditions;
- treatment decisions;
- emergency monitoring;
- life-critical applications.

Recorded signals may contain:

- motion artifacts;
- electrode artifacts;
- environmental interference;
- muscle activity;
- eye activity;
- saturation;
- dropped samples;
- digital-processing artifacts.

A signal that visually resembles a physiological waveform is not proof of clinical validity.

---

# Research and experimental use

Users performing experiments involving other participants should consider the applicable:

- institutional requirements;
- ethical review requirements;
- informed-consent requirements;
- privacy requirements;
- data-protection requirements;

for their own jurisdiction and organization.

Publishing IM-BCI8 as open hardware does not remove those responsibilities.

---

# Rev.A known safety limitations

The current hardware revision intentionally prioritizes low cost, compactness and experimental accessibility.

Known limitations include:

- no battery reverse-polarity protection;
- no medical-grade galvanic isolation;
- limited EEG input protection;
- no dedicated battery-voltage measurement;
- no certified patient-protection network;
- no formal medical-device certification;
- exposed prototype electronics;
- no complete hardware protection against incorrect external connections.

Future revisions may address some of these limitations.

Until then, they should be considered part of the operating constraints of Rev.A.

---

# Summary

For human EEG acquisition with IM-BCI8 Rev.A:

```text
✓ Battery power
✓ Wireless communication
✓ Electrically validated board
✓ Correct battery polarity
✓ Undamaged electrodes and wiring

✗ USB
✗ Charger
✗ Bench power supply
✗ Oscilloscope
✗ Function generator
✗ Wired debugging equipment
✗ External electrical signals
```

The safest operating principle for Rev.A is simple:

> **If a person is connected to the electrodes, nothing else should be electrically connected to the board except the internal battery and the electrode system.**

---

## Disclaimer

This document summarizes the safety constraints identified during development of IM-BCI8 Rev.A.

It is not an exhaustive electrical-safety analysis and does not replace professional engineering review, institutional safety procedures or applicable regulatory requirements.

IM-BCI8 remains experimental open hardware.
