//
//  Stars.h
//  interface/src
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stars_h
#define hifi_Stars_h

#include <gpu/Context.h>

class RenderArgs;

// Starfield rendering component. 
class Stars  {
public:
    Stars() = default;
    ~Stars() = default;

    Stars(Stars const&) = delete;
    Stars& operator=(Stars const&) = delete;

    // Renders the starfield from a local viewer's perspective.
    // The parameters specifiy the field of view.
    void render(RenderArgs* args, float alpha);

private:
    // Pipelines
    static gpu::PipelinePointer _gridPipeline;
    static gpu::PipelinePointer _starsPipeline;
    static int32_t _timeSlot;

    // Buffers
    gpu::BufferPointer vertexBuffer;
    gpu::Stream::FormatPointer streamFormat;
    gpu::BufferView positionView;
    gpu::BufferView colorView;
    std::once_flag once;

    void init();
};


#endif // hifi_Stars_h
