#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>

FadeEffect::FadeEffect() :
	_isDebugEnabled{ false },
	_debugFadePercent{ 0.f }
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

render::ShapeKey::Builder FadeEffect::getKeyBuilder() const
{
	render::ShapeKey::Builder	builder;

	if (_isDebugEnabled) {
		// Force fade for everyone
		builder.withFade();
	}

	return builder;
}

