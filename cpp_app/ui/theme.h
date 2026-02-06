#pragma once

#include <QString>

// Colour palette
namespace Theme {

constexpr const char* BG_DEEP     = "#0a0e1a";
constexpr const char* BG_PANEL    = "#111827";
constexpr const char* BG_CARD     = "#1a2332";
constexpr const char* BG_INPUT    = "#0f1724";
constexpr const char* BORDER      = "#2d3748";
constexpr const char* BORDER_FOCUS = "#00e5ff";

constexpr const char* CYAN    = "#00e5ff";
constexpr const char* MAGENTA = "#e040fb";
constexpr const char* VIOLET  = "#7c4dff";
constexpr const char* GREEN   = "#00e676";
constexpr const char* RED     = "#ff1744";
constexpr const char* ORANGE  = "#ff9100";

constexpr const char* TEXT         = "#e0e7ff";
constexpr const char* TEXT_DIM     = "#94a3b8";
constexpr const char* TEXT_HEADING = "#ffffff";

constexpr const char* COMPANY = "RTC d.o.o.";
constexpr const char* ADDRESS = "Cesta k Dravi 21, 2000 Maribor, Slovenia";
constexpr const char* MADE_BY = "Made by RLF";
constexpr const char* VERSION = "1.0.0";

} // namespace Theme

QString appStyleSheet();
