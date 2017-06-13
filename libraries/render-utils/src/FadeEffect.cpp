#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>
#include <NumericalConstants.h>
#include <Interpolate.h>
#include <render/ShapePipeline.h>

FadeEffect::FadeEffect() :
    _invScale{ 1.f },
    _duration{ 3.f },
    _debugFadePercent{ 0.f },
	_isDebugEnabled{ false }
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

render::ShapeKey::Builder FadeEffect::getKeyBuilder(render::ShapeKey::Builder builder) const {
	if (_isDebugEnabled) {
		// Force fade for everyone
		builder.withFade();
	}
	return builder;
}

void FadeEffect::bindPerBatch(gpu::Batch& batch, int fadeMaskMapLocation) const {
    batch.setResourceTexture(fadeMaskMapLocation, _fadeMaskMap);
}

void FadeEffect::bindPerBatch(gpu::Batch& batch, const gpu::PipelinePointer& pipeline) const {
    auto program = pipeline->getProgram();
    auto maskMapLocation = program->getTextures().findLocation("fadeMaskMap");
    bindPerBatch(batch, maskMapLocation);
}

float FadeEffect::computeFadePercent(quint64 startTime) const {
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / (double)(_duration * USECS_PER_SECOND);
    double fraction = (double)(usecTimestampNow() - startTime) * INV_FADE_PERIOD;
    if (fraction < 1.0) {
        fadeAlpha = Interpolate::easeInOutQuad(fraction);
    }
    return fadeAlpha;
}

bool FadeEffect::bindPerItem(gpu::Batch& batch, RenderArgs* args, glm::vec3 offset, quint64 startTime, bool isFading) const {
    return bindPerItem(batch, args->_pipeline->pipeline.get(), offset, startTime, isFading);
}

bool FadeEffect::bindPerItem(gpu::Batch& batch, const gpu::Pipeline* pipeline, glm::vec3 offset, quint64 startTime, bool isFading) const {
    if (isFading || _isDebugEnabled) {
        auto& uniforms = pipeline->getProgram()->getUniforms();
        auto fadeScaleLocation = uniforms.findLocation("fadeInvScale");
        auto fadeOffsetLocation = uniforms.findLocation("fadeOffset");
        auto fadePercentLocation = uniforms.findLocation("fadePercent");

        if (fadeScaleLocation >= 0 || fadeOffsetLocation >= 0 || fadePercentLocation>=0) {
            float percent;

            // A bit ugly to have the test at every bind...
            if (!_isDebugEnabled) {
                percent = computeFadePercent(startTime);
            }
            else {
                percent = _debugFadePercent;
            }
            batch._glUniform1f(fadeScaleLocation, _invScale);
            batch._glUniform1f(fadePercentLocation, percent);
            batch._glUniform3f(fadeOffsetLocation, offset.x, offset.y, offset.z);
        }
        return true;
    }
    return false;
}
