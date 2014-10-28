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

typedef const glm::vec3& (*PositionGetter)();
typedef glm::quat (*OrientationGetter)();

class AddressManager : public QObject {
    Q_OBJECT
public:
    static AddressManager& getInstance();
    
    const QUrl currentAddress();
    const QString currentPath(bool withOrientation = true) const;
    
    const QString& getCurrentDomain() const { return _currentDomain; }
    
    void attemptPlaceNameLookup(const QString& lookupString);
    
    void setPositionGetter(PositionGetter positionGetter) { _positionGetter = positionGetter; }
    void setOrientationGetter(OrientationGetter orientationGetter) { _orientationGetter = orientationGetter; }
    
public slots:
    void handleLookupString(const QString& lookupString);
    void goToUser(const QString& username);
    void goToAddress(const QVariantMap& addressMap);
    
signals:
    void lookupResultsFinished();
    void lookupResultIsOffline();
    void lookupResultIsNotFound();
    void possibleDomainChangeRequiredToHostname(const QString& newHostname);
    void possibleDomainChangeRequiredViaICEForID(const QString& iceServerHostname, const QUuid& domainID);
    void locationChangeRequired(const glm::vec3& newPosition,
                                bool hasOrientationChange, const glm::quat& newOrientation,
                                bool shouldFaceLocation);
private slots:
    void handleAPIResponse(QNetworkReply& requestReply);
    void handleAPIError(QNetworkReply& errorReply);
private:
    AddressManager();
    
    void setDomainHostnameAndName(const QString& hostname, const QString& domainName = QString());
    
    const JSONCallbackParameters& apiCallbackParameters();
    
    bool handleUrl(const QUrl& lookupUrl);
    
    bool handleNetworkAddress(const QString& lookupString);
    bool handleRelativeViewpoint(const QString& pathSubsection, bool shouldFace = false);
    bool handleUsername(const QString& lookupString);
    
    QString _currentDomain;
    PositionGetter _positionGetter;
    OrientationGetter _orientationGetter;
};

#endif // hifi_AddressManager_h