#pragma once

#include <QtCore>

namespace gpu { class BufferView;  }
class QScriptValue;
class QScriptEngine;
template <typename T> QScriptValue glmVecToScriptValue(QScriptEngine *js, const T& v, bool asArray = false);
template <typename T> const T glmVecFromScriptValue(const QScriptValue& v);
QScriptValue bufferViewElementToScriptValue(QScriptEngine* engine, const gpu::BufferView& view, quint32 index, bool asArray = false, const char* hint = "");
bool bufferViewElementFromScriptValue(const QScriptValue& v, const gpu::BufferView& view, quint32 index);
