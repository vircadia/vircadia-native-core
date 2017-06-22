//
//  DeferredFrameTransform.h
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DeferredFrameTransform_h
#define hifi_DeferredFrameTransform_h

#include <gpu/Resource.h>

#include <render/Forward.h>
#include <render/DrawTask.h>

// DeferredFrameTransform is  a helper class gathering in one place the needed camera transform
// and frame resolution needed for all the deferred rendering passes taking advantage of the Deferred buffers
class DeferredFrameTransform {
public:
    using UniformBufferView = gpu::BufferView;

    DeferredFrameTransform();

    void update(RenderArgs* args);

    UniformBufferView getFrameTransformBuffer() const { return _frameTransformBuffer; }

protected:


    // Class describing the uniform buffer with the transform info common to the AO shaders
    // It s changing every frame
    class FrameTransform {
    public:
        // Pixel info is { viewport width height}
        glm::vec4 pixelInfo;
        glm::vec4 invpixelInfo;
        // Depth info is { n.f, f - n, -f}
        glm::vec4 depthInfo;
        // Stereo info is { isStereoFrame, halfWidth }
        glm::vec4 stereoInfo{ 0.0 };
        // Mono proj matrix or Left and Right proj matrix going from Mono Eye space to side clip space
        glm::mat4 projection[2];
        // THe mono projection for sure
        glm::mat4 projectionMono;
        // Inv View matrix from eye space (mono) to world space
        glm::mat4 invView;
        // View matrix from world space to eye space (mono)
        glm::mat4 view;

        FrameTransform() {}
    };
    UniformBufferView _frameTransformBuffer;

   
};

using DeferredFrameTransformPointer = std::shared_ptr<DeferredFrameTransform>;




class GenerateDeferredFrameTransform {
public:
    using JobModel = render::Job::ModelO<GenerateDeferredFrameTransform, DeferredFrameTransformPointer>;

    GenerateDeferredFrameTransform() {}

    void run(const render::RenderContextPointer& renderContext, DeferredFrameTransformPointer& frameTransform);

private:
};

#endif // hifi_DeferredFrameTransform_h
