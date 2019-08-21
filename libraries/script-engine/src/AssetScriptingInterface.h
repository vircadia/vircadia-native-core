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
     * Details of a callback function.
     * @typedef {object} Assets.CallbackDetails
     * @property {object} scope - The scope that the <code>callback</code> function is defined in. This object is bound to 
     *     <code>this</code> when the function is called.
     * @property {Assets~putAssetCallback|Assets~getAssetCallback|Assets~resolveAssetCallback} callback -  The function to 
     *     call upon completion. May be an inline function or a function identifier. If a function identifier, it must be a 
     *     member of <code>scope</code>.
     */

    /**jsdoc
     * Source and download options for {@link Assets.getAsset}.
     * @typedef {object} Assets.GetOptions
     * @property {string} url - The mapped path or hash to download. May have a leading <code>"atp:"</code>.
     * @property {Assets.ResponseType} [responseType="text"] - The desired result type.
     * @property {boolean} [decompress=false] - <code>true</code> to gunzip decompress the downloaded data. Synonym: 
     *     <code>compressed</code>.
     */
    /**jsdoc
     * Called when an {@link Assets.getAsset} call is complete.
     * @callback Assets~getAssetCallback
     * @param {string} error - <code>null</code> if the content was downloaded, otherwise a description of the error.
     * @param {Assets.GetResult} result - Information on and the content downloaded.
     */
    /**jsdoc
     * Result value returned by {@link Assets.getAsset}. 
     * @typedef {object} Assets.GetResult
     * @property {number} [byteLength] - The number of bytes in the downloaded response.
     * @property {boolean} cached - 
     * @property {string} [contentType] - Automatically detected MIME type of the content.
     * @property {boolean} [decompressed] - <code>true</code> if the content was decompressed, <code>false</code> if it wasn't.
     * @property {string} [hash] - The hash for the downloaded asset.
     * @property {string} [hashURL] - The ATP URL of the hash file.
     * @property {string} [path] - The path for the asset, if a path was requested. Otherwise, <code>undefined</code>.
     * @property {string|ArrayBuffer|object} [response] - The downloaded content.
     * @property {Assets.ResponseType} [responseType] - The type of the downloaded content.
     * @property {string} [url] - The URL of the asset requested: the path with leading <code>"atp:"</code> if a path was 
     *     requested, otherwise the requested URL.
     * @property {boolean} [wasRedirected] - <code>true</code> if the downloaded data is the baked version of the asset, 
     *      <code>false</code> if it isn't baked.
     */
    /**jsdoc
     * Downloads content from the asset server.
     * @function Assets.getAsset
     * @param {Assets.GetOptions|string} source - What to download and download options. If a string, the mapped path or hash 
     *     to download, optionally including a leading <code>"atp:"</code>.
     * @param {Assets~getAssetCallback} callback - The function to call upon completion. May be a function identifier or an
     *     inline function.
     * @example <caption>Retrieve a string from the asset server.</caption>
     * Assets.getAsset(
     *     {
     *         url: "/assetsExamples/helloWorld.txt",
     *         responseType: "text"
     *     },
     *     function (error, result) {
     *         if (error) {
     *             print("ERROR: Data not downloaded");
     *         } else {
     *             print("Data: " + result.response);
     *         }
     *     }
     * );
     */
    /**jsdoc
     * Downloads content from the asset server.
     * @function Assets.getAsset
     * @param {Assets.GetOptions|string} source - What to download and download options. If a string, the mapped path or hash
     *     to download, optionally including a leading <code>"atp:"</code>.
     * @param {object} scope - The scope that the <code>callback</code> function is defined in. This object is bound to
     *     <code>this</code> when the function is called.
     * @param {Assets~getAssetCallback} callback - The function to call upon completion. May be an inline function, a function 
     *     identifier, or the name of a function in a string. If the name of a function or a function identifier, it must be 
     *     a member of <code>scope</code>.
     */
    /**jsdoc
     * Downloads content from the asset server.
     * @function Assets.getAsset
     * @param {Assets.GetOptions|string} source - What to download and download options. If a string, the mapped path or hash
     *     to download, optionally including a leading <code>"atp:"</code>.
     * @param {Assets.CallbackDetails} callbackDetails - Details of the function to call upon completion.
     */
    Q_INVOKABLE void getAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    /**jsdoc
     * Content and upload options for {@link Assets.putAsset}.
     * @typedef {object} Assets.PutOptions
     * @property {string|ArrayBuffer} data - The content to upload.
     * @property {string} [path] - A user-friendly path for the file in the asset server. May have a leading 
     *     <code>"atp:"</code>.
     *     <p>Note: The asset server destroys any unmapped SHA256-named file at server restart. Either set the mapping path 
     *     with this property or use {@link Assets.setMapping} to set a path-to-hash mapping for the uploaded file.</p>
     * @property {boolean} [compress=false] - <code>true</code> to gzip compress the content for upload and storage, 
     *     <code>false</code> to upload and store the data without gzip compression. Synonym: <code>compressed</code>.
     */
    /**jsdoc
     * Called when an {@link Assets.putAsset} call is complete.
     * @callback Assets~putAssetCallback
     * @param {string} error - <code>null</code> if the content was uploaded and any path-to-hash mapping set, otherwise a 
     *     description of the error.
     * @param {Assets.PutResult} result - Information on the content uploaded.
     */
    /**jsdoc
     * Result value returned by {@link Assets.putAsset}.
     * @typedef {object} Assets.PutResult
     * @property {number} [byteLength] - The number of bytes in the hash file stored on the asset server.
     * @property {boolean} [compressed] - <code>true</code> if the content stored is gzip compressed.
     * @property {string} [contentType] - <code>"application/gzip"</code> if the content stored is gzip compressed.
     * @property {string} [hash] - The SHA256 hash of the content.
     * @property {string} [url] - The <code>atp:</code> URL of the content: using the path if specified, otherwise the hash.
     * @property {string} [path] - The uploaded content's mapped path, if specified.
     */
    /**jsdoc
     * Uploads content to the assert server and sets a path-to-hash mapping.
     * @function Assets.putAsset
     * @param {Assets.PutOptions|string} options - The content to upload and upload options. If a string, the value of the
     *     string is uploaded but a path-to-hash mapping is not set.
     * @param {Assets~putAssetCallback} callback - The function to call upon completion. May be an inline function or a
     *     function identifier.
     * @example <caption>Store a string in the asset server.</caption>
     * Assets.putAsset(
     *     {
     *         data: "Hello world!",
     *         path: "/assetsExamples/helloWorld.txt"
     *     },
     *     function (error, result) {
     *         if (error) {
     *             print("ERROR: Data not uploaded or mapping not set");
     *         } else {
     *             print("URL: " + result.url);  // atp:/assetsExamples/helloWorld.txt
     *         }
     *     }
     * );
     */
    /**jsdoc
     * Uploads content to the assert server and sets a path-to-hash mapping.
     * @function Assets.putAsset
     * @param {Assets.PutOptions|string} options - The content to upload and upload options. If a string, the value of the 
     *     string is uploaded but a path-to-hash mapping is not set.
     * @param {object} scope - The scope that the <code>callback</code> function is defined in. This object is bound to
     *     <code>this</code> when the function is called.
     * @param {Assets~getAssetCallback} callback - The function to call upon completion. May be an inline function, a function 
     *     identifier, or the name of a function in a string. If the name of a function or a function identifier, it must be 
     *     a member of <code>scope</code>.
     */
    /**jsdoc
     * Uploads content to the assert server and sets a path-to-hash mapping.
     * @function Assets.putAsset
     * @param {Assets.PutOptions|string} options - The content to upload and upload options. If a string, the value of the
     *     string is uploaded but a path-to-hash mapping is not set.
     * @param {Assets.CallbackDetails} callbackDetails - Details of the function to call upon completion.
     */
    Q_INVOKABLE void putAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    /**jsdoc
     * Called when an {@link Assets.deleteAsset} call is complete.
     * <p class="important">Not implemented: This type is not implemented yet.</p>
     * @callback Assets~deleteAssetCallback
     * @param {string} error - <code>null</code> if the content was deleted, otherwise a description of the error.
     * @param {Assets.DeleteResult} result - Information on the content deleted.
     */
    /**jsdoc
     * Deletes content from the asset server.
     * <p class="important">Not implemented: This method is not implemented yet.</p>
     * @function Assets.deleteAsset
     * @param {Assets.DeleteOptions} options - The content to delete and delete options.
     * @param {object} scope - he scope that the <code>callback</code> function is defined in.
     * @param {Assets~deleteAssetCallback} callback - The function to call upon completion.
     */
    Q_INVOKABLE void deleteAsset(QScriptValue options, QScriptValue scope, QScriptValue callback = QScriptValue());

    /**jsdoc
     * Source options for {@link Assets.resolveAsset}.
     * @typedef {object} Assets.ResolveOptions
     * @property {string} url - The hash or path to resolve. May have a leading <code>"atp:"</code>.
     */
    /**jsdoc
     * Called when an {@link Assets.resolveAsset} call is complete.
     * @callback Assets~resolveAssetCallback
     * @param {string} error - <code>null</code> if the asset hash or path was resolved, otherwise a description of the error. 
     * @param {Assets.ResolveResult} result - Information on the hash or path resolved.
     */
    /**jsdoc
     * Result value returned by {@link Assets.resolveAsset}.
     * <p>Note: If resolving a hash, a file of that hash need not be present on the asset server for the hash to resolve.</p>
     * @typedef {object} Assets.ResolveResult
     * @property {string} [hash] - The hash of the asset.
     * @property {string} [hashURL] - The url of the asset's hash file, with leading <code>atp:</code>. 
     * @property {string} [path] - The path to the asset.
     * @property {string} [url] - The URL of the asset.
     * @property {boolean} [wasRedirected] - <code>true</code> if the resolved data is for the baked version of the asset,
     *      <code>false</code> if it isn't.
     */
    /**jsdoc
     * Resolves and returns information on a hash or a path in the asset server.
     * @function Assets.resolveAsset
     * @param {string|Assets.ResolveOptions} source - The hash or path to resolve if a string, otherwise an object specifying 
     *     what to resolve. If a string, it may have a leading <code>"atp:"</code>.
     * @param {Assets~resolveAssetCallback} callback - The function to call upon completion. May be a function identifier or 
     *     an inline function.
     * @example <caption>Get the hash and URL for a path.</caption>
     * Assets.resolveAsset(
     *     "/assetsExamples/helloWorld.txt",
     *     function (error, result) {
     *         if (error) {
     *             print("ERROR: " + error);
     *         } else {
     *             print("Hash: " + result.hash);
     *             print("URL: " + result.url);
     *         }
     *     }
     * );
     */
    /**jsdoc
     * Resolves and returns information on a hash or a path in the asset server.
     * @function Assets.resolveAsset
     * @param {string|Assets.ResolveOptions} source - The hash or path to resolve if a string, otherwise an object specifying
     *     what to resolve. If a string, it may have a leading <code>"atp:"</code>.
     * @param {object} scope - The scope that the <code>callback</code> function is defined in. This object is bound to
     *     <code>this</code> when the function is called.
     * @param {Assets~resolveAssetCallback} callback - The function to call upon completion. May be an inline function, a 
     *     function identifier, or the name of a function in a string. If the name of a function or a function identifier, it 
     *     must be a member of <code>scope</code>.
     */
    /**jsdoc
     * Resolves and returns information on a hash or a path in the asset server.
     * @function Assets.resolveAsset
     * @param {string|Assets.ResolveOptions} source - The hash or path to resolve if a string, otherwise an object specifying
     *     what to resolve. If a string, it may have a leading <code>"atp:"</code>.
     * @param {Assets.CallbackDetails} callbackDetails - Details of the function to call upon completion.
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
