//
//  Body.h
//  interface
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Body__
#define __interface__Body__

#include <QObject>
#include <QUrl>

#include "renderer/GeometryCache.h"

/// An avatar body with an arbitrary skeleton.
class Body : public QObject {
    Q_OBJECT
    
public:

    Body(Avatar* owningAvatar);
    
    bool isActive() const { return _skeletonGeometry && _skeletonGeometry->isLoaded(); }
    
    void simulate(float deltaTime);
    bool render(float alpha);
    
    Q_INVOKABLE void setSkeletonModelURL(const QUrl& url);
    const QUrl& getSkeletonModelURL() const { return _skeletonModelURL; }

private:
    
    Avatar* _owningAvatar;
    
    QUrl _skeletonModelURL;
    
    QSharedPointer<NetworkGeometry> _skeletonGeometry;
    
    class JointState {
    public:
        glm::quat rotation;
        glm::mat4 transform;
    };
    
    QVector<JointState> _jointStates;
};

#endif /* defined(__interface__Body__) */
