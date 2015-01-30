//
//  MetavoxelMessages.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 1/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/transform.hpp>

#include "MetavoxelMessages.h"
#include "Spanner.h"

void MetavoxelEditMessage::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    static_cast<const MetavoxelEdit*>(edit.data())->apply(data, objects);
}

MetavoxelEdit::~MetavoxelEdit() {
}

void MetavoxelEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // nothing by default
}

BoxSetEdit::BoxSetEdit(const Box& region, float granularity, const OwnedAttributeValue& value) :
    region(region), granularity(granularity), value(value) {
}

class BoxSetEditVisitor : public MetavoxelVisitor {
public:
    
    BoxSetEditVisitor(const BoxSetEdit& edit);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const BoxSetEdit& _edit;
};

BoxSetEditVisitor::BoxSetEditVisitor(const BoxSetEdit& edit) :
    MetavoxelVisitor(QVector<AttributePointer>(), QVector<AttributePointer>() << edit.value.getAttribute()),
    _edit(edit) {
}

int BoxSetEditVisitor::visit(MetavoxelInfo& info) {
    // find the intersection between volume and voxel
    glm::vec3 minimum = glm::max(info.minimum, _edit.region.minimum);
    glm::vec3 maximum = glm::min(info.minimum + glm::vec3(info.size, info.size, info.size), _edit.region.maximum);
    glm::vec3 size = maximum - minimum;
    if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f) {
        return STOP_RECURSION; // disjoint
    }
    float volume = (size.x * size.y * size.z) / (info.size * info.size * info.size);
    if (volume >= 1.0f) {
        info.outputValues[0] = _edit.value;
        return STOP_RECURSION; // entirely contained
    }
    if (info.size <= _edit.granularity) {
        if (volume >= 0.5f) {
            info.outputValues[0] = _edit.value;
        }
        return STOP_RECURSION; // reached granularity limit; take best guess
    }
    return DEFAULT_ORDER; // subdivide
}

void BoxSetEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // expand to fit the entire edit
    while (!data.getBounds().contains(region)) {
        data.expand();
    }

    BoxSetEditVisitor setVisitor(*this);
    data.guide(setVisitor);
}

GlobalSetEdit::GlobalSetEdit(const OwnedAttributeValue& value) :
    value(value) {
}

class GlobalSetEditVisitor : public MetavoxelVisitor {
public:
    
    GlobalSetEditVisitor(const GlobalSetEdit& edit);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const GlobalSetEdit& _edit;
};

GlobalSetEditVisitor::GlobalSetEditVisitor(const GlobalSetEdit& edit) :
    MetavoxelVisitor(QVector<AttributePointer>(), QVector<AttributePointer>() << edit.value.getAttribute()),
    _edit(edit) {
}

int GlobalSetEditVisitor::visit(MetavoxelInfo& info) {
    info.outputValues[0] = _edit.value;
    return STOP_RECURSION; // entirely contained
}

void GlobalSetEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    GlobalSetEditVisitor visitor(*this);
    data.guide(visitor);
}

InsertSpannerEdit::InsertSpannerEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner) :
    attribute(attribute),
    spanner(spanner) {
}

void InsertSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    data.insert(attribute, spanner);
}

RemoveSpannerEdit::RemoveSpannerEdit(const AttributePointer& attribute, int id) :
    attribute(attribute),
    id(id) {
}

void RemoveSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    SharedObject* object = objects.value(id);
    if (object) {
        data.remove(attribute, object);
    }
}

ClearSpannersEdit::ClearSpannersEdit(const AttributePointer& attribute) :
    attribute(attribute) {
}

void ClearSpannersEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    data.clear(attribute);
}

SetDataEdit::SetDataEdit(const glm::vec3& minimum, const MetavoxelData& data, bool blend) :
    minimum(minimum),
    data(data),
    blend(blend) {
}

void SetDataEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    data.set(minimum, this->data, blend);
}

PaintHeightfieldHeightEdit::PaintHeightfieldHeightEdit(const glm::vec3& position, float radius,
        float height, bool set, bool erase, float granularity) :
    position(position),
    radius(radius),
    height(height),
    set(set),
    erase(erase),
    granularity(granularity) {
}

void PaintHeightfieldHeightEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // increase the extents slightly to include neighboring tiles
    const float RADIUS_EXTENSION = 1.1f;
    glm::vec3 extents = glm::vec3(radius, radius, radius) * RADIUS_EXTENSION;
    QVector<SharedObjectPointer> results;
    data.getIntersecting(AttributeRegistry::getInstance()->getSpannersAttribute(),
        Box(position - extents, position + extents), results);
    
    foreach (const SharedObjectPointer& spanner, results) {
        Spanner* newSpanner = static_cast<Spanner*>(spanner.data())->paintHeight(position, radius,
            height, set, erase, granularity);
        if (newSpanner != spanner) {
            data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), spanner, newSpanner);
        }
    }
}

MaterialEdit::MaterialEdit(const SharedObjectPointer& material, const QColor& averageColor) :
    material(material),
    averageColor(averageColor) {
}

HeightfieldMaterialSpannerEdit::HeightfieldMaterialSpannerEdit(const SharedObjectPointer& spanner,
        const SharedObjectPointer& material, const QColor& averageColor, bool paint, bool voxelize, float granularity) :
    MaterialEdit(material, averageColor),
    spanner(spanner),
    paint(paint),
    voxelize(voxelize),
    granularity(granularity) {
}

class SpannerProjectionFetchVisitor : public SpannerVisitor {
public:
    
    SpannerProjectionFetchVisitor(const Box& bounds, QVector<SharedObjectPointer>& results);
    
    virtual bool visit(Spanner* spanner);
    
private:
    
    const Box& _bounds;
    QVector<SharedObjectPointer>& _results;
    float _closestDistance;
};

SpannerProjectionFetchVisitor::SpannerProjectionFetchVisitor(const Box& bounds, QVector<SharedObjectPointer>& results) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute()),
    _bounds(bounds),
    _results(results),
    _closestDistance(FLT_MAX) {
}

bool SpannerProjectionFetchVisitor::visit(Spanner* spanner) {
    Heightfield* heightfield = qobject_cast<Heightfield*>(spanner);
    if (!heightfield) {
        return true;
    }
    glm::mat4 transform = glm::scale(1.0f / glm::vec3(heightfield->getScale(),
        heightfield->getScale() * heightfield->getAspectY(),
        heightfield->getScale() * heightfield->getAspectZ())) *
        glm::mat4_cast(glm::inverse(heightfield->getRotation())) * glm::translate(-heightfield->getTranslation());
    Box transformedBounds = transform * _bounds;
    if (transformedBounds.maximum.x < 0.0f || transformedBounds.maximum.z < 0.0f ||
            transformedBounds.minimum.x > 1.0f || transformedBounds.minimum.z > 1.0f) {
        return true;
    }
    float distance = qMin(glm::abs(transformedBounds.minimum.y), glm::abs(transformedBounds.maximum.y));
    if (distance < _closestDistance) {
        _results.clear();
        _results.append(spanner);
        _closestDistance = distance;
    }
    return true;
}

void HeightfieldMaterialSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // make sure the color meets our transparency requirements
    QColor color = averageColor;
    if (paint) {
        color.setAlphaF(1.0f);
    
    } else if (color.alphaF() < 0.5f) {
        color = QColor(0, 0, 0, 0);    
    }
    QVector<SharedObjectPointer> results;
    data.getIntersecting(AttributeRegistry::getInstance()->getSpannersAttribute(),
        static_cast<Spanner*>(spanner.data())->getBounds(), results);
    
    // if there's nothing intersecting directly, find the closest heightfield that intersects the projection
    if (results.isEmpty()) {
        SpannerProjectionFetchVisitor visitor(static_cast<Spanner*>(spanner.data())->getBounds(), results);
        data.guide(visitor);
    }
    
    foreach (const SharedObjectPointer& result, results) {
        Spanner* newResult = static_cast<Spanner*>(result.data())->setMaterial(spanner, material, 
            color, paint, voxelize, granularity);
        if (newResult != result) {
            data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), result, newResult);
        }
    }
}

FillHeightfieldHeightEdit::FillHeightfieldHeightEdit(const glm::vec3& position, float radius, float granularity) :
    position(position),
    radius(radius),
    granularity(granularity) {
}

void FillHeightfieldHeightEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    glm::vec3 extents = glm::vec3(radius, radius, radius);
    QVector<SharedObjectPointer> results;
    data.getIntersecting(AttributeRegistry::getInstance()->getSpannersAttribute(),
        Box(position - extents, position + extents), results);
    
    foreach (const SharedObjectPointer& spanner, results) {
        Spanner* newSpanner = static_cast<Spanner*>(spanner.data())->fillHeight(position, radius, granularity);
        if (newSpanner != spanner) {
            data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), spanner, newSpanner);
        }
    }
}
