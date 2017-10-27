//
//  HFTabletWebEngineProfile.h
//  interface/src/networking
//
//  Created by Dante Ruiz on 2017-03-31.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_HFTabletWebEngineProfile_h
#define hifi_HFTabletWebEngineProfile_h

#include <QtWebEngine/QQuickWebEngineProfile>

class HFTabletWebEngineProfile : public QQuickWebEngineProfile {
public:
    HFTabletWebEngineProfile(QObject* parent = Q_NULLPTR);
};

#endif // hifi_HFTabletWebEngineProfile_h
