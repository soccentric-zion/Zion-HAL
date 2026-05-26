#include "ps_pl_backend/ps_pl_backend.h"
#include "platform_zynq.h"

PsPlBackend::PsPlBackend()
{
    reg_map_[ZYNQ_PL_REG_ID] = ZYNQ_PL_AXI_REG_MAGIC;
    reg_map_[ZYNQ_PL_REG_VERSION] = ZYNQ_PL_AXI_REG_VERSION;
    reg_map_[ZYNQ_PL_REG_STATUS] = ZYNQ_PL_STATUS_READY;
    reg_map_[ZYNQ_PL_REG_DMA_STATUS] = 0;
    reg_map_[ZYNQ_PL_REG_HANDSHAKE] = 0;
    reg_map_[ZYNQ_PL_REG_FAULT_CODE] = 0;
}

bool PsPlBackend::isValidOffset(std::uint32_t offset)
{
    return (offset < ZYNQ_PL_AXI_REG_MAP_BYTES) && ((offset % 4u) == 0u);
}

bool PsPlBackend::writeReg(std::uint32_t offset, std::uint32_t value)
{
    if (!isValidOffset(offset)) {
        return false;
    }
    reg_map_[offset] = value;
    return true;
}

bool PsPlBackend::readReg(std::uint32_t offset, std::uint32_t& value) const
{
    if (!isValidOffset(offset)) {
        return false;
    }
    auto it = reg_map_.find(offset);
    if (it == reg_map_.end()) {
        value = 0;
        return true;
    }
    value = it->second;
    return true;
}

bool PsPlBackend::startDma(const PlDmaDescriptor& desc)
{
    if (desc.length_bytes == 0u) {
        reg_map_[ZYNQ_PL_REG_DMA_STATUS] = ZYNQ_PL_DMA_STATUS_ERROR;
        return false;
    }

    reg_map_[ZYNQ_PL_REG_DMA_SRC_LO] = desc.src;
    reg_map_[ZYNQ_PL_REG_DMA_DST_LO] = desc.dst;
    reg_map_[ZYNQ_PL_REG_DMA_LEN_BYTES] = desc.length_bytes;
    reg_map_[ZYNQ_PL_REG_DMA_CONTROL] = ZYNQ_PL_DMA_CONTROL_START | ZYNQ_PL_DMA_CONTROL_IRQ_EN;
    reg_map_[ZYNQ_PL_REG_DMA_STATUS] = ZYNQ_PL_DMA_STATUS_BUSY;

    reg_map_[ZYNQ_PL_REG_DMA_STATUS] = ZYNQ_PL_DMA_STATUS_DONE;
    reg_map_[ZYNQ_PL_REG_STATUS] = ZYNQ_PL_STATUS_READY | ZYNQ_PL_STATUS_DMA_DONE;
    return true;
}

bool PsPlBackend::dmaDone() const
{
    auto it = reg_map_.find(ZYNQ_PL_REG_DMA_STATUS);
    if (it == reg_map_.end()) {
        return false;
    }
    return (it->second & ZYNQ_PL_DMA_STATUS_DONE) != 0u;
}

bool PsPlBackend::dmaError() const
{
    auto it = reg_map_.find(ZYNQ_PL_REG_DMA_STATUS);
    if (it == reg_map_.end()) {
        return false;
    }
    return (it->second & ZYNQ_PL_DMA_STATUS_ERROR) != 0u;
}

bool PsPlBackend::issueHandshake(std::uint32_t cmd)
{
    reg_map_[ZYNQ_PL_REG_CONTROL] = cmd;
    reg_map_[ZYNQ_PL_REG_HANDSHAKE] = ZYNQ_PL_HS_CMD_VALID;
    reg_map_[ZYNQ_PL_REG_STATUS] = ZYNQ_PL_STATUS_BUSY;
    return true;
}

bool PsPlBackend::ackHandshake(std::uint32_t ack)
{
    if ((reg_map_[ZYNQ_PL_REG_HANDSHAKE] & ZYNQ_PL_HS_CMD_VALID) == 0u) {
        reg_map_[ZYNQ_PL_REG_HANDSHAKE] = ZYNQ_PL_HS_TIMEOUT;
        return false;
    }
    reg_map_[ZYNQ_PL_REG_IRQ_ACK] = ack;
    reg_map_[ZYNQ_PL_REG_HANDSHAKE] = ZYNQ_PL_HS_CMD_VALID | ZYNQ_PL_HS_ACK_VALID;
    reg_map_[ZYNQ_PL_REG_STATUS] = ZYNQ_PL_STATUS_READY;
    return true;
}

bool PsPlBackend::handshakeTimedOut() const
{
    auto it = reg_map_.find(ZYNQ_PL_REG_HANDSHAKE);
    if (it == reg_map_.end()) {
        return false;
    }
    return (it->second & ZYNQ_PL_HS_TIMEOUT) != 0u;
}

void PsPlBackend::injectFault(std::uint32_t fault_code)
{
    reg_map_[ZYNQ_PL_REG_FAULT_CODE] = fault_code;
    reg_map_[ZYNQ_PL_REG_STATUS] = ZYNQ_PL_STATUS_FAULT;
}

void PsPlBackend::clearFault()
{
    reg_map_[ZYNQ_PL_REG_FAULT_CODE] = 0;
    reg_map_[ZYNQ_PL_REG_STATUS] = ZYNQ_PL_STATUS_READY;
}

bool PsPlBackend::hasFault() const
{
    auto it = reg_map_.find(ZYNQ_PL_REG_STATUS);
    if (it == reg_map_.end()) {
        return false;
    }
    return (it->second & ZYNQ_PL_STATUS_FAULT) != 0u;
}

std::uint32_t PsPlBackend::faultCode() const
{
    auto it = reg_map_.find(ZYNQ_PL_REG_FAULT_CODE);
    if (it == reg_map_.end()) {
        return 0u;
    }
    return it->second;
}
