//
//  IndexedContainer.h
//  render
//
//  Created by Sam Gateau on 9/6/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_IndexedContainer_h
#define hifi_render_IndexedContainer_h

#include <vector>

namespace render {
namespace indexed_container {

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
        Index getNumFreeIndices() const { return (Index) _freeIndices.size(); }
        Index getNumAllocatedIndices() const { return _nextNewIndex; }

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
        Index getNumFreeIndices() const { return _allocator.getNumFreeIndices(); }
        Index getNumAllocatedIndices() const { return _allocator.getNumAllocatedIndices(); }

        Index newElement(const Element& e) {
            Index index = _allocator.allocateIndex();
            if (index != INVALID_INDEX) {
                if (index < (Index) _elements.size()) {
                    _elements[index] = e;
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
        Index getNumFreeIndices() const { return _allocator.getNumFreeIndices(); }
        Index getNumAllocatedIndices() const { return _allocator.getNumAllocatedIndices(); }

        Index newElement(const ElementPtr& e) {
            Index index = _allocator.allocateIndex();
            if (index != INVALID_INDEX) {
                if (index <  (Index) _elements.size()) {
                    _elements[index] = e;
                } else {
                    assert(index == (Index) _elements.size());
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
}
#endif