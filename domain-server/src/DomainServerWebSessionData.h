//
//  DomainServerWebSessionData.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-07-21.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServerWebSessionData_h
#define hifi_DomainServerWebSessionData_h

#include <QtCore/QObject>
#include <QtCore/QSet>

class DomainServerWebSessionData : public QObject {
    Q_OBJECT
public:
    DomainServerWebSessionData();
    DomainServerWebSessionData(const QJsonObject& userObject);
    DomainServerWebSessionData(const DomainServerWebSessionData& otherSessionData);
    DomainServerWebSessionData& operator=(const DomainServerWebSessionData& otherSessionData);
    
    const QString& getUsername() const { return _username; }
    const QSet<QString>& getRoles() const { return _roles; }
    
private:
    void swap(DomainServerWebSessionData& otherSessionData);
    
    QString _username;
    QSet<QString> _roles;
};

#endif // hifi_DomainServerWebSessionData_h