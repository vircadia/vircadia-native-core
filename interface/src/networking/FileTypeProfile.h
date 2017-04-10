//
//  FileTypeProfile.h
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_FileTypeProfile_h
#define hifi_FileTypeProfile_h

#include <QtWebEngine/QQuickWebEngineProfile>

class FileTypeProfile : public QQuickWebEngineProfile {
public:
    FileTypeProfile(QObject* parent = Q_NULLPTR);
};


#endif // hifi_FileTypeProfile_h
