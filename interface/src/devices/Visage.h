//
//  Visage.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Visage_h
#define hifi_Visage_h

#include <QMultiHash>
#include <QPair>
#include <QVector>

#include <DependencyManager.h>

#include "FaceTracker.h"

namespace VisageSDK {
    class VisageTracker2;
    struct FaceData;
}

/// Handles input from the Visage webcam feature tracking software.
class Visage : public FaceTracker, public DependencyManager::Dependency  {
    Q_OBJECT
    
public:
    void init();
    
    bool isActive() const { return _active; }
    
    void update();
    void reset();

public slots:

    void updateEnabled();
    
private:
    Visage();
    virtual ~Visage();
    friend DependencyManager;

#ifdef HAVE_VISAGE
    VisageSDK::VisageTracker2* _tracker;
    VisageSDK::FaceData* _data;
    QMultiHash<int, QPair<int, float> > _actionUnitIndexMap; 
#endif
    
    void setEnabled(bool enabled);
    
    bool _enabled;
    bool _active;

    glm::vec3 _headOrigin;
};

#endif // hifi_Visage_h
