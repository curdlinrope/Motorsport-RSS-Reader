#include "rssfeedmodel.h"
#include <QDebug>
#include <QSettings>
#include <QRegularExpression>

// FilterProxyModel implementation
FeedFilterProxyModel::FeedFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent),
      m_showUnreadOnly(false)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void FeedFilterProxyModel::setFilterCategory(const QString &category)
{
    if (m_filterCategory != category) {
        m_filterCategory = category;
        invalidateFilter();
    }
}

void FeedFilterProxyModel::setShowUnreadOnly(bool unreadOnly)
{
    if (m_showUnreadOnly != unreadOnly) {
        m_showUnreadOnly = unreadOnly;
        invalidateFilter();
    }
}

void FeedFilterProxyModel::setSearchText(const QString &text)
{
    if (m_searchText != text) {
        m_searchText = text;
        invalidateFilter();
    }
}

bool FeedFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    
    // Check category filter
    if (!m_filterCategory.isEmpty()) {
        QString category = sourceModel()->data(index, RssFeedModel::CategoryRole).toString();
        if (!category.contains(m_filterCategory, Qt::CaseInsensitive)) {
            return false;
        }
    }
    
    // Check unread filter
    if (m_showUnreadOnly) {
        bool isRead = sourceModel()->data(index, RssFeedModel::IsReadRole).toBool();
        if (isRead) {
            return false;
        }
    }
    
    // Check text search
    if (!m_searchText.isEmpty()) {
        QString title = sourceModel()->data(index, RssFeedModel::TitleRole).toString();
        QString description = sourceModel()->data(index, RssFeedModel::DescriptionRole).toString();
        QString category = sourceModel()->data(index, RssFeedModel::CategoryRole).toString();
        
        return title.contains(m_searchText, Qt::CaseInsensitive) ||
               description.contains(m_searchText, Qt::CaseInsensitive) ||
               category.contains(m_searchText, Qt::CaseInsensitive);
    }
    
    return true;
}

// RssFeedModel implementation
RssFeedModel::RssFeedModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_parser = new RssParser(this);
    
    connect(m_parser, &RssParser::feedUpdated, this, &RssFeedModel::onFeedUpdated);
    connect(m_parser, &RssParser::error, this, &RssFeedModel::onError);
    connect(m_parser, &RssParser::statusMessage, this, &RssFeedModel::onStatusMessage);
    connect(m_parser, &RssParser::newItemsAvailable, this, &RssFeedModel::handleNewItems);
    
    setupCategoryIcons();
    loadSavedFeeds();
}

void RssFeedModel::setupCategoryIcons()
{
    // Map common motorsport categories to their respective logo resources
    m_categoryIcons.insert("Formula 1", ":/logos/f1.png");
    m_categoryIcons.insert("F1", ":/logos/f1.png");
    m_categoryIcons.insert("Formula1", ":/logos/f1.png");
    
    m_categoryIcons.insert("MotoGP", ":/logos/motogp.png");
    m_categoryIcons.insert("Moto GP", ":/logos/motogp.png");
    
    m_categoryIcons.insert("WRC", ":/logos/wrc.png");
    m_categoryIcons.insert("World Rally Championship", ":/logos/wrc.png");
    m_categoryIcons.insert("Rally", ":/logos/wrc.png");
    
    m_categoryIcons.insert("NASCAR", ":/logos/nascar.png");
    
    m_categoryIcons.insert("IndyCar", ":/logos/indycar.png");
    m_categoryIcons.insert("Indy Car", ":/logos/indycar.png");
    
    m_categoryIcons.insert("WEC", ":/logos/wec.png");
    m_categoryIcons.insert("World Endurance Championship", ":/logos/wec.png");
    m_categoryIcons.insert("Endurance", ":/logos/wec.png");
    m_categoryIcons.insert("Le Mans", ":/logos/wec.png");
    
    m_categoryIcons.insert("Formula E", ":/logos/formula-e.png");
    m_categoryIcons.insert("FormulaE", ":/logos/formula-e.png");
    
    m_categoryIcons.insert("DTM", ":/logos/dtm.png");
    m_categoryIcons.insert("Super GT", ":/logos/dtm.png");
    m_categoryIcons.insert("Touring Car", ":/logos/dtm.png");
}

void RssFeedModel::loadSavedFeeds()
{
    QSettings settings;
    int size = settings.beginReadArray("feeds");
    
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
        QString url = settings.value("url").toString();
        QString category = settings.value("category").toString();
        
        if (!name.isEmpty() && !url.isEmpty()) {
            m_feeds[name] = qMakePair(url, category);
        }
    }
    
    settings.endArray();
    
    // Add default feeds if none exist
    if (m_feeds.isEmpty()) {
        addFeed("Motorsport.com", "https://www.motorsport.com/rss/all/", "All");
        addFeed("Autosport", "https://www.autosport.com/rss/feed/all", "All");
        addFeed("F1 News", "https://www.motorsport.com/rss/f1/news/", "Formula 1");
        addFeed("MotoGP News", "https://www.motorsport.com/rss/motogp/news/", "MotoGP");
        addFeed("NASCAR News", "https://www.motorsport.com/rss/nascar/news/", "NASCAR");
        addFeed("WRC News", "https://www.motorsport.com/rss/wrc/news/", "WRC");
        addFeed("Formula E News", "https://www.motorsport.com/rss/formula-e/news/", "Formula E");
        addFeed("WEC News", "https://www.motorsport.com/rss/wec/news/", "WEC");
        addFeed("IMSA News", "https://www.motorsport.com/rss/imsa/news/", "IMSA");
        addFeed("IndyCar News", "https://www.motorsport.com/rss/indycar/news/", "IndyCar");
        addFeed("Super Formula News", "https://www.motorsport.com/rss/superformula/news/", "Super Formula");
    }
    
    emit feedsUpdated();
}

void RssFeedModel::saveFeedsToSettings()
{
    QSettings settings;
    settings.beginWriteArray("feeds");
    
    int i = 0;
    for (auto it = m_feeds.constBegin(); it != m_feeds.constEnd(); ++it, ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", it.key());
        settings.setValue("url", it.value().first);
        settings.setValue("category", it.value().second);
    }
    
    settings.endArray();
}

void RssFeedModel::addFeed(const QString &name, const QString &url, const QString &category)
{
    if (name.isEmpty() || url.isEmpty()) {
        return;
    }
    
    m_feeds[name] = qMakePair(url, category);
    saveFeedsToSettings();
    emit feedsUpdated();
}

void RssFeedModel::removeFeed(const QString &name)
{
    if (m_feeds.contains(name)) {
        m_feeds.remove(name);
        saveFeedsToSettings();
        emit feedsUpdated();
    }
}

QStringList RssFeedModel::availableCategories() const
{
    QStringList categories;
    categories.append("All"); // Always include "All" category
    
    // Collect all unique categories from feeds
    for (auto it = m_feeds.constBegin(); it != m_feeds.constEnd(); ++it) {
        QString category = it.value().second;
        if (!category.isEmpty() && !categories.contains(category)) {
            categories.append(category);
        }
    }
    
    categories.sort();
    return categories;
}

int RssFeedModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    
    return m_parser->getItems().count();
}

QVariant RssFeedModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_parser->getItems().count())
        return QVariant();
    
    // Get a copy to avoid dangling reference
    FeedItem item = m_parser->getItems().at(index.row());
    
    switch (role) {
    case TitleRole:
        return item.title;
    case LinkRole:
        return item.link;
    case DescriptionRole:
        return item.description;
    case PubDateRole:
        return item.pubDate;
    case ImageUrlRole:
        return item.imageUrl;
    case CategoryRole:
        return item.category;
    case IsReadRole:
        return item.isRead;
    case GuidRole:
        return item.guid;
    default:
        return QVariant();
    }
}

bool RssFeedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_parser->getItems().count())
        return false;
    
    if (role == IsReadRole) {
        QString guid = data(index, GuidRole).toString();
        if (!guid.isEmpty()) {
            m_parser->setItemAsRead(guid);
            emit dataChanged(index, index, {IsReadRole});
            return true;
        }
    }
    
    return false;
}

QHash<int, QByteArray> RssFeedModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[LinkRole] = "link";
    roles[DescriptionRole] = "description";
    roles[PubDateRole] = "pubDate";
    roles[ImageUrlRole] = "imageUrl";
    roles[CategoryRole] = "category";
    roles[IsReadRole] = "isRead";
    roles[GuidRole] = "guid";
    return roles;
}

void RssFeedModel::setFeedUrl(const QString &url)
{
    m_currentFeedUrl = url;
    
    // Find the feed name
    m_currentFeedName = "";
    m_currentCategory = "";
    
    for (auto it = m_feeds.constBegin(); it != m_feeds.constEnd(); ++it) {
        if (it.value().first == url) {
            m_currentFeedName = it.key();
            m_currentCategory = it.value().second;
            break;
        }
    }
    
    refresh();
}

void RssFeedModel::refresh()
{
    if (!m_currentFeedUrl.isEmpty()) {
        m_parser->fetchFeed(m_currentFeedUrl);
    }
}

void RssFeedModel::onFeedUpdated()
{
    beginResetModel();
    // The data is already updated in the parser
    endResetModel();
}

void RssFeedModel::onError(const QString &message)
{
    qWarning() << "Feed error:" << message;
    emit feedLoadError(message);
}

void RssFeedModel::onStatusMessage(const QString &message)
{
    emit statusMessage(message);
}

void RssFeedModel::handleNewItems(int count)
{
    if (count > 0) {
        emit newItemsNotification(count, m_currentFeedName);
    }
}

QString RssFeedModel::getCategoryIcon(const QString &category) const
{
    // Search for a matching category using case-insensitive comparison
    for (auto it = m_categoryIcons.constBegin(); it != m_categoryIcons.constEnd(); ++it) {
        if (category.contains(it.key(), Qt::CaseInsensitive)) {
            return it.value();
        }
    }
    
    // Default icon if no matching category is found
    return ":/icons/motorsportrss.png";
}

void RssFeedModel::markItemAsRead(const QModelIndex &index)
{
    setData(index, true, IsReadRole);
}

void RssFeedModel::markAllItemsAsRead()
{
    for (int i = 0; i < rowCount(); i++) {
        QModelIndex idx = index(i, 0);
        if (!data(idx, IsReadRole).toBool()) {
            setData(idx, true, IsReadRole);
        }
    }
} 