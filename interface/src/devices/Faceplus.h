//
//  Faceplus.h
//  interface
//
//  Created by Andrzej Kapolka on 4/8/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Faceplus__
#define __interface__Faceplus__

#include <QMultiHash>
#include <QPair>
#include <QVector>

#include "FaceTracker.h"

/// Interface for Mixamo FacePlus.
class Faceplus : public FaceTracker {
    Q_OBJECT
    
public:
    
    Faceplus();
    virtual ~Faceplus();
    
    void init();
    
    bool isActive() const { return _active; }
    
    void update();
    void reset();

public slots:

    void updateEnabled();
    
private:
    
    void setEnabled(bool enabled);
    
    bool _enabled;
    bool _active;
    
#ifdef HAVE_FACEPLUS
    QMultiHash<int, QPair<int, float> > _channelIndexMap; 
    QVector<float> _outputVector;
    int _headRotationIndices[3];
    int _leftEyeRotationIndices[2];
    int _rightEyeRotationIndices[2];
#endif
};

#endif /* defined(__interface__Faceplus__) */
