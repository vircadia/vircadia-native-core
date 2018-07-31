//
//  Created by Bradley Austin Davis on 2018/06/02
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shaders.h"

#include <shared/FileUtils.h>
#include <mutex>
#include <regex>
#include <sstream>
#include <unordered_map>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/qdebug.h>
#include <QtCore/QProcessEnvironment>

#include <gl/GLHelpers.h>

static bool cleanShaders() {
#if defined(Q_OS_MAC)
    static const bool CLEAN_SHADERS = true;
#else
    static const bool CLEAN_SHADERS = ::gl::disableGl45();

#endif
    return CLEAN_SHADERS;
}

// Can't use the Q_INIT_RESOURCE macro inside a namespace on Mac,
// so this is done out of line
void initShaders() {
    static std::once_flag once;
    std::call_once(once, [] {
        Q_INIT_RESOURCE(shaders);
    });
}

static std::vector<std::string> splitStringIntoLines(const std::string& s) {
    std::stringstream ss(s);
    std::vector<std::string> result;

    std::string line;
    while (std::getline(ss, line, '\n')) {
        result.push_back(line);
    }
    return result;
}

static std::string loadResource(const std::string& path) {
    return FileUtils::readFile(path.c_str()).toStdString();
}

namespace shader {

void cleanShaderSource(std::string& shaderSource) {
    static const std::regex LAYOUT_REGEX{ R"REGEX(^layout\((\s*std140\s*,\s*)?(?:binding|location)\s*=\s*(?:\b\w+\b)\)\s*(?!(?:flat\s+)?(?:out|in|buffer))\b(.*)$)REGEX" };
    static const int GROUP_STD140 = 1;
    static const int THE_REST_OF_THE_OWL = 2;
    std::vector<std::string> lines = splitStringIntoLines(shaderSource);
    std::vector<std::string> outLines;
    std::unordered_map<std::string, int> locationDefines;
    for (const auto& line : lines) {
        std::cmatch m;
        if (std::regex_match(line.c_str(), m, LAYOUT_REGEX)) {
            std::string outLine;
            if (m[GROUP_STD140].matched) {
                outLine = "layout(std140) ";
            }
            outLine += m[THE_REST_OF_THE_OWL].str();
            outLines.push_back(outLine);
            continue;
            // On mac we have to strip out all the explicit binding location layouts,
            // because GL 4.1 doesn't support them
        }
        outLines.push_back(line);
    }
    std::ostringstream joined;
    std::copy(outLines.begin(), outLines.end(), std::ostream_iterator<std::string>(joined, "\n"));
    shaderSource = joined.str();
}
    
std::string loadShaderSource(uint32_t shaderId) {
    initShaders();
    auto shaderStr = loadResource(std::string(":/shaders/") + std::to_string(shaderId));
    if (cleanShaders()) {
        // OSX only supports OpenGL 4.1 without ARB_shading_language_420pack or
        // ARB_explicit_uniform_location or basically anything useful that's
        // been released in the last 8 fucking years, so in that case we need to 
        // strip out all explicit locations and do a bunch of background magic to 
        // make the system seem like it is using the explicit locations
        cleanShaderSource(shaderStr);
    }
    return shaderStr;
}
    
std::string loadShaderReflection(uint32_t shaderId) {
    initShaders();
    auto path = std::string(":/shaders/") + std::to_string(shaderId) + std::string("_reflection");
    auto json = loadResource(path);
    return json;
}

}
