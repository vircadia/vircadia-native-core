//
//  AnimContext.h
//
//  Created by Anthony J. Thibault on 9/19/16.
//  Copyright (c) 2016 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimContext_h
#define hifi_AnimContext_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QString>
#include <QStringList>
#include <map>

enum class AnimNodeType {
    Clip = 0,
    BlendLinear,
    BlendLinearMove,
    Overlay,
    StateMachine,
    RandomSwitchStateMachine,
    Manipulator,
    InverseKinematics,
    DefaultPose,
    TwoBoneIK,
    SplineIK,
    PoleVectorConstraint,
    BlendDirectional,
    NumTypes
};

enum AnimBlendType {
    AnimBlendType_Normal,
    AnimBlendType_AddRelative,
    AnimBlendType_AddAbsolute,
    AnimBlendType_NumTypes
};

class AnimContext {
public:
    AnimContext() {}
    AnimContext(bool enableDebugDrawIKTargets, bool enableDebugDrawIKConstraints, bool enableDebugDrawIKChains,
                const glm::mat4& geometryToRigMatrix, const glm::mat4& rigToWorldMatrix, int evaluationCount);

    bool getEnableDebugDrawIKTargets() const { return _enableDebugDrawIKTargets; }
    bool getEnableDebugDrawIKConstraints() const { return _enableDebugDrawIKConstraints; }
    bool getEnableDebugDrawIKChains() const { return _enableDebugDrawIKChains; }
    const glm::mat4& getGeometryToRigMatrix() const { return _geometryToRigMatrix; }
    const glm::mat4& getRigToWorldMatrix() const { return _rigToWorldMatrix; }
    int getEvaluationCount() const { return _evaluationCount; }

    float getDebugAlpha(const QString& key) const {
        auto it = _debugAlphaMap.find(key);
        if (it != _debugAlphaMap.end()) {
            return std::get<0>(it->second);
        } else {
            return 1.0f;
        }
    }

    using DebugAlphaMapValue = std::tuple<float, AnimNodeType>;
    using DebugAlphaMap = std::map<QString, DebugAlphaMapValue>;

    void setDebugAlpha(const QString& key, float alpha, AnimNodeType type) const {
        _debugAlphaMap[key] = DebugAlphaMapValue(alpha, type);
    }

    const DebugAlphaMap& getDebugAlphaMap() const {
        return _debugAlphaMap;
    }

    using DebugStateMachineMapValue = QString;
    using DebugStateMachineMap = std::map<QString, DebugStateMachineMapValue>;

    void addStateMachineInfo(const QString& stateMachineName, const QString& currentState, const QString& previousState, bool duringInterp, float alpha) const {
        if (duringInterp) {
            _stateMachineMap[stateMachineName] = QString("%1: %2 -> %3 (%4)").arg(stateMachineName).arg(previousState).arg(currentState).arg(QString::number(alpha, 'f', 2));
        } else {
            _stateMachineMap[stateMachineName] = QString("%1: %2").arg(stateMachineName).arg(currentState);
        }
    }

    const DebugStateMachineMap& getStateMachineMap() const { return _stateMachineMap; }

protected:

    bool _enableDebugDrawIKTargets { false };
    bool _enableDebugDrawIKConstraints { false };
    bool _enableDebugDrawIKChains { false };
    glm::mat4 _geometryToRigMatrix;
    glm::mat4 _rigToWorldMatrix;
    int _evaluationCount{ 0 };

    // used for debugging internal state of animation system.
    mutable DebugAlphaMap _debugAlphaMap;
    mutable DebugStateMachineMap _stateMachineMap;
};

#endif  // hifi_AnimContext_h
