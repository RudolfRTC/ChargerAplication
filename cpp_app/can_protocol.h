#pragma once

#include <cstdint>
#include <array>
#include <string>

// Node source addresses (J1939)
constexpr uint8_t SA_BMS = 0xF4;
constexpr uint8_t SA_OBC = 0xE5;
constexpr uint8_t SA_BCA = 0x50;

// CAN message IDs (29-bit extended)
constexpr uint32_t MSG1_ID = 0x1806E5F4;  // BMS -> OBC
constexpr uint32_t MSG2_ID = 0x18FF50E5;  // OBC -> BCA

// Timing
constexpr int CYCLE_MS = 500;
constexpr double TIMEOUT_S = 5.0;

enum class ChargerControl : uint8_t {
    START_CHARGING    = 0,
    STOP_OUTPUTTING   = 1,
    HEATING_DC_SUPPLY = 2,
};

std::string chargerControlName(ChargerControl ctrl);

struct StatusFlags {
    bool hardware_failure      = false;
    bool over_temperature      = false;
    bool input_voltage_error   = false;
    bool starting_state        = false;
    bool communication_timeout = false;

    static StatusFlags fromByte(uint8_t b);
    uint8_t toByte() const;
    bool anyFault() const;
};

struct Message1 {
    double voltage_setpoint = 0.0;
    double current_setpoint = 0.0;
    ChargerControl control  = ChargerControl::STOP_OUTPUTTING;

    std::array<uint8_t, 8> encode() const;
    static Message1 decode(const uint8_t* data, size_t len);
};

struct Message2 {
    double output_voltage = 0.0;
    double output_current = 0.0;
    StatusFlags status;
    double input_voltage  = 0.0;
    double temperature    = 0.0;

    std::array<uint8_t, 8> encode() const;
    static Message2 decode(const uint8_t* data, size_t len);
};
