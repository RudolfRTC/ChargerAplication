#include "can_pcan_windows.h"

#ifdef _WIN32

#include <windows.h>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cctype>

// ---- PCAN handle constants ----
static constexpr uint16_t PCAN_USBBUS1  = 0x51;
static constexpr uint16_t PCAN_USBBUS2  = 0x52;
static constexpr uint16_t PCAN_USBBUS3  = 0x53;
static constexpr uint16_t PCAN_USBBUS4  = 0x54;
static constexpr uint16_t PCAN_USBBUS5  = 0x55;
static constexpr uint16_t PCAN_USBBUS6  = 0x56;
static constexpr uint16_t PCAN_USBBUS7  = 0x57;
static constexpr uint16_t PCAN_USBBUS8  = 0x58;
static constexpr uint16_t PCAN_USBBUS9  = 0x59;
static constexpr uint16_t PCAN_USBBUS10 = 0x5A;
static constexpr uint16_t PCAN_USBBUS11 = 0x5B;
static constexpr uint16_t PCAN_USBBUS12 = 0x5C;
static constexpr uint16_t PCAN_USBBUS13 = 0x5D;
static constexpr uint16_t PCAN_USBBUS14 = 0x5E;
static constexpr uint16_t PCAN_USBBUS15 = 0x5F;
static constexpr uint16_t PCAN_USBBUS16 = 0x60;

// ---- PCAN baudrate constants ----
static constexpr uint16_t PCAN_BAUD_1M   = 0x0014;
static constexpr uint16_t PCAN_BAUD_800K = 0x0016;
static constexpr uint16_t PCAN_BAUD_500K = 0x001C;
static constexpr uint16_t PCAN_BAUD_250K = 0x011C;
static constexpr uint16_t PCAN_BAUD_125K = 0x031C;
static constexpr uint16_t PCAN_BAUD_100K = 0x432F;
static constexpr uint16_t PCAN_BAUD_50K  = 0x472F;
static constexpr uint16_t PCAN_BAUD_20K  = 0x532F;
static constexpr uint16_t PCAN_BAUD_10K  = 0x672F;

// ---- Implementation ----

PcanIface::PcanIface() = default;

PcanIface::~PcanIface() {
    close();
    unloadDll();
}

bool PcanIface::loadDll() {
    if (m_dll) return true;

    HMODULE h = LoadLibraryA("PCANBasic.dll");
    if (!h) {
        m_lastError = "Failed to load PCANBasic.dll. "
                      "Ensure the PEAK PCAN-Basic driver is installed and "
                      "PCANBasic.dll is in your PATH or application directory.";
        return false;
    }
    m_dll = static_cast<void*>(h);

    m_fnInitialize   = reinterpret_cast<FnInitialize>(GetProcAddress(h, "CAN_Initialize"));
    m_fnUninitialize = reinterpret_cast<FnUninitialize>(GetProcAddress(h, "CAN_Uninitialize"));
    m_fnRead         = reinterpret_cast<FnRead>(GetProcAddress(h, "CAN_Read"));
    m_fnWrite        = reinterpret_cast<FnWrite>(GetProcAddress(h, "CAN_Write"));
    m_fnGetErrorText = reinterpret_cast<FnGetErrorText>(GetProcAddress(h, "CAN_GetErrorText"));
    m_fnSetValue     = reinterpret_cast<FnSetValue>(GetProcAddress(h, "CAN_SetValue"));

    if (!m_fnInitialize || !m_fnUninitialize || !m_fnRead || !m_fnWrite) {
        m_lastError = "PCANBasic.dll loaded but required functions not found. "
                      "The DLL version may be incompatible.";
        FreeLibrary(h);
        m_dll = nullptr;
        return false;
    }

    return true;
}

void PcanIface::unloadDll() {
    if (m_dll) {
        FreeLibrary(static_cast<HMODULE>(m_dll));
        m_dll = nullptr;
    }
    m_fnInitialize = nullptr;
    m_fnUninitialize = nullptr;
    m_fnRead = nullptr;
    m_fnWrite = nullptr;
    m_fnGetErrorText = nullptr;
    m_fnSetValue = nullptr;
}

uint16_t PcanIface::pcanChannelFromString(const std::string& s) {
    // Normalize to uppercase
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (upper == "PCAN_USBBUS1"  || upper == "USBBUS1")  return PCAN_USBBUS1;
    if (upper == "PCAN_USBBUS2"  || upper == "USBBUS2")  return PCAN_USBBUS2;
    if (upper == "PCAN_USBBUS3"  || upper == "USBBUS3")  return PCAN_USBBUS3;
    if (upper == "PCAN_USBBUS4"  || upper == "USBBUS4")  return PCAN_USBBUS4;
    if (upper == "PCAN_USBBUS5"  || upper == "USBBUS5")  return PCAN_USBBUS5;
    if (upper == "PCAN_USBBUS6"  || upper == "USBBUS6")  return PCAN_USBBUS6;
    if (upper == "PCAN_USBBUS7"  || upper == "USBBUS7")  return PCAN_USBBUS7;
    if (upper == "PCAN_USBBUS8"  || upper == "USBBUS8")  return PCAN_USBBUS8;
    if (upper == "PCAN_USBBUS9"  || upper == "USBBUS9")  return PCAN_USBBUS9;
    if (upper == "PCAN_USBBUS10" || upper == "USBBUS10") return PCAN_USBBUS10;
    if (upper == "PCAN_USBBUS11" || upper == "USBBUS11") return PCAN_USBBUS11;
    if (upper == "PCAN_USBBUS12" || upper == "USBBUS12") return PCAN_USBBUS12;
    if (upper == "PCAN_USBBUS13" || upper == "USBBUS13") return PCAN_USBBUS13;
    if (upper == "PCAN_USBBUS14" || upper == "USBBUS14") return PCAN_USBBUS14;
    if (upper == "PCAN_USBBUS15" || upper == "USBBUS15") return PCAN_USBBUS15;
    if (upper == "PCAN_USBBUS16" || upper == "USBBUS16") return PCAN_USBBUS16;

    // Try to parse numeric handle (e.g. "0x51")
    try {
        unsigned long val = std::stoul(s, nullptr, 0);
        return static_cast<uint16_t>(val);
    } catch (...) {}

    return 0; // invalid
}

uint16_t PcanIface::pcanBitrateFromInt(int bps) {
    switch (bps) {
        case 1000000: return PCAN_BAUD_1M;
        case 800000:  return PCAN_BAUD_800K;
        case 500000:  return PCAN_BAUD_500K;
        case 250000:  return PCAN_BAUD_250K;
        case 125000:  return PCAN_BAUD_125K;
        case 100000:  return PCAN_BAUD_100K;
        case 50000:   return PCAN_BAUD_50K;
        case 20000:   return PCAN_BAUD_20K;
        case 10000:   return PCAN_BAUD_10K;
        default:      return 0;
    }
}

std::string PcanIface::pcanErrorToString(uint32_t status) const {
    if (m_fnGetErrorText) {
        char buf[256] = {};
        uint32_t res = m_fnGetErrorText(status, 0x09 /* English */, buf);
        if (res == PCAN_ERROR_OK)
            return std::string(buf);
    }
    // Fallback
    char hex[32];
    snprintf(hex, sizeof(hex), "PCAN error 0x%05X", status);
    return std::string(hex);
}

bool PcanIface::open(const CanConfig& cfg) {
    if (m_opened) {
        m_lastError = "Already open";
        return false;
    }

    if (!loadDll())
        return false;

    // Parse channel
    m_handle = pcanChannelFromString(cfg.channel);
    if (m_handle == 0) {
        m_lastError = "Unknown PCAN channel: '" + cfg.channel + "'. "
                      "Use e.g. PCAN_USBBUS1 .. PCAN_USBBUS16.";
        return false;
    }

    // Map bitrate
    uint16_t baudrate = pcanBitrateFromInt(cfg.bitrate);
    if (baudrate == 0) {
        m_lastError = "Unsupported PCAN bitrate: " + std::to_string(cfg.bitrate) + " bps. "
                      "Supported: 10K, 20K, 50K, 100K, 125K, 250K, 500K, 800K, 1M.";
        return false;
    }

    // CAN_Initialize(handle, baudrate, hwType=0, ioPort=0, interrupt=0)
    // hwType/ioPort/interrupt are only for non-PnP (ISA) adapters
    uint32_t st = m_fnInitialize(m_handle, baudrate, 0, 0, 0);
    if (st != PCAN_ERROR_OK) {
        m_lastError = "CAN_Initialize failed: " + pcanErrorToString(st);
        return false;
    }

    m_opened = true;
    m_lastError.clear();
    return true;
}

void PcanIface::close() {
    if (m_opened && m_fnUninitialize) {
        m_fnUninitialize(m_handle);
        m_opened = false;
    }
}

bool PcanIface::isOpen() const {
    return m_opened;
}

bool PcanIface::send(const CanFrame& frame) {
    if (!m_opened || !m_fnWrite) {
        m_lastError = "Not open";
        return false;
    }

    PcanMsg msg{};
    msg.ID = frame.id;
    msg.LEN = frame.dlc;
    std::memcpy(msg.DATA, frame.data, frame.dlc);

    msg.MSGTYPE = MSGTYPE_STANDARD;
    if (frame.isExtended) msg.MSGTYPE = MSGTYPE_EXTENDED;
    if (frame.isRTR)      msg.MSGTYPE |= MSGTYPE_RTR;

    uint32_t st = m_fnWrite(m_handle, &msg);
    if (st != PCAN_ERROR_OK) {
        m_lastError = "CAN_Write failed: " + pcanErrorToString(st);
        return false;
    }
    return true;
}

bool PcanIface::recv(CanFrame& out, int timeout_ms) {
    if (!m_opened || !m_fnRead) return false;

    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline) {
        PcanMsg msg{};
        PcanTimestamp ts{};
        uint32_t st = m_fnRead(m_handle, &msg, &ts);

        if (st == PCAN_ERROR_OK) {
            // Skip status messages
            if (msg.MSGTYPE & MSGTYPE_STATUS)
                continue;

            out.id = msg.ID;
            out.dlc = msg.LEN;
            std::memcpy(out.data, msg.DATA, msg.LEN);
            out.isExtended = (msg.MSGTYPE & MSGTYPE_EXTENDED) != 0;
            out.isRTR = (msg.MSGTYPE & MSGTYPE_RTR) != 0;

            // Convert PCAN timestamp to monotonic seconds
            double tsSec = static_cast<double>(ts.millis) / 1000.0
                         + static_cast<double>(ts.micros) / 1000000.0;
            out.timestamp = tsSec;
            return true;
        }

        if (st == PCAN_ERROR_QRCVEMPTY) {
            // No data yet – sleep briefly and retry
            Sleep(2);
            continue;
        }

        // Actual error
        m_lastError = "CAN_Read failed: " + pcanErrorToString(st);
        return false;
    }

    return false; // timeout
}

#endif // _WIN32
