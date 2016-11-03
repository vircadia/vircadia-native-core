//
//  Created by Bradley Austin Davis on 2015/12/17
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_network_AbstractUriHandler_h
#define hifi_network_AbstractUriHandler_h

class AbstractUriHandler {
public:
    virtual bool canAcceptURL(const QString& url) const = 0;
    virtual bool acceptURL(const QString& url, bool defaultUpload = false) = 0;
};

#endif
