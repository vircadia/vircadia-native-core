//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "StereoDisplayPlugin.h"

class QScreen;

class SideBySideStereoDisplayPlugin : public StereoDisplayPlugin {
    Q_OBJECT
    using Parent = StereoDisplayPlugin;
public:
    virtual const QString& getName() const override { return NAME; }
    virtual grouping getGrouping() const override { return ADVANCED; }
    virtual glm::uvec2 getRecommendedRenderSize() const override;

private:
    static const QString NAME;
};
