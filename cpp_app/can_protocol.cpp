#include "can_protocol.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>

std::string chargerControlName(ChargerControl ctrl) {
    switch (ctrl) {
        case ChargerControl::START_CHARGING:    return "START_CHARGING";
        case ChargerControl::STOP_OUTPUTTING:   return "STOP_OUTPUTTING";
        case ChargerControl::HEATING_DC_SUPPLY: return "HEATING_DC_SUPPLY";
    }
    return "UNKNOWN";
}

// --- StatusFlags ---

StatusFlags StatusFlags::fromByte(uint8_t b) {
    StatusFlags f;
    f.hardware_failure      = (b & 0x01) != 0;
    f.over_temperature      = (b & 0x02) != 0;
    f.input_voltage_error   = (b & 0x04) != 0;
    f.starting_state        = (b & 0x08) != 0;
    f.communication_timeout = (b & 0x10) != 0;
    return f;
}

uint8_t StatusFlags::toByte() const {
    uint8_t b = 0;
    if (hardware_failure)      b |= 0x01;
    if (over_temperature)      b |= 0x02;
    if (input_voltage_error)   b |= 0x04;
    if (starting_state)        b |= 0x08;
    if (communication_timeout) b |= 0x10;
    return b;
}

bool StatusFlags::anyFault() const {
    return hardware_failure || over_temperature
        || input_voltage_error || communication_timeout;
}

// --- Message1 ---

std::array<uint8_t, 8> Message1::encode() const {
    int v_raw = std::clamp(static_cast<int>(std::round(voltage_setpoint * 10.0)), 0, 0xFFFF);
    int i_raw = std::clamp(static_cast<int>(std::round(current_setpoint * 10.0)), 0, 0xFFFF);

    std::array<uint8_t, 8> buf{};
    // Big-endian: voltage (2 bytes), current (2 bytes), control (1 byte), 3 padding
    buf[0] = static_cast<uint8_t>((v_raw >> 8) & 0xFF);
    buf[1] = static_cast<uint8_t>(v_raw & 0xFF);
    buf[2] = static_cast<uint8_t>((i_raw >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>(i_raw & 0xFF);
    buf[4] = static_cast<uint8_t>(control);
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;
    return buf;
}

Message1 Message1::decode(const uint8_t* data, size_t len) {
    if (len < 5)
        throw std::runtime_error("Message1 requires >= 5 bytes");

    uint16_t v_raw = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    uint16_t i_raw = (static_cast<uint16_t>(data[2]) << 8) | data[3];
    uint8_t ctrl = data[4];

    if (ctrl > 2)
        throw std::runtime_error("Unknown ChargerControl value: " + std::to_string(ctrl));

    Message1 msg;
    msg.voltage_setpoint = v_raw / 10.0;
    msg.current_setpoint = i_raw / 10.0;
    msg.control = static_cast<ChargerControl>(ctrl);
    return msg;
}

// --- Message2 ---

std::array<uint8_t, 8> Message2::encode() const {
    int v_raw   = std::clamp(static_cast<int>(std::round(output_voltage * 10.0)), 0, 0xFFFF);
    int i_raw   = std::clamp(static_cast<int>(std::round(output_current * 10.0)), 0, 0xFFFF);
    int vin_raw = std::clamp(static_cast<int>(std::round(input_voltage * 10.0)), 0, 0xFFFF);
    int tmp_raw = std::clamp(static_cast<int>(std::round(temperature + 40.0)), 0, 0xFF);

    std::array<uint8_t, 8> buf{};
    // Big-endian: output_voltage(2), output_current(2), status(1), input_voltage(2), temperature(1)
    buf[0] = static_cast<uint8_t>((v_raw >> 8) & 0xFF);
    buf[1] = static_cast<uint8_t>(v_raw & 0xFF);
    buf[2] = static_cast<uint8_t>((i_raw >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>(i_raw & 0xFF);
    buf[4] = status.toByte();
    buf[5] = static_cast<uint8_t>((vin_raw >> 8) & 0xFF);
    buf[6] = static_cast<uint8_t>(vin_raw & 0xFF);
    buf[7] = static_cast<uint8_t>(tmp_raw);
    return buf;
}

Message2 Message2::decode(const uint8_t* data, size_t len) {
    if (len < 8)
        throw std::runtime_error("Message2 requires 8 bytes");

    uint16_t v_raw   = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    uint16_t i_raw   = (static_cast<uint16_t>(data[2]) << 8) | data[3];
    uint8_t  st_byte = data[4];
    uint16_t vin_raw = (static_cast<uint16_t>(data[5]) << 8) | data[6];
    uint8_t  tmp_raw = data[7];

    Message2 msg;
    msg.output_voltage = v_raw / 10.0;
    msg.output_current = i_raw / 10.0;
    msg.status = StatusFlags::fromByte(st_byte);
    msg.input_voltage = vin_raw / 10.0;
    msg.temperature = static_cast<double>(tmp_raw) - 40.0;
    return msg;
}
