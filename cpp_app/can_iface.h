#pragma once

#include <cstdint>
#include <string>
#include <memory>

/// Generic CAN frame (classic CAN, not FD)
struct CanFrame {
    uint32_t id = 0;
    uint8_t  dlc = 0;
    uint8_t  data[8] = {};
    bool     isExtended = false;
    bool     isRTR = false;
    double   timestamp = 0.0;  // monotonic seconds (0 = not set)
};

/// Configuration for opening a CAN channel
struct CanConfig {
    std::string channel;            // e.g. "can0" or "PCAN_USBBUS1"
    int         bitrate = 250000;   // bps: 250000, 500000, 1000000, ...
    bool        extended = true;    // accept extended frames by default
    bool        listenOnly = false;
};

/// Abstract CAN bus interface
class ICanIface {
public:
    virtual ~ICanIface() = default;

    /// Open the CAN channel with given configuration.
    virtual bool open(const CanConfig& cfg) = 0;

    /// Close the channel.
    virtual void close() = 0;

    /// Returns true if the channel is currently open.
    virtual bool isOpen() const = 0;

    /// Send a CAN frame. Returns true on success.
    virtual bool send(const CanFrame& frame) = 0;

    /// Receive a CAN frame (blocking up to timeout_ms).
    /// Returns true if a frame was received, false on timeout/error.
    virtual bool recv(CanFrame& out, int timeout_ms) = 0;

    /// Human-readable name of the backend (e.g. "SocketCAN", "PCAN").
    virtual std::string backendName() const = 0;

    /// Last error description.
    virtual std::string lastError() const = 0;
};
