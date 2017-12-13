//
//  RegisteredMetaTypes.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RegisteredMetaTypes_h
#define hifi_RegisteredMetaTypes_h

#include <QtScript/QScriptEngine>
#include <QtCore/QUuid>
#include <QtCore/QUrl>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AACube.h"
#include "SharedUtil.h"
#include "shared/Bilateral.h"

class QColor;
class QUrl;

Q_DECLARE_METATYPE(glm::vec4)
Q_DECLARE_METATYPE(glm::vec3)
Q_DECLARE_METATYPE(glm::vec2)
Q_DECLARE_METATYPE(glm::quat)
Q_DECLARE_METATYPE(glm::mat4)
Q_DECLARE_METATYPE(xColor)
Q_DECLARE_METATYPE(QVector<glm::vec3>)
Q_DECLARE_METATYPE(QVector<float>)
Q_DECLARE_METATYPE(AACube)
Q_DECLARE_METATYPE(std::function<void()>);
Q_DECLARE_METATYPE(std::function<QVariant()>);

void registerMetaTypes(QScriptEngine* engine);

// Mat4
QScriptValue mat4toScriptValue(QScriptEngine* engine, const glm::mat4& mat4);
void mat4FromScriptValue(const QScriptValue& object, glm::mat4& mat4);

// Vec4
QScriptValue vec4toScriptValue(QScriptEngine* engine, const glm::vec4& vec4);
void vec4FromScriptValue(const QScriptValue& object, glm::vec4& vec4);
QVariant vec4toVariant(const glm::vec4& vec4);
glm::vec4 vec4FromVariant(const QVariant &object, bool& valid);
glm::vec4 vec4FromVariant(const QVariant &object);

// Vec3
QScriptValue vec3toScriptValue(QScriptEngine* engine, const glm::vec3 &vec3);
void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);

QVariant vec3toVariant(const glm::vec3& vec3);
glm::vec3 vec3FromVariant(const QVariant &object, bool& valid);
glm::vec3 vec3FromVariant(const QVariant &object);

// Vec2
QScriptValue vec2toScriptValue(QScriptEngine* engine, const glm::vec2 &vec2);
void vec2FromScriptValue(const QScriptValue &object, glm::vec2 &vec2);

QVariant vec2toVariant(const glm::vec2 &vec2);
glm::vec2 vec2FromVariant(const QVariant &object, bool& valid);
glm::vec2 vec2FromVariant(const QVariant &object);

// Quaternions
QScriptValue quatToScriptValue(QScriptEngine* engine, const glm::quat& quat);
void quatFromScriptValue(const QScriptValue &object, glm::quat& quat);

QVariant quatToVariant(const glm::quat& quat);
glm::quat quatFromVariant(const QVariant &object, bool& isValid);
glm::quat quatFromVariant(const QVariant &object);

// Rect
QScriptValue qRectToScriptValue(QScriptEngine* engine, const QRect& rect);
void qRectFromScriptValue(const QScriptValue& object, QRect& rect);
QRect qRectFromVariant(const QVariant& object, bool& isValid);
QRect qRectFromVariant(const QVariant& object);
QVariant qRectToVariant(const QRect& rect);


// xColor
QScriptValue xColorToScriptValue(QScriptEngine* engine, const xColor& color);
void xColorFromScriptValue(const QScriptValue &object, xColor& color);

QVariant xColorToVariant(const xColor& color);
xColor xColorFromVariant(const QVariant &object, bool& isValid);

// QColor
QScriptValue qColorToScriptValue(QScriptEngine* engine, const QColor& color);
void qColorFromScriptValue(const QScriptValue& object, QColor& color);

QScriptValue qURLToScriptValue(QScriptEngine* engine, const QUrl& url);
void qURLFromScriptValue(const QScriptValue& object, QUrl& url);

// vector<vec3>
QScriptValue qVectorVec3ToScriptValue(QScriptEngine* engine, const QVector<glm::vec3>& vector);
void qVectorVec3FromScriptValue(const QScriptValue& array, QVector<glm::vec3>& vector);
QVector<glm::vec3> qVectorVec3FromScriptValue(const QScriptValue& array);

// vector<quat>
QScriptValue qVectorQuatToScriptValue(QScriptEngine* engine, const QVector<glm::quat>& vector);
void qVectorQuatFromScriptValue(const QScriptValue& array, QVector<glm::quat>& vector);
QVector<glm::quat> qVectorQuatFromScriptValue(const QScriptValue& array);

// vector<bool>
QScriptValue qVectorBoolToScriptValue(QScriptEngine* engine, const QVector<bool>& vector);
void qVectorBoolFromScriptValue(const QScriptValue& array, QVector<bool>& vector);
QVector<bool> qVectorBoolFromScriptValue(const QScriptValue& array);

// vector<float>
QScriptValue qVectorFloatToScriptValue(QScriptEngine* engine, const QVector<float>& vector);
void qVectorFloatFromScriptValue(const QScriptValue& array, QVector<float>& vector);
QVector<float> qVectorFloatFromScriptValue(const QScriptValue& array);

// vector<uint32_t>
QScriptValue qVectorIntToScriptValue(QScriptEngine* engine, const QVector<uint32_t>& vector);
void qVectorIntFromScriptValue(const QScriptValue& array, QVector<uint32_t>& vector);

QVector<QUuid> qVectorQUuidFromScriptValue(const QScriptValue& array);

QScriptValue aaCubeToScriptValue(QScriptEngine* engine, const AACube& aaCube);
void aaCubeFromScriptValue(const QScriptValue &object, AACube& aaCube);

// MathPicks also have to overide operator== for their type
class MathPick {
public:
    virtual ~MathPick() {}
    virtual operator bool() const = 0;
    virtual QVariantMap toVariantMap() const = 0;
};

/**jsdoc
 * A PickRay defines a vector with a starting point. It is used, for example, when finding entities or overlays that lie under a
 * mouse click or intersect a laser beam.
 *
 * @typedef {object} PickRay
 * @property {Vec3} origin - The starting position of the PickRay.
 * @property {Quat} direction - The direction that the PickRay travels.
 */
class PickRay : public MathPick {
public:
    PickRay() : origin(NAN), direction(NAN)  { }
    PickRay(const QVariantMap& pickVariant) : origin(vec3FromVariant(pickVariant["origin"])), direction(vec3FromVariant(pickVariant["direction"])) {}
    PickRay(const glm::vec3& origin, const glm::vec3 direction) : origin(origin), direction(direction) {}
    glm::vec3 origin;
    glm::vec3 direction;

    operator bool() const override {
        return !(glm::any(glm::isnan(origin)) || glm::any(glm::isnan(direction)));
    }
    bool operator==(const PickRay& other) const {
        return (origin == other.origin && direction == other.direction);
    }
    QVariantMap toVariantMap() const override {
        QVariantMap pickRay;
        pickRay["origin"] = vec3toVariant(origin);
        pickRay["direction"] = vec3toVariant(direction);
        return pickRay;
    }
};
Q_DECLARE_METATYPE(PickRay)
QScriptValue pickRayToScriptValue(QScriptEngine* engine, const PickRay& pickRay);
void pickRayFromScriptValue(const QScriptValue& object, PickRay& pickRay);

/**jsdoc
 * A StylusTip defines the tip of a stylus.
 *
 * @typedef {object} StylusTip
 * @property {number} side - The hand the tip is attached to: <code>0</code> for left, <code>1</code> for right.
 * @property {Vec3} position - The position of the stylus tip.
 * @property {Quat} orientation - The orientation of the stylus tip.
 * @property {Vec3} velocity - The velocity of the stylus tip.
 */
class StylusTip : public MathPick {
public:
    StylusTip() : position(NAN), velocity(NAN) {}
    StylusTip(const QVariantMap& pickVariant) : side(bilateral::Side(pickVariant["side"].toInt())), position(vec3FromVariant(pickVariant["position"])),
        orientation(quatFromVariant(pickVariant["orientation"])), velocity(vec3FromVariant(pickVariant["velocity"])) {}

    bilateral::Side side { bilateral::Side::Invalid };
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 velocity;

    operator bool() const override { return side != bilateral::Side::Invalid; }

    bool operator==(const StylusTip& other) const {
        return (side == other.side && position == other.position && orientation == other.orientation && velocity == other.velocity);
    }

    QVariantMap toVariantMap() const override {
        QVariantMap stylusTip;
        stylusTip["side"] = (int)side;
        stylusTip["position"] = vec3toVariant(position);
        stylusTip["orientation"] = quatToVariant(orientation);
        stylusTip["velocity"] = vec3toVariant(velocity);
        return stylusTip;
    }
};


namespace std {
    inline void hash_combine(std::size_t& seed) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        hash_combine(seed, rest...);
    }

    template <>
    struct hash<bilateral::Side> {
        size_t operator()(const bilateral::Side& a) const {
            return std::hash<int>()((int)a);
        }
    };

    template <>
    struct hash<glm::vec3> {
        size_t operator()(const glm::vec3& a) const {
            size_t result = 0;
            hash_combine(result, a.x, a.y, a.z);
            return result;
        }
    };

    template <>
    struct hash<glm::quat> {
        size_t operator()(const glm::quat& a) const {
            size_t result = 0;
            hash_combine(result, a.x, a.y, a.z, a.w);
            return result;
        }
    };

    template <>
    struct hash<PickRay> {
        size_t operator()(const PickRay& a) const {
            size_t result = 0;
            hash_combine(result, a.origin, a.direction);
            return result;
        }
    };

    template <>
    struct hash<StylusTip> {
        size_t operator()(const StylusTip& a) const {
            size_t result = 0;
            hash_combine(result, a.side, a.position, a.orientation, a.velocity);
            return result;
        }
    };
}

enum ContactEventType {
    CONTACT_EVENT_TYPE_START,
    CONTACT_EVENT_TYPE_CONTINUE,
    CONTACT_EVENT_TYPE_END
};

class Collision {
public:
    Collision() : type(CONTACT_EVENT_TYPE_START), idA(), idB(), contactPoint(0.0f), penetration(0.0f), velocityChange(0.0f) { }
    Collision(ContactEventType cType, const QUuid& cIdA, const QUuid& cIdB, const glm::vec3& cPoint,
              const glm::vec3& cPenetration, const glm::vec3& velocityChange)
    :   type(cType), idA(cIdA), idB(cIdB), contactPoint(cPoint), penetration(cPenetration), velocityChange(velocityChange) { }

    void invert(); // swap A and B

    ContactEventType type;
    QUuid idA;
    QUuid idB;
    glm::vec3 contactPoint; // on B in world-frame
    glm::vec3 penetration; // from B towards A in world-frame
    glm::vec3 velocityChange;
};
Q_DECLARE_METATYPE(Collision)
QScriptValue collisionToScriptValue(QScriptEngine* engine, const Collision& collision);
void collisionFromScriptValue(const QScriptValue &object, Collision& collision);

//Q_DECLARE_METATYPE(QUuid) // don't need to do this for QUuid since it's already a meta type
QScriptValue quuidToScriptValue(QScriptEngine* engine, const QUuid& uuid);
void quuidFromScriptValue(const QScriptValue& object, QUuid& uuid);

//Q_DECLARE_METATYPE(QSizeF) // Don't need to to this becase it's arleady a meta type
QScriptValue qSizeFToScriptValue(QScriptEngine* engine, const QSizeF& qSizeF);
void qSizeFFromScriptValue(const QScriptValue& object, QSizeF& qSizeF);

class AnimationDetails {
public:
    AnimationDetails();
    AnimationDetails(QString role, QUrl url, float fps, float priority, bool loop,
        bool hold, bool startAutomatically, float firstFrame, float lastFrame, bool running, float currentFrame, bool allowTranslation);

    QString role;
    QUrl url;
    float fps;
    float priority;
    bool loop;
    bool hold;
    bool startAutomatically;
    float firstFrame;
    float lastFrame;
    bool running;
    float currentFrame;
    bool allowTranslation;
};
Q_DECLARE_METATYPE(AnimationDetails);
QScriptValue animationDetailsToScriptValue(QScriptEngine* engine, const AnimationDetails& event);
void animationDetailsFromScriptValue(const QScriptValue& object, AnimationDetails& event);

namespace model {
    class Mesh;
}

using MeshPointer = std::shared_ptr<model::Mesh>;


class MeshProxy : public QObject {
    Q_OBJECT

public:
    virtual MeshPointer getMeshPointer() const = 0;
    Q_INVOKABLE virtual int getNumVertices() const = 0;
    Q_INVOKABLE virtual glm::vec3 getPos3(int index) const = 0;
};

Q_DECLARE_METATYPE(MeshProxy*);

class MeshProxyList : public QList<MeshProxy*> {}; // typedef and using fight with the Qt macros/templates, do this instead
Q_DECLARE_METATYPE(MeshProxyList);


QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in);
void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out);

QScriptValue meshesToScriptValue(QScriptEngine* engine, const MeshProxyList &in);
void meshesFromScriptValue(const QScriptValue& value, MeshProxyList &out);

class MeshFace {

public:
    MeshFace() {}
    ~MeshFace() {}

    QVector<uint32_t> vertexIndices;
    // TODO -- material...
};

Q_DECLARE_METATYPE(MeshFace)
Q_DECLARE_METATYPE(QVector<MeshFace>)

QScriptValue meshFaceToScriptValue(QScriptEngine* engine, const MeshFace &meshFace);
void meshFaceFromScriptValue(const QScriptValue &object, MeshFace& meshFaceResult);
QScriptValue qVectorMeshFaceToScriptValue(QScriptEngine* engine, const QVector<MeshFace>& vector);
void qVectorMeshFaceFromScriptValue(const QScriptValue& array, QVector<MeshFace>& result);


#endif // hifi_RegisteredMetaTypes_h
