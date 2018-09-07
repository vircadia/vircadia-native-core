//
//  Bloom.h
//  libraries/graphics/src/graphics
//
//  Created by Sam Gondelman on 8/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Bloom_h
#define hifi_model_Bloom_h

#include <memory>

namespace graphics {
    class Bloom {
    public:
        // Initial values
        static const float INITIAL_BLOOM_INTENSITY;
        static const float INITIAL_BLOOM_THRESHOLD;
        static const float INITIAL_BLOOM_SIZE;

        Bloom() {}

        void setBloomIntensity(const float bloomIntensity) { _bloomIntensity = bloomIntensity; }
        void setBloomThreshold(const float bloomThreshold) { _bloomThreshold = bloomThreshold; }
        void setBloomSize(const float bloomSize) { _bloomSize = bloomSize; }

        float getBloomIntensity() { return _bloomIntensity; }
        float getBloomThreshold() { return _bloomThreshold; }
        float getBloomSize() { return _bloomSize; }

    private:
        float _bloomIntensity { INITIAL_BLOOM_INTENSITY };
        float _bloomThreshold {INITIAL_BLOOM_THRESHOLD };
        float _bloomSize { INITIAL_BLOOM_SIZE };
    };
    using BloomPointer = std::shared_ptr<Bloom>;
}
#endif // hifi_model_Bloom_h
