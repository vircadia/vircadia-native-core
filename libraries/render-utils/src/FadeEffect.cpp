#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>
#include <NumericalConstants.h>
#include <Interpolate.h>
#include <render/ShapePipeline.h>

FadeEffect::FadeEffect() :
	_isDebugEnabled{ false },
	_debugFadePercent{ 0.f }
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

render::ShapeKey::Builder FadeEffect::getKeyBuilder() const {
	render::ShapeKey::Builder	builder;

	if (_isDebugEnabled) {
		// Force fade for everyone
		builder.withFade();
	}

	return builder;
}

void FadeEffect::bindPerBatch(gpu::Batch& batch) const
{
    batch.setResourceTexture(render::ShapePipeline::Slot::MAP::FADE_MASK, _fadeMaskMap);
}

float FadeEffect::computeFadePercent(quint64 startTime) const {
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / (double)(3 * USECS_PER_SECOND);
    double fraction = (double)(usecTimestampNow() - startTime) * INV_FADE_PERIOD;
    if (fraction < 1.0) {
        fadeAlpha = Interpolate::simpleNonLinearBlend(fraction);
    }
    return fadeAlpha;
}

void FadeEffect::bindPerItem(gpu::Batch& batch, RenderArgs* args, glm::vec3 offset, quint64 startTime, State state) const {
    if (state != Complete || _isDebugEnabled) {
        const gpu::ShaderPointer& program = args->_pipeline->pipeline->getProgram();
        int fadeOffsetLoc = program->getUniforms().findLocation("fadeOffset");
        int fadePercentLoc = program->getUniforms().findLocation("fadePercent");
        float percent;

        // A bit ugly to have the test at every bind...
        if (!_isDebugEnabled) {
            percent = computeFadePercent(startTime);
        }
        else {
            percent = _debugFadePercent;
        }
        batch._glUniform1f(fadePercentLoc, percent);
        batch._glUniform3f(fadeOffsetLoc, offset.x, offset.y, offset.z);
    }
}
