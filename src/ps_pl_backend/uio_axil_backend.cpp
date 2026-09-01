/**
 * @file uio_axil_backend.cpp
 * @brief Live AXI-Lite register access through a /dev/uio device (kernel UIO)
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Map geometry comes from /sys/class/uio/uioN/maps/map0/size (hex, e.g.
 * "0x1000"). The device node is mmapped at file offset 0 — the generic-UIO
 * convention for the first map — and registers are accessed as 32-bit
 * reads/writes at byte offsets (see ZYNQ_PL_REG_* in platform_zynq.h).
 */

#include "uio_axil_backend.h"

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <new>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

// Maximum register window we will map (protects against bogus sysfs values).
static constexpr std::size_t kMaxUiMapBytes = 64u * 1024u * 1024u;

// ---------------------------------------------------------------------------
// Construction / destruction / moves
// ---------------------------------------------------------------------------
UioAxilBackend::UioAxilBackend(std::string device, int fd, std::uint32_t* base,
                               std::size_t map_bytes)
    : device_(std::move(device)), fd_(fd), base_(base), map_bytes_(map_bytes)
{
}

UioAxilBackend::~UioAxilBackend()
{
    release();
}

UioAxilBackend::UioAxilBackend(UioAxilBackend&& other) noexcept
    : device_(std::move(other.device_)), fd_(other.fd_), base_(other.base_),
      map_bytes_(other.map_bytes_)
{
    other.fd_ = -1;
    other.base_ = nullptr;
    other.map_bytes_ = 0;
}

UioAxilBackend& UioAxilBackend::operator=(UioAxilBackend&& other) noexcept
{
    if (this != &other) {
        release();
        device_ = std::move(other.device_);
        fd_ = other.fd_;
        base_ = other.base_;
        map_bytes_ = other.map_bytes_;
        other.fd_ = -1;
        other.base_ = nullptr;
        other.map_bytes_ = 0;
    }
    return *this;
}

void UioAxilBackend::release()
{
    if (base_ != nullptr) {
        ::munmap(base_, map_bytes_);
        base_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    map_bytes_ = 0;
}

// ---------------------------------------------------------------------------
// open() — read map size from sysfs, open node, mmap window
// ---------------------------------------------------------------------------
std::unique_ptr<UioAxilBackend> UioAxilBackend::open(const std::string& uio_dev)
{
    if (uio_dev.rfind("/dev/uio", 0) != 0) return nullptr; // sanity: must be /dev/uio*

    const std::string name = uio_dev.substr(uio_dev.find_last_of('/') + 1);
    const std::string size_path = "/sys/class/uio/" + name + "/maps/map0/size";

    // Map size is mandatory; sysfs prints it as hex (e.g. "0x1000").
    long long map_size = 0;
    {
        std::ifstream f(size_path);
        if (!(f >> std::hex >> map_size) || map_size <= 0 ||
            map_size > static_cast<long long>(kMaxUiMapBytes))
            return nullptr;
    }

    const int fd = ::open(uio_dev.c_str(), O_RDWR | O_CLOEXEC | O_SYNC);
    if (fd < 0) return nullptr;

    const long page = sysconf(_SC_PAGESIZE);
    const std::size_t map_len =
        (static_cast<std::size_t>(map_size) + page - 1) & ~static_cast<std::size_t>(page - 1);

    void* base = ::mmap(nullptr, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        const int saved = errno;
        ::close(fd);
        errno = saved;
        return nullptr;
    }

    return std::unique_ptr<UioAxilBackend>(
        new (std::nothrow) UioAxilBackend(uio_dev, fd, static_cast<std::uint32_t*>(base),
                                          static_cast<std::size_t>(map_size)));
}

// ---------------------------------------------------------------------------
// Register access (32-bit, little-endian AXI-Lite window)
// ---------------------------------------------------------------------------
bool UioAxilBackend::readReg(std::uint32_t offset, std::uint32_t& value) const
{
    if (!isOpen()) return false;
    if (offset + sizeof(std::uint32_t) > map_bytes_) return false;
    const volatile std::uint32_t* reg =
        reinterpret_cast<volatile const std::uint32_t*>(base_ + (offset >> 2));
    value = *reg;
    return true;
}

bool UioAxilBackend::writeReg(std::uint32_t offset, std::uint32_t value)
{
    if (!isOpen()) return false;
    if (offset + sizeof(std::uint32_t) > map_bytes_) return false;
    volatile std::uint32_t* reg =
        reinterpret_cast<volatile std::uint32_t*>(base_ + (offset >> 2));
    *reg = value;
    return true;
}

// ---------------------------------------------------------------------------
// Interrupt handling (uio_pdrv_genirq blocking-read + write(1) re-arm)
// ---------------------------------------------------------------------------
bool UioAxilBackend::waitInterrupt(int timeout_ms)
{
    struct pollfd p;
    p.fd = fd_;
    p.events = POLLIN;
    p.revents = 0;
    if (::poll(&p, 1, timeout_ms) <= 0) return false;
    int irq_count = 0;
    if (::read(fd_, &irq_count, sizeof(irq_count)) != static_cast<ssize_t>(sizeof(irq_count)))
        return false;
    return true;
}

bool UioAxilBackend::unmaskInterrupt()
{
    if (fd_ < 0) return false;
    const int one = 1;
    return ::write(fd_, &one, sizeof(one)) == static_cast<ssize_t>(sizeof(one));
}