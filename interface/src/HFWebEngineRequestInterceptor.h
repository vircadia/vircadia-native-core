//
//  HFWebEngineRequestInterceptor.h
//  interface/src
//
//  Created by Stephen Birarda on 2016-10-14.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_HFWebEngineRequestInterceptor_h
#define hifi_HFWebEngineRequestInterceptor_h

#include <QWebEngineUrlRequestInterceptor>

class HFWebEngineRequestInterceptor : public QWebEngineUrlRequestInterceptor {
public:
    HFWebEngineRequestInterceptor(QObject* parent) : QWebEngineUrlRequestInterceptor(parent) {};

    virtual void interceptRequest(QWebEngineUrlRequestInfo& info) override;
};

#endif // hifi_HFWebEngineRequestInterceptor_h
