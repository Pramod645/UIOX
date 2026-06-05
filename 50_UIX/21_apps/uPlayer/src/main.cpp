#include "MainWindow.h"
#include "StyleSheet.h"
#include <QApplication>
#include <QFont>

int main(int argc, char* argv[])
{
    /* High-DPI support */
    QApplication::setAttribute(
        Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName   ("UIX Player");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName  ("UIXPlayer");

    /* Dark theme */
    app.setStyle("Fusion");
    app.setStyleSheet(UIXPlayer::appStyleSheet());

    /* Base font */
    QFont font;
#ifdef Q_OS_MACOS
    font.setFamily("SF Pro Display");
    font.setPointSize(13);
#elif defined(Q_OS_ANDROID)
    font.setFamily("Roboto");
    font.setPointSize(11);
#else
    font.setFamily("Segoe UI");
    font.setPointSize(10);
#endif
    app.setFont(font);

    UIXPlayer::MainWindow window;

#ifdef Q_OS_ANDROID
    window.showMaximized();
#else
    window.resize(1000, 650);
    window.show();
#endif

    return app.exec();
}
