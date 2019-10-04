//
//  Created by Bradley Austin Davis on 2019/10/03
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <gpu/FrameIO.h>
#include <gpu/Batch.h>
#include <gpu/FrameIOKeys.h>
#include <shared/Storage.h>
#include <nlohmann/json.hpp>
#include <shared/FileUtils.h>

#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using Json = nlohmann::json;
using Paths = std::vector<std::filesystem::path>;

const char* DEFAULT_SOURCE_PATH{ "D:/Frames" };
static const std::string OLD_FRAME_EXTENSION{ ".json" };
static const std::string OLD_BINARY_EXTENSION{ ".bin" };
static const std::string NEW_EXTENSION{ gpu::hfb::EXTENSION };

inline std::string readFileToString(const fs::path& path) {
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

inline gpu::hfb::Buffer readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    size_t size = file.tellg();
    if (!size) {
        return {};
    }
    file.seekg(0, std::ios::beg);

    gpu::hfb::Buffer result;
    result.resize(size);
    if (!file.read((char*)result.data(), size)) {
        throw std::runtime_error("Failed to read file");
    }
    return result;
}

Paths getFrames(const std::string& sourcePath) {
    Paths result;
    for (auto& p : fs::directory_iterator(sourcePath)) {
        if (!p.is_regular_file()) {
            continue;
        }
        const auto& path = p.path();
        if (path.string().find(".hfb.json") != std::string::npos) {
            continue;
        }
        if (path.extension().string() == OLD_FRAME_EXTENSION) {
            result.push_back(path);
        }
    }
    return result;
}

void convertFrame(const fs::path& path) {
    auto name = path.filename().string();
    name = name.substr(0, name.length() - OLD_FRAME_EXTENSION.length());

    auto frameNode = Json::parse(readFileToString(path));
    auto capturedTexturesNode = frameNode[gpu::keys::capturedTextures];

    gpu::hfb::Buffer binary = readFile(path.parent_path() / (name + OLD_BINARY_EXTENSION));
    gpu::hfb::Buffers pngs;
    for (const auto& capturedTextureIndexNode : capturedTexturesNode) {
        int index = capturedTextureIndexNode;
        auto imageFile = path.parent_path() / (name + "." + std::to_string(index) + ".0.png");
        frameNode[gpu::keys::textures][index][gpu::keys::chunk] = 2 + pngs.size();
        pngs.push_back(readFile(imageFile));
    }
    frameNode.erase(gpu::keys::capturedTextures);
    auto outputPath = path.parent_path() / (name + NEW_EXTENSION);
    {
        auto jsonOutputPath = path.parent_path() / (name + ".hfb.json");
        std::ofstream of(jsonOutputPath);
        auto str = frameNode.dump(2);
        of.write(str.data(), str.size());
    }
    gpu::hfb::writeFrame(outputPath.string(), frameNode.dump(), binary, pngs);
    {
        auto frameBuffer = readFile(outputPath.string());
        auto descriptor = gpu::hfb::Descriptor::parse(frameBuffer.data(), frameBuffer.size());
        std::cout << descriptor.header.magic << std::endl;
    }
}

int main(int argc, char** argv) {
    for (const auto& framePath : getFrames(DEFAULT_SOURCE_PATH)) {
        std::cout << framePath << std::endl;
        convertFrame(framePath);
    }
    return 0;
}
