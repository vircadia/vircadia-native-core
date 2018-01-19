//
//  BaseAssetScriptingInterface.h
//  libraries/networking/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// BaseAssetScriptingInterface contains the engine-agnostic support code that can be used from
// both QML JS and QScriptEngine JS engine implementations

#ifndef hifi_BaseAssetScriptingInterface_h
#define hifi_BaseAssetScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QThread>
#include "AssetClient.h"
#include <shared/MiniPromises.h>
#include "NetworkAccessManager.h"
#include <QtNetwork/QNetworkDiskCache>

/**jsdoc
 * @namespace Assets
 */
class BaseAssetScriptingInterface : public QObject {
    Q_OBJECT
public:
    using Promise = MiniPromise::Promise;
    QSharedPointer<AssetClient> assetClient();

    BaseAssetScriptingInterface(QObject* parent = nullptr);

public slots:
    Promise getCacheStatus();

    /**jsdoc
     * Initialize the disk cache (returns true if already initialized)
     * @function Assets.initializeCache
     * @static
     */
    bool initializeCache();

    virtual bool isValidPath(QString input) { return AssetUtils::isValidPath(input); }
    virtual bool isValidFilePath(QString input) { return AssetUtils::isValidFilePath(input); }
    QUrl getATPUrl(QString input) { return AssetUtils::getATPUrl(input); }
    QString extractAssetHash(QString input) { return AssetUtils::extractAssetHash(input); }
    bool isValidHash(QString input) { return AssetUtils::isValidHash(input); }
    QByteArray hashData(const QByteArray& data) { return AssetUtils::hashData(data); }
    QString hashDataHex(const QByteArray& data) { return hashData(data).toHex(); }

    virtual Promise queryCacheMeta(const QUrl& url);
    virtual Promise loadFromCache(const QUrl& url);
    virtual Promise saveToCache(const QUrl& url, const QByteArray& data, const QVariantMap& metadata = QVariantMap());

protected:
    QString _cacheDirectory;
    const QString NoError{};
    //virtual bool jsAssert(bool condition, const QString& error) = 0;
    Promise loadAsset(QString asset, bool decompress, QString responseType);
    Promise getAssetInfo(QString asset);
    Promise downloadBytes(QString hash);
    Promise uploadBytes(const QByteArray& bytes);
    Promise compressBytes(const QByteArray& bytes, int level = -1);
    Promise convertBytes(const QByteArray& dataByteArray, const QString& responseType);
    Promise decompressBytes(const QByteArray& bytes);
    Promise symlinkAsset(QString hash, QString path);
};
#endif // hifi_BaseAssetScriptingInterface_h
