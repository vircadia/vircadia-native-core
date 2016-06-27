//
//  Created by Bradley Austin Davis on 2015/10/04
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AbstractHMDScriptingInterface_h
#define hifi_AbstractHMDScriptingInterface_h

#include <GLMHelpers.h>

class AbstractHMDScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ isHMDMode)
    Q_PROPERTY(float ipd READ getIPD)
    Q_PROPERTY(float eyeHeight READ getEyeHeight)
    Q_PROPERTY(float playerHeight READ getPlayerHeight)
    Q_PROPERTY(float ipdScale READ getIPDScale WRITE setIPDScale NOTIFY IPDScaleChanged)

public:
    AbstractHMDScriptingInterface();
    float getIPD() const;
    float getEyeHeight() const;
    float getPlayerHeight() const;
    float getIPDScale() const;
    void setIPDScale(float ipdScale);
    bool isHMDMode() const;

signals:
    void IPDScaleChanged();
    void displayModeChanged(bool isHMDMode);

private:
    float _IPDScale{ 1.0 };
};

#endif // hifi_AbstractHMDScriptingInterface_h
