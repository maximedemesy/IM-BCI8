# IM-BCI8

**A low-cost 8-channel wireless EEG / Brain-Computer Interface platform based on the ADS1299 and ESP32-S3.**

> **Project status:** Active development / experimental prototype  
> **Intended use:** Education, research, experimentation and BCI development  
> **Not a medical device**

IM-BCI8 is a personal open-hardware EEG project designed to explore how accessible a complete Brain-Computer Interface platform can be made using readily available components, 3D printing and open-source tools.

The system combines an **8-channel ADS1299 analog front-end**, an **ESP32-S3**, battery operation, Wi-Fi connectivity, a browser-based acquisition interface and a **3D-printable dry-electrode headset**.

The entire project is intended to be reproducible: electronics, firmware, mechanical parts, assembly documentation, characterization data and troubleshooting procedures will be published in this repository.

---

## Why this project?

IM-BCI8 started as a personal project and as a gift, but quickly evolved into a broader engineering experiment.

Commercial and research-grade EEG platforms can make experimenting with Brain-Computer Interfaces expensive, particularly for students, makers and independent developers.

The **OpenBCI ecosystem and Cyton board were a major inspiration for this project**. Their work demonstrated how open hardware can make biosignal acquisition considerably more accessible.

IM-BCI8 does not aim to replace OpenBCI.

Instead, it explores a different highly integrated architecture focused on:

- low prototype cost;
- simple wireless operation;
- no external dongle or computer-side acquisition hardware;
- a browser-based user interface;
- 3D-printable mechanical hardware;
- dry-electrode experimentation;
- easy modification of both hardware and firmware.

The current prototype costs approximately **$150 USD for the PCB and electronic components** in one-off quantities.

The complete headset adds approximately **$70 USD**, including dry electrodes, wiring, springs, mechanical hardware and 3D-printing material.

This puts the current prototype at roughly **$220 USD in material cost for the complete system**.

These figures are approximate prototype costs and exclude assembly labor, tools, shipping, import duties and development time.

---

## Current hardware

The current IM-BCI8 prototype includes:

- 8 EEG acquisition channels
- Texas Instruments ADS1299 24-bit biopotential analog front-end
- ESP32-S3 microcontroller
- 250 samples per second acquisition
- Wi-Fi communication
- Integrated web interface hosted directly by the ESP32
- Wireless firmware updates
- Single-cell LiPo battery operation
- Integrated battery charger
- Common reference electrode
- BIAS electrode
- Dry EEG electrodes
- 3D-printable headset and electronics enclosure

The current electrode layout used during development is:

`Fp1 — Fp2 — C3 — C4 — P7 — P8 — O1 — O2`

with separate reference and BIAS electrodes.

---

## Browser-based interface

One of the main design goals of IM-BCI8 is reducing the amount of software required to start experimenting.

The ESP32-S3 hosts its own Wi-Fi network and web interface.

A computer or smartphone can therefore connect directly to the board and access the acquisition interface from a standard web browser.

The current interface provides:

- eight-channel live EEG visualization;
- independent channel identification and scaling;
- FFT / frequency spectrum visualization;
- Delta, Theta, Alpha and Beta band analysis;
- electrode mapping;
- internal ADS1299 test modes;
- experimental event markers;
- CSV recording;
- firmware configuration;
- OTA firmware updates.

No dedicated acquisition dongle is required.

---

## 3D-printable dry-electrode headset

The mechanical system is strongly inspired by the **OpenBCI Ultracortex Mark IV** architecture.

The headset is assembled primarily from 3D-printed components and standard hardware.

Custom components developed for IM-BCI8 include the electronics enclosure, mounting solutions and experimental electrode-support components.

The current complete headset uses approximately **$70 USD of material and components**, including:

- dry electrodes;
- wiring;
- springs;
- screws and mechanical hardware;
- reference / BIAS contacts;
- 3D-printing material.

Detailed assembly instructions and printable files will be provided in the `mechanical/` and `docs/` directories.

---

## Current development status

IM-BCI8 is **not a finished commercial product**.

The first hardware prototype is operational and the complete acquisition chain has been successfully tested, but characterization is still ongoing.

Work currently includes:

- electrical characterization;
- input-referred noise measurements;
- frequency-response measurements;
- channel-to-channel crosstalk;
- long-term stability;
- dry-electrode performance;
- BIAS performance;
- real EEG acquisition;
- repeatability across additional boards;
- firmware and web-interface improvements.

Raw characterization data and test procedures will progressively be added to this repository.

---

## Known limitations

This first hardware revision was deliberately designed around simplicity, compactness and low cost.

It therefore has several known limitations that are important to understand before reproducing it.

The current revision does **not** include reverse-polarity protection on the battery connector.

The electrode inputs are not protected to the level expected from certified biomedical instrumentation.

Battery voltage is not currently monitored by the ESP32.

The design does not provide medical-grade galvanic isolation.

Some components and exposed PCB areas remain vulnerable to inappropriate handling, incorrect power connections or electrostatic discharge.

These compromises were accepted for the first experimental prototype and will be documented as potential improvements for future hardware revisions.

---

## ⚠️ Safety

**IM-BCI8 is experimental hardware and is not a medical device.**

When electrodes are connected to a person, the system must operate from its internal battery only.

Do not connect the board simultaneously to:

- USB;
- a charger;
- a laboratory power supply;
- a grounded oscilloscope;
- a function generator;
- other grounded laboratory equipment.

Human EEG acquisition should use **battery power and wireless communication only**.

Anyone reproducing or modifying this project is responsible for understanding the electrical and biological safety implications of their modifications.

A more complete safety document will be available in [`SAFETY.md`](SAFETY.md).

---

## Repository structure

```text
IM-BCI8/
├── hardware/           PCB design and manufacturing files
├── firmware/           ESP32-S3 firmware and web interface
├── mechanical/         Headset, enclosure and electrode mechanics
├── docs/               Build and usage documentation
├── characterization/   Test procedures, measurements and raw data
└── examples/           EEG experiments and example recordings
```

---

## Open development

IM-BCI8 is intended to remain an open project.

Hardware improvements, firmware features, mechanical designs, analysis tools, experiments and characterization data are welcome.

If you reproduce the board, test a modification or find a design issue, please consider opening an Issue or submitting a Pull Request.

This project is still evolving, and community feedback is part of the development process.

---

## Roadmap

The next development steps include improved characterization, additional EEG experiments, improved battery monitoring, hardware protection improvements, software integration with external BCI tools and preparation of a second hardware revision based on feedback from the first prototype.

---

## License and attribution

Licensing information for the original IM-BCI8 hardware, firmware and documentation will be added before the first official release.

Some mechanical components are derived from or compatible with the OpenBCI Ultracortex ecosystem and will retain the licenses and attribution required by their respective upstream projects.

IM-BCI8 is an independent project and is not affiliated with or endorsed by OpenBCI.
