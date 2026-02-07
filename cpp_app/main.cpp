#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "ui/main_window.h"
#include "ui/theme.h"
#include "settings.h"
#include "can_factory.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OBC Charger Controller");
    app.setApplicationVersion("1.0.0");
    app.setStyle("Fusion");
    app.setStyleSheet(appStyleSheet());

    // Try to load app icon from assets directory next to executable
    QString iconPath = QCoreApplication::applicationDirPath() + "/../obc_controller/ui/assets/app_icon.png";
    if (QFile::exists(iconPath))
        app.setWindowIcon(QIcon(iconPath));

    // --- CLI argument parsing ---
    QCommandLineParser parser;
    parser.setApplicationDescription("OBC Charger Controller - CAN bus charger management");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption backendOpt(
        "can-backend",
        "CAN backend: auto|socketcan|pcan (default: auto)",
        "backend");
    parser.addOption(backendOpt);

    QCommandLineOption channelOpt(
        "can-channel",
        "CAN channel: e.g. can0, PCAN_USBBUS1 (default: OS-specific)",
        "channel");
    parser.addOption(channelOpt);

    QCommandLineOption bitrateOpt(
        "can-bitrate",
        "CAN bitrate: e.g. 250K, 500K, 1M (default: 250K)",
        "bitrate");
    parser.addOption(bitrateOpt);

    QCommandLineOption testOpt(
        "can-test",
        "Run CAN self-test (send one frame, listen 2s) then exit.");
    parser.addOption(testOpt);

    parser.process(app);

    // Load base settings from settings.json, then override with CLI args
    CanSettings canSettings = loadCanSettings();

    if (parser.isSet(backendOpt))
        canSettings.backend = parser.value(backendOpt).toStdString();
    if (parser.isSet(channelOpt))
        canSettings.channel = parser.value(channelOpt).toStdString();
    if (parser.isSet(bitrateOpt)) {
        int bps = parseBitrateString(parser.value(bitrateOpt).toStdString());
        if (bps > 0)
            canSettings.bitrate = bps;
    }

    canSettings.applyDefaults();

    // --- Self-test mode ---
    if (parser.isSet(testOpt)) {
        return runCanSelfTest(canSettings.backend, canSettings.channel, canSettings.bitrate);
    }

    // Store resolved CAN settings in QApplication properties for UI access
    app.setProperty("can_backend", QString::fromStdString(canSettings.backend));
    app.setProperty("can_channel", QString::fromStdString(canSettings.channel));
    app.setProperty("can_bitrate", canSettings.bitrate);

    MainWindow window;
    window.show();

    return app.exec();
}
