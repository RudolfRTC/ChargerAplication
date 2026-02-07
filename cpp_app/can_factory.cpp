#include "can_factory.h"
#include "can_socketcan_linux.h"
#include "can_pcan_windows.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <chrono>

int parseBitrateString(const std::string& s) {
    if (s.empty()) return 0;

    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (upper == "1M")   return 1000000;
    if (upper == "800K") return 800000;
    if (upper == "500K") return 500000;
    if (upper == "250K") return 250000;
    if (upper == "125K") return 125000;
    if (upper == "100K") return 100000;
    if (upper == "50K")  return 50000;
    if (upper == "20K")  return 20000;
    if (upper == "10K")  return 10000;

    // Try to parse as plain integer
    try {
        int val = std::stoi(s);
        if (val > 0) return val;
    } catch (...) {}

    return 0;
}

std::unique_ptr<ICanIface> makeCanInterface(const std::string& backend,
                                            std::string* errorOut) {
    std::string be = backend;
    std::transform(be.begin(), be.end(), be.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (be == "auto") {
#ifdef __linux__
        be = "socketcan";
#elif defined(_WIN32)
        be = "pcan";
#else
        if (errorOut) *errorOut = "No CAN backend available on this platform.";
        return nullptr;
#endif
    }

    if (be == "socketcan") {
#ifdef __linux__
        return std::make_unique<SocketCanIface>();
#else
        if (errorOut) *errorOut = "SocketCAN is only available on Linux.";
        return nullptr;
#endif
    }

    if (be == "pcan") {
#ifdef _WIN32
        return std::make_unique<PcanIface>();
#else
        if (errorOut) *errorOut = "PCAN-Basic is only available on Windows.";
        return nullptr;
#endif
    }

    if (errorOut)
        *errorOut = "Unknown CAN backend: '" + backend + "'. "
                    "Supported: auto, socketcan, pcan.";
    return nullptr;
}

int runCanSelfTest(const std::string& backend,
                   const std::string& channel,
                   int bitrate) {
    std::printf("=== CAN Self-Test ===\n");
    std::printf("Backend:  %s\n", backend.c_str());
    std::printf("Channel:  %s\n", channel.c_str());
    std::printf("Bitrate:  %d bps\n", bitrate);

    std::string err;
    auto iface = makeCanInterface(backend, &err);
    if (!iface) {
        std::printf("ERROR: Cannot create CAN interface: %s\n", err.c_str());
        return 1;
    }
    std::printf("Backend created: %s\n", iface->backendName().c_str());

    CanConfig cfg;
    cfg.channel = channel;
    cfg.bitrate = bitrate;
    cfg.extended = true;

    if (!iface->open(cfg)) {
        std::printf("ERROR: open() failed: %s\n", iface->lastError().c_str());
        return 1;
    }
    std::printf("Channel opened successfully.\n");

    // Send test frame: ID=0x123, data=01 02 03
    CanFrame txFrame;
    txFrame.id = 0x123;
    txFrame.dlc = 3;
    txFrame.data[0] = 0x01;
    txFrame.data[1] = 0x02;
    txFrame.data[2] = 0x03;
    txFrame.isExtended = false;

    if (iface->send(txFrame)) {
        std::printf("TX: ID=0x%03X DLC=%d DATA=%02X %02X %02X  [OK]\n",
                    txFrame.id, txFrame.dlc,
                    txFrame.data[0], txFrame.data[1], txFrame.data[2]);
    } else {
        std::printf("TX: FAILED - %s\n", iface->lastError().c_str());
    }

    // Listen for 2 seconds
    std::printf("Listening for 2 seconds...\n");
    auto t0 = std::chrono::steady_clock::now();
    int rxCount = 0;

    while (true) {
        auto elapsed = std::chrono::steady_clock::now() - t0;
        if (std::chrono::duration<double>(elapsed).count() >= 2.0)
            break;

        CanFrame rxFrame;
        if (iface->recv(rxFrame, 100)) {
            std::printf("RX: ID=0x%08X DLC=%d DATA=", rxFrame.id, rxFrame.dlc);
            for (int i = 0; i < rxFrame.dlc; ++i)
                std::printf("%02X ", rxFrame.data[i]);
            std::printf(" %s\n", rxFrame.isExtended ? "[EXT]" : "[STD]");
            ++rxCount;
        }
    }

    std::printf("Received %d frame(s) in 2 seconds.\n", rxCount);
    iface->close();
    std::printf("Channel closed. Self-test complete.\n");
    return 0;
}
