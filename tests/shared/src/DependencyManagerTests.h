//
//  Created by Anthony Thibault on May 9th 2019
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DependencyManagerTests_h
#define hifi_DependencyManagerTests_h

#include <QtCore/QObject>

class DependencyManagerTests : public QObject {
    Q_OBJECT
private slots:
    void testDependencyManager();
    void testDependencyManagerMultiThreaded();
};

#endif // hifi_DependencyManagerTests_h
