#ifndef RSSPARSER_H
#define RSSPARSER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

struct FeedItem {
    QString title;
    QString link;
    QString description;
    QString pubDate;
    QString imageUrl;
    QString category;
    QString guid;
    bool isRead = false;
    QDateTime fetchTime = QDateTime::currentDateTime();
};

class RssParser : public QObject
{
    Q_OBJECT

public:
    explicit RssParser(QObject *parent = nullptr);
    ~RssParser();

    void fetchFeed(const QString &url);
    QList<FeedItem> getItems() const;
    void clearItems();
    
    // New methods for caching
    void saveFeedCache(const QString &feedUrl);
    bool loadFeedCache(const QString &feedUrl);
    static QString getCacheFilePath(const QString &feedUrl);
    void clearCache();
    
    // Set item as read
    void setItemAsRead(const QString &guid);
    
    // Retry mechanism
    void setMaxRetryAttempts(int attempts) { m_maxRetryAttempts = attempts; }
    int maxRetryAttempts() const { return m_maxRetryAttempts; }
    
    // Feeds management
    QHash<QString, QPair<QString, QString>> getFeeds() const;
    void addFeed(const QString &name, const QString &url, const QString &category);
    void removeFeed(const QString &name);
    
signals:
    void feedUpdated();
    void error(const QString &message);
    void newItemsAvailable(int count);
    void statusMessage(const QString &message);

private slots:
    void parseReply(QNetworkReply *reply);
    void retryFetchFeed();
    void handleNetworkTimeout();

private:
    QNetworkAccessManager *m_networkManager;
    QList<FeedItem> m_feedItems;
    QString m_currentUrl;
    int m_retryCount;
    int m_maxRetryAttempts;
    QTimer *m_retryTimer;
    QTimer *m_networkTimeoutTimer;
    QStringList m_processedGuids; // To track which items we've already processed
    QHash<QString, QPair<QString, QString>> m_feeds; // name -> (url, category)
    
    bool parseXml(QXmlStreamReader &xml);
    void parseItem(QXmlStreamReader &xml, FeedItem &item);
    void parseAtom(QXmlStreamReader &xml);
    void parseAtomEntry(QXmlStreamReader &xml, FeedItem &item);
    void processNewItems(const QList<FeedItem> &newItems);
    
    // Return caching directory
    QString getCacheDir() const;
    
    // Load and save feeds
    void loadSavedFeeds();
    void saveFeedsToSettings();
};

#endif // RSSPARSER_H 