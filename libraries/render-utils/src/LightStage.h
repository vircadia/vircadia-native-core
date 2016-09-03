//
//  LightStage.h
//  render-utils/src
//
//  Created by Zach Pomerantz on 1/14/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_LightStage_h
#define hifi_render_utils_LightStage_h

#include <set>
#include <unordered_map>

#include "gpu/Framebuffer.h"

#include "model/Light.h"

class ViewFrustum;

namespace indexed_elements {

    using Index = int32_t;
    const Index MAXIMUM_INDEX { 1 << 30 };
    const Index INVALID_INDEX { -1 };
    using Indices = std::vector< Index >;

    template <Index MaxNumElements = MAXIMUM_INDEX>
    class Allocator {
    public:
        Allocator() {}
        Indices _freeIndices;
        Index _nextNewIndex { 0 };

        bool checkIndex(Index index) const { return ((index >= 0) && (index < _nextNewIndex)); }
        Index getNumIndices() const { return _nextNewIndex - (Index) _freeIndices.size(); }

        Index allocateIndex() {
            if (_freeIndices.empty()) {
                Index index = _nextNewIndex;
                if (index >= MaxNumElements) {
                    // abort! we are trying to go overboard with the total number of allocated elements
                    assert(false);
                    // This should never happen because Bricks are allocated along with the cells and there
                    // is already a cap on the cells allocation
                    return INVALID_INDEX;
                }
                _nextNewIndex++;
                return index;
            } else {
                Index index = _freeIndices.back();
                _freeIndices.pop_back();
                return index;
            }
        }

        void freeIndex(Index index) {
            if (checkIndex(index)) {
                _freeIndices.push_back(index);
            }
        }

        void clear() {
            _freeIndices.clear();
            _nextNewIndex = 0;
        }
    };

    template <class T, Index MaxNumElements = MAXIMUM_INDEX>
    class IndexedVector {
        Allocator<MaxNumElements> _allocator;
    public:
        using Element = T;
        using Elements = std::vector<T>;

        Elements _elements;

        bool checkIndex(Index index) const { return _allocator.checkIndex(index); };
        Index getNumElements() const { return _allocator.getNumIndices(); }

        Index newElement(const Element& e) {
            Index index = _allocator.allocateIndex();
            if (index != INVALID_INDEX) {
                if (index < _elements.size()) {
                    _elements.emplace(_elements.begin() + index, e);
                } else {
                    assert(index == _elements.size());
                    _elements.emplace_back(e);
                }
            }
            return index;
        }

        const Element& freeElement(Index index) {
            _allocator.freeIndex(index);
            return _elements[index];
        }

        const Element& get(Index index) const {
            return _elements[index];
        }
        Element& edit(Index index) {
            return _elements[index];
        }
    };

    template <class T, Index MaxNumElements = MAXIMUM_INDEX>
    class IndexedPointerVector {
        Allocator<MaxNumElements> _allocator;
    public:
        using Data = T;
        using ElementPtr = std::shared_ptr<Data>;
        using Elements = std::vector<ElementPtr>;

        Elements _elements;

        bool checkIndex(Index index) const { return _allocator.checkIndex(index); };
        Index getNumElements() const { return _allocator.getNumIndices(); }

        Index newElement(const ElementPtr& e) {
            Index index = _allocator.allocateIndex();
            if (index != INVALID_INDEX) {
                if (index < _elements.size()) {
                    _elements.emplace(_elements.begin() + index, e);
                } else {
                    assert(index == _elements.size());
                    _elements.emplace_back(e);
                }
            }
            return index;
        }

        ElementPtr freeElement(Index index) {
            ElementPtr freed;
            if (checkIndex(index)) {
                _allocator.freeIndex(index);
                freed = _elements[index];
                _elements[index].reset(); // really forget it
            }
            return freed;
        }

        ElementPtr get(Index index) const {
            if (checkIndex(index)) {
                return _elements[index];
            } else {
                return ElementPtr();
            }
        }
    };



};


// Light stage to set up light-related rendering tasks
class LightStage {
public:
    using Index = indexed_elements::Index;
    static const Index INVALID_INDEX { indexed_elements::INVALID_INDEX };

    using LightPointer = model::LightPointer;
    using Lights = indexed_elements::IndexedPointerVector<model::Light>;
    using LightMap = std::unordered_map<LightPointer, Index>;

    class Shadow {
    public:
        using UniformBufferView = gpu::BufferView;
        static const int MAP_SIZE = 1024;

        Shadow(model::LightPointer light);

        void setKeylightFrustum(const ViewFrustum& viewFrustum, float nearDepth, float farDepth);

        const std::shared_ptr<ViewFrustum> getFrustum() const { return _frustum; }

        const glm::mat4& getView() const;
        const glm::mat4& getProjection() const;

        const UniformBufferView& getBuffer() const { return _schemaBuffer; }

        gpu::FramebufferPointer framebuffer;
        gpu::TexturePointer map;
    protected:
        model::LightPointer _light;
        std::shared_ptr<ViewFrustum> _frustum;

        class Schema {
        public:
            glm::mat4 projection;
            glm::mat4 viewInverse;

            glm::float32 bias = 0.005f;
            glm::float32 scale = 1 / MAP_SIZE;
        };
        UniformBufferView _schemaBuffer = nullptr;
        
        friend class Light;
    };
    using ShadowPointer = std::shared_ptr<Shadow>;
    using Shadows = indexed_elements::IndexedPointerVector<Shadow>;

    struct Desc {
        Index shadowId { INVALID_INDEX };
    };
    using Descs = std::vector<Desc>;


    Index findLight(const LightPointer& light) const;
    Index addLight(const LightPointer& light);

    bool checkLightId(Index index) const { return _lights.checkIndex(index); }

    Index getNumLights() const { return _lights.getNumElements(); }

    LightPointer getLight(Index lightId) const {
        return _lights.get(lightId);
    }
    Index getShadowId(Index lightId) const {
        if (checkLightId(lightId)) {
            return _descs[lightId].shadowId;
        } else {
            return INVALID_INDEX;
        }
    }
    ShadowPointer getShadow(Index lightId) const {
        return _shadows.get(getShadowId(lightId));
    }

    using LightAndShadow = std::pair<LightPointer, ShadowPointer>;
    LightAndShadow getLightAndShadow(Index lightId) const {
        return LightAndShadow(getLight(lightId), getShadow(lightId));
    }

    Lights _lights;
    LightMap _lightMap;
    Descs _descs;

    gpu::BufferPointer _lightArrayBuffer;
    void updateLightArrayBuffer(Index lightId);

    Shadows _shadows;
};

#endif
