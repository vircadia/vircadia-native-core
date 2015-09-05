//
//  Created by Bradley Austin Davis on 2015/09/05
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_RenderableProcedrualItem_h
#define hifi_RenderableProcedrualItem_h

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/qglobal.h>

#include <ShaderCache.h>
#include <gpu/Shader.h>
#include <gpu/Pipeline.h>
#include <gpu/Batch.h>

class EntityItem;

class RenderableProceduralItem {
protected:
    struct ProceduralInfo {
        ProceduralInfo(EntityItem* entity);
        bool ready();
        void prepare(gpu::Batch& batch);

        bool _enabled{ false };
        gpu::PipelinePointer _pipeline;
        gpu::ShaderPointer _vertexShader;
        gpu::ShaderPointer _fragmentShader;
        gpu::ShaderPointer _shader;
        QString _shaderSource;
        QString _shaderPath;
        QUrl _shaderUrl;
        quint64 _shaderModified{ 0 };
        bool _pipelineDirty{ true };
        int32_t _timeSlot{ gpu::Shader::INVALID_LOCATION };
        int32_t _scaleSlot{ gpu::Shader::INVALID_LOCATION };
        uint64_t _start{ 0 };
        NetworkShaderPointer _networkShader;
        EntityItem* _entity;
    };

    QSharedPointer<ProceduralInfo> _procedural;
};

#endif
