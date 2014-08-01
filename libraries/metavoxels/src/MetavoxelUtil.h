//
//  MetavoxelUtil.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelUtil_h
#define hifi_MetavoxelUtil_h

#include <QColor>
#include <QComboBox>
#include <QSharedPointer>
#include <QUrl>
#include <QWidget>

#include <RegisteredMetaTypes.h>

#include "Bitstream.h"

class QByteArray;
class QDoubleSpinBox;
class QPushButton;

class NetworkProgram;

/// Performs the runtime equivalent of Qt's SIGNAL macro, which is to attach a prefix to the signature.
QByteArray signal(const char* signature);

/// A streamable axis-aligned bounding box.
class Box {
    STREAMABLE

public:
    
    static const int VERTEX_COUNT = 8;
    
    STREAM glm::vec3 minimum;
    STREAM glm::vec3 maximum;
    
    explicit Box(const glm::vec3& minimum = glm::vec3(), const glm::vec3& maximum = glm::vec3());
    
    bool contains(const glm::vec3& point) const;
    
    bool contains(const Box& other) const;
    
    bool intersects(const Box& other) const;
    
    float getLongestSide() const { return qMax(qMax(maximum.x - minimum.x, maximum.y - minimum.y), maximum.z - minimum.z); }
    
    glm::vec3 getVertex(int index) const;
    
    glm::vec3 getCenter() const { return (minimum + maximum) * 0.5f; }
    
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
};

DECLARE_STREAMABLE_METATYPE(Box)

Box operator*(const glm::mat4& matrix, const Box& box);

QDebug& operator<<(QDebug& out, const Box& box);

/// Represents the extents along an axis.
class AxisExtents {
public:
    glm::vec3 axis;
    float minimum;
    float maximum;
    
    /// Creates a set of extents given three points on the first plane and one on the second.
    AxisExtents(const glm::vec3& first0, const glm::vec3& first1, const glm::vec3& first2, const glm::vec3& second);
    
    AxisExtents(const glm::vec3& axis = glm::vec3(), float minimum = 0.0f, float maximum = 0.0f);
};

/// A simple pyramidal frustum for intersection testing.
class Frustum {
public:
    
    Frustum(const glm::vec3& farTopLeft, const glm::vec3& farTopRight, const glm::vec3& farBottomLeft,
        const glm::vec3& farBottomRight, const glm::vec3& nearTopLeft, const glm::vec3& nearTopRight,
        const glm::vec3& nearBottomLeft, const glm::vec3& nearBottomRight);
    
    enum IntersectionType { NO_INTERSECTION, PARTIAL_INTERSECTION, CONTAINS_INTERSECTION };
    
    IntersectionType getIntersectionType(const Box& box) const;
    
private:
    
    static const int VERTEX_COUNT = 8;
    static const int SIDE_EXTENT_COUNT = 5;
    static const int CROSS_PRODUCT_EXTENT_COUNT = 18;
    
    glm::vec3 _vertices[VERTEX_COUNT];
    Box _bounds;
    AxisExtents _sideExtents[SIDE_EXTENT_COUNT];
    AxisExtents _crossProductExtents[CROSS_PRODUCT_EXTENT_COUNT];
};

/// Editor for meta-object values.
class QMetaObjectEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(const QMetaObject* metaObject MEMBER _metaObject WRITE setMetaObject NOTIFY metaObjectChanged USER true)

public:
    
    QMetaObjectEditor(QWidget* parent);

signals:
    
    void metaObjectChanged(const QMetaObject* metaObject);

public slots:
    
    void setMetaObject(const QMetaObject* metaObject);

private slots:
    
    void updateMetaObject();
    
private:
    
    QComboBox* _box;
    const QMetaObject* _metaObject;
};

/// Editor for color values.
class QColorEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color MEMBER _color WRITE setColor NOTIFY colorChanged USER true)

public:
    
    QColorEditor(QWidget* parent);

signals:

    void colorChanged(const QColor& color);

public slots:

    void setColor(const QColor& color);
        
private slots:

    void selectColor();    
    
private:
    
    QPushButton* _button;
    QColor _color;
};

/// Editor for URL values.
class QUrlEditor : public QComboBox {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ getURL WRITE setURL NOTIFY urlChanged USER true)

public:
    
    QUrlEditor(QWidget* parent = NULL);

    void setURL(const QUrl& url);
    const QUrl& getURL() { return _url; }
    
signals:

    void urlChanged(const QUrl& url);

private slots:

    void updateURL(const QString& text);
    void updateSettings();

private:
    
    QUrl _url;
};

/// Base class for Vec3Editor and QuatEditor.
class BaseVec3Editor : public QWidget {
    Q_OBJECT

public:
    
    BaseVec3Editor(QWidget* parent);

    void setSingleStep(double singleStep);

protected slots:
    
    virtual void updateValue() = 0;
    
protected:
    
    QDoubleSpinBox* createComponentBox();
    
    QDoubleSpinBox* _x;
    QDoubleSpinBox* _y;
    QDoubleSpinBox* _z;
};

/// Editor for vector values.
class Vec3Editor : public BaseVec3Editor {
    Q_OBJECT
    Q_PROPERTY(glm::vec3 value MEMBER _value WRITE setValue NOTIFY valueChanged USER true)
    
public:
    
    Vec3Editor(QWidget* parent);

    const glm::vec3& getValue() const { return _value; }

signals:

    void valueChanged(const glm::vec3& vector);

public slots:

    void setValue(const glm::vec3& vector);    

protected:
    
    virtual void updateValue();

private:
    
    glm::vec3 _value;
};

/// Editor for quaternion values.
class QuatEditor : public BaseVec3Editor {
    Q_OBJECT
    Q_PROPERTY(glm::quat value MEMBER _value WRITE setValue NOTIFY valueChanged USER true)

public:
    
    QuatEditor(QWidget* parent);

signals:
    
    void valueChanged(const glm::quat& value);
    
public slots:
    
    void setValue(const glm::quat& value);

protected:
    
    virtual void updateValue();

private:
    
    glm::quat _value;
};

typedef QHash<QScriptString, QVariant> ScriptHash;

Q_DECLARE_METATYPE(ScriptHash)

/// Combines a URL with a set of typed parameters.
class ParameterizedURL {
public:
    
    ParameterizedURL(const QUrl& url = QUrl(), const ScriptHash& parameters = ScriptHash());
    
    bool isValid() const { return _url.isValid(); }
    
    void setURL(const QUrl& url) { _url = url; }
    const QUrl& getURL() const { return _url; }
    
    void setParameters(const ScriptHash& parameters) { _parameters = parameters; }
    const ScriptHash& getParameters() const { return _parameters; }
    
    bool operator==(const ParameterizedURL& other) const;
    bool operator!=(const ParameterizedURL& other) const;
    
private:
    
    QUrl _url;
    ScriptHash _parameters;
};

uint qHash(const ParameterizedURL& url, uint seed = 0);

Bitstream& operator<<(Bitstream& out, const ParameterizedURL& url);
Bitstream& operator>>(Bitstream& in, ParameterizedURL& url);

Q_DECLARE_METATYPE(ParameterizedURL)

/// Allows editing parameterized URLs.
class ParameterizedURLEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(ParameterizedURL url MEMBER _url WRITE setURL NOTIFY urlChanged USER true)

public:
    
    ParameterizedURLEditor(QWidget* parent = NULL);

signals:

    void urlChanged(const ParameterizedURL& url);

public slots:

    void setURL(const ParameterizedURL& url);

private slots:

    void updateURL();    
    void updateParameters();
    void continueUpdatingParameters();
    
private:
    
    ParameterizedURL _url;
    QSharedPointer<NetworkProgram> _program;
    
    QUrlEditor _urlEditor;
};

#endif // hifi_MetavoxelUtil_h
