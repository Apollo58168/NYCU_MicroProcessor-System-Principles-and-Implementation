# HW#5 Domain-Specific Accelerator (DSA) — Detailed Notes

Source: `mpd25_HW5_DSA (1).pdf` (slides 1–31)

> This document is a slide-by-slide, highly detailed Markdown transcription and expansion of the provided PPT/PDF. It preserves all key technical details (paths, signals, code snippets, steps) and adds copy-paste-ready code blocks.


---

## Slide 1: Title

- **Homework:** HW#5 Domain-Specific Accelerator
- **Instructor:** Chun-Jen Tsai
- **Institution:** NYCU
- **Date on slide:** 12/02/2025


---

## Slide 2: Homework Goal

### Objective
Integrate a **domain-specific accelerator (DSA)** into the **Aquila SoC** to improve the inference speed of a **CNN neural network**.

### Tasks
- **Design an AI accelerator HW IP**.
- Numeric formats allowed:
  - **FP32**
  - **FP16**
  - **INT8** (accuracy must be maintained)
- If using **FP32/FP16**, use **Xilinx floating-point IP**.
- If using **INT8**, you must **keep accuracy** (proper scaling required).

### Deliverables & Deadline
- Upload **report** and **code** to **E3** by **12/22, 23:55**.


---

## Slide 3: Neural Network as Computers

### Core idea
All computing systems compute functions:

- \( f: \mathbb{R}^m \to \mathbb{R}^n \)

### Kolmogorov (1957)
Any continuous function can be decomposed into a linear combination of a finite number of \( \mathbb{R}^m \to \mathbb{R} \) functions.

- Decomposition size mentioned on slide: **(2m+1)+n** different \( \mathbb{R}^m \to \mathbb{R} \) linear functions.

### Citation on slide
- A. N. Kolmogorov, “On the representation of continuous function of many variables by superpositions of continuous functions of one variable and addition,” *Doklady Akademii Nauk USSR*, 114(5):953-956, 1957.


---

## Slide 4: Basic Neural Network Components

### Perceptron / Neuron (McCulloch & Pitts, 1943)
A perceptron computes:
- Weighted sum of inputs + bias
- Apply activation function \( f(\cdot) \)

Formula on slide:
- \( v_o = f(v_1 w_1 + v_2 w_2 + v_3 w_3 + b) \)

### Diagram annotations (as shown on slide)
- Inputs: \(v_1, v_2, v_3\)
- Weights: \(w_1, w_2, w_3\)
- Bias: \(b\)
- Output: \(v_o\)
- \(f()\) is the activation function (threshold logic).

### Citation on slide
- W. McCulloch and W. Pitts, "A Logical Calculus of Ideas Immanent in Nervous Activity". *Bulletin of Mathematical Biophysics*. 5 (4), 1943, pp. 115–133.


---

## Slide 5: A Universal Neural Network (MLP)

### Kolmogorov-inspired 3-layer network
A continuous function \( f: \mathbb{R}^m \to \mathbb{R}^n \) can be computed using **3 layers of perceptrons**.

### Program = weights
The “program” of a neural network is in the parameters:
- \(\{ w_{ji}^k, b_i^k \}\)

### Architecture name
- **Multi-Level Perceptrons (MLP)**

### Diagram specifics (as shown)
- Input layer: \(x_1, x_2, \dots, x_m\)
- Hidden layer has \(h = (2m+1)\) neurons (highlighted on slide)
- Output layer: \(y_1, \dots, y_n\)


---

## Slide 6: Different Neural Network Layer Types

### 1) Convolutional Layer
- Computes **3D dot product** between **input data cube** and **kernel**.
- Output cube neurons in the **same channel** share the **same kernel**.
- Shape notes on slide:
  - Input cube: \(w_i \times h_i \times c_i\)
  - Output cube: \(w_o \times h_o \times c_o\)
  - Kernel: \(k_w \times k_h \times c_i\)

### 2) Avg-Pooling Layer
- Reduces width/height by averaging over small \(m \times n\) rectangles within each channel.

### 3) Fully-Connected Layer
- Same style as MLP.


---

## Slide 7: Convolutional Neural Network (CNN) model used in this HW

### Model overview (6-layer CNN for MNIST-like digits)
The slide provides a concrete pipeline with tensor sizes:

1. **Input image:** 28×28×1 pixels
2. **Layer 1 (Conv1):**
   - Kernel: 5×5
   - Output: **24×24×3 neurons**
3. **Layer 2 (Pool2):**
   - Output: **12×12×3 neurons**
4. **Layer 3 (Conv3):**
   - Output: **8×8×12 neurons**
5. **Layer 4 (Pool4):**
   - Output: **4×4×12 neurons**
6. **Layer 5 (FC5):**
   - Output: **30×1×1 neurons**
7. **Layer 6 (FC6):**
   - Output: **10×1×1 neurons** (digits 0..9)

The diagram also labels the final outputs explicitly as 0..9.


---

## Slide 8: Hand-Written Character Recognition

### Task
Use the 6-layer CNN in this homework for **hand-written character recognition**.

### Training data referenced
- MNIST dataset: **60,000 images**
- Each image: **28×28 pixels**


---

## Slide 9: The HW/SW for HW#5 — Source tree

### Location of C code
- The C code for HW#5 is in directory **`cnn_ocr`**.

### Source tree shown on slide
```text
cnn_ocr2/
├── Makefile
├── data/
│   ├── test-images.dat
│   ├── test-labels.dat
│   └── weights.dat
│
├── inc_cnn/*.h
│
├── file_read.c
├── file_read.h
├── cnn_ocr.c
└── cnn_ocr.ld
```
- **Data files** (to be copied to SD card): `test-images.dat`, `test-labels.dat`, `weights.dat`
- **OCR application source**: `cnn_ocr.c`
- **CNN model headers**: `inc_cnn/*.h`
- **Low-level SD I/O routines** are in: `elibc/fileio/`


---

## Slide 10: Building an FPU-enabled Aquila

### Key point
To build `cnn_ocr` **with** or **without** FPU, you must change the **top two lines** of the Makefile.

### With FPU (double-precision ABI)
Copy-paste-ready Makefile fragment:
```make
RISCV_ABI = -march=rv32imad_zicsr_zifencei -mabi=ilp32d
GCC_LIB = rv32imad/ilp32d
CROSS = riscv32-unknown-elf
CCPATH = $(RISCV)/bin
```

### Without FPU (integer ABI)
Copy-paste-ready Makefile fragment:
```make
RISCV_ABI = -march=rv32ima_zicsr_zifencei -mabi=ilp32
GCC_LIB = rv32im/ilp32
CROSS = riscv32-unknown-elf
CCPATH = $(RISCV)/bin
```

> Note on slide: **don't type `rv32ima` here!**  
(Keep the exact string as shown above.)


---

## Slide 11: Important Notice

### Accelerator integration requirement
When you integrate your AI Accelerator with Aquila:
- You **shall use** the Aquila configuration **without FPU**.

### Performance comparison
However, performance will be compared against:
- Aquila **with FPU**

### INT8 note
- To beat Aquila+FPU, using **INT8** is likely easier, but you must do **proper scaling** to avoid accuracy drop.

### Bonus
- Extra points if you use **FP IP** successfully and still get good performance.


---

## Slide 12: The HW Workspace for HW#5

### Resource note
- Aquila with FPU uses **a lot of FPGA resources**.
- The slide screenshot highlights that the design includes many **IPs inserted from the Vivado IP Catalog**.


---

## Slide 13: Example: AXI Quad SPI Controller

### SD Card connectivity in Vivado
- The SD card interface uses a **Xilinx AXI Quad SPI Controller** (SPI-based SD card access).

### What the slide emphasizes
- Double-click **`SD_Card` IP** in Vivado to open parameters.
- The diagram labels:
  - **Connect to Aquila** (AXI side)
  - **Connects to SD card** (SPI IO side)


---

## Slide 14: Instantiation of the IP in Aquila (SPI controller)

### Verilog instantiation example (copy-paste)
The slide shows how the SPI controller is instantiated and wired.

```verilog
// ----------------------------------
// SPI controller
// ----------------------------------
// This controller connects to the PMOD microSD module in
// the JD connector of the Arty A7-100T.
//
axi_quad_spi_0 SD_Card_Controller(
  // Interface ports to the Aquila SoC.
  .s_axi_aclk(clk),
  .s_axi_aresetn(~rst),
  .s_axi_awaddr(axi_awaddr),
  .s_axi_awvalid(axi_awvalid), // master signals write addr/ctrl valid.
  .s_axi_awready(axi_awready), // slave ready to fetch write address.
  .s_axi_wdata(axi_wdata),     // write data to the slave.
  .s_axi_wstrb(axi_wstrb),     // byte select signal for write operation.
  .s_axi_wvalid(axi_wvalid),   // master signals write data is valid.
  .s_axi_wready(axi_wready),   // slave ready to accept the write data.
  .s_axi_araddr(axi_araddr),
  .s_axi_arready(axi_arready), // slave ready to fetch read address.
  .s_axi_arvalid(axi_arvalid), // master signals read addr/ctrl valid.
  .s_axi_bready(axi_bready),   // master is ready to accept the response.
  .s_axi_bresp(axi_bresp),     // response code from the slave.
  .s_axi_bvalid(axi_bvalid),   // slave has sent the respond signal.
  .s_axi_rdata(axi_rdata),     // read data from the slave.
  .s_axi_rready(axi_rready),   // master is ready to accept the read data.
  .s_axi_rresp(axi_rresp),     // slave sent read response.
  .s_axi_rvalid(axi_rvalid),   // slave signals read data ready.

  // Interface ports to the SD Card.
  .ext_spi_clk(clk),
  .io0_o(spi_mosi),
  .io1_i(spi_miso),
  .sck_o(spi_sck),
  .ss_o(spi_ss)
);
```


---

## Slide 15: The Aquila Device Interface

### Aquila MMIO-style device interface
Aquila core talks to external devices using a **simple memory-mapped I/O** interface.

Ports shown on slide:
```verilog
aquila_top Aquila_SoC
(
  .clk_i(clk), .rst_i(rst), .base_addr_i(32'b0),

  // External instruction memory ports.
  .M_IMEM_strobe_o(IMEM_strobe),
  . . .
  .M_IMEM_data_i(IMEM_data),

  // External data memory ports.
  .M_DMEM_strobe_o(DMEM_strobe),
  . . .
  .M_DMEM_data_i(DMEM_rd_data),

  // I/O device ports.
  .M_DEVICE_strobe_o(dev_strobe),       // Issue read/write requests.
  .M_DEVICE_addr_o(dev_addr),           // Target device address.
  .M_DEVICE_rw_o(dev_we),               // Read or write?
  .M_DEVICE_byte_enable_o(dev_be),      // Byte-select signal.
  .M_DEVICE_data_o(dev_din),            // Data input to the device.
  .M_DEVICE_data_ready_i(dev_ready),    // Is device ready?
  .M_DEVICE_data_i(dev_dout)            // Data output from the device.
);
```

### Important implication
- Many Xilinx IPs are AXI-based.
- Therefore, a **bridge** is required to translate Aquila device interface signals into AXI signals.


---

## Slide 16: Bridging the IP Interface

### AXI interfaces in Xilinx IP Catalog
- **AXI Full**: supports burst + single-beat transfers
- **AXI Lite**: supports single-beat transfers
- **AXI Stream**: supports burst-only transfers

### Why you need a bridge
Aquila's native device interface is not AXI.  
To integrate Xilinx IPs, you must convert Aquila signals → AXI.

### Provided bridge module
- `core2axi_if.v` is used for Aquila to connect to **any IP that supports AXI Lite**.


---

## Slide 17: Running the CNN_OCR Program

### End-to-end run procedure (as listed on slide)
1. **Copy the data files to the SD card**  
   - Ensure the SD card is formatted as **FAT32**
2. Insert the SD card into the **SD card pmod module**
3. Insert the pmod module into **socket JD** on **Arty**
4. Build `cnn_ocr.elf` by typing `make`
5. Synthesize the Aquila SoC and configure the FPGA
6. Load and run `cnn_ocr.elf`

### Copy-paste checklist commands (host side)
Assuming you are in the `cnn_ocr` directory and your SD card is mounted at `/media/$USER/SDCARD`:

```bash
# 1) Verify the three data files exist
ls -lh data/test-images.dat data/test-labels.dat data/weights.dat

# 2) Copy to SD root (recommended)
cp -v data/test-images.dat data/test-labels.dat data/weights.dat /media/$USER/SDCARD/

# 3) Flush writes before unplugging the SD card
sync
```

### Build command (host side)
```bash
make
```


---

## Slide 18: Output of the CNN_OCR Program

### Example UART console output (as shown)
```text
=======================================================================
Copyright (c) 2019-2025, EISL@NYCU, Hsinchu, Taiwan.
The Aquila SoC is ready.
Waiting for an ELF file to be sent from the UART ...
Program entry point at 0x80002958, size = 0xB90C.
-----------------------------------------------------------------------
(1) Reading the test images, labels, and neural weights.
It took 981 msec to read files from the SD card.
(2) Perform the hand-written digits recognition test.
Here, we use a 6-layer CNN neural network model.
Begin computing ... tested 100 images. The accuracy is 98.00%
It took 7915 msec to perform the test.
-----------------------------------------------------------------------
Program exit with a status code 0
Press <reset> on the FPGA board to reboot the cpu ...
```

### Interpretation
- Phase (1): SD card file I/O time
- Phase (2): CNN compute time (your accelerator aims to reduce this)
- The example accuracy shown: **98.00%** over **100 images**


---

## Slide 19: Key Hotspot of the Program

### Acceleration candidates (hotspots)
Two functions are highlighted as good candidates for hardware acceleration:

1. `conv_3d()` in `convolution_layer.h`
2. `fully_connected_layer_forward_propagation()` in `fully_connected_layer.h`

### HW design trade-offs (as described)
- Fully-connected function:
  - Computes **1D dot-product**
  - Easier to accelerate
  - But gives **less overall speedup**
- Convolution `conv_3d()`:
  - Computes **3D dot-product**
  - Most time-consuming hotspot
  - Harder to implement in hardware
- Both can use a **1D dot-product IP** from Xilinx as a building block.


---

## Slide 20: Floating-Point IP in Xilinx IP Catalog

### What to do in Vivado
- Open the **IP Catalog** in Vivado.
- Locate **Floating-Point** IP.
- The slide indicates you want a floating-point IP to compute **inner-product**.

### Screenshot highlights
- "Click this to show the IP catalog!"
- Emphasizes "bus interface of the IP"


---

## Slide 21: How to Add an IP into Aquila SoC

### Adding an IP in Vivado GUI (as shown)
- **Double-click** the target IP in the IP Catalog to add it into the project workspace.
- **Right-click** the IP entry to show the IP menu and open its **datasheet / product guide**.

This slide is about the workflow of inserting Xilinx IP blocks into your Aquila Vivado project.


---

## Slide 22: Configure the IP (Floating-Point)

### Configuration goal
Configure the floating-point IP to do **Fused Multiply-Add (FMA)**, which is useful for dot-product.

### Defaults stated on slide
- Mode: **Blocking mode**
- Precision: **32-bit precision**
- Latency: **17 cycles**

### Notes on latency
- The slide notes latency can be reduced (example mentions **to 2** without timing error on Arty),
  but also states latency is not the main issue for this HW:
  - "The bottleneck is somewhere else."


---

## Slide 23: Inserting the IP from the TCL Script

### Submission requirement / recommendation
For homework submission, you should modify the **TCL script** to insert the floating-point IP into the workspace.

### Copy-paste-ready TCL snippet (as shown)
```tcl
# Adding an Floating-Point IP
create_ip -name floating_point -vendor xilinx.com -library ip -module_name floating_point_0

set_property -dict [list   CONFIG.A_Precision_Type {Single}   CONFIG.OPERATION_TYPE {FMA}   CONFIG.ADD_SUB_VALUE {Add}   CONFIG.Flow_Control {NonBlocking}] [get_ips floating_point_0]

generate_target all [get_files   ${proj_name}/${proj_name}.srcs/sources_1/ip/floating_point_0/floating_point_0.xci]

report_property [get_ips floating_point_0]
```

### How to list options (as stated)
In Vivado TCL console:
```tcl
report_property [get_ips floating_point_0]
```


---

## Slide 24: AXI Floating-Point IP Interface

### Stream-based operation
The floating-point IP is designed to handle a **sequence of floating-point operations**.

### Bus interface
Uses **AXI Stream** for I/O ports:
- Data stream A
- Data stream B
- Data stream C
- Result stream

### Handshake signals (important)
- `*_tvalid` indicates data valid
- `*_tready` is required when the IP is configured in **default blocking modes**

### Optional signals
- `TLAST` and `TUSER` are optional.
- `TLAST` can reset the accumulator when IP is configured as **FMA** (nice but not necessary).


---

## Slide 25: Blocking vs Non-Blocking

### Two modes
- **Blocking mode**
  - Requires controlling extra `tready` feedbacks
- **Non-blocking mode**
  - No `tready` feedbacks
  - Assumes data will be fetched at the **next rising edge** after `tvalid` is high

### Additional note (explicit on slide)
The two inputs A and B do **not** have to enter the IP at the same clock cycle in either mode.


---

## Slide 26: Blocking FMA IP Usage Example (Dot-product)

### Dot-product using FMA
Set the floating-point IP to **Fused Multiply-Add (FMA)**:

- The recurrence is: **R = R + (A * B)** (accumulation)

### Example scenario on slide
- Vector sizes: **1×4**
- Pipeline delay example: **3 cycles** (note: default delay is 17 cycles)

### Dataflow concept
- Feed A stream: a1..a4 (or reverse order depending on diagram)
- Feed B stream: b1..b4
- Feed C stream with feedback from previous R (accumulator)
- Final dot-product output is **R4**

### Key mechanism
Use a FIFO / feedback path:
- R1, R2, R3, R4, ... are produced after pipeline delay
- Feed R back as input **C** for accumulation


---

## Slide 27: Interface between Aquila and HW FP

### Recommended system architecture
Design a **data feeder module** between Aquila and the AXI floating-point IP.

High-level block diagram described on slide:
- Aquila ↔ (mP MMIO interface) ↔ Data Feeder ↔ (AXI4-Stream) ↔ AXI FP IP

### Aquila side (software interaction)
- Use `memcpy()` to copy vectors into an **input buffer**
- Read the final result from an **output buffer**

### AXI FP side (hardware streaming)
- Use **AXI4-Stream** to transmit input streams and receive result stream.

### Reference example mentioned
- A good example of the **mP MMIO interface** is in `clint.v`.


---

## Slide 28: Examples of conv_3d() HW Interface

### What changes in software
The slide shows `conv_3d()` original software structure (nested loops) and suggests replacing the compute core with a **hardware trigger**.

### Original loop structure (conceptual)
The software version includes:
- Pointer/index setup code
- Multiple nested loops over input depth and spatial dimensions
- Inner loop computing sum of element-wise multiplications

### Example MMIO trigger code (copy-paste-ready)
```c
volatile int *trigger_hw = (int *) 0xC4000000;
*trigger_hw = 1;
while (*trigger_hw) /* busy waiting */;
```

### Where to look (as instructed)
- Read `soc_top.v`, **line 251 ~ 260** (for MMIO mapping / address decoding)
- Read `clint.v` to see how the **MMIO interface** can be implemented on the circuit side.


---

## Slide 29: Some Shortcuts to This HW

### Shortcut ideas (memory/throughput trade-offs)
To reduce memory overhead, you can:

1. Store the entire **weight data** in **on-chip BRAM** at program start
   - Not feasible for large CNNs, but **this CNN model is small**

2. Store feature maps of the previous layer in BRAM as well (fits for this model)

### Model constants you can exploit
- Padding = **0**
- Stride = **1**

These constants can simplify the accelerator design and the dataflow in the convolution operations.


---

## Slide 30: The Grading of This HW

### Grading factors listed
- Quality of the report
- Performance
- Whether FP IP is used
- Whether weights / feature maps are stored in BRAM (**not good** per slide note)
- Scalability of the design (to larger AI model)

### INT8 note
- Replacing computations with **INT8** likely yields best performance (if accuracy maintained).

### FP extra point expectation
- FP IP is harder; extra points if you use FP IP and achieve good performance,
  e.g., **at least 2× faster** than **Aquila + FPU**.


---

## Slide 31: Comments on the Homework

### Bottleneck reminder
One bottleneck is often **feeding data streams** to the accelerator.

### Measurement recommendation
You should measure separately:
- Time spent on **data feeding**
- Time spent on **computations**

### Key learning point
Learn how to integrate an **AXI accelerator** to speed up computations.

### Submission note
Even if there is **no demo**, you still need to upload your code to E3 so TAs can verify results.


---

# Appendix: Copy-paste command snippets

## A. Format an SD card as FAT32 (Linux)
> WARNING: Replace `/dev/sdX` with your actual device (e.g., `/dev/sdb`). This will erase the disk.

```bash
lsblk
sudo umount /dev/sdX* 2>/dev/null || true
sudo mkfs.vfat -F 32 /dev/sdX
```

## B. Mount + copy data files (Linux)
```bash
# Example mount (adjust device and mount point)
sudo mkdir -p /mnt/sd
sudo mount /dev/sdX1 /mnt/sd

# Copy the three .dat files to SD root
cp -v data/test-images.dat data/test-labels.dat data/weights.dat /mnt/sd/
sync
sudo umount /mnt/sd
```

## C. Build application ELF
```bash
make clean
make
```
