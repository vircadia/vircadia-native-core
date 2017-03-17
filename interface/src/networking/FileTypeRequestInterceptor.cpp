//
//  FileTypeRequestInterceptor.cpp
//  interface/src/networking
//
//  Created by Kunal Gosar on 2017-03-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileTypeRequestInterceptor.h"

#include <QtCore/QDebug>

#include "RequestFilters.h"

void FileTypeRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
    RequestFilters::interceptHFWebEngineRequest(info);
    RequestFilters::interceptFileType(info);
}
