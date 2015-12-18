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

class SoftAttachmentModel : public Model {
    Q_OBJECT

public:

    SoftAttachmentModel(RigPointer rig, QObject* parent, RigPointer rigOverride);
    ~SoftAttachmentModel();

    virtual void updateRig(float deltaTime, glm::mat4 parentTransform) override;
    virtual void updateClusterMatrices(glm::vec3 modelPosition, glm::quat modelOrientation) override;

protected:
    int getJointIndexOverride(int i) const;

    RigPointer _rigOverride;
};

#endif // hifi_SoftAttachmentModel_h
