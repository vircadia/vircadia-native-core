//
//  OctreeTests.h
//  tests/octree/src
//
//  Created by Brad Hefta-Gaub on 06/04/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeTests_h
#define hifi_OctreeTests_h

#include <QtTest/QtTest>

class OctreeTests : public QObject {
    Q_OBJECT
    
private slots:
    // FIXME: These two tests are broken and need to be fixed / updated
    void propertyFlagsTests();
    void byteCountCodingTests();
    
    // This test is fine
    void modelItemTests();

    void elementAddChildTests();

    // TODO: Break these into separate test functions
};

// Helper functions
inline QByteArray makeQByteArray (std::initializer_list<char> bytes) {
    return QByteArray {
        bytes.begin(), static_cast<int>(bytes.size() * sizeof(char))
    };
}

#endif // hifi_OctreeTests_h
