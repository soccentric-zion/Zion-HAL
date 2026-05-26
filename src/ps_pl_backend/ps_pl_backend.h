#ifndef ZION_PS_PL_BACKEND_H
#define ZION_PS_PL_BACKEND_H

#include <cstdint>
#include <map>
#include <string>

struct PlDmaDescriptor
{
    std::uint32_t src;
    std::uint32_t dst;
    std::uint32_t length_bytes;
};

class PsPlBackend
{
  public:
    PsPlBackend();

    bool writeReg(std::uint32_t offset, std::uint32_t value);
    bool readReg(std::uint32_t offset, std::uint32_t& value) const;

    bool startDma(const PlDmaDescriptor& desc);
    bool dmaDone() const;
    bool dmaError() const;

    bool issueHandshake(std::uint32_t cmd);
    bool ackHandshake(std::uint32_t ack);
    bool handshakeTimedOut() const;

    void injectFault(std::uint32_t fault_code);
    void clearFault();
    bool hasFault() const;
    std::uint32_t faultCode() const;

    static bool isValidOffset(std::uint32_t offset);

  private:
    std::map<std::uint32_t, std::uint32_t> reg_map_;
};

#endif // ZION_PS_PL_BACKEND_H
