//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OtherAvatar.h"
#include "../../interface/src/Application.h"	

OtherAvatar::OtherAvatar(QThread* thread) : Avatar(thread) {
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = new Head(this);
    _skeletonModel = std::make_shared<SkeletonModel>(this, nullptr);
    _skeletonModel->setLoadingPriority(OTHERAVATAR_LOADING_PRIORITY);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, &Avatar::setModelURLFinished);
    connect(_skeletonModel.get(), &Model::rigReady, this, &Avatar::rigReady);
    connect(_skeletonModel.get(), &Model::rigReset, this, &Avatar::rigReset);

    //add the purple orb
    
    if (_purpleOrbMeshPlaceholderID == UNKNOWN_OVERLAY_ID || !qApp->getOverlays().isAddedOverlay(_purpleOrbMeshPlaceholderID)) {
        _purpleOrbMeshPlaceholder = std::make_shared<Sphere3DOverlay>();
        _purpleOrbMeshPlaceholder->setAlpha(1.0f);
        _purpleOrbMeshPlaceholder->setColor({ 0xFF, 0x00, 0xFF });
        _purpleOrbMeshPlaceholder->setIsSolid(false);
        _purpleOrbMeshPlaceholder->setPulseMin(0.5);
        _purpleOrbMeshPlaceholder->setPulseMax(1.0);
        _purpleOrbMeshPlaceholder->setColorPulse(1.0);
        _purpleOrbMeshPlaceholder->setIgnoreRayIntersection(true);
        _purpleOrbMeshPlaceholder->setDrawInFront(false);
        _purpleOrbMeshPlaceholderID = qApp->getOverlays().addOverlay(_purpleOrbMeshPlaceholder);
        // Position focus
        _purpleOrbMeshPlaceholder->setWorldOrientation(glm::quat(0.0f, 0.0f, 0.0f, 1.0));
        _purpleOrbMeshPlaceholder->setWorldPosition(glm::vec3(476.0f, 500.0f, 493.0f));
        _purpleOrbMeshPlaceholder->setDimensions(glm::vec3(0.5f, 0.5f, 0.5f));
        _purpleOrbMeshPlaceholder->setVisible(true);
    }
    
}
