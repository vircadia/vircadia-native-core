//
//  AssetScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AssetScriptingInterface_h
#define hifi_AssetScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptable>
#include <AssetClient.h>
#include <NetworkAccessManager.h>
#include <BaseAssetScriptingInterface.h>
#include <QtNetwork/QNetworkDiskCache>

/**jsdoc
 * @namespace Assets
 */
class AssetScriptingInterface : public BaseAssetScriptingInterface, QScriptable {
    Q_OBJECT
public:
    AssetScriptingInterface(QObject* parent = nullptr);

    /**jsdoc
     * Upload content to the connected domain's asset server.
     * @function Assets.uploadData
     * @static
     * @param data {string} content to upload
     * @param callback {Assets~uploadDataCallback} called when upload is complete
     */

    /**jsdoc
     * Called when uploadData is complete
     * @callback Assets~uploadDataCallback
     * @param {string} url
     * @param {string} hash
     */

    Q_INVOKABLE void uploadData(QString data, QScriptValue callback);

    /**jsdoc
     * Download data from the connected domain's asset server.
     * @function Assets.downloadData
     * @static
     * @param url {string} url of asset to download, must be atp scheme url.
     * @param callback {Assets~downloadDataCallback}
     */

    /**jsdoc
     * Called when downloadData is complete
     * @callback Assets~downloadDataCallback
     * @param data {string} content that was downloaded
     */

    Q_INVOKABLE void downloadData(QString url, QScriptValue downloadComplete);

    /**jsdoc
     * Sets up a path to hash mapping within the connected domain's asset server
     * @function Assets.setMapping
     * @static
     * @param path {string}
     * @param hash {string}
     * @param callback {Assets~setMappingCallback}
     */

    /**jsdoc
     * Called when setMapping is complete
     * @callback Assets~setMappingCallback
     * @param {string} error
     */
    Q_INVOKABLE void setMapping(QString path, QString hash, QScriptValue callback);

    /**jsdoc
     * Look up a path to hash mapping within the connected domain's asset server
     * @function Assets.getMapping
     * @static
     * @param path {string}
     * @param callback {Assets~getMappingCallback}
     */

    /**jsdoc
     * Called when getMapping is complete.
     * @callback Assets~getMappingCallback
     * @param error {string} error description if the path could not be resolved; otherwise a null value.
     * @param assetID {string} hash value if found, else an empty string
     */
    Q_INVOKABLE void getMapping(QString path, QScriptValue callback);

    Q_INVOKABLE void setBakingEnabled(QString path, bool enabled, QScriptValue callback);

#if (PR_BUILD || DEV_BUILD)
    Q_INVOKABLE void sendFakedHandshake();
#endif

    // Advanced APIs
    // getAsset(options, scope[callback(error, result)]) -- fetches an Asset from the Server
    //   [options.url] an "atp:" style URL, hash, or relative mapped path to fetch
    //   [options.responseType] the desired reponse type (text | arraybuffer | json)
    //   [options.decompress] whether to apply gunzip decompression on the stream
    //   [scope[callback]] continuation-style (error, { responseType, data, byteLength, ... }) callback
    const QStringList RESPONSE_TYPES{ "text", "arraybuffer", "json" };
    Q_INVOKABLE void getAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    // putAsset(options, scope[callback(error, result)]) -- upload a new Aset to the Server
    //   [options.data] -- (ArrayBuffer|String)
    //   [options.compress] -- (true|false)
    //   [options.path=undefined] -- option path mapping to set on the created hash result
    //   [
    Q_INVOKABLE void putAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void deleteAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void resolveAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void decompressData(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void compressData(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

protected:
    void jsCallback(const QScriptValue& handler, const QScriptValue& error, const QVariantMap& result);
    void jsCallback(const QScriptValue& handler, const QScriptValue& error, const QScriptValue& result);
    bool jsAssert(bool condition, const QString& error);
};

#endif // hifi_AssetScriptingInterface_h
