//
//  ModelLoader.cpp
//  libraries/model-networking/src/model-networking
//
//  Created by Sabrina Shanman on 2018/11/14.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelLoader.h"

#include "FBXSerializer.h"
#include "OBJSerializer.h"
#include "GLTFSerializer.h"

ModelLoader::ModelLoader() {
    // Add supported model formats

    MIMEType fbxMIMEType("fbx");
    fbxMIMEType.extensions.push_back("fbx");
    fbxMIMEType.fileSignatures.emplace_back("Kaydara FBX Binary  \x00", 0);
    addSupportedFormat<FBXSerializer>(fbxMIMEType);

    MIMEType objMIMEType("obj");
    objMIMEType.extensions.push_back("obj");
    addSupportedFormat<OBJSerializer>(objMIMEType);

    MIMEType gltfMIMEType("gltf");
    gltfMIMEType.extensions.push_back("gltf");
    gltfMIMEType.webMediaTypes.push_back("model/gltf+json");
    addSupportedFormat<GLTFSerializer>(gltfMIMEType);
}

hfm::Model::Pointer ModelLoader::load(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const {
    // Check file contents
    for (auto& supportedFormat : supportedFormats) {
        for (auto& fileSignature : supportedFormat.mimeType.fileSignatures) {
            auto testBytes = data.mid(fileSignature.byteOffset, (int)fileSignature.bytes.size()).toStdString();
            if (testBytes == fileSignature.bytes) {
                return supportedFormat.loader(data, mapping, url);
            }
        }
    }

    // Check file extension
    std::string urlString = url.path().toStdString();
    std::size_t extensionSeparator = urlString.rfind('.');
    if (extensionSeparator != std::string::npos) {
        std::string detectedExtension = urlString.substr(extensionSeparator + 1);
        for (auto& supportedFormat : supportedFormats) {
            for (auto& extension : supportedFormat.mimeType.extensions) {
                if (extension == detectedExtension) {
                    return supportedFormat.loader(data, mapping, url);
                }
            }
        }
    }

    // Check web media type
    if (webMediaType != "") {
        for (auto& supportedFormat : supportedFormats) {
            for (auto& candidateWebMediaType : supportedFormat.mimeType.webMediaTypes) {
                if (candidateWebMediaType == webMediaType) {
                    return supportedFormat.loader(data, mapping, url);
                }
            }
        }
    }

    // Supported file type not found. Abort loading.
    return hfm::Model::Pointer();
}
