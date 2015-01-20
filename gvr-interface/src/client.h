//
//  Client.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Client_h
#define hifi_Client_h

#include <QtCore/QObject>

class Client : public QObject {
    Q_OBJECT
public:
    Client(QObject* parent = 0);
};

#endif // hifi_Client_h
