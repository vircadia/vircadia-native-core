//
//  AddressManager.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-09-10.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AddressManager_h
#define hifi_AddressManager_h

#include <qobject.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AccountManager.h"

static const QString HIFI_URL_SCHEME = "hifi:";

const glm::quat EMPTY_QUAT = glm::quat();

class AddressManager : public QObject {
    Q_OBJECT
public:
    static AddressManager& getInstance();
    
    static QString pathForPositionAndOrientation(const glm::vec3& position, bool hasOrientation = false,
                                                 const glm::quat& orientation = EMPTY_QUAT);
    
    bool handleUrl(const QUrl& lookupUrl);
    void handleLookupString(const QString& lookupString);
    void attemptPlaceNameLookup(const QString& lookupString);
public slots:
    void handleAPIResponse(const QJsonObject& jsonObject);
    void handleAPIError(QNetworkReply& errorReply);
signals:
    void lookupResultIsOffline();
    void possibleDomainChangeRequired(const QString& newHostname);
    void locationChangeRequired(const glm::vec3& newPosition, bool hasOrientationChange, const glm::vec3& newOrientation);
private:
    const JSONCallbackParameters& apiCallbackParameters();
    
    bool isLookupHandledAsNetworkAddress(const QString& lookupString);
    bool isLookupHandledAsViewpoint(const QString& lookupString);
    bool isLookupHandledAsUsername(const QString& lookupString);
};

#endif // hifi_AddressManager_h