#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MotorsportRSS");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Motorsport");
    app.setOrganizationDomain("motorsportrss.example.com");
    app.setWindowIcon(QIcon(":/icons/logo.png"));
    
    // Show splash screen
    QPixmap splashPixmap(":/icons/logo.png");
    QSplashScreen splash(splashPixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    splash.show();
    app.processEvents();
    
    // Simulate loading time
    splash.showMessage("Loading application...", Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    
    // Command line parser for future extensions
    QCommandLineParser parser;
    parser.setApplicationDescription("Motorsport RSS Reader");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);
    
    // Create main window but don't show it immediately
    MainWindow w;
    
    // Allow splash to be visible for at least 1.5 seconds
    QThread::msleep(1500);
    
    // Show main window and close splash
    w.show();
    splash.finish(&w);
    
    return app.exec();
} 