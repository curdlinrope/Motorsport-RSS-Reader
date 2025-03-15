#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QLabel>

#include "newsfeedwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAboutApp();
    void onThemeChange();
    void updateStatusMessage(const QString &message);

private:
    Ui::MainWindow *ui;
    NewsFeedWidget *m_feedWidget;
    bool m_darkThemeEnabled;
    
    void setupActions();
    void setupStatusBar();
    void loadStyleSheet(const QString &fileName);
    void saveSettings();
    void loadSettings();
};

#endif // MAINWINDOW_H 