//
//  FadeManager.h
//  libraries/render-utils/src/
//
//  Created by Olivier Prat on 06/06/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_FadeManager_h
#define hifi_FadeManager_h

#include <DependencyManager.h>
#include <gpu/Pipeline.h>
#include <render/ShapePipeline.h>

// Centralizes fade effect data and functions
class FadeManager : public Dependency {
	SINGLETON_DEPENDENCY
public:

	FadeManager();

	const gpu::TexturePointer getFadeMaskMap() const { return _fadeMaskMap; }

    void setDebugEnabled(bool isEnabled) { _isDebugEnabled = isEnabled; }
	bool isDebugEnabled() const { return _isDebugEnabled; }

    void setDebugFadePercent(float value) { assert(value >= 0.f && value <= 1.f); _debugFadePercent = value; }
    float getDebugFadePercent() const { return _debugFadePercent; }

	render::ShapeKey::Builder getKeyBuilder() const;

private:

	gpu::TexturePointer _fadeMaskMap;
	float _debugFadePercent;
	bool _isDebugEnabled;

};

#endif // hifi_FadeManager_h
