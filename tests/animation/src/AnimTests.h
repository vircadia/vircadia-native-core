//
//  AnimTests.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimTests_h
#define hifi_AnimTests_h

#include <QtTest/QtTest>
#include <glm/glm.hpp>

class AnimTests : public QObject {
    Q_OBJECT
public:
    void testAccumulateTimeWithParameters(float startFrame, float endFrame, float timeScale) const;
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testClipInternalState();
    void testClipEvaulate();
    void testClipEvaulateWithVars();
    void testLoader();
    void testVariant();
    void testAccumulateTime();
    void testAnimPose();
    void testExpressionTokenizer();
    void testExpressionParser();
    void testExpressionEvaluator();
};

#endif // hifi_AnimTests_h
