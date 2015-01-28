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

#include <DependencyManager.h>

#include "AccountManager.h"

const QString HIFI_URL_SCHEME = "hifi";
const QString DEFAULT_HIFI_ADDRESS = "hifi://sandbox";

typedef const glm::vec3& (*PositionGetter)();
typedef glm::quat (*OrientationGetter)();

class AddressManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(bool isConnected READ isConnected)
    Q_PROPERTY(QUrl href READ currentAddress)
    Q_PROPERTY(QString protocol READ getProtocol)
    Q_PROPERTY(QString hostname READ getRootPlaceName)
    Q_PROPERTY(QString pathname READ currentPath)
public:
    bool isConnected();
    const QString& getProtocol() { return HIFI_URL_SCHEME; };
    
    const QUrl currentAddress() const;
    const QString currentPath(bool withOrientation = true) const;
    
    const QUuid& getRootPlaceID() const { return _rootPlaceID; }
    
    const QString& getRootPlaceName() const { return _rootPlaceName; }
    void setRootPlaceName(const QString& rootPlaceName);
    
    void attemptPlaceNameLookup(const QString& lookupString, const QString& overridePath = QString());
    
    void setPositionGetter(PositionGetter positionGetter) { _positionGetter = positionGetter; }
    void setOrientationGetter(OrientationGetter orientationGetter) { _orientationGetter = orientationGetter; }
    
    void loadSettings(const QString& lookupString = QString());
    
public slots:
    void handleLookupString(const QString& lookupString);
    void goToUser(const QString& username);
    void goToAddressFromObject(const QVariantMap& addressMap, const QNetworkReply& reply);
    
    void storeCurrentAddress();
    
    void copyAddress();
    void copyPath();
    
signals:
    void lookupResultsFinished();
    void lookupResultIsOffline();
    void lookupResultIsNotFound();
    void possibleDomainChangeRequired(const QString& newHostname, quint16 newPort);
    void possibleDomainChangeRequiredViaICEForID(const QString& iceServerHostname, const QUuid& domainID);
    void locationChangeRequired(const glm::vec3& newPosition,
                                bool hasOrientationChange, const glm::quat& newOrientation,
                                bool shouldFaceLocation);
    void rootPlaceNameChanged(const QString& newRootPlaceName);
protected:
    AddressManager();
private slots:
    void handleAPIResponse(QNetworkReply& requestReply);
    void handleAPIError(QNetworkReply& errorReply);
private:
    void setDomainInfo(const QString& hostname, quint16 port);
    
    const JSONCallbackParameters& apiCallbackParameters();
    
    bool handleUrl(const QUrl& lookupUrl);
    
    bool handleNetworkAddress(const QString& lookupString);
    bool handleRelativeViewpoint(const QString& pathSubsection, bool shouldFace = false);
    bool handleUsername(const QString& lookupString);
    
    QString _rootPlaceName;
    QUuid _rootPlaceID;
    PositionGetter _positionGetter;
    OrientationGetter _orientationGetter;
};

#endif // hifi_AddressManager_h