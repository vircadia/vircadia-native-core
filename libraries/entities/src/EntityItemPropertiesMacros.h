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

#define READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(P,O)   \
        if (propertyFlags.getHasProperty(P)) {          \
            xColor color;                               \
            memcpy(&color, dataAt, sizeof(color));      \
            dataAt += sizeof(color);                    \
            processedBytes += sizeof(color);            \
            properties.O(color);                        \
        }

#define SET_ENTITY_PROPERTY_FROM_PROPERTIES(P,M)    \
    if (properties._##P##Changed || forceCopy) {    \
        M(properties._##P);                         \
        somethingChanged = true;                    \
    }

#define SET_ENTITY_PROPERTY_FROM_PROPERTIES_GETTER(C,G,S)    \
    if (properties.C() || forceCopy) {    \
        S(properties.G());                         \
        somethingChanged = true;                    \
    }

#define COPY_ENTITY_PROPERTY_TO_PROPERTIES(M,G) \
    properties._##M = G();                      \
    properties._##M##Changed = false;

#define CHECK_PROPERTY_CHANGE(P,M) \
    if (_##M##Changed) {           \
        changedProperties += P;    \
    }


#define COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(P) \
    QScriptValue P = vec3toScriptValue(engine, _##P); \
    properties.setProperty(#P, P);

#define COPY_PROPERTY_TO_QSCRIPTVALUE_QUAT(P) \
    QScriptValue P = quatToScriptValue(engine, _##P); \
    properties.setProperty(#P, P);

#define COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR(P) \
    QScriptValue P = xColorToScriptValue(engine, _##P); \
    properties.setProperty(#P, P);

#define COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(P,G) \
    QScriptValue P = xColorToScriptValue(engine, G); \
    properties.setProperty(#P, P);

#define COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(P, G) \
    properties.setProperty(#P, G);

#define COPY_PROPERTY_TO_QSCRIPTVALUE(P) \
    properties.setProperty(#P, _##P);

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(P, S) \
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        float newValue = P.toVariant().toFloat();   \
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

#define COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(P, S)\
    QScriptValue P = object.property(#P);           \
    if (P.isValid()) {                              \
        QString newValue = P.toVariant().toString();\
        if (_defaultSettings || newValue != _##P) { \
            S(newValue);                            \
        }                                           \
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
    
#define CONSTRUCT_PROPERTY(n, V)        \
    _##n(V),                            \
    _##n##Changed(false)

#define DEFINE_PROPERTY(P, N, n, T)        \
    public: \
    T get##N() const { return _##n; } \
    void set##N(T value) { _##n = value; _##n##Changed = true; } \
    private: \
    T _##n; \
    bool _##n##Changed;

#define DEFINE_PROPERTY_REF(P, N, n, T)        \
    public: \
    const T& get##N() const { return _##n; } \
    void set##N(const T& value) { _##n = value; _##n##Changed = true; } \
    private: \
    T _##n; \
    bool _##n##Changed;



#endif // hifi_EntityItemPropertiesMacros_h
