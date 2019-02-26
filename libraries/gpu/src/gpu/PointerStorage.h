//
//  Created by Bradley Austin Davis on 2019/01/25
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_PointerStorage_h
#define hifi_gpu_PointerStorage_h

namespace gpu {

//
// GL Backend pointer storage mechanism
// One of the following three defines must be defined.
// GPU_POINTER_STORAGE_SHARED

// The platonic ideal, use references to smart pointers.
// However, this produces artifacts because there are too many places in the code right now that
// create temporary values (undesirable smart pointer duplications) and then those temp variables
// get passed on and have their reference taken, and then invalidated
// GPU_POINTER_STORAGE_REF

// Raw pointer manipulation.  Seems more dangerous than the reference wrappers,
// but in practice, the danger of grabbing a reference to a temporary variable
// is causing issues
// GPU_POINTER_STORAGE_RAW

#if defined(GPU_POINTER_STORAGE_SHARED)
template <typename T>
static inline bool compare(const std::shared_ptr<T>& a, const std::shared_ptr<T>& b) {
    return a == b;
}

template <typename T>
static inline T* acquire(const std::shared_ptr<T>& pointer) {
    return pointer.get();
}

template <typename T>
static inline void reset(std::shared_ptr<T>& pointer) {
    return pointer.reset();
}

template <typename T>
static inline bool valid(const std::shared_ptr<T>& pointer) {
    return pointer.operator bool();
}

template <typename T>
static inline void assign(std::shared_ptr<T>& pointer, const std::shared_ptr<T>& source) {
    pointer = source;
}

using BufferReference = BufferPointer;
using TextureReference = TexturePointer;
using FramebufferReference = FramebufferPointer;
using FormatReference = Stream::FormatPointer;
using PipelineReference = PipelinePointer;

#define GPU_REFERENCE_INIT_VALUE nullptr

#elif defined(GPU_POINTER_STORAGE_REF)

template <typename T>
class PointerReferenceWrapper : public std::reference_wrapper<const std::shared_ptr<T>> {
    using Parent = std::reference_wrapper<const std::shared_ptr<T>>;

public:
    using Pointer = std::shared_ptr<T>;
    PointerReferenceWrapper() : Parent(EMPTY()) {}
    PointerReferenceWrapper(const Pointer& pointer) : Parent(pointer) {}
    void clear() { *this = EMPTY(); }

private:
    static const Pointer& EMPTY() {
        static const Pointer EMPTY_VALUE;
        return EMPTY_VALUE;
    };
};

template <typename T>
static bool compare(const PointerReferenceWrapper<T>& reference, const std::shared_ptr<T>& pointer) {
    return reference.get() == pointer;
}

template <typename T>
static inline T* acquire(const PointerReferenceWrapper<T>& reference) {
    return reference.get().get();
}

template <typename T>
static void assign(PointerReferenceWrapper<T>& reference, const std::shared_ptr<T>& pointer) {
    reference = pointer;
}

template <typename T>
static bool valid(const PointerReferenceWrapper<T>& reference) {
    return reference.get().operator bool();
}

template <typename T>
static inline void reset(PointerReferenceWrapper<T>& reference) {
    return reference.clear();
}

using BufferReference = PointerReferenceWrapper<Buffer>;
using TextureReference = PointerReferenceWrapper<Texture>;
using FramebufferReference = PointerReferenceWrapper<Framebuffer>;
using FormatReference = PointerReferenceWrapper<Stream::Format>;
using PipelineReference = PointerReferenceWrapper<Pipeline>;

#define GPU_REFERENCE_INIT_VALUE

#elif defined(GPU_POINTER_STORAGE_RAW)

template <typename T>
static bool compare(const T* const& rawPointer, const std::shared_ptr<T>& pointer) {
    return rawPointer == pointer.get();
}

template <typename T>
static inline T* acquire(T* const& rawPointer) {
    return rawPointer;
}

template <typename T>
static inline bool valid(const T* const& rawPointer) {
    return rawPointer;
}

template <typename T>
static inline void reset(T*& rawPointer) {
    rawPointer = nullptr;
}

template <typename T>
static inline void assign(T*& rawPointer, const std::shared_ptr<T>& pointer) {
    rawPointer = pointer.get();
}

using BufferReference = Buffer*;
using TextureReference = Texture*;
using FramebufferReference = Framebuffer*;
using FormatReference = Stream::Format*;
using PipelineReference = Pipeline*;

#define GPU_REFERENCE_INIT_VALUE nullptr

#else

#error "No GPU pointer storage scheme defined"

#endif

}

#endif // hifi_gpu_PointerStorage_h