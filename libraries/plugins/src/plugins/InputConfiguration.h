//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InputConfiguration_h
#define hifi_InputConfiguration_h

#include <mutex>

#include <QObject>
#include <DependencyManager.h>

class InputConfiguration : public QObject, public Dependency {
    Q_OBJECT
public:
    InputConfiguration();
    
    void inputPlugins();
    void enabledInputPlugins();
};

#endif
