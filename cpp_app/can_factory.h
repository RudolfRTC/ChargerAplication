#pragma once

#include "can_iface.h"
#include <memory>
#include <string>

/// Parse bitrate string ("500K", "250K", "1M", "250000", etc.) to bps integer.
/// Returns 0 on failure.
int parseBitrateString(const std::string& s);

/// Create a CAN interface for the given backend.
/// backend: "auto", "socketcan", "pcan"
/// On "auto": Linux → SocketCAN, Windows → PCAN.
/// Returns nullptr if the backend is not available on this platform.
/// errorOut (if non-null) receives a description on failure.
std::unique_ptr<ICanIface> makeCanInterface(const std::string& backend,
                                            std::string* errorOut = nullptr);

/// Run a minimal self-test: open, send one frame, listen for 2 s.
/// Prints results to stdout. Returns 0 on success, 1 on error.
int runCanSelfTest(const std::string& backend,
                   const std::string& channel,
                   int bitrate);
