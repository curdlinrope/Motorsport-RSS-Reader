#include "newsfeedwidget.h"

#include <QDesktopServices>
#include <QUrl>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QBuffer>
#include <QPixmap>
#include <QScreen>

// Add Feed Dialog Implementation
AddFeedDialog::AddFeedDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setWindowTitle(tr("Add New Feed"));
    resize(400, 200);
}

void AddFeedDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QFormLayout *formLayout = new QFormLayout();
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Feed name (e.g. Formula 1 News)"));
    formLayout->addRow(tr("Name:"), m_nameEdit);
    
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("https://example.com/rss/feed.xml"));
    formLayout->addRow(tr("URL:"), m_urlEdit);
    
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems(QStringList() << "Formula 1" << "MotoGP" << "NASCAR" << "WRC" 
                              << "IndyCar" << "WEC" << "Formula E" << "DTM" << "All");
    formLayout->addRow(tr("Category:"), m_categoryCombo);
    
    mainLayout->addLayout(formLayout);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton(tr("Add"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    
    connect(m_addButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

QString AddFeedDialog::feedName() const
{
    return m_nameEdit->text().trimmed();
}

QString AddFeedDialog::feedUrl() const
{
    return m_urlEdit->text().trimmed();
}

QString AddFeedDialog::feedCategory() const
{
    return m_categoryCombo->currentText().trimmed();
}

// Custom delegate to display the feed items in a more attractive way
class FeedItemDelegate : public QStyledItemDelegate
{
public:
    explicit FeedItemDelegate(RssFeedModel *model, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), m_model(model) {}
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        if (!index.isValid())
            return;
        
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        
        // Get data from model
        QString title = index.data(RssFeedModel::TitleRole).toString();
        QString pubDate = index.data(RssFeedModel::PubDateRole).toString();
        QString category = index.data(RssFeedModel::CategoryRole).toString();
        QString imageUrl = index.data(RssFeedModel::ImageUrlRole).toString();
        bool isRead = index.data(RssFeedModel::IsReadRole).toBool();
        
        // Get category icon
        QString categoryIconPath = m_model->getCategoryIcon(category);
        QPixmap categoryIcon(categoryIconPath);
        
        // Background
        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, opt.palette.highlight());
            painter->setPen(opt.palette.highlightedText().color());
        } else {
            painter->fillRect(opt.rect, opt.state & QStyle::State_MouseOver 
                             ? QColor(240, 240, 240) : Qt::white);
            painter->setPen(Qt::black);
        }
        
        int padding = 10;
        int iconSize = 40;
        
        // Draw unread indicator
        if (!isRead) {
            QRect indicator(opt.rect.left() + 2, opt.rect.top() + (opt.rect.height() - 8) / 2, 4, 8);
            painter->fillRect(indicator, QColor(41, 128, 185)); // Blue indicator for unread
        }
        
        // Draw category icon
        if (!categoryIcon.isNull()) {
            QRect iconRect = QRect(opt.rect.left() + padding + (isRead ? 0 : 4), 
                                  opt.rect.top() + padding,
                                  iconSize, iconSize);
            painter->drawPixmap(iconRect, categoryIcon.scaled(iconSize, iconSize, 
                                                             Qt::KeepAspectRatio, 
                                                             Qt::SmoothTransformation));
        }
        
        // Format date
        QDateTime dateTime = QDateTime::fromString(pubDate, Qt::RFC2822Date);
        QString formattedDate = dateTime.isValid() 
                              ? dateTime.toString("dd MMM yyyy - hh:mm") 
                              : pubDate;
        
        // Draw title
        QFont titleFont = opt.font;
        titleFont.setBold(true);
        titleFont.setPointSize(10);
        if (isRead) {
            titleFont.setBold(false);
        }
        painter->setFont(titleFont);
        
        int leftMargin = padding + iconSize + padding + (isRead ? 0 : 4);
        QRect textRect = opt.rect.adjusted(leftMargin, padding, -padding, -padding);
        QRect titleRect = textRect;
        titleRect.setHeight(painter->fontMetrics().height());
        
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, 
                         painter->fontMetrics().elidedText(title, Qt::ElideRight, titleRect.width()));
        
        // Draw date and category
        QFont normalFont = opt.font;
        normalFont.setPointSize(8);
        painter->setFont(normalFont);
        
        QString catText = category.isEmpty() ? "" : " | " + category;
        QString dateCategory = formattedDate + catText;
        
        QRect dateRect = textRect;
        dateRect.setTop(titleRect.bottom() + 5);
        dateRect.setHeight(painter->fontMetrics().height());
        
        QColor dateColor = opt.state & QStyle::State_Selected 
                         ? opt.palette.highlightedText().color() 
                         : QColor(120, 120, 120);
        painter->setPen(dateColor);
        painter->drawText(dateRect, Qt::AlignLeft | Qt::AlignTop, dateCategory);
    }
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        return QSize(option.rect.width(), 70);
    }
    
private:
    RssFeedModel *m_model;
};

NewsFeedWidget::NewsFeedWidget(QWidget *parent) : QWidget(parent),
    m_notificationsEnabled(true),
    m_autoRefreshEnabled(true),
    m_autoRefreshInterval(30)
{
    // Create models
    m_model = new RssFeedModel(this);
    m_proxyModel = new FeedFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    
    // Connect model signals
    connect(m_model, &RssFeedModel::newItemsNotification, this, &NewsFeedWidget::handleNewItemsNotification);
    connect(m_model, &RssFeedModel::feedLoadError, this, &NewsFeedWidget::handleFeedError);
    connect(m_model, &RssFeedModel::statusMessage, this, &NewsFeedWidget::handleStatusMessage);
    connect(m_model, &RssFeedModel::feedsUpdated, this, &NewsFeedWidget::updateFeedSelector);
    
    // Setup UI
    setupUi();
    setupTrayIcon();
    loadSettings();
    
    // Create auto-refresh timer
    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &NewsFeedWidget::onRefreshClicked);
    
    if (m_autoRefreshEnabled) {
        m_autoRefreshTimer->start(m_autoRefreshInterval * 60 * 1000);
    }
    
    // Set default feed
    updateFeedSelector();
    if (m_feedSelector->count() > 0) {
        m_feedSelector->setCurrentIndex(0);
        onFeedSelectionChanged(0);
    }
}

NewsFeedWidget::~NewsFeedWidget()
{
    saveSettings();
    
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

void NewsFeedWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Setup toolbars
    setupToolbars();
    mainLayout->addWidget(m_mainToolbar);
    mainLayout->addWidget(m_filterToolbar);
    
    // Main content area with splitter
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    
    // List view for feed items
    m_listView = new QListView(this);
    m_listView->setUniformItemSizes(true);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setAlternatingRowColors(true);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new FeedItemDelegate(m_model, this));
    
    // Detail view for selected item
    QWidget *detailWidget = new QWidget(this);
    QVBoxLayout *detailLayout = new QVBoxLayout(detailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *detailHeaderLayout = new QHBoxLayout();
    m_categoryIcon = new QLabel(this);
    m_categoryIcon->setFixedSize(32, 32);
    
    QWidget *actionButtonsWidget = new QWidget(this);
    QHBoxLayout *actionButtonsLayout = new QHBoxLayout(actionButtonsWidget);
    actionButtonsLayout->setContentsMargins(0, 0, 0, 0);
    
    m_markReadButton = new QPushButton(tr("Mark as Read"), this);
    m_markReadButton->setEnabled(false);
    m_openLinkButton = new QPushButton(tr("Open in Browser"), this);
    m_openLinkButton->setEnabled(false);
    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setEnabled(false);
    m_shareButton = new QPushButton(tr("Share"), this);
    m_shareButton->setEnabled(false);
    
    actionButtonsLayout->addWidget(m_markReadButton);
    actionButtonsLayout->addWidget(m_openLinkButton);
    actionButtonsLayout->addWidget(m_saveButton);
    actionButtonsLayout->addWidget(m_shareButton);
    
    detailHeaderLayout->addWidget(m_categoryIcon);
    detailHeaderLayout->addStretch(1);
    detailHeaderLayout->addWidget(actionButtonsWidget);
    
    m_detailView = new QTextBrowser(this);
    m_detailView->setOpenLinks(false);
    
    detailLayout->addLayout(detailHeaderLayout);
    detailLayout->addWidget(m_detailView);
    
    // Connect action buttons
    connect(m_markReadButton, &QPushButton::clicked, this, &NewsFeedWidget::onMarkReadClicked);
    connect(m_openLinkButton, &QPushButton::clicked, this, &NewsFeedWidget::onOpenLinkClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &NewsFeedWidget::onSaveArticleClicked);
    connect(m_shareButton, &QPushButton::clicked, this, &NewsFeedWidget::onShareArticleClicked);
    
    // Set up the splitter
    m_mainSplitter->addWidget(m_listView);
    m_mainSplitter->addWidget(detailWidget);
    m_mainSplitter->setSizes(QList<int>() << 300 << 400);
    
    // Status bar at the bottom
    m_statusLabel = new QLabel(this);
    m_statusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_statusLabel->setText(tr("Ready"));
    
    // Add everything to the main layout
    mainLayout->addWidget(m_mainSplitter, 1);
    mainLayout->addWidget(m_statusLabel);
    
    // Connect signals/slots
    connect(m_listView, &QListView::clicked, this, &NewsFeedWidget::onItemSelected);
    connect(m_detailView, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        QDesktopServices::openUrl(url);
    });
}

void NewsFeedWidget::setupToolbars()
{
    // Main toolbar
    m_mainToolbar = new QToolBar(tr("Main Toolbar"), this);
    m_mainToolbar->setMovable(false);
    m_mainToolbar->setIconSize(QSize(24, 24));
    
    m_feedSelector = new QComboBox(this);
    m_feedSelector->setMinimumWidth(200);
    
    m_refreshButton = new QPushButton(this);
    m_refreshButton->setIcon(QIcon::fromTheme("view-refresh", 
                            QApplication::style()->standardIcon(QStyle::SP_BrowserReload)));
    m_refreshButton->setToolTip(tr("Refresh feed"));
    
    m_addFeedButton = new QToolButton(this);
    m_addFeedButton->setIcon(QIcon::fromTheme("list-add", 
                            QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder)));
    m_addFeedButton->setToolTip(tr("Add new feed"));
    
    m_removeFeedButton = new QToolButton(this);
    m_removeFeedButton->setIcon(QIcon::fromTheme("list-remove", 
                            QApplication::style()->standardIcon(QStyle::SP_TrashIcon)));
    m_removeFeedButton->setToolTip(tr("Remove selected feed"));
    
    m_settingsButton = new QToolButton(this);
    m_settingsButton->setIcon(QIcon::fromTheme("preferences-system", 
                            QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
    m_settingsButton->setToolTip(tr("Settings"));
    
    m_mainToolbar->addWidget(new QLabel(tr("Feed: ")));
    m_mainToolbar->addWidget(m_feedSelector);
    m_mainToolbar->addWidget(m_refreshButton);
    m_mainToolbar->addWidget(m_addFeedButton);
    m_mainToolbar->addWidget(m_removeFeedButton);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addWidget(m_settingsButton);
    
    // Filter toolbar
    m_filterToolbar = new QToolBar(tr("Filter Toolbar"), this);
    m_filterToolbar->setMovable(false);
    
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setMinimumWidth(120);
    setupCategoryCombo();
    
    m_unreadOnlyCheck = new QCheckBox(tr("Unread only"), this);
    
    QLabel *filterLabel = new QLabel(tr("Search:"), this);
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setPlaceholderText(tr("Search in feed..."));
    
    m_markAllReadButton = new QPushButton(tr("Mark All Read"), this);
    
    m_filterToolbar->addWidget(new QLabel(tr("Category: ")));
    m_filterToolbar->addWidget(m_categoryCombo);
    m_filterToolbar->addWidget(m_unreadOnlyCheck);
    m_filterToolbar->addWidget(filterLabel);
    m_filterToolbar->addWidget(m_filterEdit);
    m_filterToolbar->addWidget(m_markAllReadButton);
    
    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &NewsFeedWidget::onRefreshClicked);
    connect(m_feedSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewsFeedWidget::onFeedSelectionChanged);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &NewsFeedWidget::onFilterTextChanged);
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, &NewsFeedWidget::onCategoryFilterChanged);
    connect(m_unreadOnlyCheck, &QCheckBox::toggled, this, &NewsFeedWidget::onShowUnreadOnlyToggled);
    connect(m_markAllReadButton, &QPushButton::clicked, this, &NewsFeedWidget::onMarkAllReadClicked);
    connect(m_addFeedButton, &QToolButton::clicked, this, &NewsFeedWidget::onAddFeedClicked);
    connect(m_removeFeedButton, &QToolButton::clicked, this, &NewsFeedWidget::onRemoveFeedClicked);
    connect(m_settingsButton, &QToolButton::clicked, this, &NewsFeedWidget::onSettingsClicked);
}

void NewsFeedWidget::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/motorsportrss.png"), this);
    
    m_trayMenu = new QMenu(this);
    QAction *openAction = m_trayMenu->addAction(tr("Open MotorsportRSS"));
    m_trayMenu->addSeparator();
    QAction *refreshAction = m_trayMenu->addAction(tr("Refresh Feeds"));
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(tr("Quit"));
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    connect(openAction, &QAction::triggered, this, [this]() {
        window()->show();
        window()->raise();
        window()->activateWindow();
    });
    
    connect(refreshAction, &QAction::triggered, this, &NewsFeedWidget::onRefreshClicked);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &NewsFeedWidget::onTrayIconActivated);
    
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon->show();
    }
}

void NewsFeedWidget::setupCategoryCombo()
{
    m_categoryCombo->clear();
    m_categoryCombo->addItem(tr("All Categories"));
    
    QStringList categories = m_model->availableCategories();
    for (const QString &category : categories) {
        if (category != "All") {
            m_categoryCombo->addItem(category);
        }
    }
}

void NewsFeedWidget::updateFeedSelector()
{
    // Store the current selection
    QString currentFeed;
    if (m_feedSelector->currentIndex() >= 0) {
        currentFeed = m_feedSelector->currentText();
    }
    
    // Clear and rebuild
    m_feedSelector->blockSignals(true);
    m_feedSelector->clear();
    
    QHash<QString, QPair<QString, QString>> feeds = m_model->parser()->getFeeds();
    for (auto it = feeds.constBegin(); it != feeds.constEnd(); ++it) {
        m_feedSelector->addItem(it.key());
    }
    
    // Try to restore selection
    if (!currentFeed.isEmpty()) {
        int index = m_feedSelector->findText(currentFeed);
        if (index >= 0) {
            m_feedSelector->setCurrentIndex(index);
        }
    }
    
    m_feedSelector->blockSignals(false);
    
    // Update categories in filter combo
    setupCategoryCombo();
}

void NewsFeedWidget::setFeedUrl(const QString &url)
{
    m_model->setFeedUrl(url);
    m_statusLabel->setText(tr("Loading feed..."));
}

void NewsFeedWidget::onItemSelected(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    
    // Need to map through the proxy model to the source model
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    
    QString title = m_model->data(sourceIndex, RssFeedModel::TitleRole).toString();
    QString description = m_model->data(sourceIndex, RssFeedModel::DescriptionRole).toString();
    m_currentLink = m_model->data(sourceIndex, RssFeedModel::LinkRole).toString();
    m_currentGuid = m_model->data(sourceIndex, RssFeedModel::GuidRole).toString();
    QString pubDate = m_model->data(sourceIndex, RssFeedModel::PubDateRole).toString();
    QString category = m_model->data(sourceIndex, RssFeedModel::CategoryRole).toString();
    bool isRead = m_model->data(sourceIndex, RssFeedModel::IsReadRole).toBool();
    
    // Format date
    QDateTime dateTime = QDateTime::fromString(pubDate, Qt::RFC2822Date);
    QString formattedDate = dateTime.isValid() 
                          ? dateTime.toString("dd MMMM yyyy - hh:mm") 
                          : pubDate;
    
    // Set category icon
    QString categoryIconPath = m_model->getCategoryIcon(category);
    QPixmap pixmap(categoryIconPath);
    if (!pixmap.isNull()) {
        m_categoryIcon->setPixmap(pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_categoryIcon->clear();
    }
    
    // Prepare HTML content for the detail view
    QString htmlContent = QString(
        "<html>"
        "<head>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 0; padding: 10px; }"
        "h1 { font-size: 20px; color: #333; margin-top: 0; }"
        ".meta { color: #666; font-size: 12px; margin-bottom: 15px; }"
        ".content { line-height: 1.5; }"
        "a { color: #0066cc; text-decoration: none; }"
        "img { max-width: 100%; height: auto; margin: 10px 0; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>%1</h1>"
        "<div class='meta'>%2%3</div>"
        "<div class='content'>%4</div>"
        "</body>"
        "</html>"
    ).arg(
        title,
        formattedDate,
        category.isEmpty() ? "" : " | " + category,
        description
    );
    
    m_detailView->setHtml(htmlContent);
    m_openLinkButton->setEnabled(!m_currentLink.isEmpty());
    m_markReadButton->setEnabled(!isRead);
    m_saveButton->setEnabled(true);
    m_shareButton->setEnabled(true);
    
    // Mark item as read if it's not already
    if (!isRead) {
        m_model->markItemAsRead(sourceIndex);
    }
}

void NewsFeedWidget::onRefreshClicked()
{
    m_model->refresh();
    m_statusLabel->setText(tr("Refreshing feed..."));
}

void NewsFeedWidget::onOpenLinkClicked()
{
    if (!m_currentLink.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_currentLink));
    }
}

void NewsFeedWidget::onFilterTextChanged(const QString &text)
{
    m_proxyModel->setSearchText(text);
}

void NewsFeedWidget::onFeedSelectionChanged(int index)
{
    if (index >= 0 && index < m_feedSelector->count()) {
        QString feedName = m_feedSelector->itemText(index);
        QHash<QString, QPair<QString, QString>> feeds = m_model->parser()->getFeeds();
        if (feeds.contains(feedName)) {
            QString url = feeds[feedName].first;
            if (!url.isEmpty()) {
                setFeedUrl(url);
            }
        }
    }
}

void NewsFeedWidget::onShowUnreadOnlyToggled(bool checked)
{
    m_proxyModel->setShowUnreadOnly(checked);
}

void NewsFeedWidget::onCategoryFilterChanged(const QString &category)
{
    if (category == tr("All Categories")) {
        m_proxyModel->setFilterCategory("");
    } else {
        m_proxyModel->setFilterCategory(category);
    }
}

void NewsFeedWidget::onMarkAllReadClicked()
{
    m_model->markAllItemsAsRead();
    // Update buttons state if an item is selected
    if (m_listView->currentIndex().isValid()) {
        m_markReadButton->setEnabled(false);
    }
}

void NewsFeedWidget::onMarkReadClicked()
{
    QModelIndex current = m_listView->currentIndex();
    if (current.isValid()) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
        m_model->markItemAsRead(sourceIndex);
        m_markReadButton->setEnabled(false);
    }
}

void NewsFeedWidget::onAddFeedClicked()
{
    AddFeedDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.feedName();
        QString url = dialog.feedUrl();
        QString category = dialog.feedCategory();
        
        if (!name.isEmpty() && !url.isEmpty()) {
            m_model->addFeed(name, url, category);
            updateFeedSelector();
            
            // Select the newly added feed
            int index = m_feedSelector->findText(name);
            if (index >= 0) {
                m_feedSelector->setCurrentIndex(index);
            }
        }
    }
}

void NewsFeedWidget::onRemoveFeedClicked()
{
    QString currentFeed = m_feedSelector->currentText();
    if (!currentFeed.isEmpty()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, 
            tr("Remove Feed"),
            tr("Are you sure you want to remove the feed '%1'?").arg(currentFeed),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (result == QMessageBox::Yes) {
            m_model->removeFeed(currentFeed);
            updateFeedSelector();
        }
    }
}

void NewsFeedWidget::onSaveArticleClicked()
{
    if (m_currentLink.isEmpty()) {
        return;
    }
    
    QModelIndex current = m_listView->currentIndex();
    if (!current.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(current);
    QString title = m_model->data(sourceIndex, RssFeedModel::TitleRole).toString();
    QString content = m_model->data(sourceIndex, RssFeedModel::DescriptionRole).toString();
    
    // Sanitize the title for use as a filename
    QString sanitizedTitle = title;
    sanitizedTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Article"),
        QDir::homePath() + "/" + sanitizedTitle + ".html",
        tr("HTML Files (*.html);;Text Files (*.txt);;All Files (*)"));
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        if (fileName.endsWith(".txt", Qt::CaseInsensitive)) {
            // Save as plain text
            out << title << "\n\n";
            
            // Strip HTML
            QTextDocument doc;
            doc.setHtml(content);
            out << doc.toPlainText() << "\n\n";
            out << m_currentLink;
        } else {
            // Save as HTML
            out << "<!DOCTYPE html>\n";
            out << "<html>\n";
            out << "<head>\n";
            out << "    <meta charset=\"UTF-8\">\n";
            out << "    <title>" << title << "</title>\n";
            out << "    <style>\n";
            out << "        body { font-family: Arial, sans-serif; margin: 20px; }\n";
            out << "        h1 { color: #333; }\n";
            out << "        .source { margin-top: 20px; color: #666; }\n";
            out << "    </style>\n";
            out << "</head>\n";
            out << "<body>\n";
            out << "    <h1>" << title << "</h1>\n";
            out << "    <div class=\"content\">" << content << "</div>\n";
            out << "    <p class=\"source\">Source: <a href=\"" << m_currentLink << "\">" << m_currentLink << "</a></p>\n";
            out << "</body>\n";
            out << "</html>";
        }
        
        file.close();
        m_statusLabel->setText(tr("Article saved to %1").arg(fileName));
    } else {
        QMessageBox::warning(this, tr("Save Error"), tr("Could not save the file."));
    }
}

void NewsFeedWidget::onShareArticleClicked()
{
    if (m_currentLink.isEmpty()) {
        return;
    }
    
    // Copy link to clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_currentLink);
    
    m_statusLabel->setText(tr("Link copied to clipboard"));
}

void NewsFeedWidget::onSettingsClicked()
{
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle(tr("Settings"));
    settingsDialog.resize(400, 300);
    
    QVBoxLayout *layout = new QVBoxLayout(&settingsDialog);
    
    // Notification settings
    QGroupBox *notificationGroup = new QGroupBox(tr("Notifications"), &settingsDialog);
    QVBoxLayout *notificationLayout = new QVBoxLayout(notificationGroup);
    
    QCheckBox *enableNotifications = new QCheckBox(tr("Enable notifications for new articles"), &settingsDialog);
    enableNotifications->setChecked(m_notificationsEnabled);
    notificationLayout->addWidget(enableNotifications);
    
    // Auto-refresh settings
    QGroupBox *refreshGroup = new QGroupBox(tr("Auto-Refresh"), &settingsDialog);
    QVBoxLayout *refreshLayout = new QVBoxLayout(refreshGroup);
    
    QCheckBox *enableAutoRefresh = new QCheckBox(tr("Enable auto-refresh"), &settingsDialog);
    enableAutoRefresh->setChecked(m_autoRefreshEnabled);
    refreshLayout->addWidget(enableAutoRefresh);
    
    QHBoxLayout *intervalLayout = new QHBoxLayout();
    QLabel *intervalLabel = new QLabel(tr("Refresh interval (minutes):"), &settingsDialog);
    QSpinBox *intervalSpinBox = new QSpinBox(&settingsDialog);
    intervalSpinBox->setRange(1, 120);
    intervalSpinBox->setValue(m_autoRefreshInterval);
    intervalSpinBox->setEnabled(m_autoRefreshEnabled);
    
    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(intervalSpinBox);
    refreshLayout->addLayout(intervalLayout);
    
    connect(enableAutoRefresh, &QCheckBox::toggled, intervalSpinBox, &QSpinBox::setEnabled);
    
    // Cache settings
    QGroupBox *cacheGroup = new QGroupBox(tr("Cache"), &settingsDialog);
    QVBoxLayout *cacheLayout = new QVBoxLayout(cacheGroup);
    
    QPushButton *clearCacheButton = new QPushButton(tr("Clear Cache"), &settingsDialog);
    cacheLayout->addWidget(clearCacheButton);
    
    // Add everything to the main layout
    layout->addWidget(notificationGroup);
    layout->addWidget(refreshGroup);
    layout->addWidget(cacheGroup);
    
    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                      &settingsDialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &settingsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &settingsDialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    // Clear cache action
    connect(clearCacheButton, &QPushButton::clicked, [this]() {
        m_model->parser()->clearCache();
        m_statusLabel->setText(tr("Cache cleared"));
    });
    
    // Execute the dialog
    if (settingsDialog.exec() == QDialog::Accepted) {
        // Save settings
        m_notificationsEnabled = enableNotifications->isChecked();
        m_autoRefreshEnabled = enableAutoRefresh->isChecked();
        m_autoRefreshInterval = intervalSpinBox->value();
        
        // Update auto-refresh timer
        if (m_autoRefreshEnabled) {
            m_autoRefreshTimer->start(m_autoRefreshInterval * 60 * 1000);
        } else {
            m_autoRefreshTimer->stop();
        }
        
        saveSettings();
    }
}

void NewsFeedWidget::handleNewItemsNotification(int count, const QString &feedName)
{
    if (m_notificationsEnabled && count > 0) {
        QString title = tr("New Articles");
        QString message = tr("%1 new article(s) in %2").arg(count).arg(feedName);
        showNotification(title, message);
    }
}

void NewsFeedWidget::handleFeedError(const QString &message)
{
    m_statusLabel->setText(message);
    emit statusMessageChanged(message);
}

void NewsFeedWidget::handleStatusMessage(const QString &message)
{
    m_statusLabel->setText(message);
    emit statusMessageChanged(message);
}

void NewsFeedWidget::showNotification(const QString &title, const QString &message)
{
    if (QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
}

void NewsFeedWidget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        window()->show();
        window()->raise();
        window()->activateWindow();
    }
}

void NewsFeedWidget::saveSettings()
{
    QSettings settings;
    
    // Save window geometry
    settings.setValue("mainSplitterSizes", m_mainSplitter->saveState());
    
    // Save notification and refresh settings
    settings.setValue("notificationsEnabled", m_notificationsEnabled);
    settings.setValue("autoRefreshEnabled", m_autoRefreshEnabled);
    settings.setValue("autoRefreshInterval", m_autoRefreshInterval);
    
    // Save current feed
    settings.setValue("currentFeed", m_feedSelector->currentText());
}

void NewsFeedWidget::loadSettings()
{
    QSettings settings;
    
    // Load window geometry
    if (settings.contains("mainSplitterSizes")) {
        m_mainSplitter->restoreState(settings.value("mainSplitterSizes").toByteArray());
    }
    
    // Load notification and refresh settings
    m_notificationsEnabled = settings.value("notificationsEnabled", true).toBool();
    m_autoRefreshEnabled = settings.value("autoRefreshEnabled", true).toBool();
    m_autoRefreshInterval = settings.value("autoRefreshInterval", 30).toInt();
    
    // Restore selected feed
    QString lastFeed = settings.value("currentFeed").toString();
    if (!lastFeed.isEmpty()) {
        int index = m_feedSelector->findText(lastFeed);
        if (index >= 0) {
            m_feedSelector->setCurrentIndex(index);
        }
    }
} 