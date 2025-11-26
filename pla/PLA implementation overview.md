
---

# **PLA Implementation (Swing Door Algorithm) – Lossy Compression Module**

This document explains the real implementation of the **Piecewise Linear Approximation (PLA)** lossy compression algorithm using the **Swing Door Trending (SDT)** method.
Additionally, it also summarizes all modifications and additions made to the *WSN-data-compression* project in order to integrate PLA as a selectable compression algorithm.

---

# **1. Overview of PLA Algorithm Implementation**

This implementation replaces the older "fixed 8-point linear regression" method with a **true error-bounded PLA** that dynamically determines segment boundaries based on a configurable threshold (`MAX_ERROR_THRESHOLD`).

| Feature                        | Sprintz Implementation | PLA Implementation           |
| ------------------------------ | ----------------------- | ----------------------------- |
| Algorithm                      | Fixed-block linear fit  | **Swing Door PLA**            |
| Segment length                 | Always 8 samples        | **Dynamic (adaptive)**        |
| Error threshold                | None                    | **Yes — fully enforced**      |
| Reconstruction error guarantee | No                      | **<= MAX_ERROR_THRESHOLD**  |
| Compression                    | Fixed                   | Adjusts with data variability |

---

# **2. Summary of All Modifications to the WSN Project：**

Below is a complete list of what was added or modified in the **WSN-data-compression** repository to support the new PLA (Swing Door) lossy compression algorithm.

---

## **2.1 New `pla/` directory added (new algorithm module)**

A new directory containing the full PLA implementation:

```
pla/
 ├── pla_encoder.c        # Swing Door encoder
 ├── pla_encoder.h
 ├── pla_decoder.c        # Segment-based decoder
 └── pla_decoder.h
```

This module performs:

* Online Swing Door segmentation
* Error-bounded segment creation
* Slope calculation
* Reconstruction via linear interpolation

This is the core functionality required for a complete lossy compression algorithm.

---

## **2.2 Modified `compression/` module to support algorithm selection**

The following files were updated:

```
compression/encoder.c
compression/encoder.h
compression/decoder.c
compression/decoder.h
```

Changes include:

* Adding **PLA** as a selectable algorithm via `ALGO=pla`
* Redirecting encode/decode calls to `pla_encode()` and `pla_decode()`
* Allowing coexistence of both algorithms:

  * **Sprintz** (lossless)
  * **PLA** (lossy)

This makes the compression pipeline algorithm-agnostic and extensible.

---

## **2.3 Makefile updated to include `ALGO=pla`**

The Makefile now supports:

```
make TARGET=sky ALGO=pla producer.upload
make TARGET=sky ALGO=pla CLASS=sink sink.upload
```

Changes include:

* Adding PLA compiler flags
* Including `pla/*.c` during compilation
* Allowing dynamic selection of compression algorithm

The project can now switch between algorithms without code changes.

---

## **2.4 `project-conf.h` updated with PLA configuration**

A new parameter was added for PLA algorithm:

```c
#define MAX_ERROR_THRESHOLD 10
```

This controls:

* Allowed deviation from the approximating line
* Segment length
* Compression ratio
* Reconstruction accuracy

This addition is essential for PLA.



---

#  **3. How PLA Works**

Swing Door maintains an upper door and lower door from the anchor point.
For each new sample:

1. If the sample lies **within** the door → keep extending the segment
2. If the sample lies **outside** the door →

   * close the current segment
   * start a new segment

This ensures **error-bounded lossy compression**.


In code, it is defined in `project-conf.h`:

```c
#define MAX_ERROR_THRESHOLD 10
```

---

# **4. Packet Format (PLA – Swing Door Algorithm)**

Same as the Sprintz algorithm implementation, the PLA encoder transmits segment representation to the sink. The packet consists of a **1-byte header** followed by a **variable-length payload** containing PLA segments.


```
Packet-level header:
Byte 0  → number_of_segments
```

```
Payload (5 bytes per segment):
Byte 0 (segment offset 0) → end_index
Byte 1–2                  → start_value
Byte 3–4                  → slope_q
```

### Notes:

* **start_index** is not transmitted because it is implicitly:

```
start_index = last_end_index + 1
```

* **slope_q** is the quantized slope used for reconstruction.

* The sink reconstructs samples using:

```
value[t] = start_value + (slope_q * (t - start_index)) / SLOPE_SCALE
```

This format ensures compact transmission (5 bytes per segment) and efficient reconstruction on TelosB motes.

---


---

# **5. Added Build Commands**

```bash
# PLA (lossy) - Producer
make TARGET=sky ALGO=pla producer.upload

# PLA (lossy) - Sink
make TARGET=sky ALGO=pla CLASS=sink sink.upload

# Sprintz (lossless)
make TARGET=sky ALGO=sprintz producer.upload
```


---

# **Added File Structure**

```
lossy_algorithm_implementation/
├── pla/                       # New PLA module
├── compression/               # Modified interface
├── project-conf.h             # Added MAX_ERROR_THRESHOLD
├── Makefile                   # Added ALGO=pla support
└── README.md                  # This document
```

---