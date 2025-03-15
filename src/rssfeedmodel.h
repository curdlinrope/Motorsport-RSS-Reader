#ifndef RSSFEEDMODEL_H
#define RSSFEEDMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include "rssparser.h"

// Custom sort filter model for filtering by category and read status
class FeedFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    
public:
    explicit FeedFilterProxyModel(QObject *parent = nullptr);
    
    // Filter settings
    void setFilterCategory(const QString &category);
    void setShowUnreadOnly(bool unreadOnly);
    void setSearchText(const QString &text);
    
    QString filterCategory() const { return m_filterCategory; }
    bool showUnreadOnly() const { return m_showUnreadOnly; }
    QString searchText() const { return m_searchText; }
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    
private:
    QString m_filterCategory;
    bool m_showUnreadOnly;
    QString m_searchText;
};

class RssFeedModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum FeedRoles {
        TitleRole = Qt::UserRole + 1,
        LinkRole,
        DescriptionRole,
        PubDateRole,
        ImageUrlRole,
        CategoryRole,
        IsReadRole,
        GuidRole
    };

    explicit RssFeedModel(QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;
    
    RssParser* parser() const { return m_parser; }
    
    void setFeedUrl(const QString &url);
    void refresh();
    QString getCategoryIcon(const QString &category) const;
    
    // New methods for enhanced functionality
    void markItemAsRead(const QModelIndex &index);
    void markAllItemsAsRead();
    void setFeedCategory(const QString &category) { m_currentCategory = category; }
    QString feedCategory() const { return m_currentCategory; }
    QStringList availableCategories() const;
    
    // Add/remove feeds
    void addFeed(const QString &name, const QString &url, const QString &category);
    void removeFeed(const QString &name);
    
public slots:
    void handleNewItems(int count);
    
signals:
    void feedsUpdated();
    void newItemsNotification(int count, const QString &feedName);
    void feedLoadError(const QString &message);
    void statusMessage(const QString &message);
    
private slots:
    void onFeedUpdated();
    void onError(const QString &message);
    void onStatusMessage(const QString &message);
    
private:
    RssParser *m_parser;
    QString m_currentFeedUrl;
    QString m_currentFeedName;
    QString m_currentCategory;
    QHash<QString, QString> m_categoryIcons;
    QHash<QString, QPair<QString, QString>> m_feeds; // name -> (url, category)
    
    void setupCategoryIcons();
    void loadSavedFeeds();
    void saveFeedsToSettings();
};

#endif // RSSFEEDMODEL_H 