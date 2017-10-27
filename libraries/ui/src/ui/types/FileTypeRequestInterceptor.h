//
//  FileTypeRequestInterceptor.h
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_FileTypeRequestInterceptor_h
#define hifi_FileTypeRequestInterceptor_h

#include <QWebEngineUrlRequestInterceptor>

class FileTypeRequestInterceptor : public QWebEngineUrlRequestInterceptor {
public:
    FileTypeRequestInterceptor(QObject* parent) : QWebEngineUrlRequestInterceptor(parent) {};

    virtual void interceptRequest(QWebEngineUrlRequestInfo& info) override;
};

#endif // hifi_FileTypeRequestInterceptor_h
