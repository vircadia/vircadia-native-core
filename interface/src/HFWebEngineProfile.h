//
//  HFWebEngineProfile.h
//  interface/src
//
//  Created by Stephen Birarda on 2016-10-17.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_HFWebEngineProfile_h
#define hifi_HFWebEngineProfile_h

#include <QtWebEngine/QQuickWebEngineProfile>

class HFWebEngineProfile : public QQuickWebEngineProfile {
public:
    HFWebEngineProfile(QObject* parent = Q_NULLPTR);
};


#endif // hifi_HFWebEngineProfile_h
