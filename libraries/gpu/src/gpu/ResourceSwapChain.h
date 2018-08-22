//
//  Created by Olivier Prat on 2018/02/19
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_ResourceSwapChain_h
#define hifi_gpu_ResourceSwapChain_h

#include <memory>
#include <array>

namespace gpu {
    class SwapChain {
    public:

        SwapChain(uint8_t size = 2U) : _size{ size } {}
        virtual ~SwapChain() {}

        void advance() {
            _frontIndex = (_frontIndex + 1) % _size;
        }

        uint8_t getSize() const { return _size; }

    protected:
        const uint8_t _size;
        uint8_t _frontIndex{ 0U };

    };
    typedef std::shared_ptr<SwapChain> SwapChainPointer;

    template <class R>
    class ResourceSwapChain : public SwapChain {
    public:

        enum {
            MAX_SIZE = 4
        };

        using Type = R;
        using TypePointer = std::shared_ptr<R>;
        using TypeConstPointer = std::shared_ptr<const R>;

        ResourceSwapChain(const std::vector<TypePointer>& v) : SwapChain{ std::min<uint8_t>((uint8_t)v.size(), MAX_SIZE) } {
            for (size_t i = 0; i < _size; ++i) {
                _resources[i] = v[i];
            }
        }
        const TypePointer& get(unsigned int index) const { return _resources[(index + _frontIndex) % _size]; }

    private:

        std::array<TypePointer, MAX_SIZE> _resources;
    };
}

#endif
