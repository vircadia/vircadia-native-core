//
//  RequestFilters.h
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_RequestFilters_h
#define hifi_RequestFilters_h

#include <QObject>
#include <QWebEngineUrlRequestInfo>

class RequestFilters : public QObject {
    Q_OBJECT

public: 
    static void interceptHFWebEngineRequest(QWebEngineUrlRequestInfo& info);
    static void interceptFileType(QWebEngineUrlRequestInfo& info);
};

#endif // hifi_RequestFilters_h
