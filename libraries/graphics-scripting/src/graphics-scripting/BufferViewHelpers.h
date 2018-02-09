//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtCore>

namespace gpu {
    class BufferView;
    class Element;
}

template <typename T> QVariant glmVecToVariant(const T& v, bool asArray = false);
template <typename T> const T glmVecFromVariant(const QVariant& v);
QVariant bufferViewElementToVariant(const gpu::BufferView& view, quint32 index, bool asArray = false, const char* hint = "");
bool bufferViewElementFromVariant(const gpu::BufferView& view, quint32 index, const QVariant& v);

template <typename T> gpu::BufferView bufferViewFromVector(QVector<T> elements, gpu::Element elementType);

gpu::BufferView cloneBufferView(const gpu::BufferView& input);
gpu::BufferView resizedBufferView(const gpu::BufferView& input, quint32 numElements);

