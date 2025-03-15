#ifndef NEWSFEEDWIDGET_H
#define NEWSFEEDWIDGET_H

#include <QWidget>
#include <QListView>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QSplitter>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QToolBar>
#include <QTabWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QSettings>
#include <QDialog>
#include <QTimer>
#include <QGroupBox>
#include <QSpinBox>

#include "rssfeedmodel.h"

class AddFeedDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AddFeedDialog(QWidget *parent = nullptr);
    
    QString feedName() const;
    QString feedUrl() const;
    QString feedCategory() const;
    
private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_urlEdit;
    QComboBox *m_categoryCombo;
    QPushButton *m_addButton;
    QPushButton *m_cancelButton;
    
    void setupUi();
};

class NewsFeedWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit NewsFeedWidget(QWidget *parent = nullptr);
    ~NewsFeedWidget();
    
    void setFeedUrl(const QString &url);
    void onRefreshClicked();
    void onSettingsClicked();
    
    // Get the active model for main window access
    RssFeedModel* getFeedModel() const { return m_model; }
    
public slots:
    void handleNewItemsNotification(int count, const QString &feedName);
    void handleFeedError(const QString &message);
    void handleStatusMessage(const QString &message);
    
signals:
    void statusMessageChanged(const QString &message);
    
private slots:
    void onItemSelected(const QModelIndex &index);
    void onOpenLinkClicked();
    void onFilterTextChanged(const QString &text);
    void onFeedSelectionChanged(int index);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowUnreadOnlyToggled(bool checked);
    void onCategoryFilterChanged(const QString &category);
    void onMarkAllReadClicked();
    void onMarkReadClicked();
    void onAddFeedClicked();
    void onRemoveFeedClicked();
    void onSaveArticleClicked();
    void onShareArticleClicked();
    
private:
    void setupUi();
    void setupTrayIcon();
    void setupCategoryCombo();
    void setupToolbars();
    void updateFeedSelector();
    void showNotification(const QString &title, const QString &message);
    void saveSettings();
    void loadSettings();
    
    // Models and views
    RssFeedModel *m_model;
    FeedFilterProxyModel *m_proxyModel;
    QListView *m_listView;
    QTextBrowser *m_detailView;
    
    // UI controls
    QSplitter *m_mainSplitter;
    QComboBox *m_feedSelector;
    QComboBox *m_categoryCombo;
    QCheckBox *m_unreadOnlyCheck;
    QPushButton *m_refreshButton;
    QPushButton *m_openLinkButton;
    QPushButton *m_markReadButton;
    QPushButton *m_markAllReadButton;
    QPushButton *m_saveButton;
    QPushButton *m_shareButton;
    QLineEdit *m_filterEdit;
    QLabel *m_categoryIcon;
    QLabel *m_statusLabel;
    QStackedWidget *m_stackedWidget;
    
    // Toolbar and actions
    QToolBar *m_mainToolbar;
    QToolBar *m_filterToolbar;
    QToolButton *m_addFeedButton;
    QToolButton *m_removeFeedButton;
    QToolButton *m_settingsButton;
    
    // Tray icon
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    
    // Current state
    QString m_currentLink;
    QString m_currentGuid;
    bool m_notificationsEnabled;
    bool m_autoRefreshEnabled;
    int m_autoRefreshInterval; // in minutes
    
    // Timer for auto-refresh
    QTimer *m_autoRefreshTimer;
};

#endif // NEWSFEEDWIDGET_H 