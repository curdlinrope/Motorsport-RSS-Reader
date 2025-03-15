#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MotorsportRSS");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Motorsport");
    app.setOrganizationDomain("motorsportrss.example.com");
    app.setWindowIcon(QIcon(":/icons/motorsportrss.png"));
    
    // Command line parser for future extensions
    QCommandLineParser parser;
    parser.setApplicationDescription("Motorsport RSS Reader");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);
    
    MainWindow w;
    w.show();
    
    return app.exec();
} 