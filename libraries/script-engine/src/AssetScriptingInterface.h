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
#include <BaseScriptEngine.h>
#include <QtNetwork/QNetworkDiskCache>

/**jsdoc
 * @namespace Assets
 */
class AssetScriptingInterface : public BaseAssetScriptingInterface, QScriptable {
    Q_OBJECT
public:
    using Parent = BaseAssetScriptingInterface;
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

    /**jsdoc
     * Request Asset data from the ATP Server
     * @function Assets.getAsset
     * @param {URL|Assets.GetOptions} options An atp: style URL, hash, or relative mapped path; or an {@link Assets.GetOptions} object with request parameters
     * @param {Assets~getAssetCallback} scope[callback] A scope callback function to receive (error, results) values
     */
   /**jsdoc
    * A set of properties that can be passed to {@link Assets.getAsset}.
    * @typedef {Object} Assets.GetOptions
    * @property {URL} [url] an "atp:" style URL, hash, or relative mapped path to fetch
    * @property {string} [responseType=text] the desired reponse type (text | arraybuffer | json)
    * @property {boolean} [decompress=false] whether to attempt gunzip decompression on the fetched data
    *    See: {@link Assets.putAsset} and its .compress=true option
    */
    /**jsdoc
     * Called when Assets.getAsset is complete.
     * @callback Assets~getAssetCallback
     * @param {string} error - contains error message or null value if no error occured fetching the asset
     * @param {Asset~getAssetResult} result - result object containing, on success containing asset metadata and contents
     */
   /**jsdoc
    * Result value returned by {@link Assets.getAsset}.
    * @typedef {Object} Assets~getAssetResult
    * @property {url} [url] the resolved "atp:" style URL for the fetched asset
    * @property {string} [hash] the resolved hash for the fetched asset
    * @property {string|ArrayBuffer|Object} [response] response data (possibly converted per .responseType value)
    * @property {string} [responseType] response type (text | arraybuffer | json)
    * @property {string} [contentType] detected asset mime-type (autodetected)
    * @property {number} [byteLength] response data size in bytes
    * @property {number} [decompressed] flag indicating whether data was decompressed
    */
    Q_INVOKABLE void getAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    /**jsdoc
     * Upload Asset data to the ATP Server
     * @function Assets.putAsset
     * @param {Assets.PutOptions} options A PutOptions object with upload parameters
     * @param {Assets~putAssetCallback} scope[callback] A scoped callback function invoked with (error, results)
     */
   /**jsdoc
    * A set of properties that can be passed to {@link Assets.putAsset}.
    * @typedef {Object} Assets.PutOptions
    * @property {ArrayBuffer|string} [data] byte buffer or string value representing the new asset's content
    * @property {string} [path=null] ATP path mapping to automatically create (upon successful upload to hash)
    * @property {boolean} [compress=false] whether to gzip compress data before uploading
    */
    /**jsdoc
     * Called when Assets.putAsset is complete.
     * @callback Assets~puttAssetCallback
     * @param {string} error - contains error message (or null value if no error occured while uploading/mapping the new asset)
     * @param {Asset~putAssetResult} result - result object containing error or result status of asset upload
     */
   /**jsdoc
    * Result value returned by {@link Assets.putAsset}.
    * @typedef {Object} Assets~putAssetResult
    * @property {url} [url] the resolved "atp:" style URL for the uploaded asset (based on .path if specified, otherwise on the resulting ATP hash)
    * @property {string} [path] the uploaded asset's resulting ATP path (or undefined if no path mapping was assigned)
    * @property {string} [hash] the uploaded asset's resulting ATP hash
    * @property {boolean} [compressed] flag indicating whether the data was compressed before upload
    * @property {number} [byteLength] flag indicating final byte size of the data uploaded to the ATP server
    */
    Q_INVOKABLE void putAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    Q_INVOKABLE void deleteAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void resolveAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void decompressData(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void compressData(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    Q_INVOKABLE bool initializeCache() { return Parent::initializeCache(); }

    Q_INVOKABLE bool canWriteCacheValue(const QUrl& url);

    Q_INVOKABLE void getCacheStatus(QScriptValue scope, QScriptValue callback = QScriptValue()) {
        jsPromiseReady(Parent::getCacheStatus(), scope, callback);
    }

    Q_INVOKABLE void queryCacheMeta(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void loadFromCache(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void saveToCache(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void saveToCache(const QUrl& url, const QByteArray& data, const QVariantMap& metadata,
                                 QScriptValue scope, QScriptValue callback = QScriptValue());
protected:
    QScriptValue jsBindCallback(QScriptValue scope, QScriptValue callback = QScriptValue());
    Promise jsPromiseReady(Promise promise, QScriptValue scope, QScriptValue callback = QScriptValue());

    void jsCallback(const QScriptValue& handler, const QScriptValue& error, const QVariantMap& result);
    void jsCallback(const QScriptValue& handler, const QScriptValue& error, const QScriptValue& result);
    bool jsVerify(bool condition, const QString& error);
};

#endif // hifi_AssetScriptingInterface_h
