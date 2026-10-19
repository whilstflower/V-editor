#include "vgeditor/main_window.hpp"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("V-Editor");
    QCoreApplication::setOrganizationName("V-Editor");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("library", "VG-88 SysEx library to open", "library.syx");
    parser.process(app);

    vgeditor::MainWindow window;
    window.show();
    if (!parser.positionalArguments().isEmpty()) {
        const auto path = parser.positionalArguments().front();
        QMetaObject::invokeMethod(&window, [&window, path] { window.openLibrary(path); },
            Qt::QueuedConnection);
    }

    return app.exec();
}