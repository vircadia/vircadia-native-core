//
//  Visage.h
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Visage__
#define __interface__Visage__

#include <QMultiHash>
#include <QPair>
#include <QVector>

#include "FaceTracker.h"

namespace VisageSDK {
    class VisageTracker2;
    struct FaceData;
}

/// Handles input from the Visage webcam feature tracking software.
class Visage : public FaceTracker {
    Q_OBJECT
    
public:
    
    Visage();
    virtual ~Visage();
    
    void init();
    
    bool isActive() const { return _active; }
    
    void update();
    void reset();

public slots:

    void updateEnabled();
    
private:

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

#endif /* defined(__interface__Visage__) */
