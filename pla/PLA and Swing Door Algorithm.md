
Update on the lossy compression part of our mini-project:



---
## **1. Literature Research**

I performed a systematic Google Scholar search using the following keywords:

* **lossy compression algorithm sensor data**
* **"piecewise linear approximation" sensor network compression**
* **"Swing Door" compression time series**
* **"Swinging Door" compression Wireless Sensor Network**

I have read and reviewed the following papers:

1. [https://ieeexplore.ieee.org/document/5735832](https://ieeexplore.ieee.org/document/5735832)
2. [https://ieeexplore.ieee.org/document/8360262](https://ieeexplore.ieee.org/document/8360262)
3. [https://www.mdpi.com/1424-8220/18/6/1672](https://www.mdpi.com/1424-8220/18/6/1672)
4. [https://www.mdpi.com/1996-1073/12/13/2523](https://www.mdpi.com/1996-1073/12/13/2523)
5. [https://www.semanticscholar.org/paper/Swinging-Door-Trending-Compression-Algorithm-for-Correa-Pinto/8c48e040dcc394e847ecde929c28f6054e5d8b41](https://www.semanticscholar.org/paper/Swinging-Door-Trending-Compression-Algorithm-for-Correa-Pinto/8c48e040dcc394e847ecde929c28f6054e5d8b41)
6. [https://scispace.com/pdf/performance-evaluation-of-a-compression-algorithm-for-4xas5mn377.pdf](https://scispace.com/pdf/performance-evaluation-of-a-compression-algorithm-for-4xas5mn377.pdf)

These papers show that **Piecewise Linear Approximation (PLA)** and its practical implementation **Swing Door Algorithm** are widely used lossy compression techniques in Wireless Sensor Networks due to their low computational overhead and good error-control properties.



---
## **2. Algorithm Choice**

After reviewing the literature, I decided to use:

**Piecewise Linear Approximation (PLA) with a Swing Door Algorithm implementation** as our group’s lossy compression algorithm.



---
## **3. What is PLA and what is Swing Door Algorithm?**

**PLA (Piecewise Linear Approximation)**
→ Represent a long time series using multiple straight-line segments.

**Swing Door Algorithm**
→ A specific implementation of PLA.
→ It dynamically decides when to close a segment based on an **error threshold**.

Relationship:

> **Swing Door Algorithm is one of the classical methods to implement PLA.**



---
### 4. How PLA / Swing Door Algorithm Works (Theory vs Actual Implementation)

The Piecewise Linear Approximation (PLA) method represents a time series as a
sequence of straight-line segments. The Swing Door Algorithm (SDA) is a classical,
efficient algorithm for generating these segments by dynamically adjusting segment
boundaries based on a bounded error threshold.


#### 4.1 Theoretical PLA Segment Representation

In generic PLA theory, each segment is defined by its two endpoints:

- **start_index**
- **start_value**
- **end_index**
- **end_value**

These four values define a line segment uniquely, and the intermediate points can be
reconstructed using linear interpolation:

    value[i] = start_value + slope * (i - start_index)
where

    slope = (end_value - start_value) / (end_index - start_index)

This is the standard equation of a line:  y = y₀ + m(x - x₀).


#### 4.2 Swing Door Segmentation (Concept)

The Swing Door Algorithm works as follows:

1. Start a segment at the first point.
2. For each new incoming sample, check whether it fits within the upper and lower
   error bounds (the “swing doors”).
3. If the sample remains within bounds, it is included in the current segment.
4. If it exceeds the bound, the segment is closed and a new segment begins.
5. The process continues until all samples are processed.

This produces a series of segments with bounded reconstruction error, often yielding
very high compression on smooth data such as DK1 wind-power time series.


#### 4.3 Difference Between Theory and Our Implementation

While theoretical PLA uses the two endpoints (start and end) to define each segment,
**our actual implementation on TelosB motes uses a more compact representation** to
reduce payload size and computational cost.

The exact implementation details are described in **Section 5**, where each segment is
encoded using:

- **end_index**
- **start_value**
- **slope_q** (quantized slope)

The **start_index** is inferred implicitly, and the **end_value is reconstructed** at the sink
using the quantized slope. This produces significantly smaller packets while keeping the reconstruction error controlled by `MAX_ERROR_THRESHOLD`.



---

## **5. How Swing Door Algorithm is implemented in our group project**

So, in the actual implementation on TelosB motes, we use a compact PLA
representation that does not transmit both endpoints of every segment.
Instead, each segment is encoded using:

1. end_index   (uint8_t)
2. start_value (int16_t)
3. slope_q     (int16_t)

This means each segment occupies exactly 5 bytes.

The start_index is **not transmitted**; it is implicitly computed on the sink as:

    start_index = last_end_index + 1

The end_value is **also not transmitted**. Instead, the sink reconstructs all
samples (including the final endpoint) using the quantized slope:

    value[t] = start_value + (slope_q * (t - start_index)) / SLOPE_SCALE

This slope-based representation is more efficient than sending both endpoints
(start_value and end_value), and significantly reduces payload size while
keeping reconstruction error bounded by MAX_ERROR_THRESHOLD.



---
# **6. Explanation of key points**

### **1. Is each PLA segment always 8 samples (index 0–7)?**

**No.**
The “8 integers per row” format is only for **Sprintz** (lossless compression).
PLA does **not** use fixed-size blocks.

PLA segments are **variable length**, determined entirely by the error threshold.

Example:

```
Segment 1: 0 → 237
Segment 2: 237 → 480
```

So one segment may contain **hundreds of samples**, especially for smooth DK1 data.

---

### **2. How exactly is MAX_ERROR_THRESHOLD applied in Encoding side?**

Error is calculated as:

```
error = | real_value - predicted_value |
```

If `error > threshold` → new segment starts.

This ensures the reconstructed data never deviates from the original more than the allowed threshold.

---

### **3. What does the slope formula mean in Decoding side? Why (i - start_index)?**

Because we reconstruct using the line equation:

```
y = y0 + m*(x - x0)
```

Mapping to our case:

* y  = reconstructed value
* y0 = start_value
* x0 = start_index
* x  = i
* m  = slope of the segment

Thus:

```
value[i] = start_value + slope * (i - start_index)
```

The algorithm uses straight-line interpolation to fill missing samples.




---

## **7. Why choose Swing Door Algorithm as the lossy algoritgm**

The DK1 wind generation time series is smooth, slowly varying, and without high-frequency noise, which means each Swing Door Algorithm segment can span **hundreds of samples**, resulting in:

* very high compression ratio
* low computation cost
* error fully controlled by threshold
* ideal for energy-constrained TelosB motes

---

