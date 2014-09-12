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

static const QString HIFI_URL_SCHEME = "hifi";

class AddressManager : public QObject {
    Q_OBJECT
public:
    static AddressManager& getInstance();
    
    static QString pathForPositionAndOrientation(const glm::vec3& position, bool hasOrientation = false,
                                                 const glm::quat& orientation = glm::quat());
    
    void handleLookupString(const QString& lookupString);
    void attemptPlaceNameLookup(const QString& lookupString);
public slots:
    void handleAPIResponse(const QJsonObject& jsonObject);
    void handleAPIError(QNetworkReply& errorReply);
    void goToUser(const QString& username);
signals:
    void lookupResultIsOffline();
    void possibleDomainChangeRequired(const QString& newHostname);
    void locationChangeRequired(const glm::vec3& newPosition, bool hasOrientationChange, const glm::vec3& newOrientation);
private:
    const JSONCallbackParameters& apiCallbackParameters();
    
    bool handleUrl(const QUrl& lookupUrl);
    
    bool handleNetworkAddress(const QString& lookupString);
    bool handleRelativeViewpoint(const QString& pathSubsection);
    bool handleUsername(const QString& lookupString);
};

#endif // hifi_AddressManager_h