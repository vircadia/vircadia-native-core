//
//  HMDScriptingInterface.h
//  interface/src/scripting
//
//  Created by Thijs Wenker on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HMDScriptingInterface_h
#define hifi_HMDScriptingInterface_h

#include <QtScript/QScriptValue>
class QScriptContext;
class QScriptEngine;

#include <GLMHelpers.h>
#include <DependencyManager.h>
#include <display-plugins/AbstractHMDScriptingInterface.h>


class HMDScriptingInterface : public AbstractHMDScriptingInterface, public Dependency {
    Q_OBJECT
    Q_PROPERTY(bool magnifier READ getMagnifier)
    Q_PROPERTY(glm::vec3 position READ getPosition)
    Q_PROPERTY(glm::quat orientation READ getOrientation)
public:
    HMDScriptingInterface();
    static QScriptValue getHUDLookAtPosition2D(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue getHUDLookAtPosition3D(QScriptContext* context, QScriptEngine* engine);

public slots:
    void toggleMagnifier();

private:
    bool getMagnifier() const; 
    // Get the position of the HMD
    glm::vec3 getPosition() const;
    
    // Get the orientation of the HMD
    glm::quat getOrientation() const;

    bool getHUDLookAtPosition3D(glm::vec3& result) const;
    glm::mat4 getWorldHMDMatrix() const;
};

#endif // hifi_HMDScriptingInterface_h
