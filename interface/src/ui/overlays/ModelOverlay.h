//
//  ModelOverlay.h
//
//
//  Created by Clement on 6/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelOverlay_h
#define hifi_ModelOverlay_h

#include <Model.h>

#include "Volume3DOverlay.h"

class ModelOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    ModelOverlay();
    ModelOverlay(const ModelOverlay* modelOverlay);

    virtual void update(float deltatime) override;
    virtual void render(RenderArgs* args) override;
    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                        BoxFace& face, glm::vec3& surfaceNormal) override;
    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction,
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, QString& extraInfo) override;

    virtual ModelOverlay* createClone() const override;

    virtual bool addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) override;
    virtual void removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) override;

	void locationChanged(bool tellPhysics = true) override;

	// Basic ease-in-ease-out function for smoothing values.
	inline float easeInOutQuad(float lerpValue) {
		assert(!((lerpValue < 0.0f) || (lerpValue > 1.0f)));

		if (lerpValue < 0.5f) {
			return (2.0f * lerpValue * lerpValue);
		}

		return (lerpValue*(4.0f - 2.0f * lerpValue) - 1.0f);
	}

protected:
	const float SMOOTH_TIME_POSITION = 0.125f;
	const float SMOOTH_TIME_ORIENTATION = 0.15f;

	// Smoothing data for blending from one position/orientation to another on remote agents.
	float _smoothPositionTime;
	float _smoothPositionTimer;
	float _smoothOrientationTime;
	float _smoothOrientationTimer;
	glm::vec3 _smoothPositionInitial;
	glm::vec3 _smoothPositionTarget;
	glm::quat _smoothOrientationInitial;
	glm::quat _smoothOrientationTarget;

protected:
    // helper to extract metadata from our Model's rigged joints
    template <typename itemType> using mapFunction = std::function<itemType(int jointIndex)>;
    template <typename vectorType, typename itemType>
        vectorType mapJoints(mapFunction<itemType> function) const;

private:

    ModelPointer _model;
    QVariantMap _modelTextures;

    QUrl _url;
    bool _updateModel = { false };
    bool _scaleToFit = { false };
};

#endif // hifi_ModelOverlay_h
