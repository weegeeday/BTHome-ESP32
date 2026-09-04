#ifndef BTHOME_ENCODER_H
#define BTHOME_ENCODER_H

#include <Arduino.h>

/**
 * BTHome V2 Payload Encoder
 * Builds standard unencrypted BTHome V2 BLE advertising payload.
 * BTHome Service UUID: 0xFCD2
 */
class BTHomeEncoder {
public:
    BTHomeEncoder();

    void reset();
    
    // Check if buffer has room for N bytes
    bool hasCapacity(size_t bytesNeeded) const;

    // Add Packet ID for deduplication (0x00)
    bool addPacketId(uint8_t packetId);

    // Add Door binary sensor state (0x1A: 0 = closed, 1 = open)
    bool addDoorState(bool isOpen);

    // Add Light binary power state (0x10: 0 = off, 1 = on)
    bool addLightState(bool isOn);

    // Add Button event (0x3A: 0x01 = press / NFC tag event)
    bool addButtonEvent(uint8_t eventType);

    // Add 4-byte NFC Tag UID as generic uint32 data (0x0C)
    bool addTagUid(uint32_t tagUid);

    // Get final payload bytes and length
    const uint8_t* getPayload() const;
    size_t getPayloadLength() const;

private:
    uint8_t m_buffer[31];
    size_t m_length;
};

#endif // BTHOME_ENCODER_H
