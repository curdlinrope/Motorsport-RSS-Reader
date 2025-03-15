#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFile>
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QSplashScreen>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_darkThemeEnabled(true) // Default to dark theme
{
    ui->setupUi(this);
    
    // Set window properties
    setWindowTitle(tr("Motorsport RSS Reader"));
    setWindowIcon(QIcon(":/icons/logo.png"));
    resize(1024, 768);
    
    // Apply dark theme by default
    loadStyleSheet(":/styles/dark.qss");
    
    // Create central widget
    m_feedWidget = new NewsFeedWidget(this);
    setCentralWidget(m_feedWidget);
    
    // Connect signals
    connect(m_feedWidget, &NewsFeedWidget::statusMessageChanged, this, &MainWindow::updateStatusMessage);
    
    // Setup menu actions and status bar
    setupActions();
    setupStatusBar();
    
    // Load saved window state
    loadSettings();
}

MainWindow::~MainWindow()
{
    // Save window state
    saveSettings();
    delete ui;
}

void MainWindow::setupActions()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QAction *refreshAction = fileMenu->addAction(tr("&Refresh"));
    refreshAction->setIcon(QIcon::fromTheme("view-refresh", 
                          QApplication::style()->standardIcon(QStyle::SP_BrowserReload)));
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, m_feedWidget, &NewsFeedWidget::onRefreshClicked);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction(tr("&Exit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    QAction *themeAction = viewMenu->addAction(tr("&Dark Theme"));
    themeAction->setCheckable(true);
    themeAction->setChecked(m_darkThemeEnabled);
    connect(themeAction, &QAction::triggered, this, &MainWindow::onThemeChange);
    
    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    
    QAction *settingsAction = toolsMenu->addAction(tr("&Settings"));
    settingsAction->setIcon(QIcon::fromTheme("preferences-system", 
                          QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
    connect(settingsAction, &QAction::triggered, m_feedWidget, &NewsFeedWidget::onSettingsClicked);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutApp);
    
    QAction *aboutQtAction = helpMenu->addAction(tr("About &Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    
    // Toolbar - using modern flat style
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(24, 24));
    toolBar->addAction(refreshAction);
    toolBar->addSeparator();
    toolBar->addAction(settingsAction);
    toolBar->addSeparator();
    toolBar->addAction(themeAction);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::onAboutApp()
{
    QPixmap logo(":/icons/logo.png");
    
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(tr("About Motorsport RSS Reader"));
    aboutBox.setIconPixmap(logo.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    aboutBox.setText(tr("<h2>Motorsport RSS Reader</h2>"
                       "<p><b>Version 1.0.0</b></p>"));
    aboutBox.setInformativeText(tr("An application for reading motorsport news feeds from various sources."
                                  "<p>Stay updated with the latest news from Formula 1, MotoGP, NASCAR, WRC, and more.</p>"
                                  "<p>Copyright &copy; %1</p>"
                                  "<p><a href='https://github.com/curdlinrope/Motorsport-RSS-Reader'>GitHub Repository</a></p>")
                               .arg(QDate::currentDate().year()));
    
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void MainWindow::onThemeChange()
{
    m_darkThemeEnabled = !m_darkThemeEnabled;
    
    if (m_darkThemeEnabled) {
        loadStyleSheet(":/styles/dark.qss");
    } else {
        qApp->setStyleSheet("");
    }
}

void MainWindow::loadStyleSheet(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
}

void MainWindow::updateStatusMessage(const QString &message)
{
    statusBar()->showMessage(message);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // Save window state
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());
    settings.setValue("mainWindow/darkThemeEnabled", m_darkThemeEnabled);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Restore window state
    if (settings.contains("mainWindow/geometry")) {
        restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    }
    
    if (settings.contains("mainWindow/windowState")) {
        restoreState(settings.value("mainWindow/windowState").toByteArray());
    }
    
    // Load theme setting
    if (settings.contains("mainWindow/darkThemeEnabled")) {
        m_darkThemeEnabled = settings.value("mainWindow/darkThemeEnabled").toBool();
        if (m_darkThemeEnabled) {
            loadStyleSheet(":/styles/dark.qss");
        } else {
            qApp->setStyleSheet("");
        }
    }
} 