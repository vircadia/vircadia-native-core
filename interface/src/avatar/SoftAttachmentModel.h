//
//  SoftAttachmentModel.h
//  interface/src/avatar
//
//  Created by Anthony J. Thibault on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SoftAttachmentModel_h
#define hifi_SoftAttachmentModel_h

#include <Model.h>

// A model that allows the creator to specify a secondary rig instance.
// When the cluster matrices are created for rendering, the
// cluster matrices will use the secondary rig for the joint poses
// instead of the primary rig.
//
// This is used by Avatar instances to wear clothing that follows the same
// animated pose as the SkeletonModel.

class SoftAttachmentModel : public Model {
    Q_OBJECT

public:

    SoftAttachmentModel(RigPointer rig, QObject* parent, RigPointer rigOverride);
    ~SoftAttachmentModel();

    virtual void updateRig(float deltaTime, glm::mat4 parentTransform) override;
    virtual void updateClusterMatrices() override;

protected:
    int getJointIndexOverride(int i) const;

    RigPointer _rigOverride;
};

#endif // hifi_SoftAttachmentModel_h
