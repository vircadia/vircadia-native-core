//
//  RenderingClient.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_RenderingClient_h
#define hifi_RenderingClient_h

#include "Client.h"

class RenderingClient : public Client {
    Q_OBJECT
public:
    RenderingClient(QObject* parent = 0);
};

#endif // hifi_RenderingClient_h
