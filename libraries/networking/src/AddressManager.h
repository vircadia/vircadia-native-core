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

class AddressManager : public QObject {
    Q_OBJECT
public:
    static AddressManager& getInstance();
    
    void handleLookupString(const QString& lookupString);
signals:
    void locationChangeRequired(const glm::vec3& newPosition, bool hasOrientationChange, const glm::vec3& newOrientation);
private:
    bool lookupHandledAsNetworkAddress(const QString& lookupString);
    bool lookupHandledAsLocationString(const QString& lookupString);
    bool lookupHandledAsUsername(const QString& lookupString);
};

#endif // hifi_AddressManager_h