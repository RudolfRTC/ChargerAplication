#include <QApplication>
#include <QIcon>
#include <QDir>

#include "ui/main_window.h"
#include "ui/theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OBC Charger Controller");
    app.setStyle("Fusion");
    app.setStyleSheet(appStyleSheet());

    // Try to load app icon from assets directory next to executable
    QString iconPath = QCoreApplication::applicationDirPath() + "/../obc_controller/ui/assets/app_icon.png";
    if (QFile::exists(iconPath))
        app.setWindowIcon(QIcon(iconPath));

    MainWindow window;
    window.show();

    return app.exec();
}
