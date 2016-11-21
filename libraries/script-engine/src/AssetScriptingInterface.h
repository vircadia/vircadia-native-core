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
#include <QtScript/QScriptValue>

#include <AssetClient.h>

/**jsdoc
 * @namespace Assets
 */
class AssetScriptingInterface : public QObject {
    Q_OBJECT
public:
    AssetScriptingInterface(QScriptEngine* engine);

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
     */
    Q_INVOKABLE void setMapping(QString path, QString hash, QScriptValue callback);

#if (PR_BUILD || DEV_BUILD)
    Q_INVOKABLE void sendFakedHandshake();
#endif

protected:
    QSet<AssetRequest*> _pendingRequests;
    QScriptEngine* _engine;
};

#endif // hifi_AssetScriptingInterface_h
