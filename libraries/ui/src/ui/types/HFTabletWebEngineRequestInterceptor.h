//
//  HFTabletWebEngineRequestInterceptor.h
//  interface/src/networking
//
//  Created by Dante Ruiz on 2017-3-31.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFTabletWebEngineRequestInterceptor_h
#define hifi_HFTabletWebEngineRequestInterceptor_h
#if !defined(Q_OS_ANDROID)
#include <QtCore/QObject>

#include <QWebEngineUrlRequestInterceptor>

class HFTabletWebEngineRequestInterceptor
    : public QWebEngineUrlRequestInterceptor
{
public:
    HFTabletWebEngineRequestInterceptor(QObject* parent) 
    : QWebEngineUrlRequestInterceptor(parent)
 {};
    virtual void interceptRequest(QWebEngineUrlRequestInfo& info) override;
};
#endif

#endif // hifi_HFWebEngineRequestInterceptor_h
