//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestHelpers.h"
#include <QtCore/QFileInfo>

gpu::ShaderPointer makeShader(const std::string & vertexShaderSrc, const std::string & fragmentShaderSrc, const gpu::Shader::BindingSet & bindings) {
    auto vs = gpu::Shader::createVertex(vertexShaderSrc);
    auto fs = gpu::Shader::createPixel(fragmentShaderSrc);
    auto shader = gpu::Shader::createProgram(vs, fs);
    if (!gpu::Shader::makeProgram(*shader, bindings)) {
        printf("Could not compile shader\n");
        exit(-1);
    }
    return shader;
}

QString projectRootDir() {
    static QString projectRootPath = QFileInfo(QFileInfo(__FILE__).absolutePath() + "/..").absoluteFilePath();
    return projectRootPath;
}
