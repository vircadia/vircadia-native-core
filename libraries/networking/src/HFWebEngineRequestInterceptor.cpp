//
//  HFWebEngineRequestInterceptor.cpp
//  interface/src/networking
//
//  Created by Stephen Birarda on 2016-10-14.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFWebEngineRequestInterceptor.h"

#include <QtCore/QDebug>

#include "AccountManager.h"
#include "RequestFilters.h"

void HFWebEngineRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    RequestFilters::interceptHFWebEngineRequest(info);
}
