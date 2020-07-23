//
//  DomainAccountManager.h
//  libraries/networking/src
//
//  Created by David Rowe on 23 Jul 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainAccountManager_h
#define hifi_DomainAccountManager_h

#include <QtCore/QObject>

#include <DependencyManager.h>


class DomainAccountManager : public QObject, public Dependency {
    Q_OBJECT
public:
    DomainAccountManager();

    Q_INVOKABLE bool checkAndSignalForAccessToken();

public slots:

signals:
    void authRequired();

private slots:

private:
    bool hasValidAccessToken();

};

#endif  // hifi_DomainAccountManager_h
