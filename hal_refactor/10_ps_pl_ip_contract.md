# Zion PS-PL IP Contract ## Scope
This document defines the backend contract between the Processing System (PS) and the Programmable Logic (PL) deterministic I/O engine for Zion HAL running on Xilinx Zynq-7000 and Zynq UltraScale+ MPSoC with PetaLinux (Vitis flow). The HAL public API remains unchanged. The backend implementation is retargeted to AXI/AXI-Lite register and DMA transactions. ## Runtime Topology
- PS userspace on PetaLinux calls HAL module APIs.
- HAL backend maps PL control and DMA windows via UIO or a kernel driver.
- AXI-Lite controls deterministic IP cores.
- AXI DMA (or equivalent PL DMA IP) moves payloads.
- Optional: MPSoC Cortex-R5 handles hard real-time loops and exchanges commands with APU software. ## Device Nodes
- Control UIO node: /dev/uio0
- DMA UIO node: /dev/uio1 Kernel-driver mode may replace UIO nodes with a character device while preserving this register contract. ## AXI-Lite Register Contract
All offsets are 32-bit aligned and little-endian. | Offset | Name | Description |
|---|---|---|
| 0x0000 | ID | Must equal 0x5A494F4E ("ZION") |
| 0x0004 | VERSION | IP contract version |
| 0x0008 | CONTROL | ENABLE, RESET, START bits |
| 0x000C | STATUS | READY, BUSY, FAULT, DMA_DONE bits |
| 0x0010 | IRQ_ACK | Interrupt acknowledge |
| 0x0014 | FAULT_CODE | Latched fault code |
| 0x0020 | DMA_SRC_LO | DMA source address low |
| 0x0024 | DMA_DST_LO | DMA destination address low |
| 0x0028 | DMA_LEN_BYTES | DMA transfer length |
| 0x002C | DMA_CONTROL | DMA START/IRQ enable |
| 0x0030 | DMA_STATUS | DMA BUSY/DONE/ERROR |
| 0x0040 | HANDSHAKE | CMD_VALID, ACK_VALID, TIMEOUT |
| 0x0044 | HEARTBEAT | Free-running heartbeat counter | ## Control and Status Bits
- CONTROL.ENABLE (bit 0): enable deterministic I/O datapath.
- CONTROL.RESET (bit 1): synchronous reset of deterministic core.
- CONTROL.START (bit 2): start command for one deterministic cycle. - STATUS.READY (bit 0): core idle and ready.
- STATUS.BUSY (bit 1): core processing command.
- STATUS.FAULT (bit 2): core in fault state.
- STATUS.DMA_DONE (bit 3): last DMA completed. ## DMA Contract
1. PS writes DMA source, destination, and length.
2. PS sets DMA_CONTROL.START.
3. PL sets DMA_STATUS.BUSY while transfer is active.
4. PL sets DMA_STATUS.DONE on success or DMA_STATUS.ERROR on failure.
5. PL updates STATUS.DMA_DONE.
6. PS acknowledges interrupt via IRQ_ACK when IRQ mode is enabled. Zero-length DMA is invalid and must set DMA_STATUS.ERROR. ## Handshake Contract
- PS writes command and sets HANDSHAKE.CMD_VALID.
- PL processes command and sets HANDSHAKE.ACK_VALID.
- If ACK does not arrive in configured watchdog window, HAL sets HANDSHAKE.TIMEOUT and raises fault handling. ## Fault Handling Contract
- Any fatal core or DMA condition latches FAULT_CODE and sets STATUS.FAULT.
- HAL must clear fault through reset/clear flow before accepting new start commands.
- Faults are exposed through module test outputs and diagnostics. ## Optional Cortex-R5 Path (MPSoC)
When enabled, hard real-time paths run on R5 firmware.
- APU HAL backend writes commands via shared memory/mailbox.
- R5 firmware executes deterministic steps and updates PL registers.
- Contract above still governs final PL-facing behavior. ## Validation Requirements
The test suite must verify:
- AXI register read/write and invalid offset rejection.
- DMA normal and fault paths.
- Command/ack handshake success and timeout behavior.
- Fault latching and clear semantics.
