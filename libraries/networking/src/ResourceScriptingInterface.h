//
//  AssetClient.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_networking_ResourceScriptingInterface_h
#define hifi_networking_ResourceScriptingInterface_h

#include <QtCore/QObject>

#include <DependencyManager.h>

class ResourceScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    Q_INVOKABLE void overrideUrlPrefix(const QString& prefix, const QString& replacement);

    Q_INVOKABLE void restoreUrlPrefix(const QString& prefix) {
        overrideUrlPrefix(prefix, "");
    }
};


#endif
