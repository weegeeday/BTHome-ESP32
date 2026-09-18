#pragma once

#include "transport.hpp"
#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <vector>
#include "pn532_cxx/span.hpp"
#include "loggable.hpp"

namespace pn532 {

namespace cmd {
    inline constexpr uint8_t GetFirmwareVersion = 0x02;
    inline constexpr uint8_t GetGeneralStatus   = 0x04;
    inline constexpr uint8_t ReadRegister       = 0x06;
    inline constexpr uint8_t WriteRegister      = 0x08;
    inline constexpr uint8_t SAMConfiguration   = 0x14;
    inline constexpr uint8_t PowerDown          = 0x16;
    inline constexpr uint8_t RFConfiguration    = 0x32;
    inline constexpr uint8_t InDataExchange     = 0x40;
    inline constexpr uint8_t InCommunicateThru  = 0x42;
    inline constexpr uint8_t InListPassiveTarget= 0x4A;
    inline constexpr uint8_t InRelease          = 0x52;
    inline constexpr uint8_t InSelect           = 0x54;
}

class Frontend : public loggable::Loggable {
public:
    explicit Frontend(Transport &protocol);

    Status begin();
    Status setPassiveActivationRetries(uint8_t maxRetries);
    Status RFConfiguration(uint8_t cfgItem, span<const uint8_t> confData);
    inline Status RFConfiguration(uint8_t cfgItem, std::initializer_list<uint8_t> confData) {
      return RFConfiguration(cfgItem, span<const uint8_t>(confData.begin(), confData.size()));
    }
    std::optional<uint32_t> GetFirmwareVersion();
    Status GetGeneralStatus(std::vector<uint8_t> &response);
    Status PowerDown(uint8_t wakeupEnable, uint8_t genIrqEnable);
    Status ReadRegister(span<const uint8_t> reg, std::vector<uint8_t> &response);
    inline Status ReadRegister(std::initializer_list<uint8_t> reg, std::vector<uint8_t> &response) {
      return ReadRegister(span<const uint8_t>(reg.begin(), reg.size()), response);
    }
    Status WriteRegister(span<const uint8_t> data);
    inline Status WriteRegister(std::initializer_list<uint8_t> data) {
      return WriteRegister(span<const uint8_t>(data.begin(), data.size()));
    }
    Status SAMConfig();

    Status InRelease(uint8_t target);
    Status InSelect(uint8_t target);

    Status InListPassiveTarget(uint8_t cardbaudrate,
                              std::vector<uint8_t> &uid,
                              std::array<uint8_t, 2> &sens_res,
                              uint8_t &sel_res,
                              uint16_t timeout = 1000);

    Status InCommunicateThru(span<const uint8_t> send,
                            std::vector<uint8_t> &response,
                            uint16_t timeout = 1000);
    inline Status InCommunicateThru(std::initializer_list<uint8_t> send,
                            std::vector<uint8_t> &response,
                            uint16_t timeout = 1000){
      return InCommunicateThru(span<const uint8_t>(send.begin(),send.size()), response, timeout);
    }

    Status InDataExchange(span<const uint8_t> send,
                                       std::vector<uint8_t> &response,
                                       uint16_t timeout = 1000);
    inline Status InDataExchange(std::initializer_list<uint8_t> send,
                            std::vector<uint8_t> &response,
                            uint16_t timeout = 1000){
      return InDataExchange(span<const uint8_t>(send.begin(),send.size()), response, timeout);
    }

private:
    static const char* TAG;
    Transport &_protocol;
    std::vector<uint8_t> _packetBuffer;

    Status transceive(span<const uint8_t> cmd,
                      std::vector<uint8_t> &response,
                      uint32_t timeout_ms = 1000);

    Status parseResponse(Transaction &txn, std::vector<uint8_t> &buffer);
};

} // namespace pn532
