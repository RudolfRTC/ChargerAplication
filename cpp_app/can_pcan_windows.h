#pragma once

#include "can_iface.h"

#ifdef _WIN32

#include <cstdint>

class PcanIface : public ICanIface {
public:
    PcanIface();
    ~PcanIface() override;

    bool open(const CanConfig& cfg) override;
    void close() override;
    bool isOpen() const override;
    bool send(const CanFrame& frame) override;
    bool recv(CanFrame& out, int timeout_ms) override;
    std::string backendName() const override { return "PCAN-Basic"; }
    std::string lastError() const override { return m_lastError; }

    /// Parse channel string (e.g. "PCAN_USBBUS1") to PCAN handle value.
    static uint16_t pcanChannelFromString(const std::string& s);

    /// Map bitrate (bps) to TPCANBaudrate value.
    static uint16_t pcanBitrateFromInt(int bps);

    /// Convert TPCANStatus to human-readable string.
    std::string pcanErrorToString(uint32_t status) const;

private:
    bool loadDll();
    void unloadDll();

    uint16_t    m_handle = 0;
    bool        m_opened = false;
    std::string m_lastError;

    // DLL module handle
    void* m_dll = nullptr;

    // ---- PCAN types & constants (match PCANBasic.h) ----

    // TPCANMsg – must match DLL struct layout
#pragma pack(push, 8)
    struct PcanMsg {
        uint32_t ID;
        uint8_t  MSGTYPE;
        uint8_t  LEN;
        uint8_t  DATA[8];
    };
    struct PcanTimestamp {
        uint32_t millis;
        uint16_t millis_overflow;
        uint16_t micros;
    };
#pragma pack(pop)

    // Message type flags
    static constexpr uint8_t MSGTYPE_STANDARD  = 0x00;
    static constexpr uint8_t MSGTYPE_RTR       = 0x01;
    static constexpr uint8_t MSGTYPE_EXTENDED  = 0x02;
    static constexpr uint8_t MSGTYPE_STATUS    = 0x80;

    // PCAN error codes
    static constexpr uint32_t PCAN_ERROR_OK        = 0x00000;
    static constexpr uint32_t PCAN_ERROR_QRCVEMPTY = 0x00020;

    // Function pointer types (PCAN-Basic uses __stdcall on Win32)
#ifdef _MSC_VER
    #define PCAN_CALLCONV __stdcall
#else
    #define PCAN_CALLCONV __stdcall
#endif

    using FnInitialize   = uint32_t(PCAN_CALLCONV*)(uint16_t, uint16_t, uint8_t, uint32_t, uint16_t);
    using FnUninitialize = uint32_t(PCAN_CALLCONV*)(uint16_t);
    using FnRead         = uint32_t(PCAN_CALLCONV*)(uint16_t, PcanMsg*, PcanTimestamp*);
    using FnWrite        = uint32_t(PCAN_CALLCONV*)(uint16_t, PcanMsg*);
    using FnGetErrorText = uint32_t(PCAN_CALLCONV*)(uint32_t, uint16_t, char*);
    using FnSetValue     = uint32_t(PCAN_CALLCONV*)(uint16_t, uint16_t, void*, uint32_t);

    FnInitialize   m_fnInitialize   = nullptr;
    FnUninitialize m_fnUninitialize = nullptr;
    FnRead         m_fnRead         = nullptr;
    FnWrite        m_fnWrite        = nullptr;
    FnGetErrorText m_fnGetErrorText = nullptr;
    FnSetValue     m_fnSetValue     = nullptr;
};

#endif // _WIN32
