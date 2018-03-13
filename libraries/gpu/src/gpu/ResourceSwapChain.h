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

        SwapChain(unsigned int size = 2U) : _size{ size } {}
        virtual ~SwapChain() {}

        void advance() {
            _frontIndex = (_frontIndex + 1) % _size;
        }

        unsigned int getSize() const { return _size; }

    protected:
        unsigned int _size;
        unsigned int _frontIndex{ 0U };

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

        ResourceSwapChain(unsigned int size = 2U) : SwapChain{ size } {}

        void reset() {
            for (auto& ptr : _resources) {
                ptr.reset();
            }
        }

        TypePointer& edit(unsigned int index) { return _resources[(index + _frontIndex) % _size]; }
        const TypePointer& get(unsigned int index) const { return _resources[(index + _frontIndex) % _size]; }

    private:

        std::array<TypePointer, MAX_SIZE> _resources;
    };
}

#endif
