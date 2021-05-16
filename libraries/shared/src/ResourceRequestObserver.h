//
//  ResourceRequestObserver.h
//  libraries/shared/src
//
//  Created by Kerry Ivan Kurian on 9/27/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QJsonObject>
#include <QString>
#include <QNetworkRequest>

#include "DependencyManager.h"

/*@jsdoc
 * The <code>ResourceRequestObserver</code> API provides notifications when an observable resource request is made.
 *
 * @namespace ResourceRequestObserver
 * 
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class ResourceRequestObserver : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void update(const QUrl& requestUrl, const qint64 callerId = -1, const QString& extra = "");

signals:
    /*@jsdoc
     * Triggered when an observable resource request is made.
     * @function ResourceRequestObserver.resourceRequestEvent
     * @param {ResourceRequestObserver.ResourceRequest} request - Information about the resource request.
     * @returns {Signal}
     * @example <caption>Report when a particular Clipboard.importEntities() resource request is made.</caption>
     * ResourceRequestObserver.resourceRequestEvent.connect(function (request) {
     *     if (request.callerId === 100) {
     *         print("Resource request: " + JSON.stringify(request));
     *     }
     * });
     * 
     * function importEntities() {
     *     var filename = Window.browse("Import entities to clipboard", "", "*.json");
     *     if (filename) {
     *         Clipboard.importEntities(filename, true, 100);
     *         pastedEntities = Clipboard.pasteEntities(Vec3.sum(MyAvatar.position,
     *             Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })));
     *         print("Entities pasted: " + JSON.stringify(pastedEntities));
     *     }
     * }
     * 
     * Script.setTimeout(importEntities, 2000);
     */
    void resourceRequestEvent(QVariantMap result);
};
