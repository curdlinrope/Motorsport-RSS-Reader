#include "rssparser.h"

#include <QNetworkRequest>
#include <QDebug>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QDomDocument>

RssParser::RssParser(QObject *parent) : QObject(parent), 
    m_retryCount(0), 
    m_maxRetryAttempts(3)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &RssParser::parseReply);
    
    // Initialize retry timer
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &RssParser::retryFetchFeed);
    
    // Initialize network timeout timer
    m_networkTimeoutTimer = new QTimer(this);
    m_networkTimeoutTimer->setSingleShot(true);
    connect(m_networkTimeoutTimer, &QTimer::timeout, this, &RssParser::handleNetworkTimeout);
    
    // Create cache directory if it doesn't exist
    QDir cacheDir(getCacheDir());
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
    
    // Load saved feeds
    loadSavedFeeds();
}

RssParser::~RssParser()
{
}

void RssParser::fetchFeed(const QString &url)
{
    // Reset retry counter
    m_retryCount = 0;
    m_currentUrl = url;
    
    // Try to load from cache first
    if (loadFeedCache(url)) {
        emit statusMessage(tr("Loaded from cache, fetching updates..."));
    } else {
        emit statusMessage(tr("Fetching feed..."));
    }
    
    // Make a network request
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MotorsportRSS Reader 1.0");
    
    // Add conditional GET headers if we have cached content
    QSettings settings;
    QString lastModifiedKey = "lastModified_" + url;
    QString etagKey = "etag_" + url;
    
    if (settings.contains(lastModifiedKey)) {
        request.setRawHeader("If-Modified-Since", settings.value(lastModifiedKey).toString().toUtf8());
    }
    
    if (settings.contains(etagKey)) {
        request.setRawHeader("If-None-Match", settings.value(etagKey).toString().toUtf8());
    }
    
    QNetworkReply *reply = m_networkManager->get(request);
    
    // Start timeout timer
    m_networkTimeoutTimer->start(15000); // 15 seconds timeout
    
    // Monitor for SSL errors
    connect(reply, &QNetworkReply::sslErrors, this, [this, reply](const QList<QSslError> &errors) {
        QString errorString;
        for (const QSslError &error : errors) {
            errorString += error.errorString() + "\n";
        }
        emit error(tr("SSL Error: %1").arg(errorString));
        reply->ignoreSslErrors(); // Proceed anyway, but user is informed
    });
}

QList<FeedItem> RssParser::getItems() const
{
    return m_feedItems;
}

void RssParser::clearItems()
{
    m_feedItems.clear();
    m_processedGuids.clear();
}

QString RssParser::getCacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/feeds";
}

QString RssParser::getCacheFilePath(const QString &feedUrl)
{
    // Create a hash of the feed URL to use as filename
    QString urlHash = QCryptographicHash::hash(feedUrl.toUtf8(), QCryptographicHash::Md5).toHex();
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/feeds/" + urlHash + ".json";
}

void RssParser::saveFeedCache(const QString &feedUrl)
{
    QFile file(getCacheFilePath(feedUrl));
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open cache file for writing:" << file.fileName();
        return;
    }
    
    QJsonArray feedArray;
    
    for (const FeedItem &item : m_feedItems) {
        QJsonObject itemObject;
        itemObject["title"] = item.title;
        itemObject["link"] = item.link;
        itemObject["description"] = item.description;
        itemObject["pubDate"] = item.pubDate;
        itemObject["imageUrl"] = item.imageUrl;
        itemObject["category"] = item.category;
        itemObject["guid"] = item.guid;
        itemObject["isRead"] = item.isRead;
        itemObject["fetchTime"] = item.fetchTime.toString(Qt::ISODate);
        
        feedArray.append(itemObject);
    }
    
    QJsonDocument doc(feedArray);
    file.write(doc.toJson());
    
    // Store cache metadata
    QSettings settings;
    settings.setValue("lastCacheUpdate_" + feedUrl, QDateTime::currentDateTime().toString(Qt::ISODate));
    
    qDebug() << "Feed cache saved to" << file.fileName();
}

bool RssParser::loadFeedCache(const QString &feedUrl)
{
    QFile file(getCacheFilePath(feedUrl));
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isArray()) {
        qWarning() << "Invalid cache file format:" << file.fileName();
        return false;
    }
    
    QJsonArray feedArray = doc.array();
    QList<FeedItem> cachedItems;
    
    for (const QJsonValue &value : feedArray) {
        QJsonObject obj = value.toObject();
        FeedItem item;
        
        item.title = obj["title"].toString();
        item.link = obj["link"].toString();
        item.description = obj["description"].toString();
        item.pubDate = obj["pubDate"].toString();
        item.imageUrl = obj["imageUrl"].toString();
        item.category = obj["category"].toString();
        item.guid = obj["guid"].toString();
        item.isRead = obj["isRead"].toBool();
        item.fetchTime = QDateTime::fromString(obj["fetchTime"].toString(), Qt::ISODate);
        
        if (!item.guid.isEmpty()) {
            m_processedGuids.append(item.guid);
        }
        
        cachedItems.append(item);
    }
    
    // If we have cached items, update our current items
    if (!cachedItems.isEmpty()) {
        m_feedItems = cachedItems;
        emit feedUpdated();
        
        // Check if cache is too old (more than 30 minutes)
        QSettings settings;
        QString lastUpdateKey = "lastCacheUpdate_" + feedUrl;
        if (settings.contains(lastUpdateKey)) {
            QDateTime lastUpdate = QDateTime::fromString(settings.value(lastUpdateKey).toString(), Qt::ISODate);
            if (lastUpdate.secsTo(QDateTime::currentDateTime()) > 1800) {
                qDebug() << "Cache is older than 30 minutes";
            }
        }
        
        return true;
    }
    
    return false;
}

void RssParser::setItemAsRead(const QString &guid)
{
    for (int i = 0; i < m_feedItems.size(); ++i) {
        if (m_feedItems[i].guid == guid) {
            m_feedItems[i].isRead = true;
            break;
        }
    }
    
    // Save the updated state
    saveFeedCache(m_currentUrl);
}

void RssParser::parseReply(QNetworkReply *reply)
{
    m_networkTimeoutTimer->stop();
    
    if (reply->error() == QNetworkReply::NoError || reply->error() == QNetworkReply::ContentNotFoundError) {
        // If we get a 304 Not Modified, the feed hasn't changed
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 304) {
            emit statusMessage(tr("Feed has not changed since last update"));
            reply->deleteLater();
            return;
        }
        
        // Extract and save cache headers
        QSettings settings;
        QVariant lastModified = reply->header(QNetworkRequest::LastModifiedHeader);
        QVariant etag = reply->rawHeader("ETag");
        
        if (lastModified.isValid()) {
            settings.setValue("lastModified_" + m_currentUrl, lastModified.toString());
        }
        
        if (!etag.toString().isEmpty()) {
            settings.setValue("etag_" + m_currentUrl, etag.toString());
        }
        
        // Save the original items for comparing later
        QList<FeedItem> oldItems = m_feedItems;
        QStringList oldGuids = m_processedGuids;
        
        // Try to parse the XML response
        QXmlStreamReader xml(reply);
        
        if (parseXml(xml)) {
            emit statusMessage(tr("Feed successfully updated"));
            
            // Check for new items and emit a signal if found
            QList<FeedItem> newItems;
            for (const FeedItem &item : m_feedItems) {
                if (!oldGuids.contains(item.guid)) {
                    newItems.append(item);
                }
            }
            
            if (!newItems.isEmpty()) {
                emit newItemsAvailable(newItems.size());
            }
            
            // Save feed cache
            saveFeedCache(m_currentUrl);
            
            emit feedUpdated();
        } else {
            // Try to interpret as Atom if RSS parsing failed
            xml.clear();
            xml.addData(reply->readAll());
            
            if (xml.atEnd() || xml.hasError()) {
                // If we can't read as Atom either, try a fallback approach with a DOM parser
                QString content = reply->readAll();
                QDomDocument doc;
                if (doc.setContent(content)) {
                    qDebug() << "Attempting to parse with DOM parser as a fallback";
                    // DOM parsing logic would be here if needed
                    // For this example we'll just use a simpler approach
                    
                    bool parsingWorked = false;
                    // Simple pattern-based extraction could be added here
                    
                    if (parsingWorked) {
                        emit statusMessage(tr("Feed parsed with fallback mechanism"));
                        emit feedUpdated();
                    } else {
                        emit error(tr("XML parsing error: %1").arg(xml.errorString()));
                        retryFetchFeed();
                    }
                } else {
                    emit error(tr("XML parsing error: %1").arg(xml.errorString()));
                    retryFetchFeed();
                }
            } else {
                parseAtom(xml);
                
                // Check for new items
                QList<FeedItem> newItems;
                for (const FeedItem &item : m_feedItems) {
                    if (!oldGuids.contains(item.guid)) {
                        newItems.append(item);
                    }
                }
                
                if (!newItems.isEmpty()) {
                    emit newItemsAvailable(newItems.size());
                }
                
                // Save feed cache
                saveFeedCache(m_currentUrl);
                
                emit statusMessage(tr("Feed parsed as Atom format"));
                emit feedUpdated();
            }
        }
    } else {
        emit error(tr("Network error: %1").arg(reply->errorString()));
        retryFetchFeed();
    }
    
    reply->deleteLater();
}

void RssParser::retryFetchFeed()
{
    if (m_retryCount < m_maxRetryAttempts) {
        m_retryCount++;
        int delayMs = 1000 * m_retryCount; // Exponential backoff
        
        emit statusMessage(tr("Retrying in %1 seconds (attempt %2/%3)...")
            .arg(delayMs / 1000)
            .arg(m_retryCount)
            .arg(m_maxRetryAttempts));
            
        m_retryTimer->start(delayMs);
    } else {
        emit statusMessage(tr("Failed after %1 attempts. Using cached data if available.").arg(m_maxRetryAttempts));
        
        // Try to load from cache as a fallback
        if (m_feedItems.isEmpty()) {
            loadFeedCache(m_currentUrl);
        }
    }
}

void RssParser::handleNetworkTimeout()
{
    emit error(tr("Network request timed out"));
    retryFetchFeed();
}

bool RssParser::parseXml(QXmlStreamReader &xml)
{
    bool foundItems = false;
    
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::StartElement) {
            // Check format type - RSS or Atom
            if (xml.name() == "rss" || xml.name() == "channel") {
                // Continue with RSS parsing
            } else if (xml.name() == "feed") {
                // This is an Atom feed - handle differently
                parseAtom(xml);
                return true;
            } else if (xml.name() == "item") {
                FeedItem item;
                parseItem(xml, item);
                
                // Only add if we have a valid item with a unique GUID
                if (!item.title.isEmpty() && !item.link.isEmpty()) {
                    // Generate a GUID if one wasn't provided
                    if (item.guid.isEmpty()) {
                        item.guid = QCryptographicHash::hash(
                            (item.link + item.title).toUtf8(), 
                            QCryptographicHash::Md5).toHex();
                    }
                    
                    // Check if we've already processed this item
                    if (!m_processedGuids.contains(item.guid)) {
                        m_feedItems.append(item);
                        m_processedGuids.append(item.guid);
                        foundItems = true;
                    }
                }
            }
        }
    }
    
    return !xml.hasError() && foundItems;
}

void RssParser::parseItem(QXmlStreamReader &xml, FeedItem &item)
{
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::EndElement && xml.name() == "item") {
            break;
        }
        
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "title") {
                item.title = xml.readElementText();
            } else if (xml.name() == "link") {
                item.link = xml.readElementText();
            } else if (xml.name() == "description") {
                item.description = xml.readElementText();
                
                // Try to extract image URL from description if it contains HTML
                if (item.description.contains("<img")) {
                    int imgStart = item.description.indexOf("<img");
                    int srcStart = item.description.indexOf("src=\"", imgStart) + 5;
                    if (srcStart > 5) {
                        int srcEnd = item.description.indexOf("\"", srcStart);
                        if (srcEnd > srcStart) {
                            item.imageUrl = item.description.mid(srcStart, srcEnd - srcStart);
                        }
                    }
                }
            } else if (xml.name() == "pubDate") {
                item.pubDate = xml.readElementText();
            } else if (xml.name() == "category") {
                item.category = xml.readElementText();
            } else if (xml.name() == "guid") {
                item.guid = xml.readElementText();
            } else if (xml.name() == "enclosure" && item.imageUrl.isEmpty()) {
                QXmlStreamAttributes attrs = xml.attributes();
                if (attrs.hasAttribute("url") && attrs.hasAttribute("type") && 
                    attrs.value("type").toString().startsWith("image/")) {
                    item.imageUrl = attrs.value("url").toString();
                }
            } else if ((xml.name() == "media:content" || xml.name() == "media:thumbnail") && 
                       item.imageUrl.isEmpty()) {
                QXmlStreamAttributes attrs = xml.attributes();
                if (attrs.hasAttribute("url")) {
                    item.imageUrl = attrs.value("url").toString();
                }
            }
        }
    }
}

// Parse Atom feed
void RssParser::parseAtom(QXmlStreamReader &xml)
{
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "entry") {
                FeedItem item;
                parseAtomEntry(xml, item);
                
                // Only add if we have a valid item with a unique ID
                if (!item.title.isEmpty() && !item.link.isEmpty()) {
                    // Generate a GUID if one wasn't provided
                    if (item.guid.isEmpty()) {
                        item.guid = QCryptographicHash::hash(
                            (item.link + item.title).toUtf8(), 
                            QCryptographicHash::Md5).toHex();
                    }
                    
                    // Check if we've already processed this item
                    if (!m_processedGuids.contains(item.guid)) {
                        m_feedItems.append(item);
                        m_processedGuids.append(item.guid);
                    }
                }
            }
        }
    }
}

// Parse an Atom entry
void RssParser::parseAtomEntry(QXmlStreamReader &xml, FeedItem &item)
{
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::EndElement && xml.name() == "entry") {
            break;
        }
        
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "title") {
                item.title = xml.readElementText();
            } else if (xml.name() == "link") {
                QXmlStreamAttributes attrs = xml.attributes();
                if (attrs.hasAttribute("href")) {
                    item.link = attrs.value("href").toString();
                } else {
                    item.link = xml.readElementText();
                }
            } else if (xml.name() == "id") {
                item.guid = xml.readElementText();
            } else if (xml.name() == "summary" || xml.name() == "content") {
                item.description = xml.readElementText();
                
                // Try to extract image URL from description if it contains HTML
                if (item.description.contains("<img")) {
                    int imgStart = item.description.indexOf("<img");
                    int srcStart = item.description.indexOf("src=\"", imgStart) + 5;
                    if (srcStart > 5) {
                        int srcEnd = item.description.indexOf("\"", srcStart);
                        if (srcEnd > srcStart) {
                            item.imageUrl = item.description.mid(srcStart, srcEnd - srcStart);
                        }
                    }
                }
            } else if (xml.name() == "published" || xml.name() == "updated") {
                // Only set pubDate if it's not already set
                if (item.pubDate.isEmpty()) {
                    item.pubDate = xml.readElementText();
                }
            } else if (xml.name() == "category") {
                QXmlStreamAttributes attrs = xml.attributes();
                if (attrs.hasAttribute("term")) {
                    item.category = attrs.value("term").toString();
                } else {
                    item.category = xml.readElementText();
                }
            }
        }
    }
}

void RssParser::processNewItems(const QList<FeedItem> &newItems)
{
    if (newItems.isEmpty()) {
        return;
    }
    
    // Add new items to the m_feedItems list
    m_feedItems.append(newItems);
    
    // Save the updated feed
    saveFeedCache(m_currentUrl);
    
    emit newItemsAvailable(newItems.size());
    emit feedUpdated();
}

// New methods for feed management
QHash<QString, QPair<QString, QString>> RssParser::getFeeds() const
{
    return m_feeds;
}

void RssParser::addFeed(const QString &name, const QString &url, const QString &category)
{
    if (name.isEmpty() || url.isEmpty()) {
        return;
    }
    
    m_feeds[name] = qMakePair(url, category);
    saveFeedsToSettings();
}

void RssParser::removeFeed(const QString &name)
{
    if (m_feeds.contains(name)) {
        m_feeds.remove(name);
        saveFeedsToSettings();
    }
}

void RssParser::loadSavedFeeds()
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
    }
}

void RssParser::saveFeedsToSettings()
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

void RssParser::clearCache()
{
    QDir cacheDir(getCacheDir());
    if (cacheDir.exists()) {
        // Remove all cache files
        QStringList files = cacheDir.entryList(QDir::Files);
        for (const QString &file : files) {
            cacheDir.remove(file);
        }
        
        // Clear memory cache
        m_feedItems.clear();
        m_processedGuids.clear();
        
        emit statusMessage(tr("Cache cleared successfully"));
    }
} 