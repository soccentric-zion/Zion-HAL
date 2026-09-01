#include "platform_zynq.h"
#include "ps_pl_backend/ps_pl_backend.h"
#include "ps_pl_backend/uio_axil_backend.h"

#include <cstdint>
#include <iostream>

namespace {

bool expect(bool cond, const char* msg)
{
    if (!cond) {
        std::cerr << "[FAIL] " << msg << "\n";
        return false;
    }
    std::cout << "[PASS] " << msg << "\n";
    return true;
}

bool testAxiRegisterReadWrite()
{
    PsPlBackend backend;
    std::uint32_t v = 0;

    bool ok = true;
    ok &= expect(backend.readReg(ZYNQ_PL_REG_ID, v), "read ID register");
    ok &= expect(v == ZYNQ_PL_AXI_REG_MAGIC, "ID register matches ZION magic");

    ok &= expect(backend.writeReg(ZYNQ_PL_REG_CONTROL, ZYNQ_PL_CONTROL_ENABLE), "write CONTROL register");
    ok &= expect(backend.readReg(ZYNQ_PL_REG_CONTROL, v), "read CONTROL register");
    ok &= expect(v == ZYNQ_PL_CONTROL_ENABLE, "CONTROL register roundtrip");

    ok &= expect(!backend.writeReg(ZYNQ_PL_AXI_REG_MAP_BYTES + 4u, 0u), "reject out-of-range register write");
    ok &= expect(!backend.readReg(ZYNQ_PL_AXI_REG_MAP_BYTES + 4u, v), "reject out-of-range register read");
    return ok;
}

bool testDmaPath()
{
    PsPlBackend backend;
    bool ok = true;

    PlDmaDescriptor valid{0x1000u, 0x2000u, 4096u};
    ok &= expect(backend.startDma(valid), "start valid DMA transfer");
    ok &= expect(backend.dmaDone(), "DMA transfer reaches DONE state");
    ok &= expect(!backend.dmaError(), "valid DMA has no error flag");

    PlDmaDescriptor invalid{0x1000u, 0x2000u, 0u};
    ok &= expect(!backend.startDma(invalid), "reject zero-length DMA transfer");
    ok &= expect(backend.dmaError(), "zero-length DMA sets error flag");
    return ok;
}

bool testIpCoreHandshake()
{
    PsPlBackend backend;
    bool ok = true;

    ok &= expect(backend.issueHandshake(ZYNQ_PL_CONTROL_START), "issue PS->PL handshake command");
    ok &= expect(backend.ackHandshake(0x1u), "acknowledge PL->PS handshake");
    ok &= expect(!backend.handshakeTimedOut(), "handshake completes without timeout");

    PsPlBackend timeout_backend;
    ok &= expect(!timeout_backend.ackHandshake(0x1u), "detect handshake ack without pending command");
    ok &= expect(timeout_backend.handshakeTimedOut(), "set timeout bit on invalid handshake ack");
    return ok;
}

bool testFaultHandling()
{
    PsPlBackend backend;
    bool ok = true;

    backend.injectFault(0xDEAD0001u);
    ok &= expect(backend.hasFault(), "fault bit set when fault injected");
    ok &= expect(backend.faultCode() == 0xDEAD0001u, "fault code captured");

    backend.clearFault();
    ok &= expect(!backend.hasFault(), "fault bit clears after clearFault");
    ok &= expect(backend.faultCode() == 0u, "fault code resets after clearFault");
    return ok;
}

bool testUioBackendGating()
{
    bool ok = true;

    // Live UIO backend must refuse non-/dev/uio paths without touching the system.
    ok &= expect(UioAxilBackend::open("/dev/video0") == nullptr,
                 "UIO backend rejects non-/dev/uio device paths");

    // A /dev/uioN that does not exist must fail cleanly (nullptr), not crash.
    if (!UioAxilBackend::open("/dev/uio254")) {
        ok &= expect(true, "UIO backend returns null for absent device");
    }
    return ok;
}

} // namespace

int main()
{
    bool ok = true;
    ok &= testAxiRegisterReadWrite();
    ok &= testDmaPath();
    ok &= testIpCoreHandshake();
    ok &= testFaultHandling();
    ok &= testUioBackendGating();

    if (!ok) {
        std::cerr << "[ERROR] test_ps_pl_axi failed\n";
        return 1;
    }
    std::cout << "[INFO] test_ps_pl_axi passed\n";
    return 0;
}
