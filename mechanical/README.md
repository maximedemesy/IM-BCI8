# IM-BCI8 Mechanical Integration

This directory documents the mechanical configuration used for the current **IM-BCI8 Rev.A headset**.

The headset is based on the open-source **OpenBCI Ultracortex Mark IV**, with several modified and original components developed specifically for the IM-BCI8 prototype.

The mechanical system is therefore divided into three clearly identified categories:

1. **Unmodified OpenBCI components** archived locally for reproducibility.
2. **OpenBCI-derived components** modified for the IM-BCI8 build.
3. **Original IM-BCI8 components** designed specifically for this project.

The objective of this directory is not to claim authorship of the Ultracortex headset, but to preserve the exact mechanical configuration used by IM-BCI8 Rev.A so that the prototype remains reproducible even if upstream files or external product links change in the future.

---

# 1. Directory structure

The intended mechanical directory is:

```text
mechanical/
├── README.md
│
├── upstream/
│   └── openbci-ultracortex-mark-iv/
│       ├── frame/
│       ├── inserts/
│       ├── clips/
│       ├── electrode-holders/
│       │   ├── M4_Hardware_09_Eholder.STL
│       │   └── M4_Hardware_09_Octabolt.STL
│       ├── LICENSE.md
│       └── UPSTREAM.md
│
├── derived/
│   └── IM-BCI8_Comfort_Tip.stl
│
├── enclosure/
│   ├── IM-BCI8_Enclosure_Body.stl
│   └── IM-BCI8_Enclosure_Cover.stl
│
└── earclip/
    └── IM-BCI8_Compliant_Earclip.stl
```

Only the OpenBCI components actually used for the IM-BCI8 Rev.A headset need to be archived locally.

---

# 2. Upstream headset

The base headset is the **OpenBCI Ultracortex Mark IV**.

The current IM-BCI8 build uses OpenBCI designs for:

- the headset frame;
- frame inserts;
- wire clips;
- electrode-holder components;
- the Octabolt adjustment mechanism.

The original Mark IV documentation is available from OpenBCI:

`https://docs.openbci.com/AddOns/Headwear/MarkIV/`

The upstream source repository is:

`https://github.com/OpenBCI/Ultracortex`

The following OpenBCI files are also used directly:

```text
Mark_IV/M4_Hardware_09_Eholder.STL
Mark_IV/M4_Hardware_09_Octabolt.STL
```

For reproducibility, the upstream repository revision checked when preparing the IM-BCI8 mechanical archive was:

```text
338b52b4aba4182ae77f697ff0dc544d95cc6ab2
```

The original OpenBCI files are archived locally under:

```text
mechanical/upstream/openbci-ultracortex-mark-iv/
```

so that the exact mechanical configuration used by Rev.A does not depend entirely on external links remaining available indefinitely.

The upstream OpenBCI repository remains the authoritative source for these designs.

---

# 3. OpenBCI licensing

The `OpenBCI/Ultracortex` repository distributes its content under the:

**GNU General Public License version 3 — GPLv3**

A copy of the upstream license is preserved together with the archived files:

```text
mechanical/upstream/openbci-ultracortex-mark-iv/LICENSE.md
```

Files copied without modification should retain their original names and provenance.

Any modified OpenBCI-derived component should be clearly identified as a modified work.

---

# 4. FDM printing of the Ultracortex parts

Some Mark IV electrode-holder components were originally designed as prototypes for parts normally manufactured using other processes.

For the IM-BCI8 prototype, the relevant parts were instead produced directly using consumer FDM printing.

This includes:

```text
M4_Hardware_09_Eholder.STL
M4_Hardware_09_Octabolt.STL
```

Despite not originally being intended as production FDM components, both have worked satisfactorily on the current headset.

This should be considered an **IM-BCI8 manufacturing adaptation**, not an official OpenBCI manufacturing recommendation.

Some light fitting, cleaning or sanding may be required depending on printer accuracy.

Particular attention should be given to:

- Octabolt geometry;
- sliding or threaded surfaces;
- electrode-holder fit;
- insert fit;
- moving contact surfaces.

---

# 5. Printing material

The complete current headset was printed in **PETG**.

The printer used for the Rev.A mechanical prototype was an:

**Elegoo Centauri Carbon**

PETG was used for:

- Ultracortex frame components;
- frame inserts;
- clips;
- Eholders;
- Octabolts;
- custom comfort tips;
- electronics enclosure;
- compliant ear clips.

PETG was selected because it provides a useful combination of:

- toughness;
- moderate flexibility;
- resistance to brittle fracture;
- good durability for repeatedly handled parts.

Exact print parameters are not yet considered a strict requirement, since they can depend strongly on printer, PETG formulation and slicer.

Dimensional accuracy is more important than reproducing one exact slicer profile.

---

# 6. IM-BCI8 comfort tip

The original OpenBCI files provided the Eholder and Octabolt mechanisms, but the exact comfort tip required for the current printed configuration was not available as a directly usable printable part.

A custom comfort tip was therefore created by modifying:

```text
M4_Hardware_09_Eholder.STL
```

The resulting component is:

```text
mechanical/derived/IM-BCI8_Comfort_Tip.stl
```

This part is **not a completely original IM-BCI8 design**.

It is a derivative of the OpenBCI Ultracortex Eholder geometry and should therefore remain clearly identified as such.

Its provenance should be documented as:

```text
Original:
OpenBCI Ultracortex Mark IV
M4_Hardware_09_Eholder.STL

Modification:
Geometry adapted into a printable comfort support compatible with
the Octabolt mechanism used on the IM-BCI8 headset.
```

Because it is derived from the OpenBCI GPLv3 work, the modified part is distributed under the corresponding GPLv3 terms.

---

# 7. IM-BCI8 electronics enclosure

The electronics enclosure was designed specifically for the **IM-BCI8 Rev.A PCB**.

It consists of two original parts:

```text
mechanical/enclosure/
├── IM-BCI8_Enclosure_Body.stl
└── IM-BCI8_Enclosure_Cover.stl
```

The enclosure mounts to the Ultracortex headset in the same general location used for the original OpenBCI electronics enclosure, while the internal geometry is specifically designed around the IM-BCI8 board.

---

# 8. PCB retention system

The PCB is retained without conventional PCB screws.

Instead, internal enclosure features lightly constrain the PCB when the cover is installed.

These features contact selected non-critical areas of the board and keep it mechanically constrained once the enclosure is closed.

This approach was selected to:

- minimize additional hardware;
- keep the enclosure compact;
- simplify assembly;
- allow the board to remain removable.

The retention features must not apply significant pressure to sensitive components.

In particular, enclosure geometry should avoid direct loading on:

- ADS1299;
- ESP32-S3 module;
- TPS61099;
- USB-C connector;
- small SMD components;
- solder joints.

A future PCB revision may require a corresponding enclosure revision.

---

# 9. Current enclosure limitation

The Rev.A enclosure is functional but remains a prototype.

During development, one of the cover clips fractured.

The damaged clip was successfully repaired and the enclosure remains functional, but this demonstrates that the current clip geometry is not yet mechanically optimized.

Potential future improvements include:

- larger clip-root radius;
- increased root thickness;
- reduced snap-fit deflection;
- more compliant geometry;
- modified print orientation.

The current files represent the enclosure used on the working prototype rather than a fully optimized production enclosure.

---

# 10. Dry EEG electrodes

The current prototype uses dry EEG electrodes purchased from OpenBCI.

The current eight-channel configuration is:

```text
CH1 → Fp1
CH2 → Fp2
CH3 → C3
CH4 → C4
CH5 → P7
CH6 → P8
CH7 → O1
CH8 → O2
```

with separate:

```text
REF
BIAS
```

contacts.

This layout corresponds to the current development configuration and firmware defaults.

It is not imposed by the PCB or mechanical headset.

Different electrode layouts may be used.

---

# 11. Electrode springs

Spring-loaded electrode holders are used to maintain contact between the dry electrodes and the scalp.

The springs used on the current IM-BCI8 prototype are generic compression springs originally purchased through AliExpress.

The historical product listing was:

`https://fr.aliexpress.com/item/1005009057742797.html`

Because marketplace listings may disappear or change, this URL should **not** be considered the primary component specification.

The relevant nominal spring characteristics are:

```text
Type:           compression spring
Material:       65Mn spring steel
Free length:    approximately 20 mm
Outer diameter: approximately 10 mm
Wire diameter:  approximately 0.6 mm
```

One spring is used for each spring-loaded electrode holder.

Equivalent springs may therefore be sourced independently as long as their dimensions and mechanical behavior remain reasonably similar.

The spring stiffness has not yet been formally characterized.

If a mechanically equivalent replacement is required, spring rate should ideally be measured or specified in addition to the dimensions above.

---

# 12. Electrode contact force

Spring force should be sufficient to maintain reliable electrical contact between the dry electrode and the scalp without causing pain.

Changing:

- spring length;
- wire diameter;
- coil diameter;
- number of active turns;
- spring material;

can significantly change electrode contact force.

A mechanically stronger spring is not automatically better.

Comfort and electrical stability should both be considered.

---

# 13. Electrode wiring

Connecting flexible wires to the dry electrodes requires a mechanically secure and electrically stable connection.

The current IM-BCI8 prototype uses a very simple improvised solution based on hardware that was available during assembly.

A stripped flexible conductor is wrapped around a small metal nail / pin.

The pin is then retained inside a short hollow 3D-printed cylinder sized specifically to the available nail, allowing the metal pin to press against the electrode through its mounting hole.

Conceptually:

```text
flexible wire
     │
     ▼
stripped conductor
     │
     ▼
wrapped around metal pin
     │
     ▼
pin mechanically retained
     │
     ▼
electrical contact with electrode
```

This connection has worked reliably on the current prototype and has the advantage of requiring no soldering directly to the electrode.

However, the printed retaining cylinder is **not considered a standardized or particularly reproducible IM-BCI8 component**.

Its geometry depends directly on the diameter of the locally available nail / pin.

It is therefore not included as a project STL.

---

# 14. Recommended electrode connection approach

Builders should use whatever mechanically secure connection method is compatible with their available electrodes and hardware.

A more reproducible solution than the current prototype would likely use:

- a small screw;
- a matching nut;
- a crimp terminal, ring terminal or similar cable termination.

For example:

```text
wire
 │
 ▼
crimped terminal
 │
 ▼
small screw
 │
 ▼
electrode mounting hole
 │
 ▼
nut
```

This would provide:

- controlled mechanical clamping;
- easy disassembly;
- replaceable wiring;
- no soldering directly to the electrode;
- improved reproducibility;
- less dependence on improvised hardware dimensions.

The exact solution is deliberately left open because electrode attachment hardware may vary between builds.

The important requirement is a **low-resistance, mechanically stable and serviceable electrical connection**.

---

# 15. Electrical verification of electrode connections

Every electrode connection should be checked using a multimeter after assembly.

The resistance should be low and stable.

The measurement should also be repeated while gently moving:

- the cable;
- electrode holder;
- contact hardware;
- surrounding mechanical components.

This helps identify intermittent connections that may otherwise appear as EEG artifacts.

Possible causes of unstable resistance include:

- poor mechanical contact;
- insufficient clamping pressure;
- oxidized metal parts;
- broken conductors;
- poorly stripped wire;
- movement of the connection hardware.

Every channel should be checked before human EEG acquisition.

The connection method should not be considered acceptable solely because electrical continuity is present when stationary.

---

# 16. Cable routing

Electrode wiring is routed along the Ultracortex frame.

Existing OpenBCI-style wire clips are used where practical.

Flexible lightweight wire is preferred so that cable stiffness does not significantly affect:

- headset position;
- electrode pressure;
- electrode-holder motion.

Strain relief should be considered near:

- electrode connections;
- REF and BIAS contacts;
- PCB connector;
- enclosure cable entry.

Electrode wires should not support the mechanical weight of the electronics enclosure.

---

# 17. REF and BIAS ear clips

The current prototype uses custom compliant ear clips rather than the standard OpenBCI ear-clip assembly.

The mechanical part is provided as:

```text
mechanical/earclip/
└── IM-BCI8_Compliant_Earclip.stl
```

The clip is an original IM-BCI8 mechanical design.

Its compliant structure provides the clamping force without requiring a conventional hinge and separate metal spring.

---

# 18. Ear-clip conductive surface

A small conductive plate is attached to the ear-contact region of the compliant clip.

Materials used experimentally include:

- stainless steel;
- silver-coated conductive material.

The conductive plate is electrically connected to either:

```text
REF
```

or:

```text
BIAS
```

depending on the clip.

The mechanical clip itself does not need to be conductive.

Only the contact plate and its electrical connection participate in the EEG circuit.

---

# 19. Ear-clip alternatives

The custom compliant ear clip is not required for IM-BCI8 operation.

It may be replaced with another electrically suitable solution, including a commercial or official OpenBCI ear-clip electrode.

The important requirements are:

- reliable electrical contact;
- stable mechanical positioning;
- comfortable contact pressure;
- correct REF / BIAS wiring.

The custom clip exists primarily as a low-cost printable solution for the current prototype.

---

# 20. Ear-clip comfort

Clamping force should be sufficient to maintain stable contact without causing pain.

If the custom clip is modified, its:

- thickness;
- compliant-arm length;
- material;
- print orientation;
- contact spacing;

can all influence the resulting force.

If the ear clip causes pain, excessive pressure or persistent irritation, it should be removed and adjusted before continued use.

---

# 21. Reproducing the Rev.A mechanical configuration

A complete mechanical reproduction uses three categories of parts.

## Archived OpenBCI parts

Located under:

```text
mechanical/upstream/openbci-ultracortex-mark-iv/
```

These include the exact Ultracortex Mark IV components used for:

```text
frame
inserts
wire clips
Eholder
Octabolt
```

These files retain their OpenBCI attribution and GPLv3 licensing.

---

## OpenBCI-derived IM-BCI8 part

```text
mechanical/derived/
└── IM-BCI8_Comfort_Tip.stl
```

This part is derived from the OpenBCI Eholder geometry and remains clearly identified as a modified GPLv3 work.

---

## Original IM-BCI8 parts

```text
mechanical/enclosure/
├── IM-BCI8_Enclosure_Body.stl
└── IM-BCI8_Enclosure_Cover.stl

mechanical/earclip/
└── IM-BCI8_Compliant_Earclip.stl
```

These components were designed specifically for the IM-BCI8 project.

The electrode wire attachment used on the original prototype is documented in this README but is intentionally **not distributed as a dedicated mechanical file**, since it was an improvised solution based on locally available hardware.

---

# 22. Upstream archive policy

The OpenBCI files stored in this repository are preserved for reproducibility.

They should not be interpreted as original IM-BCI8 work.

For each archived upstream set, the repository should preserve:

```text
original filenames
upstream project name
upstream repository
upstream revision / commit
original license
```

This ensures that a future user can determine exactly where each component originated even if the upstream repository structure changes.

The current archived reference revision is:

```text
OpenBCI/Ultracortex
commit 338b52b4aba4182ae77f697ff0dc544d95cc6ab2
```

---

# 23. Mechanical revision status

The current mechanical files correspond to the working IM-BCI8 Rev.A prototype.

They should still be considered experimental.

Known areas for future improvement include:

- stronger enclosure cover clips;
- improved enclosure retention features;
- more standardized electrode wiring;
- more reproducible REF / BIAS contacts;
- characterization of electrode spring force;
- optimization of print orientation;
- long-term wear testing of printed Eholder / Octabolt components.

Future mechanical changes should receive a clear revision history rather than silently replacing the geometry used for Rev.A.

---

# 24. Safety

Mechanical reproduction does not change the electrical safety requirements of IM-BCI8.

Before connecting the headset to a person, read:

[`../SAFETY.md`](../SAFETY.md)

For human EEG acquisition with Rev.A:

```text
battery power
+
wireless communication
```

must be used.

USB, chargers, grounded laboratory instruments and other wired external equipment must be disconnected while electrodes are attached to a person.

Electrodes and ear contacts should also be inspected for:

- sharp edges;
- exposed damaged wire;
- excessive mechanical pressure;
- unstable electrical connections.

---

# 25. Attribution

The Ultracortex Mark IV and associated mechanical components are developed by **OpenBCI and contributors**.

IM-BCI8 uses and archives selected OpenBCI files under their original GPLv3 terms.

The IM-BCI8 comfort tip is a modified OpenBCI-derived component.

The electronics enclosure, enclosure cover and compliant ear clip were designed independently for IM-BCI8.

IM-BCI8 is an independent project and is not affiliated with or endorsed by OpenBCI.
