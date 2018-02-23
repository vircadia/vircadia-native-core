#pragma once

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtCore/QPointer>
#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QDebug>
#include <memory>
#include <functional>
Q_DECLARE_LOGGING_CATEGORY(graphics_scripting)

namespace scriptable {
    // derive current context's C++ QObject (based on current JS "this" value)
    template <typename T>
    T this_qobject_cast(QScriptEngine* engine) {
        auto context = engine ? engine->currentContext() : nullptr;
        return qscriptvalue_cast<T>(context ? context->thisObject() : QScriptValue::NullValue);
    }

    // JS => QPointer<QObject>
    template <typename T>
    QPointer<T> qpointer_qobject_cast(const QScriptValue& value) {
        auto obj = value.toQObject();
#ifdef SCRIPTABLE_MESH_DEBUG
        qCInfo(graphics_scripting) << "qpointer_qobject_cast" << obj << value.toString();
#endif
        if (auto tmp = qobject_cast<T*>(obj)) {
            return QPointer<T>(tmp);
        }
        if (auto tmp = static_cast<T*>(obj)) {
            return QPointer<T>(tmp);
        }
        return nullptr;
    }
    inline QString toDebugString(QObject* tmp) {
        return QString("%0 (0x%1%2)")
            .arg(tmp ? tmp->metaObject()->className() : "QObject")
            .arg(qulonglong(tmp), 16, 16, QChar('0'))
            .arg(tmp && tmp->objectName().size() ? " name=" + tmp->objectName() : "");
    }
    template <typename T> QString toDebugString(std::shared_ptr<T> tmp) {
        return toDebugString(qobject_cast<QObject*>(tmp.get()));
    }

    // Helper for creating C++ > ScriptOwned JS instances
    // (NOTE: this also helps track in the code where we need to update later if switching to
    //  std::shared_ptr's -- something currently non-trivial given mixed JS/C++ object ownership)
    template <typename T, class... Rest>
    QPointer<T> make_scriptowned(Rest... rest) {
        auto instance = QPointer<T>(new T(rest...));
        Q_ASSERT(instance && instance->parent());
        return instance;
    }
}
