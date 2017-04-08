//
//  ZoneRenderer.h
//  render/src/render-utils
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ZoneRenderer_h
#define hifi_ZoneRenderer_h

#include "render/Engine.h"

class ZoneRendererConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
public:

    int maxDrawn { -1 };

signals:
    void dirty();

protected:
};

class ZoneRendererTask : public render::Task {
public:

    static const render::Selection::Name ZONES_SELECTION;


    using Inputs = render::ItemBounds;
    using Config = ZoneRendererConfig;
    using JobModel = Model<ZoneRendererTask, Config>;

    ZoneRendererTask(const Inputs& inputs);

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }

protected:
    int _maxDrawn; // initialized by Config
};

#endif