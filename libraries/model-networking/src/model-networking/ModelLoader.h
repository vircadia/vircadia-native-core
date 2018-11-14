//
//  ModelLoader.h
//  libraries/model-networking/src/model-networking
//
//  Created by Sabrina Shanman on 2018/11/13.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelLoader_h
#define hifi_ModelLoader_h

#include <vector>
#include <string>
#include <functional>

#include <shared/Hifitypes.h>
#include <hfm/HFM.h>

class ModelLoader {
public:

    // A short sequence of bytes, typically at the beginning of the file, which identifies the file format
    class FileSignature {
    public:
        FileSignature(const std::string& bytes, int byteOffset) :
            bytes(bytes),
            byteOffset(byteOffset) {
        }

        std::string bytes;
        int byteOffset;
    };

    // A named file extension with a list of known ways to positively identify the file type
    class MIMEType {
    public:
        MIMEType(const std::string& name) :
            name(name) {
        }

        std::string name;
        std::vector<std::string> extensions;
        std::vector<std::string> webMediaTypes;
        std::vector<FileSignature> fileSignatures;
    };

    // T is a subclass of hfm::Serializer
    template <typename T>
    void addSupportedFormat(const MIMEType& mimeType) {
        supportedFormats.push_back(SupportedFormat(mimeType, SupportedFormat::getLoader<T>()));
    }

    // Given the currently stored list of supported file formats, determine how to load a model from the given parameters.
    // If successful, return an owned reference to the newly loaded model.
    // If failed, return an empty reference.
    hfm::Model::Pointer load(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const;
    
protected:
    using Loader = std::function<hfm::Model::Pointer(const hifi::ByteArray&, const hifi::VariantHash&, const hifi::URL&)>;

    class SupportedFormat {
    public:
        SupportedFormat(const MIMEType& mimeType, const Loader& loader) :
            mimeType(mimeType),
            loader(loader) {
        }

        MIMEType mimeType;
        Loader loader;

        template <typename T>
        static Loader getLoader() {
            assert(dynamic_cast<hfm::Serializer*>(&T()));

            return [](const hifi::ByteArray& bytes, const hifi::VariantHash& mapping, const hifi::URL& url) -> hfm::Model::Pointer {
                return T().read(bytes, mapping, url);
            };
        }
    };

    std::vector<SupportedFormat> supportedFormats;
};

#endif // hifi_ModelLoader_h
