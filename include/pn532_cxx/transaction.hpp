#pragma once

#include <cstdint>
#include "pn532_cxx/span.hpp"

namespace pn532 {

enum class [[nodiscard]] Status : uint8_t {
  SUCCESS = 0,
  TIMEOUT = 1,
  INVALID_FRAME = 2,
  INVALID_TFI = 3,
  NO_SPACE = 4,
  ERROR_FRAME = 5,
  CHECKSUM_ERROR = 6,
  TRANSPORT_ERROR = 7,
  INVALID_TARGET = 8,
  NO_TAGS_FOUND = 9,
};

class Transport;

class Transaction {
public:
    Transaction() = default;
    Transaction(Transport &transport, bool valid);
    
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;
    Transaction(Transaction &&other) noexcept;
    Transaction &operator=(Transaction &&other) noexcept;
    ~Transaction();

    Status write(span<const uint8_t> data);
    Status waitForAck(uint32_t timeout_ms = 100);
    Status waitForResponse(uint32_t timeout_ms = 1000);
    Status read(span<uint8_t> buffer);

    explicit operator bool() const noexcept { return _valid; }

private:
    Transport *_transport = nullptr;
    bool _valid = false;
    bool _in_read_mode = false;
    bool _ended = false;

    void finish() noexcept;
};

} // namespace pn532
