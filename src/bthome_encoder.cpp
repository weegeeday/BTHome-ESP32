#include "bthome_encoder.h"

// BTHome Object IDs (V2 Specification)
#define BTHOME_OBJ_PACKET_ID   0x00 // 1-byte uint8
#define BTHOME_OBJ_POWER       0x10 // 1-byte uint8 (0 = off, 1 = on)
#define BTHOME_OBJ_DOOR        0x1A // 1-byte uint8 (0 = closed, 1 = open)
#define BTHOME_OBJ_BUTTON      0x3A // 1-byte uint8 (event code)
#define BTHOME_OBJ_GENERIC_32  0x0C // 4-byte uint32

BTHomeEncoder::BTHomeEncoder() {
    reset();
}

void BTHomeEncoder::reset() {
    m_length = 0;
    // BTHome V2 header byte: 0x40 (Bits 7..5 = Version 2, Bit 0 = Unencrypted)
    m_buffer[m_length++] = 0x40;
}

bool BTHomeEncoder::hasCapacity(size_t bytesNeeded) const {
    return (m_length + bytesNeeded) <= sizeof(m_buffer);
}

bool BTHomeEncoder::addPacketId(uint8_t packetId) {
    if (hasCapacity(2)) {
        m_buffer[m_length++] = BTHOME_OBJ_PACKET_ID;
        m_buffer[m_length++] = packetId;
        return true;
    }
    return false;
}

bool BTHomeEncoder::addDoorState(bool isOpen) {
    if (hasCapacity(2)) {
        m_buffer[m_length++] = BTHOME_OBJ_DOOR;
        m_buffer[m_length++] = isOpen ? 0x01 : 0x00;
        return true;
    }
    return false;
}

bool BTHomeEncoder::addLightState(bool isOn) {
    if (hasCapacity(2)) {
        m_buffer[m_length++] = BTHOME_OBJ_POWER;
        m_buffer[m_length++] = isOn ? 0x01 : 0x00;
        return true;
    }
    return false;
}

bool BTHomeEncoder::addButtonEvent(uint8_t eventType) {
    if (hasCapacity(2)) {
        m_buffer[m_length++] = BTHOME_OBJ_BUTTON;
        m_buffer[m_length++] = eventType;
        return true;
    }
    return false;
}

bool BTHomeEncoder::addTagUid(uint32_t tagUid) {
    if (hasCapacity(5)) {
        m_buffer[m_length++] = BTHOME_OBJ_GENERIC_32;
        m_buffer[m_length++] = (tagUid) & 0xFF;
        m_buffer[m_length++] = (tagUid >> 8) & 0xFF;
        m_buffer[m_length++] = (tagUid >> 16) & 0xFF;
        m_buffer[m_length++] = (tagUid >> 24) & 0xFF;
        return true;
    }
    return false;
}

const uint8_t* BTHomeEncoder::getPayload() const {
    return m_buffer;
}

size_t BTHomeEncoder::getPayloadLength() const {
    return m_length;
}
