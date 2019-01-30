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
 * The Assets API allows you to communicate with the Asset Browser.
 * @namespace Assets
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
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
     * @param url {string} URL of asset to download, must be ATP scheme URL.
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
     * @param path {string}
     * @param callback {Assets~getMappingCallback}
     */
    /**jsdoc
     * Called when getMapping is complete.
     * @callback Assets~getMappingCallback
     * @param assetID {string} hash value if found, else an empty string
     * @param error {string} error description if the path could not be resolved; otherwise a null value.
     */
    Q_INVOKABLE void getMapping(QString path, QScriptValue callback);

    /**jsdoc
     * @function Assets.setBakingEnabled
     * @param path {string}
     * @param enabled {boolean}
     * @param callback {}
     */
    /**jsdoc
     * Called when setBakingEnabled is complete.
     * @callback Assets~setBakingEnabledCallback
     */
    Q_INVOKABLE void setBakingEnabled(QString path, bool enabled, QScriptValue callback);

#if (PR_BUILD || DEV_BUILD)
    /**
     * This function is purely for development purposes, and not meant for use in a
     * production context. It is not a public-facing API, so it should not contain jsdoc.
     */
    Q_INVOKABLE void sendFakedHandshake();
#endif

    /**jsdoc
     * Request Asset data from the ATP Server
     * @function Assets.getAsset
     * @param {URL|Assets.GetOptions} options An atp: style URL, hash, or relative mapped path; or an {@link Assets.GetOptions} object with request parameters
     * @param {Assets~getAssetCallback} scope A scope callback function to receive (error, results) values
     * @param {function} [callback=undefined]
     */

    /**jsdoc
     * A set of properties that can be passed to {@link Assets.getAsset}.
     * @typedef {object} Assets.GetOptions
     * @property {string} [url] an "atp:" style URL, hash, or relative mapped path to fetch
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
     * @typedef {object} Assets~getAssetResult
     * @property {string} [url] the resolved "atp:" style URL for the fetched asset
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
     * @param {function} [callback=undefined]
     */

    /**jsdoc
     * A set of properties that can be passed to {@link Assets.putAsset}.
     * @typedef {object} Assets.PutOptions
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
     * @typedef {object} Assets~putAssetResult
     * @property {string} [url] the resolved "atp:" style URL for the uploaded asset (based on .path if specified, otherwise on the resulting ATP hash)
     * @property {string} [path] the uploaded asset's resulting ATP path (or undefined if no path mapping was assigned)
     * @property {string} [hash] the uploaded asset's resulting ATP hash
     * @property {boolean} [compressed] flag indicating whether the data was compressed before upload
     * @property {number} [byteLength] flag indicating final byte size of the data uploaded to the ATP server
     */

    Q_INVOKABLE void putAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    /**jsdoc
     * @function Assets.deleteAsset
     * @param {} options
     * @param {} scope
     * @param {} [callback = ""]
     */

    Q_INVOKABLE void deleteAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.resolveAsset
     * @param {} options
     * @param {} scope
     * @param {} [callback = ""]
     */

    Q_INVOKABLE void resolveAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.decompressData
     * @param {} options
     * @param {} scope
     * @param {} [callback = ""]
     */

    Q_INVOKABLE void decompressData(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.compressData
     * @param {} options
     * @param {} scope
     * @param {} [callback = ""]
     */

    Q_INVOKABLE void compressData(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.initializeCache
     * @returns {boolean}
     */

    Q_INVOKABLE bool initializeCache() { return Parent::initializeCache(); }
    
    /**jsdoc
     * @function Assets.canWriteCacheValue
     * @param {string} url
     * @returns {boolean}
     */

    Q_INVOKABLE bool canWriteCacheValue(const QUrl& url);
    
    /**jsdoc
     * @function Assets.getCacheStatus
     * @param {} scope
     * @param {} [callback=undefined]
     */

    Q_INVOKABLE void getCacheStatus(QScriptValue scope, QScriptValue callback = QScriptValue()) {
        jsPromiseReady(Parent::getCacheStatus(), scope, callback);
    }

    /**jsdoc
     * @function Assets.queryCacheMeta
     * @param {} options
     * @param {} scope
     * @param {} [callback=undefined]
     */

    Q_INVOKABLE void queryCacheMeta(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.loadFromCache
     * @param {} options
     * @param {} scope
     * @param {} [callback=undefined]
     */

    Q_INVOKABLE void loadFromCache(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.saveToCache
     * @param {} options
     * @param {} scope
     * @param {} [callback=undefined]
     */

    Q_INVOKABLE void saveToCache(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());
    
    /**jsdoc
     * @function Assets.saveToCache
     * @param {} url
     * @param {} data
     * @param {} metadata
     * @param {} scope
     * @param {} [callback=undefined]
     */

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
