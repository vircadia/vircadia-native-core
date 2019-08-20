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
 * The <code>Assets</code> API provides facilities for interacting with the domain's asset server. Assets are stored in the 
 * asset server in files with SHA256 names. These files are mapped to user-friendly URLs of the format: 
 * <code>atp:/path/filename</code>.
 * @namespace Assets
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */
class AssetScriptingInterface : public BaseAssetScriptingInterface, QScriptable {
    Q_OBJECT
public:
    using Parent = BaseAssetScriptingInterface;
    AssetScriptingInterface(QObject* parent = nullptr);

    /**jsdoc
     * Uploads content to the asset server, storing it in a SHA256-named file.
     * <p>Note: The asset server destroys any unmapped SHA256-named file at server restart. Use {@link Assets.setMapping} to 
     * set a path-to-hash mapping for the new file.</p>
     * @function Assets.uploadData
     * @param {string} data - The content to upload.
     * @param {Assets~uploadDataCallback} callback - The function to call upon completion.
     * @example <caption>Store a string in the asset server.</caption>
     * Assets.uploadData("Hello world!", function (url, hash) {
     *     print("URL: " + url);  // atp:0a1b...9g
     *     Assets.setMapping("/assetsExamples/helloWorld.txt", hash, function (error) {
     *         if (error) {
     *             print("ERROR: Could not set mapping!");
     *             return;
     *         }
     *     });
     * });
     */
    /**jsdoc
     * Called when an {@link Assets.uploadData} call is complete.
     * @callback Assets~uploadDataCallback
     * @param {string} url - The raw URL of the file that the content is stored in, with <code>atp:</code> as the scheme and 
     *     the SHA256 hash as the filename (with no extension).
     * @param {string} hash - The SHA256 hash of the content.
     */
    Q_INVOKABLE void uploadData(QString data, QScriptValue callback);

    /**jsdoc
     * Downloads content from the asset server, form a SHA256-named file.
     * @function Assets.downloadData
     * @param {string} url - The raw URL of asset to download: <code>atp:</code> followed by the assets's SHA256 hash.
     * @param {Assets~downloadDataCallback} callback - The function to call upon completion.
     * @example <caption>Store and retrieve a string from the asset server.</caption>
     * var assetURL;
     * 
     * // Store the string.
     * Assets.uploadData("Hello world!", function (url, hash) {
     *     assetURL = url;
     *     print("url: " + assetURL);  // atp:a0g89...
     *     Assets.setMapping("/assetsExamples/hellowWorld.txt", hash, function (error) {
     *         if (error) {
     *             print("ERROR: Could not set mapping!");
     *             return;
     *         }
     *     });
     * });
     * 
     * // Retrieve the string.
     * Script.setTimeout(function () {
     *     Assets.downloadData(assetURL, function (data, error) {
     *         print("Downloaded data: " + data);
     *         print("Error: " + JSON.stringify(error));
     *     });
     * }, 1000);
     */
    /**jsdoc
     * Called when an {@link Assets.downloadData} call is complete.
     * @callback Assets~downloadDataCallback
     * @param {string} data - The content that was downloaded.
     * @param {Assets.DownloadDataError} error - The success or failure of the download.
     */
    /**jsdoc
     * The success or failure of an {@link Assets.downloadData} call.
     * @typedef {object} Assets.DownloadDataError
     * @property {string} errorMessage - <code>""</code> if the download was successful, otherwise a description of the error.
     */
    Q_INVOKABLE void downloadData(QString url, QScriptValue callback);

    /**jsdoc
     * Sets a path-to-hash mapping within the asset server.
     * @function Assets.setMapping
     * @param {string} path - A user-friendly path for the file in the asset server, without leading <code>"atp:"</code>.
     * @param {string} hash - The hash in the asset server.
     * @param {Assets~setMappingCallback} callback - The function to call upon completion.
     */
    /**jsdoc
     * Called when an {@link Assets.setMapping} call is complete.
     * @callback Assets~setMappingCallback
     * @param {string} error - <code>null</code> if the path-to-hash mapping was set, otherwise a description of the error.
     */
    Q_INVOKABLE void setMapping(QString path, QString hash, QScriptValue callback);

    /**jsdoc
     * Gets the hash for a path within the asset server. The hash is for the unbaked or baked version of the
     * asset, according to the asset server setting for the particular path.
     * @function Assets.getMapping
     * @param {string} path - The path to a file in the asset server to get the hash of.
     * @param {Assets~getMappingCallback} callback - The function to call upon completion.
     * @example <caption>Report the hash of an asset server item.</caption>
     * var assetPath = Window.browseAssets();
     * if (assetPath) {
     *     var mapping = Assets.getMapping(assetPath, function (error, hash) {
     *         print("Asset: " + assetPath);
     *         print("- hash:  " + hash);
     *         print("- error: " + error);
     *     });
     * }
     */
    /**jsdoc
     * Called when an {@link Assets.getMapping} call is complete.
     * @callback Assets~getMappingCallback
     * @param {string} error - <code>null</code> if the path was found, otherwise a description of the error.
     * @param {string} hash - The hash value if the path was found, <code>""</code> if it wasn't.
     */
    Q_INVOKABLE void getMapping(QString path, QScriptValue callback);

    /**jsdoc
     * Sets whether or not to bake an asset in the asset server.
     * @function Assets.setBakingEnabled
     * @param {string} path - The path to a file in the asset server.
     * @param {boolean} enabled - <code>true</code> to enable baking of the asset, <code>false</code> to disable.
     * @param {Assets~setBakingEnabledCallback} callback - The function to call upon completion.
     */
    /**jsdoc
     * Called when an {@link Assets.setBakingEnabled} call is complete.
     * @callback Assets~setBakingEnabledCallback
     * @param {string} error - <code>null</code> if baking was successfully enabled or disabled, otherwise a description of the 
     * error.
     */
    // Note: Second callback parameter not documented because it's always {}.
    Q_INVOKABLE void setBakingEnabled(QString path, bool enabled, QScriptValue callback);

#if (PR_BUILD || DEV_BUILD)
    /**
     * This function is purely for development purposes, and not meant for use in a
     * production context. It is not a public-facing API, so it should not have JSDoc.
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
