/**
 * @file uio_axil_backend.h
 * @brief Live AXI-Lite register access through a /dev/uio device (kernel UIO)
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * A UIO device backed by the PL (uio_pdrv_genirq bound to a fabric IP with a
 * reg window) exposes a contiguous register window through
 * /sys/class/uio/uioN/maps/map0 (addr / size sysfs files) and one mmap-able
 * device node. This class wraps that window for the ZION PL AXI register
 * contract (platform constants ZYNQ_PL_REG_*). The in-memory PsPlBackend
 * remains the contract mock for CI; on the board, pair both and compare.
 */
#ifndef ZION_UIO_AXIL_BACKEND_H
#define ZION_UIO_AXIL_BACKEND_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class UioAxilBackend
{
  public:
    /// Open `uio_dev` (e.g. "/dev/uio0") and mmap map0 with size from sysfs.
    /// Returns nullptr on failure; errno is preserved from the failing syscall.
    static std::unique_ptr<UioAxilBackend> open(const std::string& uio_dev);

    ~UioAxilBackend();
    UioAxilBackend(const UioAxilBackend&) = delete;
    UioAxilBackend& operator=(const UioAxilBackend&) = delete;
    UioAxilBackend(UioAxilBackend&&) noexcept;
    UioAxilBackend& operator=(UioAxilBackend&&) noexcept;

    /// Read a 32-bit register at `offset` within the mapped window.
    bool readReg(std::uint32_t offset, std::uint32_t& value) const;
    /// Write a 32-bit register at `offset` within the mapped window.
    bool writeReg(std::uint32_t offset, std::uint32_t value);

    /// Size in bytes of the mapped register window (from sysfs map size).
    std::size_t mapBytes() const { return map_bytes_; }
    /// True while the device node is open and mmapped.
    bool isOpen() const { return fd_ >= 0 && base_ != nullptr; }
    /// Device path this backend was opened on (e.g. "/dev/uio0").
    const std::string& device() const { return device_; }

    /// Block up to `timeout_ms` for a UIO interrupt (read(2) semantics).
    bool waitInterrupt(int timeout_ms);
    /// Re-arm the interrupt after handling (write 1 to the UIO fd).
    bool unmaskInterrupt();

  private:
    UioAxilBackend(std::string device, int fd, std::uint32_t* base, std::size_t map_bytes);
    void release();

    std::string device_;
    int fd_{-1};
    std::uint32_t* base_{nullptr};
    std::size_t map_bytes_{0};
};

#endif // ZION_UIO_AXIL_BACKEND_H