#pragma once

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtCore/QPointer>
#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QDebug>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

class Extents;
class AABox;
namespace gpu {
    class Element;
}
Q_DECLARE_LOGGING_CATEGORY(graphics_scripting)

namespace scriptable {
    QVariant toVariant(const Extents& box);
    QVariant toVariant(const AABox& box);
    QVariant toVariant(const gpu::Element& element);
    QVariant toVariant(const glm::mat4& mat4);

    // helper that automatically resolves Qt-signal-like scoped callbacks
    // ... C++ side: `void MyClass::asyncMethod(..., QScriptValue callback)` 
    // ... JS side:
    //     * `API.asyncMethod(..., function(){})`
    //     * `API.asyncMethod(..., scope, function(){})`
    //     * `API.asyncMethod(..., scope, "methodName")`
    QScriptValue jsBindCallback(QScriptValue callback);

    // cast engine->thisObject() => C++ class instance
    template <typename T> T this_qobject_cast(QScriptEngine* engine);

    QString toDebugString(QObject* tmp);
    template <typename T> QString toDebugString(std::shared_ptr<T> tmp);

    // Helper for creating C++ > ScriptOwned JS instances
    // (NOTE: this also helps track in the code where we need to update later if switching to
    //  std::shared_ptr's -- something currently non-trivial given mixed JS/C++ object ownership)
    template <typename T, class... Rest> inline QPointer<T> make_scriptowned(Rest... rest) {
        auto instance = QPointer<T>(new T(rest...));
        Q_ASSERT(instance && instance->parent());
        return instance;
    }
}
