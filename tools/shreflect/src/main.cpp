//
//  Bradley Austin Davis on 2018/05/24
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


#include <algorithm>
#include <ctime>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <fstream>
#include <streambuf>
#include <json.hpp>

using json = nlohmann::json;

std::vector<std::string> splitStringIntoLines(const std::string& s) {
    std::stringstream ss(s);
    std::vector<std::string> result;
    std::string line;
    while (std::getline(ss, line, '\n')) {
        result.push_back(line);
    }
    return result;
}

std::string readFile(const std::string& file) {
    std::ifstream t(file);
    std::string str((std::istreambuf_iterator<char>(t)),
        std::istreambuf_iterator<char>());
    return str;
}

void writeFile(const std::string& file, const std::string& out) {
    std::ofstream t(file, std::ios::trunc);
    t << out;
    t.close();
}

// Convert a Perl style multi-line commented regex into a C++ style regex
// All whitespace will be removed and lines with '#' comments will have the comments removed
std::string getUnformattedRegex(const std::string& formatted) {
    static const std::regex WHITESPACE = std::regex("\\s+");
    static const std::string EMPTY;
    std::string result;
    result.reserve(formatted.size());
    auto lines = splitStringIntoLines(formatted);
    for (auto line : lines) {
        auto commentStart = line.find('#');
        if (std::string::npos != commentStart) {
            line = line.substr(0, commentStart);
        }
        line = std::regex_replace(line, WHITESPACE, EMPTY);
        result += line;
    }

    return result;
}

static std::string LAYOUT_REGEX_STRING{ R"REGEX(
^layout\(                       # BEGIN layout declaration block
  (\s*std140\s*,\s*)?           # Optional std140 marker
  (binding|location)            # binding / location
  \s*=\s*
  (?:
    (\b\d+\b)                   # literal numeric binding like binding=1
    |
    (\b[A-Z_0-9]+\b)            # Preprocessor macro binding like binding=GPU_TEXTURE_FOO
  )
\)\s*                           # END layout declaration block
(?:
  (                             # Texture or simple uniform like `layout(binding=0) uniform sampler2D originalTexture;`
    uniform\s+
    (\b\w+\b)\s+
    (\b\w+\b)\s*
    (?:\[\d*\])?
    (?:\s*=.*)?
    \s*;.*$
  )
  |
  (                             # UBO or SSBO like `layout(std140, binding=GPU_STORAGE_TRANSFORM_OBJECT) buffer transformObjectBuffer {`
    \b(uniform|buffer)\b\s+
    \b(\w+\b)
    \s*\{.*$
  )
  | 
  (                             # Input or output attribute like `layout(location=GPU_ATTR_POSITION) in vec4 inPosition;`
    \b(in|out)\b\s+
    \b(\w+)\b\s+
    \b(\w+)\b\s*;\s*$
  )
)
)REGEX" };

enum Groups {
    STD140 = 1,
    LOCATION_TYPE = 2,
    LOCATION_LITERAL = 3,
    LOCATION_DEFINE = 4,
    DECL_SIMPLE = 5,
    SIMPLE_TYPE = 6,
    SIMPLE_NAME = 7,
    DECL_STRUCT = 8,
    STRUCT_TYPE = 9,
    STRUCT_NAME = 10,
    DECL_INOUT = 11,
    INOUT_DIRECTION = 12,
    INOUT_TYPE = 13,
    INOUT_NAME = 14,
};

json reflectShader(const std::string& shaderPath) {
    static const std::regex DEFINE("^#define\\s+([_A-Z0-9]+)\\s+(\\d+)\\s*$");
    static const std::regex LAYOUT_QUALIFIER{ getUnformattedRegex(LAYOUT_REGEX_STRING) };


    auto shaderSource = readFile(shaderPath);
    std::vector<std::string> lines = splitStringIntoLines(shaderSource);
    using Map = std::unordered_map<std::string, int>;

    json inputs;
    json outputs;
    json textures;
    json textureTypes;
    json uniforms;
    json storageBuffers;
    json uniformBuffers;
    Map locationDefines;
    for (const auto& line : lines) {
        std::cmatch m;
        if (std::regex_match(line.c_str(), m, DEFINE)) {
            locationDefines[m[1].str()] = std::stoi(m[2].first);
        } else if (std::regex_match(line.c_str(), m, LAYOUT_QUALIFIER)) {
            int binding = -1;
            if (m[LOCATION_LITERAL].matched) {
                binding = std::stoi(m[LOCATION_LITERAL].str());
            } else {
                binding = locationDefines[m[LOCATION_DEFINE].str()];
            }
            if (m[DECL_SIMPLE].matched) {
                auto name = m[SIMPLE_NAME].str();
                auto type = m[SIMPLE_TYPE].str();
                bool isTexture = 0 == type.find("sampler");
                auto& map = isTexture ? textures : uniforms;
                map[name] = binding;
                if (isTexture) {
                    textureTypes[name] = type;
                }
            } else if (m[DECL_STRUCT].matched) {
                auto name = m[STRUCT_NAME].str();
                auto type = m[STRUCT_TYPE].str();
                auto& map = (type == "buffer") ? storageBuffers : uniformBuffers;
                map[name] = binding;
            } else if (m[DECL_INOUT].matched) {
                auto name = m[INOUT_NAME].str();
                auto& map = (m[INOUT_DIRECTION].str() == "in") ? inputs : outputs;
                map[name] = binding;
            }
        }
    }

    json result;
    if (!inputs.empty()) {
        result["inputs"] = inputs;
    }
    if (!outputs.empty()) {
        result["outputs"] = outputs;
    }
    if (!textures.empty()) {
        result["textures"] = textures;
    }
    if (!textureTypes.empty()) {
        result["texturesTypes"] = textureTypes;
    }
    if (!uniforms.empty()) {
        result["uniforms"] = uniforms;
    }
    if (!storageBuffers.empty()) {
        result["storageBuffers"] = storageBuffers;
    }
    if (!uniformBuffers.empty()) {
        result["uniformBuffers"] = uniformBuffers;
    }

    result["name"] = shaderPath;

    return result;
}

int main (int argc, char** argv) {
    auto path = std::string(argv[1]);
    auto shaderReflection = reflectShader(path);
    writeFile(path + ".json", shaderReflection.dump(4));
    return 0;
}
