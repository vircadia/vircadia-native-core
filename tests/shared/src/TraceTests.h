//
//  Created by Bradley Austin Davis on 2016/12/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TraceTests_h
#define hifi_TraceTests_h

#include <QtCore/QObject>

class TraceTests : public QObject {
    Q_OBJECT
private slots:
    void testTraceSerialization();
};

#endif // hifi_TraceTests_h
