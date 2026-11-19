# OpenBCI Ultracortex Mark IV — Upstream Information

This directory contains selected mechanical files originating from the **OpenBCI Ultracortex Mark IV** project.

These files are archived locally as part of the IM-BCI8 repository in order to preserve the exact mechanical configuration used for the **IM-BCI8 Rev.A prototype**.

They are not original IM-BCI8 designs.

---

## Upstream project

**Project:** OpenBCI Ultracortex  
**Variant used:** Ultracortex Mark IV  
**Organization:** OpenBCI

Official documentation:

`https://docs.openbci.com/AddOns/Headwear/MarkIV/`

Upstream repository:

`https://github.com/OpenBCI/Ultracortex`

---

## Archived upstream revision

The files archived here correspond to the OpenBCI Ultracortex repository state referenced during preparation of the IM-BCI8 Rev.A documentation.

```text
Repository:
OpenBCI/Ultracortex

Branch:
master

Reference commit:
338b52b4aba4182ae77f697ff0dc544d95cc6ab2
```

This commit identifier is recorded so that the exact upstream state used by IM-BCI8 remains traceable even if the upstream repository changes in the future.

---

## Files used by IM-BCI8

The IM-BCI8 Rev.A mechanical build uses selected OpenBCI Ultracortex Mark IV components including:

- headset frame components;
- frame inserts;
- wire clips;
- electrode-holder geometry;
- Octabolt geometry.

In particular, the following upstream files are used directly:

```text
Mark_IV/M4_Hardware_09_Eholder.STL
Mark_IV/M4_Hardware_09_Octabolt.STL
```

Additional frame, insert and clip files stored in this directory should retain their original OpenBCI filenames whenever practical.

---

## Local archive policy

Only OpenBCI files actually used to reproduce the IM-BCI8 Rev.A headset are intended to be stored here.

The purpose of this local archive is reproducibility.

It protects the Rev.A build instructions against situations such as:

- upstream file renaming;
- repository restructuring;
- removal of legacy files;
- documentation changes;
- external links becoming unavailable.

The upstream OpenBCI repository remains the authoritative source for the original designs.

---

## Modification status

Unless explicitly stated otherwise, files in this directory are intended to be **unmodified copies** of the corresponding upstream OpenBCI files.

Original filenames should be preserved where practical.

Modified OpenBCI geometry should **not** be stored in this directory.

Instead, modified parts used by IM-BCI8 are stored separately under:

```text
mechanical/derived/
```

This separation is intentional so that users can easily distinguish:

```text
upstream/
    original OpenBCI files

derived/
    OpenBCI-based files modified for IM-BCI8

enclosure/
earclip/
    original IM-BCI8 designs
```

---

## IM-BCI8 derived part

The current IM-BCI8 build includes a custom comfort tip derived from the OpenBCI:

```text
M4_Hardware_09_Eholder.STL
```

The modified part is stored separately as:

```text
mechanical/derived/IM-BCI8_Comfort_Tip.stl
```

It should not be interpreted as an original independent mechanical design.

Its relationship to the OpenBCI source is documented explicitly in the IM-BCI8 mechanical documentation.

---

## FDM manufacturing note

The IM-BCI8 prototype uses several OpenBCI Mark IV components in a way that differs from their original production approach.

In particular:

```text
M4_Hardware_09_Eholder.STL
M4_Hardware_09_Octabolt.STL
```

were manufactured directly using FDM printing in PETG for the IM-BCI8 prototype.

This approach has worked satisfactorily on the current headset.

However, it should be considered an IM-BCI8 manufacturing adaptation rather than an official OpenBCI manufacturing recommendation.

---

## License

The upstream `OpenBCI/Ultracortex` repository states that its material is distributed under the:

**GNU General Public License version 3 — GPLv3**

The original upstream license file is:

```text
OpenBCI/Ultracortex/LICENSE.md
```

A verbatim copy of that license should be stored in this directory as:

```text
LICENSE.md
```

The license applies to the archived OpenBCI material according to the terms defined by the upstream project.

Modified OpenBCI-derived works distributed by IM-BCI8 must retain the applicable licensing and attribution requirements.

---

## Attribution

The Ultracortex Mark IV and associated mechanical files were developed by **OpenBCI and contributors**.

Their inclusion in the IM-BCI8 repository is solely for attribution, preservation and reproducibility of the mechanical configuration used by IM-BCI8 Rev.A.

IM-BCI8 does not claim authorship of these files.

IM-BCI8 is an independent project and is not affiliated with or endorsed by OpenBCI.
