//
//  ImageOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ImageOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>

#include "qapplication.h"

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

ImageOverlay::ImageOverlay() :
    _imageURL(),
    _renderImage(false),
    _wantClipFromImage(false)
{
}

ImageOverlay::ImageOverlay(const ImageOverlay* imageOverlay) :
    Overlay2D(imageOverlay),
    _imageURL(imageOverlay->_imageURL),
    _textureImage(imageOverlay->_textureImage),
    _texture(imageOverlay->_texture),
    _fromImage(imageOverlay->_fromImage),
    _renderImage(imageOverlay->_renderImage),
    _wantClipFromImage(imageOverlay->_wantClipFromImage)
{
}

// TODO: handle setting image multiple times, how do we manage releasing the bound texture?
void ImageOverlay::setImageURL(const QUrl& url) {
    _imageURL = url;
    if (url.isEmpty()) {
        _isLoaded = true;
        _renderImage = false;
        _texture.clear();
    } else {
        _isLoaded = false;
        _renderImage = true;
    }
}

void ImageOverlay::render(RenderArgs* args) {
    if (!_isLoaded && _renderImage) {
        _isLoaded = true;
        _texture = DependencyManager::get<TextureCache>()->getTexture(_imageURL);
    }

    // If we are not visible or loaded, return.  If we are trying to render an
    // image but the texture hasn't loaded, return.
    if (!_visible || !_isLoaded || (_renderImage && !_texture->isLoaded())) {
        return;
    }

    // TODO: I commented all the code needed to migrate this ImageOverlay rendering from naked gl to gpu::Batch
    /*gpu::Batch localBatch;
    gpu::Batch& batch = (args->_batch ? (*args->_batch) : localBatch);
    static gpu::PipelinePointer drawPipeline;
    static int texcoordRectLoc = -1;
    static int colorLoc = -1;
    if (!drawPipeline) {
        auto blitProgram = gpu::StandardShaderLib::getProgram(gpu::StandardShaderLib::getDrawTexcoordRectTransformUnitQuadVS, gpu::StandardShaderLib::getDrawColoredTexturePS);
        gpu::Shader::makeProgram(*blitProgram);
        texcoordRectLoc = blitProgram->getUniforms().findLocation("texcoordRect");
        colorLoc = blitProgram->getUniforms().findLocation("color");

        gpu::StatePointer blitState = gpu::StatePointer(new gpu::State());
        blitState->setBlendFunction(false, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        blitState->setColorWriteMask(true, true, true, true);
        drawPipeline = gpu::PipelinePointer(gpu::Pipeline::create(blitProgram, blitState));
    }
    */
    // TODO: batch.setPipeline(drawPipeline);
    glUseProgram(0);

    if (_renderImage) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _texture->getID());
        // TODO: batch.setResourceTexture(0, _texture->getGPUTexture());
    } // TODO: else {
    // TODO:     batch.setResourceTexture(0, args->_whiteTexture);
    // TODO: }

    // TODO: batch.setViewTransform(Transform());

    const float MAX_COLOR = 255.0f;
    xColor color = getColor();
    float alpha = getAlpha();
    glm::vec4 quadColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);
    // TODO: batch._glUniform4fv(colorLoc, 1, (const float*) &quadColor);

    int left = _bounds.left();
    int right = _bounds.right() + 1;
    int top = _bounds.top();
    int bottom = _bounds.bottom() + 1;

    glm::vec2 topLeft(left, top);
    glm::vec2 bottomRight(right, bottom);

    // TODO: Transform model;
    // TODO: model.setTranslation(glm::vec3(0.5f * (right + left), 0.5f * (top + bottom), 0.0f));
    // TODO: model.setScale(glm::vec3(0.5f * (right - left), 0.5f * (bottom - top), 1.0f));
    // TODO: batch.setModelTransform(model);


    // if for some reason our image is not over 0 width or height, don't attempt to render the image
    if (_renderImage) {
        float imageWidth = _texture->getWidth();
        float imageHeight = _texture->getHeight();
        if (imageWidth > 0 && imageHeight > 0) {
            QRect fromImage;
            if (_wantClipFromImage) {
                float scaleX = imageWidth / _texture->getOriginalWidth();
                float scaleY = imageHeight / _texture->getOriginalHeight();

                fromImage.setX(scaleX * _fromImage.x());
                fromImage.setY(scaleY * _fromImage.y());
                fromImage.setWidth(scaleX * _fromImage.width());
                fromImage.setHeight(scaleY * _fromImage.height());
            }
            else {
                fromImage.setX(0);
                fromImage.setY(0);
                fromImage.setWidth(imageWidth);
                fromImage.setHeight(imageHeight);
            }

            float x = fromImage.x() / imageWidth;
            float y = fromImage.y() / imageHeight;
            float w = fromImage.width() / imageWidth; // ?? is this what we want? not sure
            float h = fromImage.height() / imageHeight;

            glm::vec2 texCoordTopLeft(x, y);
            glm::vec2 texCoordBottomRight(x + w, y + h);
            glm::vec4 texcoordRect(texCoordTopLeft, w, h);

            // TODO: batch._glUniform4fv(texcoordRectLoc, 1, (const float*) &texcoordRect);
            // TODO: batch.draw(gpu::TRIANGLE_STRIP, 4);
            DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor);
        } else {
            // TODO: batch.draw(gpu::TRIANGLE_STRIP, 4);
            DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, quadColor);
        }
        glDisable(GL_TEXTURE_2D);
    } else {
        // TODO: batch.draw(gpu::TRIANGLE_STRIP, 4);
        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, quadColor);
    }

    // TODO: if (!args->_batch) {
    // TODO:     args->_context->render(batch);
    // TODO: }
}

void ImageOverlay::setProperties(const QScriptValue& properties) {
    Overlay2D::setProperties(properties);

    QScriptValue subImageBounds = properties.property("subImage");
    if (subImageBounds.isValid()) {
        QRect oldSubImageRect = _fromImage;
        QRect subImageRect = _fromImage;
        if (subImageBounds.property("x").isValid()) {
            subImageRect.setX(subImageBounds.property("x").toVariant().toInt());
        } else {
            subImageRect.setX(oldSubImageRect.x());
        }
        if (subImageBounds.property("y").isValid()) {
            subImageRect.setY(subImageBounds.property("y").toVariant().toInt());
        } else {
            subImageRect.setY(oldSubImageRect.y());
        }
        if (subImageBounds.property("width").isValid()) {
            subImageRect.setWidth(subImageBounds.property("width").toVariant().toInt());
        } else {
            subImageRect.setWidth(oldSubImageRect.width());
        }
        if (subImageBounds.property("height").isValid()) {
            subImageRect.setHeight(subImageBounds.property("height").toVariant().toInt());
        } else {
            subImageRect.setHeight(oldSubImageRect.height());
        }
        setClipFromSource(subImageRect);
    }

    QScriptValue imageURL = properties.property("imageURL");
    if (imageURL.isValid()) {
        setImageURL(imageURL.toVariant().toString());
    }
}

QScriptValue ImageOverlay::getProperty(const QString& property) {
    if (property == "subImage") {
        return qRectToScriptValue(_scriptEngine, _fromImage);
    }
    if (property == "imageURL") {
        return _imageURL.toString();
    }

    return Overlay2D::getProperty(property);
}
ImageOverlay* ImageOverlay::createClone() const {
    return new ImageOverlay(this);
}
