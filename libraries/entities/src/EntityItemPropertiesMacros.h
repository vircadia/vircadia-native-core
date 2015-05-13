//
//  EntityItemPropertiesMacros.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 9/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItemPropertiesMacros_h
#define hifi_EntityItemPropertiesMacros_h

#define APPEND_ENTITY_PROPERTY(P,V) \
        if (requestedProperties.getHasProperty(P)) {                \
            LevelDetails propertyLevel = packetData->startLevel();  \
            successPropertyFits = packetData->appendValue(V);       \
            if (successPropertyFits) {                              \
                propertyFlags |= P;                                 \
                propertiesDidntFit -= P;                            \
                propertyCount++;                                    \
                packetData->endLevel(propertyLevel);                \
            } else {                                                \
                packetData->discardLevel(propertyLevel);            \
                appendState = OctreeElement::PARTIAL;               \
            }                                                       \
        } else {                                                    \
            propertiesDidntFit -= P;                                \
        }

#define READ_ENTITY_PROPERTY(P,T,S)                                                \
        if (propertyFlags.getHasProperty(P)) {                                     \
            T fromBuffer;                                                          \
            int bytes = OctreePacketData::uppackDataFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                       \
            bytesRead += bytes;                                                    \
            if (overwriteLocalData) {                                              \
                S(fromBuffer);                                                     \
            }                                                                      \
        }

#define DECODE_GROUP_PROPERTY_HAS_CHANGED(P,N) \
        if (propertyFlags.getHasProperty(P)) {  \
            set##N##Changed(true); \
        }


#define READ_ENTITY_PROPERTY_TO_PROPERTIES(P,T,O)                                  \
        if (propertyFlags.getHasProperty(P)) {                                     \
            T fromBuffer;                                                          \
            int bytes = OctreePacketData::uppackDataFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                       \
            processedBytes += bytes;                                               \
            properties.O(fromBuffer);                                              \
        }

#define SET_ENTITY_PROPERTY_FROM_PROPERTIES(P,M)    \
    if (properties._##P##Changed) {    \
        M(properties._##P);                         \
        somethingChanged = true;                    \
    }

#define SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(G,P,p,M)  \
    if (properties.get##G().p##Changed()) {                 \
        M(properties.get##G().get##P());                    \
        somethingChanged = true;                            \
    }

#define SET_ENTITY_PROPERTY_FROM_PROPERTIES_GETTER(C,G,S)    \
    if (properties.C()) {    \
        S(properties.G());                         \
        somethingChanged = true;                    \
    }

#define COPY_ENTITY_PROPERTY_TO_PROPERTIES(P,M) \
    properties._##P = M();                      \
    properties._##P##Changed = false;

#define COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(G,P,M)  \
    properties.get##G().set##P(M());                     \
    properties.get##G().set##P##Changed(false);

#define CHECK_PROPERTY_CHANGE(P,M) \
    if (_##M##Changed) {           \
        changedProperties += P;    \
    }

inline QScriptValue convertScriptValue(QScriptEngine* e, const glm::vec3& v) { return vec3toScriptValue(e, v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, float v) { return QScriptValue(v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, int v) { return QScriptValue(v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, quint32 v) { return QScriptValue(v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, const QString& v) { return QScriptValue(v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, const xColor& v) { return xColorToScriptValue(e, v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, const glm::quat& v) { return quatToScriptValue(e, v); }
inline QScriptValue convertScriptValue(QScriptEngine* e, const QScriptValue& v) { return v; }

#define COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(G,g,P,p) \
    if (!skipDefaults || defaultEntityProperties.get##G().get##P() != get##P()) { \
        QScriptValue groupProperties = properties.property(#g); \
        if (!groupProperties.isValid()) { \
            groupProperties = engine->newObject(); \
        } \
        QScriptValue V = convertScriptValue(engine, get##P()); \
        groupProperties.setProperty(#p, V); \
        properties.setProperty(#g, groupProperties); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE(P) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        QScriptValue V = convertScriptValue(engine, _##P); \
        properties.setProperty(#P, V); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(P, G) \
    properties.setProperty(#P, G);

#define COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(P, G) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        QScriptValue V = convertScriptValue(engine, G); \
        properties.setProperty(#P, V); \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(P, S) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        float newValue = P.toVariant().toFloat();   \
        if (_defaultSettings || newValue != _##P) { \
            S(newValue);                            \
        }                                           \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT_GETTER(P, S, G) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        float newValue = P.toVariant().toFloat();   \
        if (_defaultSettings || newValue != G()) { \
            S(newValue);                            \
        }                                           \
    }

#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(G, P, S)  \
    {                                                         \
        QScriptValue G = object.property(#G);                 \
        if (G.isValid()) {                                    \
            QScriptValue P = G.property(#P);                  \
            if (P.isValid()) {                                \
                float newValue = P.toVariant().toFloat();     \
                if (_defaultSettings || newValue != _##P) {   \
                    S(newValue);                              \
                }                                             \
            }                                                 \
        }                                                     \
    }

#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_UINT16(G, P, S)  \
    {                                                         \
        QScriptValue G = object.property(#G);                 \
        if (G.isValid()) {                                    \
            QScriptValue P = G.property(#P);                  \
            if (P.isValid()) {                                \
                uint16_t newValue = P.toVariant().toInt();     \
                if (_defaultSettings || newValue != _##P) {   \
                    S(newValue);                              \
                }                                             \
            }                                                 \
        }                                                     \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_INT(P, S) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        int newValue = P.toVariant().toInt();   \
        if (_defaultSettings || newValue != _##P) { \
            S(newValue);                            \
        }                                           \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_INT_GETTER(P, S, G) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        int newValue = P.toVariant().toInt();   \
        if (_defaultSettings || newValue != G()) { \
            S(newValue);                            \
        }                                           \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(P, S)  \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        bool newValue = P.toVariant().toBool();     \
        if (_defaultSettings || newValue != _##P) { \
            S(newValue);                            \
        }                                           \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL_GETTER(P, S, G)  \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        bool newValue = P.toVariant().toBool();     \
        if (_defaultSettings || newValue != G()) { \
            S(newValue);                            \
        }                                           \
    }


#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_BOOL(G, P, S)  \
    {                                                         \
        QScriptValue G = object.property(#G);                 \
        if (G.isValid()) {                                    \
            QScriptValue P = G.property(#P);                  \
            if (P.isValid()) {                                \
                bool newValue = P.toVariant().toBool();      \
                if (_defaultSettings || newValue != _##P) {   \
                    S(newValue);                              \
                }                                             \
            }                                                 \
        }                                                     \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(P, S)\
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        QString newValue = P.toVariant().toString().trimmed();\
        if (_defaultSettings || newValue != _##P) { \
            S(newValue);                            \
        }                                           \
    }

#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_STRING(G, P, S)\
    {                                                       \
        QScriptValue G = object.property(#G);               \
        if (G.isValid()) {                                  \
            QScriptValue P = G.property(#P);           \
            if (P.isValid()) {                              \
                QString newValue = P.toVariant().toString().trimmed();\
                if (_defaultSettings || newValue != _##P) { \
                    S(newValue);                            \
                }                                           \
            }                                               \
        }                                                   \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_UUID(P, S)           \
    QScriptValue P = object.property(#P);                    \
    if (P.isValid()) {                                       \
        QUuid newValue = P.toVariant().toUuid();             \
        if (_defaultSettings || newValue != _##P) {          \
            S(newValue);                                     \
        }                                                    \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(P, S)        \
    QScriptValue P = object.property(#P);                 \
    if (P.isValid()) {                                    \
        QScriptValue x = P.property("x");                 \
        QScriptValue y = P.property("y");                 \
        QScriptValue z = P.property("z");                 \
        if (x.isValid() && y.isValid() && z.isValid()) {  \
            glm::vec3 newValue;                           \
            newValue.x = x.toVariant().toFloat();         \
            newValue.y = y.toVariant().toFloat();         \
            newValue.z = z.toVariant().toFloat();         \
            bool isValid = !glm::isnan(newValue.x) &&     \
                         !glm::isnan(newValue.y) &&       \
                         !glm::isnan(newValue.z);         \
            if (isValid &&                                \
                (_defaultSettings || newValue != _##P)) { \
                S(newValue);                              \
            }                                             \
        }                                                 \
    }

#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_VEC3(G, P, S)       \
    {                                                             \
        QScriptValue G = object.property(#G);                     \
        if (G.isValid()) {                                        \
            QScriptValue P = G.property(#P);                      \
            if (P.isValid()) {                                    \
                QScriptValue x = P.property("x");                 \
                QScriptValue y = P.property("y");                 \
                QScriptValue z = P.property("z");                 \
                if (x.isValid() && y.isValid() && z.isValid()) {  \
                    glm::vec3 newValue;                           \
                    newValue.x = x.toVariant().toFloat();         \
                    newValue.y = y.toVariant().toFloat();         \
                    newValue.z = z.toVariant().toFloat();         \
                    bool isValid = !glm::isnan(newValue.x) &&     \
                                 !glm::isnan(newValue.y) &&       \
                                 !glm::isnan(newValue.z);         \
                    if (isValid &&                                \
                        (_defaultSettings || newValue != _##P)) { \
                        S(newValue);                              \
                    }                                             \
                }                                                 \
            }                                                     \
        }                                                         \
    }
    
#define COPY_PROPERTY_FROM_QSCRIPTVALUE_QUAT(P, S)                      \
    QScriptValue P = object.property(#P);                               \
    if (P.isValid()) {                                                  \
        QScriptValue x = P.property("x");                               \
        QScriptValue y = P.property("y");                               \
        QScriptValue z = P.property("z");                               \
        QScriptValue w = P.property("w");                               \
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) { \
            glm::quat newValue;                                         \
            newValue.x = x.toVariant().toFloat();                       \
            newValue.y = y.toVariant().toFloat();                       \
            newValue.z = z.toVariant().toFloat();                       \
            newValue.w = w.toVariant().toFloat();                       \
            bool isValid = !glm::isnan(newValue.x) &&                   \
                           !glm::isnan(newValue.y) &&                   \
                           !glm::isnan(newValue.z) &&                   \
                           !glm::isnan(newValue.w);                     \
            if (isValid &&                                              \
                (_defaultSettings || newValue != _##P)) {               \
                S(newValue);                                            \
            }                                                           \
        }                                                               \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(P, S)     \
    QScriptValue P = object.property(#P);               \
    if (P.isValid()) {                                  \
        QScriptValue r = P.property("red");             \
        QScriptValue g = P.property("green");           \
        QScriptValue b = P.property("blue");            \
        if (r.isValid() && g.isValid() && b.isValid()) {\
            xColor newColor;                            \
            newColor.red = r.toVariant().toInt();       \
            newColor.green = g.toVariant().toInt();     \
            newColor.blue = b.toVariant().toInt();      \
            if (_defaultSettings ||                     \
                (newColor.red != _color.red ||          \
                newColor.green != _color.green ||       \
                newColor.blue != _color.blue)) {        \
                S(newColor);                            \
            }                                           \
        }                                               \
    }

#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_COLOR(G, P, S)    \
    {                                                           \
        QScriptValue G = object.property(#G);                   \
        if (G.isValid()) {                                      \
            QScriptValue P = G.property(#P);                    \
            if (P.isValid()) {                                  \
                QScriptValue r = P.property("red");             \
                QScriptValue g = P.property("green");           \
                QScriptValue b = P.property("blue");            \
                if (r.isValid() && g.isValid() && b.isValid()) {\
                    xColor newColor;                            \
                    newColor.red = r.toVariant().toInt();       \
                    newColor.green = g.toVariant().toInt();     \
                    newColor.blue = b.toVariant().toInt();      \
                    if (_defaultSettings ||                     \
                        (newColor.red != _color.red ||          \
                        newColor.green != _color.green ||       \
                        newColor.blue != _color.blue)) {        \
                        S(newColor);                            \
                    }                                           \
                }                                               \
            }                                                   \
        }                                                       \
    }

#define COPY_PROPERTY_FROM_QSCRITPTVALUE_ENUM(P, S)               \
    QScriptValue P = object.property(#P);                         \
    if (P.isValid()) {                                            \
        QString newValue = P.toVariant().toString();              \
        if (_defaultSettings || newValue != get##S##AsString()) { \
            set##S##FromString(newValue);                         \
        }                                                         \
    }
    
#define CONSTRUCT_PROPERTY(n, V)        \
    _##n(V),                            \
    _##n##Changed(false)

#define DEFINE_PROPERTY_GROUP(N, n, T)        \
    public: \
        const T& get##N() const { return _##n; } \
        T& get##N() { return _##n; } \
    private: \
        T _##n; \
        static T _static##N; 

#define DEFINE_PROPERTY(P, N, n, T)        \
    public: \
        T get##N() const { return _##n; } \
        void set##N(T value) { _##n = value; _##n##Changed = true; } \
        bool n##Changed() const { return _##n##Changed; } \
        void set##N##Changed(bool value) { _##n##Changed = value; } \
    private: \
        T _##n; \
        bool _##n##Changed = false;

#define DEFINE_PROPERTY_REF(P, N, n, T)        \
    public: \
        const T& get##N() const { return _##n; } \
        void set##N(const T& value) { _##n = value; _##n##Changed = true; } \
        bool n##Changed() const { return _##n##Changed; } \
        void set##N##Changed(bool value) { _##n##Changed = value; } \
    private: \
        T _##n; \
        bool _##n##Changed = false;

#define DEFINE_PROPERTY_REF_WITH_SETTER(P, N, n, T)        \
    public: \
        const T& get##N() const { return _##n; } \
        void set##N(const T& value); \
        bool n##Changed() const; \
        void set##N##Changed(bool value) { _##n##Changed = value; } \
    private: \
        T _##n; \
        bool _##n##Changed;

#define DEFINE_PROPERTY_REF_WITH_SETTER_AND_GETTER(P, N, n, T)        \
    public: \
        T get##N() const; \
        void set##N(const T& value); \
        bool n##Changed() const; \
        void set##N##Changed(bool value) { _##n##Changed = value; } \
    private: \
        T _##n; \
        bool _##n##Changed;

#define DEFINE_PROPERTY_REF_ENUM(P, N, n, T) \
    public: \
        const T& get##N() const { return _##n; } \
        void set##N(const T& value) { _##n = value; _##n##Changed = true; } \
        bool n##Changed() const { return _##n##Changed; } \
        QString get##N##AsString() const; \
        void set##N##FromString(const QString& name); \
        void set##N##Changed(bool value) { _##n##Changed = value; } \
    private: \
        T _##n; \
        bool _##n##Changed;

#define DEBUG_PROPERTY(D, P, N, n, x)                \
    D << "  " << #n << ":" << P.get##N() << x << "[changed:" << P.n##Changed() << "]\n";

#define DEBUG_PROPERTY_IF_CHANGED(D, P, N, n, x)                \
    if (P.n##Changed()) {                                       \
        D << "  " << #n << ":" << P.get##N() << x << "\n";      \
    }

#endif // hifi_EntityItemPropertiesMacros_h
