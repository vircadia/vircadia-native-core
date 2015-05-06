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

#define APPEND_ENTITY_PROPERTY(P,O,V) \
        if (requestedProperties.getHasProperty(P)) {                \
            LevelDetails propertyLevel = packetData->startLevel();  \
            successPropertyFits = packetData->O(V);                 \
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

#define READ_ENTITY_PROPERTY(P,T,M)                             \
        if (propertyFlags.getHasProperty(P)) {                  \
            T fromBuffer;                                       \
            memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));    \
            dataAt += sizeof(fromBuffer);                       \
            bytesRead += sizeof(fromBuffer);                    \
            if (overwriteLocalData) {                           \
                M = fromBuffer;                                 \
            }                                                   \
        }

#define READ_ENTITY_PROPERTY_SETTER(P,T,M)                      \
        if (propertyFlags.getHasProperty(P)) {                  \
            T fromBuffer;                                       \
            memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));    \
            dataAt += sizeof(fromBuffer);                       \
            bytesRead += sizeof(fromBuffer);                    \
            if (overwriteLocalData) {                           \
                M(fromBuffer);                                  \
            }                                                   \
        }

#define READ_ENTITY_PROPERTY_QUAT(P,M)                                      \
        if (propertyFlags.getHasProperty(P)) {                              \
            glm::quat fromBuffer;                                           \
            int bytes = unpackOrientationQuatFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                \
            bytesRead += bytes;                                             \
            if (overwriteLocalData) {                                       \
                M = fromBuffer;                                             \
            }                                                               \
        }

#define READ_ENTITY_PROPERTY_QUAT_SETTER(P,M)                               \
        if (propertyFlags.getHasProperty(P)) {                              \
            glm::quat fromBuffer;                                           \
            int bytes = unpackOrientationQuatFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                \
            bytesRead += bytes;                                             \
            if (overwriteLocalData) {                                       \
                M(fromBuffer);                                              \
            }                                                               \
        }

#define READ_ENTITY_PROPERTY_STRING(P,O)                \
        if (propertyFlags.getHasProperty(P)) {          \
            uint16_t length;                            \
            memcpy(&length, dataAt, sizeof(length));    \
            dataAt += sizeof(length);                   \
            bytesRead += sizeof(length);                \
            QString value((const char*)dataAt);         \
            dataAt += length;                           \
            bytesRead += length;                        \
            if (overwriteLocalData) {                   \
                O(value);                               \
            }                                           \
        }

#define READ_ENTITY_PROPERTY_UUID(P,O)                      \
        if (propertyFlags.getHasProperty(P)) {              \
            uint16_t length;                                \
            memcpy(&length, dataAt, sizeof(length));        \
            dataAt += sizeof(length);                       \
            bytesRead += sizeof(length);                    \
            QUuid value;                                    \
            if (length == 0) {                              \
                value = QUuid();                            \
            } else {                                        \
                QByteArray ba((const char*)dataAt, length); \
                value = QUuid::fromRfc4122(ba);             \
                dataAt += length;                           \
                bytesRead += length;                        \
            }                                               \
            if (overwriteLocalData) {                       \
                O(value);                                   \
            }                                               \
        }

#define READ_ENTITY_PROPERTY_COLOR(P,M)         \
        if (propertyFlags.getHasProperty(P)) {  \
            if (overwriteLocalData) {           \
                memcpy(M, dataAt, sizeof(M));   \
            }                                   \
            dataAt += sizeof(rgbColor);         \
            bytesRead += sizeof(rgbColor);      \
        }

#define READ_ENTITY_PROPERTY_TO_PROPERTIES(P,T,O)               \
        if (propertyFlags.getHasProperty(P)) {                  \
            T fromBuffer;                                       \
            memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));    \
            dataAt += sizeof(fromBuffer);                       \
            processedBytes += sizeof(fromBuffer);               \
            properties.O(fromBuffer);                           \
        }

#define READ_ENTITY_PROPERTY_QUAT_TO_PROPERTIES(P,O)                        \
        if (propertyFlags.getHasProperty(P)) {                              \
            glm::quat fromBuffer;                                           \
            int bytes = unpackOrientationQuatFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                \
            processedBytes += bytes;                                        \
            properties.O(fromBuffer);                                       \
        }

#define READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(P,O)  \
        if (propertyFlags.getHasProperty(P)) {          \
            uint16_t length;                            \
            memcpy(&length, dataAt, sizeof(length));    \
            dataAt += sizeof(length);                   \
            processedBytes += sizeof(length);           \
            QString value((const char*)dataAt);         \
            dataAt += length;                           \
            processedBytes += length;                   \
            properties.O(value);                        \
        }


#define READ_ENTITY_PROPERTY_UUID_TO_PROPERTIES(P,O)        \
        if (propertyFlags.getHasProperty(P)) {              \
            uint16_t length;                                \
            memcpy(&length, dataAt, sizeof(length));        \
            dataAt += sizeof(length);                       \
            processedBytes += sizeof(length);               \
            QUuid value;                                    \
            if (length == 0) {                              \
                value = QUuid();                            \
            } else {                                        \
                QByteArray ba((const char*)dataAt, length); \
                value = QUuid::fromRfc4122(ba);             \
                dataAt += length;                           \
                processedBytes += length;                   \
            }                                               \
            properties.O(value);                            \
        }

#define READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(P,O)   \
        if (propertyFlags.getHasProperty(P)) {          \
            xColor color;                               \
            memcpy(&color, dataAt, sizeof(color));      \
            dataAt += sizeof(color);                    \
            processedBytes += sizeof(color);            \
            properties.O(color);                        \
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


#define COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_VEC3(G,P,p) \
    if (!skipDefaults || defaultEntityProperties.get##G().get##P() != _##p) { \
        QScriptValue groupProperties = properties.property(#G); \
        if (!groupProperties.isValid()) { \
            groupProperties = engine->newObject(); \
        } \
        QScriptValue V = vec3toScriptValue(engine, _##p); \
        groupProperties.setProperty(#p, V); \
        properties.setProperty(#G, groupProperties); \
    }

#define COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(G,P,p) \
    if (!skipDefaults || defaultEntityProperties.get##G().get##P() != _##p) { \
        QScriptValue groupProperties = properties.property(#G); \
        if (!groupProperties.isValid()) { \
            groupProperties = engine->newObject(); \
        } \
        groupProperties.setProperty(#p, _##p); \
        properties.setProperty(#G, groupProperties); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(P) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        QScriptValue P = vec3toScriptValue(engine, _##P); \
        properties.setProperty(#P, P); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE_QUAT(P) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        QScriptValue P = quatToScriptValue(engine, _##P); \
        properties.setProperty(#P, P); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR(P) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        QScriptValue P = xColorToScriptValue(engine, _##P); \
        properties.setProperty(#P, P); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(P,G) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        QScriptValue P = xColorToScriptValue(engine, G); \
        properties.setProperty(#P, P); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(P, G) \
    properties.setProperty(#P, G);

#define COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(P, G) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        properties.setProperty(#P, G); \
    }

#define COPY_PROPERTY_TO_QSCRIPTVALUE(P) \
    if (!skipDefaults || defaultEntityProperties._##P != _##P) { \
        properties.setProperty(#P, _##P); \
    }

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(P, S) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        float newValue = P.toVariant().toFloat();   \
        if (_defaultSettings || newValue != _##P) { \
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

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_INT(P, S) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        int newValue = P.toVariant().toInt();   \
        if (_defaultSettings || newValue != _##P) { \
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

#define COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE_BOOL(G, P, S)  \
    {                                                         \
        QScriptValue G = object.property(#G);                 \
        if (G.isValid()) {                                    \
            QScriptValue P = G.property(#P);                  \
            if (P.isValid()) {                                \
                float newValue = P.toVariant().toBool();      \
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
