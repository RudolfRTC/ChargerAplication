#pragma once

#include "can_iface.h"

#ifdef __linux__

class SocketCanIface : public ICanIface {
public:
    SocketCanIface();
    ~SocketCanIface() override;

    bool open(const CanConfig& cfg) override;
    void close() override;
    bool isOpen() const override;
    bool send(const CanFrame& frame) override;
    bool recv(CanFrame& out, int timeout_ms) override;
    std::string backendName() const override { return "SocketCAN"; }
    std::string lastError() const override { return m_lastError; }

private:
    int m_fd = -1;
    std::string m_lastError;
};

#endif // __linux__
