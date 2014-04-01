//
//  MetavoxelMessages.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 1/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "MetavoxelMessages.h"

void MetavoxelEditMessage::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    static_cast<const MetavoxelEdit*>(edit.data())->apply(data, objects);
}

MetavoxelEdit::~MetavoxelEdit() {
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
    MetavoxelVisitor(QVector<AttributePointer>(), QVector<AttributePointer>() << edit.value.getAttribute() <<
        AttributeRegistry::getInstance()->getSpannerMaskAttribute()),
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
        info.outputValues[1] = AttributeValue(_outputs.at(1), encodeInline<float>(1.0f));
        return STOP_RECURSION; // entirely contained
    }
    if (info.size <= _edit.granularity) {
        if (volume >= 0.5f) {
            info.outputValues[0] = _edit.value;
            info.outputValues[1] = AttributeValue(_outputs.at(1), encodeInline<float>(1.0f));
        }
        return STOP_RECURSION; // reached granularity limit; take best guess
    }
    return DEFAULT_ORDER; // subdivide
}

class GatherUnmaskedSpannersVisitor : public SpannerVisitor {
public:
    
    GatherUnmaskedSpannersVisitor(const Box& bounds);

    const QList<SharedObjectPointer>& getUnmaskedSpanners() const { return _unmaskedSpanners; }

    virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);

private:
    
    Box _bounds;
    QList<SharedObjectPointer> _unmaskedSpanners;
};

GatherUnmaskedSpannersVisitor::GatherUnmaskedSpannersVisitor(const Box& bounds) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute()),
    _bounds(bounds) {
}

bool GatherUnmaskedSpannersVisitor::visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize) {
    if (!spanner->isMasked() && spanner->getBounds().intersects(_bounds)) {
        _unmaskedSpanners.append(spanner);
    }
    return true;
}

static void setIntersectingMasked(const Box& bounds, MetavoxelData& data) {
    GatherUnmaskedSpannersVisitor visitor(bounds);
    data.guide(visitor);
    
    foreach (const SharedObjectPointer& object, visitor.getUnmaskedSpanners()) {
        Spanner* newSpanner = static_cast<Spanner*>(object->clone(true));
        newSpanner->setMasked(true);
        data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), object, newSpanner);
    }
}

void BoxSetEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // expand to fit the entire edit
    while (!data.getBounds().contains(region)) {
        data.expand();
    }

    BoxSetEditVisitor setVisitor(*this);
    data.guide(setVisitor);
    
    // flip the mask flag of all intersecting spanners
    setIntersectingMasked(region, data);
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

class UpdateSpannerVisitor : public MetavoxelVisitor {
public:
    
    UpdateSpannerVisitor(const QVector<AttributePointer>& attributes, Spanner* spanner);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Spanner* _spanner;
    float _voxelizationSize;
    int _steps;
};

UpdateSpannerVisitor::UpdateSpannerVisitor(const QVector<AttributePointer>& attributes, Spanner* spanner) :
    MetavoxelVisitor(QVector<AttributePointer>() << attributes << AttributeRegistry::getInstance()->getSpannersAttribute(),
        attributes),
    _spanner(spanner),
    _voxelizationSize(qMax(spanner->getBounds().getLongestSide(), spanner->getPlacementGranularity()) * 2.0f /
        AttributeRegistry::getInstance()->getSpannersAttribute()->getLODThresholdMultiplier()),
    _steps(roundf(logf(AttributeRegistry::getInstance()->getSpannersAttribute()->getLODThresholdMultiplier()) /
        logf(2.0f) - 2.0f)) {
}

int UpdateSpannerVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_spanner->getBounds())) {
        return STOP_RECURSION;
    }
    MetavoxelInfo* parentInfo = info.parentInfo;
    for (int i = 0; i < _steps && parentInfo; i++) {
        parentInfo = parentInfo->parentInfo;
    }
    for (int i = 0; i < _outputs.size(); i++) {
        info.outputValues[i] = AttributeValue(_outputs.at(i));
    }
    if (parentInfo) {
        foreach (const SharedObjectPointer& object,
                parentInfo->inputValues.at(_outputs.size()).getInlineValue<SharedObjectSet>()) {
            static_cast<const Spanner*>(object.data())->blendAttributeValues(info, true);
        }
    }
    return (info.size > _voxelizationSize) ? DEFAULT_ORDER : STOP_RECURSION;
}

void InsertSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    data.insert(attribute, this->spanner);
    
    Spanner* spanner = static_cast<Spanner*>(this->spanner.data());
    UpdateSpannerVisitor visitor(spanner->getVoxelizedAttributes(), spanner);
    data.guide(visitor);  
}

RemoveSpannerEdit::RemoveSpannerEdit(const AttributePointer& attribute, int id) :
    attribute(attribute),
    id(id) {
}

void RemoveSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    SharedObject* object = objects.value(id);
    if (!object) {
        qDebug() << "Missing object to remove" << id;
        return;
    }
    // keep a strong reference to the object
    SharedObjectPointer sharedPointer = object;
    data.remove(attribute, object);
    
    Spanner* spanner = static_cast<Spanner*>(object);
    UpdateSpannerVisitor visitor(spanner->getVoxelizedAttributes(), spanner);
    data.guide(visitor);
}

ClearSpannersEdit::ClearSpannersEdit(const AttributePointer& attribute) :
    attribute(attribute) {
}

class GatherSpannerAttributesVisitor : public SpannerVisitor {
public:
    
    GatherSpannerAttributesVisitor(const AttributePointer& attribute);
    
    const QSet<AttributePointer>& getAttributes() const { return _attributes; }
    
    virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);

protected:
    
    QSet<AttributePointer> _attributes;
};

GatherSpannerAttributesVisitor::GatherSpannerAttributesVisitor(const AttributePointer& attribute) :
    SpannerVisitor(QVector<AttributePointer>() << attribute) {
}

bool GatherSpannerAttributesVisitor::visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize) {
    foreach (const AttributePointer& attribute, spanner->getVoxelizedAttributes()) {
        _attributes.insert(attribute);
    }
    return true;
}

void ClearSpannersEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // find all the spanner attributes
    GatherSpannerAttributesVisitor visitor(attribute);
    data.guide(visitor);
    
    data.clear(attribute);
    foreach (const AttributePointer& attribute, visitor.getAttributes()) {
        data.clear(attribute);
    }
}

SetSpannerEdit::SetSpannerEdit(const SharedObjectPointer& spanner) :
    spanner(spanner) {
}

class SetSpannerEditVisitor : public MetavoxelVisitor {
public:
    
    SetSpannerEditVisitor(const QVector<AttributePointer>& attributes, Spanner* spanner);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Spanner* _spanner;
};

SetSpannerEditVisitor::SetSpannerEditVisitor(const QVector<AttributePointer>& attributes, Spanner* spanner) :
    MetavoxelVisitor(attributes, QVector<AttributePointer>() << attributes <<
        AttributeRegistry::getInstance()->getSpannerMaskAttribute()),
    _spanner(spanner) {
}

int SetSpannerEditVisitor::visit(MetavoxelInfo& info) {
    if (_spanner->blendAttributeValues(info)) {
        return DEFAULT_ORDER;
    }
    if (info.outputValues.at(0).getAttribute()) {
        info.outputValues.last() = AttributeValue(_outputs.last(), encodeInline<float>(1.0f));
    }
    return STOP_RECURSION;
}

void SetSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    Spanner* spanner = static_cast<Spanner*>(this->spanner.data());
    
    // expand to fit the entire spanner
    while (!data.getBounds().contains(spanner->getBounds())) {
        data.expand();
    }
    
    SetSpannerEditVisitor visitor(spanner->getAttributes(), spanner);
    data.guide(visitor);
    
    setIntersectingMasked(spanner->getBounds(), data);
}
