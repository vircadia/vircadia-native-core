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

#include "MetavoxelMessages.h"

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
    _steps(glm::round(logf(AttributeRegistry::getInstance()->getSpannersAttribute()->getLODThresholdMultiplier()) /
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

SetDataEdit::SetDataEdit(const glm::vec3& minimum, const MetavoxelData& data, bool blend) :
    minimum(minimum),
    data(data),
    blend(blend) {
}

void SetDataEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    data.set(minimum, this->data, blend);
}

PaintHeightfieldHeightEdit::PaintHeightfieldHeightEdit(const glm::vec3& position, float radius, float height) :
    position(position),
    radius(radius),
    height(height) {
}

class PaintHeightfieldHeightEditVisitor : public MetavoxelVisitor {
public:
    
    PaintHeightfieldHeightEditVisitor(const PaintHeightfieldHeightEdit& edit);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    PaintHeightfieldHeightEdit _edit;
    Box _bounds;
};

PaintHeightfieldHeightEditVisitor::PaintHeightfieldHeightEditVisitor(const PaintHeightfieldHeightEdit& edit) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute(),
        QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute()),
    _edit(edit) {
    
    glm::vec3 extents(_edit.radius, _edit.radius, _edit.radius);
    _bounds = Box(_edit.position - extents, _edit.position + extents);
}

const int EIGHT_BIT_MAXIMUM = 255;

int PaintHeightfieldHeightEditVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    HeightfieldHeightDataPointer pointer = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    if (!pointer) {
        return STOP_RECURSION;
    }
    QByteArray contents(pointer->getContents());
    int size = glm::sqrt((float)contents.size());
    int highest = size - 1;
    float heightScale = size / info.size;
    
    glm::vec3 center = (_edit.position - info.minimum) * heightScale;
    float scaledRadius = _edit.radius * heightScale;
    glm::vec3 extents(scaledRadius, scaledRadius, scaledRadius);
    
    glm::vec3 start = glm::floor(center - extents);
    glm::vec3 end = glm::ceil(center + extents);
    
    // raise/lower all points within the radius
    float z = qMax(start.z, 0.0f);
    float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highest);
    uchar* lineDest = (uchar*)contents.data() + (int)z * size + (int)startX;
    float squaredRadius = scaledRadius * scaledRadius;
    float squaredRadiusReciprocal = 1.0f / squaredRadius; 
    float scaledHeight = _edit.height * EIGHT_BIT_MAXIMUM / info.size;
    bool changed = false;
    for (float endZ = qMin(end.z, (float)highest); z <= endZ; z += 1.0f) {
        uchar* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest++) {
            float dx = x - center.x, dz = z - center.z;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                // height falls off towards edges
                int value = *dest + scaledHeight * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                if (value != *dest) {
                    *dest = qMin(qMax(value, 0), EIGHT_BIT_MAXIMUM);
                    changed = true;
                }
            }
        }
        lineDest += size;
    }
    if (changed) {
        HeightfieldHeightDataPointer newPointer(new HeightfieldHeightData(contents));
        info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline<HeightfieldHeightDataPointer>(newPointer));
    }
    return STOP_RECURSION;
}

void PaintHeightfieldHeightEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    PaintHeightfieldHeightEditVisitor visitor(*this);
    data.guide(visitor);
}

MaterialEdit::MaterialEdit(const SharedObjectPointer& material, const QColor& averageColor) :
    material(material),
    averageColor(averageColor) {
}

class PaintHeightfieldMaterialEditVisitor : public MetavoxelVisitor {
public:
    
    PaintHeightfieldMaterialEditVisitor(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& color);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    glm::vec3 _position;
    float _radius;
    SharedObjectPointer _material;
    QColor _color;
    Box _bounds;
};

PaintHeightfieldMaterialEditVisitor::PaintHeightfieldMaterialEditVisitor(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& color) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute(), QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
            AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute()),
    _position(position),
    _radius(radius),
    _material(material),
    _color(color) {
    
    glm::vec3 extents(_radius, _radius, _radius);
    _bounds = Box(_position - extents, _position + extents);
}

static QHash<uchar, int> countIndices(const QByteArray& contents) {
    QHash<uchar, int> counts;
    for (const uchar* src = (const uchar*)contents.constData(), *end = src + contents.size(); src != end; src++) {
        if (*src != 0) {
            counts[*src]++;
        }
    }
    return counts;
}

uchar getMaterialIndex(const SharedObjectPointer& material, QVector<SharedObjectPointer>& materials, QByteArray& contents) {
    if (!(material && static_cast<MaterialObject*>(material.data())->getDiffuse().isValid())) {
        return 0;
    }
    // first look for a matching existing material, noting the first reusable slot
    int firstEmptyIndex = -1;
    for (int i = 0; i < materials.size(); i++) {
        const SharedObjectPointer& existingMaterial = materials.at(i);
        if (existingMaterial) {
            if (existingMaterial->equals(material.data())) {
                return i + 1;
            }
        } else if (firstEmptyIndex == -1) {
            firstEmptyIndex = i;
        }
    }
    // if nothing found, use the first empty slot or append
    if (firstEmptyIndex != -1) {
        materials[firstEmptyIndex] = material;
        return firstEmptyIndex + 1;
    }
    if (materials.size() < EIGHT_BIT_MAXIMUM) {
        materials.append(material);
        return materials.size();
    }
    // last resort: find the least-used material and remove it
    QHash<uchar, int> counts = countIndices(contents);
    uchar materialIndex = 0;
    int lowestCount = INT_MAX;
    for (QHash<uchar, int>::const_iterator it = counts.constBegin(); it != counts.constEnd(); it++) {
        if (it.value() < lowestCount) {
            materialIndex = it.key();
            lowestCount = it.value();
        }
    }
    contents.replace((char)materialIndex, (char)0);
    return materialIndex;
}

void clearUnusedMaterials(QVector<SharedObjectPointer>& materials, QByteArray& contents) {
    QHash<uchar, int> counts = countIndices(contents);
    for (int i = 0; i < materials.size(); i++) {
        if (counts.value(i + 1) == 0) {
            materials[i] = SharedObjectPointer();
        }
    }
    while (!(materials.isEmpty() || materials.last())) {
        materials.removeLast();
    }
}

int PaintHeightfieldMaterialEditVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    HeightfieldColorDataPointer colorPointer = info.inputValues.at(0).getInlineValue<HeightfieldColorDataPointer>();
    if (colorPointer) {
        QByteArray contents(colorPointer->getContents());
        int size = glm::sqrt((float)contents.size() / DataBlock::COLOR_BYTES);
        int highest = size - 1;
        float heightScale = size / info.size;
        
        glm::vec3 center = (_position - info.minimum) * heightScale;
        float scaledRadius = _radius * heightScale;
        glm::vec3 extents(scaledRadius, scaledRadius, scaledRadius);
        
        glm::vec3 start = glm::floor(center - extents);
        glm::vec3 end = glm::ceil(center + extents);
        
        // paint all points within the radius
        float z = qMax(start.z, 0.0f);
        float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highest);
        int stride = size * DataBlock::COLOR_BYTES;
        char* lineDest = contents.data() + (int)z * stride + (int)startX * DataBlock::COLOR_BYTES;
        float squaredRadius = scaledRadius * scaledRadius; 
        char red = _color.red(), green = _color.green(), blue = _color.blue();
        bool changed = false;
        for (float endZ = qMin(end.z, (float)highest); z <= endZ; z += 1.0f) {
            char* dest = lineDest;
            for (float x = startX; x <= endX; x += 1.0f, dest += DataBlock::COLOR_BYTES) {
                float dx = x - center.x, dz = z - center.z;
                if (dx * dx + dz * dz <= squaredRadius) {
                    dest[0] = red;
                    dest[1] = green;
                    dest[2] = blue;
                    changed = true;
                }
            }
            lineDest += stride;
        }
        if (changed) {
            HeightfieldColorDataPointer newPointer(new HeightfieldColorData(contents));
            info.outputValues[0] = AttributeValue(info.inputValues.at(0).getAttribute(),
                encodeInline<HeightfieldColorDataPointer>(newPointer));
        }
    }
    
    HeightfieldMaterialDataPointer materialPointer = info.inputValues.at(1).getInlineValue<HeightfieldMaterialDataPointer>();
    if (materialPointer) {
        QVector<SharedObjectPointer> materials = materialPointer->getMaterials();
        QByteArray contents(materialPointer->getContents());
        uchar materialIndex = getMaterialIndex(_material, materials, contents);
        int size = glm::sqrt((float)contents.size());
        int highest = size - 1;
        float heightScale = highest / info.size;
        
        glm::vec3 center = (_position - info.minimum) * heightScale;
        float scaledRadius = _radius * heightScale;
        glm::vec3 extents(scaledRadius, scaledRadius, scaledRadius);
        
        glm::vec3 start = glm::floor(center - extents);
        glm::vec3 end = glm::ceil(center + extents);
        
        // paint all points within the radius
        float z = qMax(start.z, 0.0f);
        float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highest);
        uchar* lineDest = (uchar*)contents.data() + (int)z * size + (int)startX;
        float squaredRadius = scaledRadius * scaledRadius; 
        bool changed = false;
        QHash<uchar, int> counts;
        for (float endZ = qMin(end.z, (float)highest); z <= endZ; z += 1.0f) {
            uchar* dest = lineDest;
            for (float x = startX; x <= endX; x += 1.0f, dest++) {
                float dx = x - center.x, dz = z - center.z;
                if (dx * dx + dz * dz <= squaredRadius) {
                    *dest = materialIndex;
                    changed = true;
                }
            }
            lineDest += size;
        }
        if (changed) {
            clearUnusedMaterials(materials, contents);
            HeightfieldMaterialDataPointer newPointer(new HeightfieldMaterialData(contents, materials));
            info.outputValues[1] = AttributeValue(_outputs.at(1), encodeInline<HeightfieldMaterialDataPointer>(newPointer));
        }
    }
    return STOP_RECURSION;
}

PaintHeightfieldMaterialEdit::PaintHeightfieldMaterialEdit(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& averageColor) :
    MaterialEdit(material, averageColor),
    position(position),
    radius(radius) {
}

void PaintHeightfieldMaterialEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    PaintHeightfieldMaterialEditVisitor visitor(position, radius, material, averageColor);
    data.guide(visitor);
}

const int VOXEL_BLOCK_SIZE = 16;
const int VOXEL_BLOCK_SAMPLES = VOXEL_BLOCK_SIZE + 1;
const int VOXEL_BLOCK_AREA = VOXEL_BLOCK_SAMPLES * VOXEL_BLOCK_SAMPLES;
const int VOXEL_BLOCK_VOLUME = VOXEL_BLOCK_AREA * VOXEL_BLOCK_SAMPLES;

VoxelMaterialSpannerEdit::VoxelMaterialSpannerEdit(const SharedObjectPointer& spanner,
        const SharedObjectPointer& material, const QColor& averageColor) :
    MaterialEdit(material, averageColor),
    spanner(spanner) {
}

class VoxelHeightfieldFetchVisitor : public MetavoxelVisitor {
public:
    
    VoxelHeightfieldFetchVisitor();
    
    void init(const Box& bounds);
    
    bool isEmpty() const { return _heightContents.isEmpty(); }
    
    int getContentsWidth() const { return _contentsWidth; }
    int getContentsHeight() const { return _contentsHeight; }
    
    const Box& getHeightBounds() const { return _heightBounds; }
    
    float getInterpolatedHeight(float x, float y) const;
    
    QRgb getInterpolatedColor(float x, float y) const;
    
    int getNearestMaterial(float x, float y) const;
    
    const QVector<SharedObjectPointer>& getMaterials() const { return _materials; }
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
    Box _expandedBounds;
    int _contentsWidth;
    int _contentsHeight;
    QByteArray _heightContents;
    QByteArray _colorContents;
    QByteArray _materialContents;
    QVector<SharedObjectPointer> _materials;
    Box _heightBounds;
};

VoxelHeightfieldFetchVisitor::VoxelHeightfieldFetchVisitor() :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute()) {
}

void VoxelHeightfieldFetchVisitor::init(const Box& bounds) {
    _expandedBounds = _bounds = bounds;
    float increment = (bounds.maximum.x - bounds.minimum.x) / VOXEL_BLOCK_SIZE;
    _expandedBounds.maximum.x += increment;
    _expandedBounds.maximum.z += increment;
    _heightContents.clear();
    _colorContents.clear();
    _materialContents.clear();
    _materials.clear();
}

float VoxelHeightfieldFetchVisitor::getInterpolatedHeight(float x, float y) const {
    int floorX = glm::floor(x), floorY = glm::floor(y);
    int ceilX = glm::ceil(x), ceilY = glm::ceil(y);
    const uchar* src = (const uchar*)_heightContents.constData();
    float upperLeft = src[floorY * _contentsWidth + floorX];
    float lowerRight = src[ceilY * _contentsWidth + ceilX];
    float fractX = glm::fract(x), fractY = glm::fract(y);
    float interpolatedHeight = glm::mix(upperLeft, lowerRight, fractY);
    if (fractX >= fractY) {
        float upperRight = src[floorY * _contentsWidth + ceilX];
        interpolatedHeight = glm::mix(interpolatedHeight, glm::mix(upperRight, lowerRight, fractY),
            (fractX - fractY) / (1.0f - fractY));
        
    } else {
        float lowerLeft = src[ceilY * _contentsWidth + floorX];
        interpolatedHeight = glm::mix(glm::mix(upperLeft, lowerLeft, fractY), interpolatedHeight, fractX / fractY);
    }
    return interpolatedHeight;
}

QRgb VoxelHeightfieldFetchVisitor::getInterpolatedColor(float x, float y) const {
    int floorX = glm::floor(x), floorY = glm::floor(y);
    int ceilX = glm::ceil(x), ceilY = glm::ceil(y);
    const uchar* src = (const uchar*)_colorContents.constData();
    const uchar* upperLeft = src + (floorY * _contentsWidth + floorX) * DataBlock::COLOR_BYTES;
    const uchar* lowerRight = src + (ceilY * _contentsWidth + ceilX) * DataBlock::COLOR_BYTES;
    float fractX = glm::fract(x), fractY = glm::fract(y);
    glm::vec3 interpolatedColor = glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
        glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fractY);
    if (fractX >= fractY) {
        const uchar* upperRight = src + (floorY * _contentsWidth + ceilX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(interpolatedColor, glm::mix(glm::vec3(upperRight[0], upperRight[1], upperRight[2]),
            glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fractY), (fractX - fractY) / (1.0f - fractY));
        
    } else {
        const uchar* lowerLeft = src + (ceilY * _contentsWidth + floorX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
            glm::vec3(lowerLeft[0], lowerLeft[1], lowerLeft[2]), fractY), interpolatedColor, fractX / fractY);
    }
    return qRgb(interpolatedColor.r, interpolatedColor.g, interpolatedColor.b);
}

int VoxelHeightfieldFetchVisitor::getNearestMaterial(float x, float y) const {
    const uchar* src = (const uchar*)_materialContents.constData();
    return src[(int)glm::round(y) * _contentsWidth + (int)glm::round(x)];
}

int VoxelHeightfieldFetchVisitor::visit(MetavoxelInfo& info) {
    Box bounds = info.getBounds();
    if (!bounds.intersects(_expandedBounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    HeightfieldHeightDataPointer heightPointer = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    if (!heightPointer) {
        return STOP_RECURSION;
    }
    const QByteArray& heightContents = heightPointer->getContents();
    int size = glm::sqrt((float)heightContents.size());
    float heightIncrement = info.size / size;
    
    // the first heightfield we encounter determines the resolution
    if (_heightContents.isEmpty()) {
        _heightBounds.minimum = glm::floor(_bounds.minimum / heightIncrement) * heightIncrement;
        _heightBounds.maximum = (glm::ceil(_bounds.maximum / heightIncrement) + glm::vec3(1.0f, 0.0f, 1.0f)) * heightIncrement;
        _heightBounds.minimum.y = bounds.minimum.y;
        _heightBounds.maximum.y = bounds.maximum.y;
        _contentsWidth = (int)glm::round((_heightBounds.maximum.x - _heightBounds.minimum.x) / heightIncrement);
        _contentsHeight = (int)glm::round((_heightBounds.maximum.z - _heightBounds.minimum.z) / heightIncrement);
        _heightContents = QByteArray(_contentsWidth * _contentsHeight, 0);
        _colorContents = QByteArray(_contentsWidth * _contentsHeight * DataBlock::COLOR_BYTES, 0);
        _materialContents = QByteArray(_contentsWidth * _contentsHeight, 0);
    }
    
    Box overlap = _heightBounds.getIntersection(bounds);
    if (overlap.isEmpty()) {
        return STOP_RECURSION;
    }
    
    int srcX = (overlap.minimum.x - bounds.minimum.x) / heightIncrement;
    int srcY = (overlap.minimum.z - bounds.minimum.z) / heightIncrement;
    int destX = (overlap.minimum.x - _heightBounds.minimum.x) / heightIncrement;
    int destY = (overlap.minimum.z - _heightBounds.minimum.z) / heightIncrement;
    int width = (overlap.maximum.x - overlap.minimum.x) / heightIncrement;
    int height = (overlap.maximum.z - overlap.minimum.z) / heightIncrement;
    char* heightDest = _heightContents.data() + destY * _contentsWidth + destX;
    const char* heightSrc = heightContents.constData() + srcY * size + srcX;
    
    for (int y = 0; y < height; y++, heightDest += _contentsWidth, heightSrc += size) {
        memcpy(heightDest, heightSrc, width);
    }
    
    HeightfieldColorDataPointer colorPointer = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
    if (colorPointer && colorPointer->getContents().size() == heightContents.size() * DataBlock::COLOR_BYTES) {
        char* colorDest = _colorContents.data() + (destY * _contentsWidth + destX) * DataBlock::COLOR_BYTES;
        const char* colorSrc = colorPointer->getContents().constData() + (srcY * size + srcX) * DataBlock::COLOR_BYTES;
        
        for (int y = 0; y < height; y++, colorDest += _contentsWidth * DataBlock::COLOR_BYTES,
                colorSrc += size * DataBlock::COLOR_BYTES) {
            memcpy(colorDest, colorSrc, width * DataBlock::COLOR_BYTES);
        }     
    }
    
    HeightfieldMaterialDataPointer materialPointer = info.inputValues.at(2).getInlineValue<HeightfieldMaterialDataPointer>();
    if (materialPointer && materialPointer->getContents().size() == heightContents.size()) {
        uchar* materialDest = (uchar*)_materialContents.data() + destY * _contentsWidth + destX;
        const uchar* materialSrc = (const uchar*)materialPointer->getContents().constData() + srcY * size + srcX;
        const QVector<SharedObjectPointer>& materials = materialPointer->getMaterials();
        QHash<int, int> materialMap;
        
        for (int y = 0; y < height; y++, materialDest += _contentsWidth, materialSrc += size) {
            const uchar* lineSrc = materialSrc;
            for (uchar* lineDest = materialDest, *end = lineDest + width; lineDest != end; lineDest++, lineSrc++) {
                int value = *lineSrc;
                if (value == 0) {
                    continue;
                }
                int& mapping = materialMap[value];
                if (mapping == 0) {
                    _materials.append(materials.at(value - 1));
                    mapping = _materials.size();
                }
                *lineDest = mapping;
            }
        }
    }
    
    return STOP_RECURSION;
}

class VoxelMaterialSpannerEditVisitor : public MetavoxelVisitor {
public:
    
    VoxelMaterialSpannerEditVisitor(Spanner* spanner, const SharedObjectPointer& material, const QColor& color);

    virtual int visit(MetavoxelInfo& info);

private:
    
    Spanner* _spanner;
    SharedObjectPointer _material;
    QColor _color;
    float _blockSize;
    VoxelHeightfieldFetchVisitor _heightfieldVisitor;
};

VoxelMaterialSpannerEditVisitor::VoxelMaterialSpannerEditVisitor(Spanner* spanner,
        const SharedObjectPointer& material, const QColor& color) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getVoxelColorAttribute() <<
        AttributeRegistry::getInstance()->getVoxelHermiteAttribute() <<
        AttributeRegistry::getInstance()->getVoxelMaterialAttribute(), QVector<AttributePointer>() <<
            AttributeRegistry::getInstance()->getVoxelColorAttribute() <<
                AttributeRegistry::getInstance()->getVoxelHermiteAttribute() <<
                AttributeRegistry::getInstance()->getVoxelMaterialAttribute()),
    _spanner(spanner),
    _material(material),
    _color(color),
    _blockSize(spanner->getVoxelizationGranularity() * VOXEL_BLOCK_SIZE) {
}

int VoxelMaterialSpannerEditVisitor::visit(MetavoxelInfo& info) {
    Box bounds = info.getBounds();
    if (!bounds.intersects(_spanner->getBounds())) {
        return STOP_RECURSION;
    }
    if (info.size > _blockSize) {
        return DEFAULT_ORDER;
    }
    bool fetchFromHeightfield = false;
    QVector<QRgb> oldColorContents;
    VoxelColorDataPointer colorPointer = info.inputValues.at(0).getInlineValue<VoxelColorDataPointer>();
    if (colorPointer && colorPointer->getSize() == VOXEL_BLOCK_SAMPLES) {
        oldColorContents = colorPointer->getContents();
    } else {
        oldColorContents = QVector<QRgb>(VOXEL_BLOCK_VOLUME);
        fetchFromHeightfield = true;
    }
    
    QVector<QRgb> hermiteContents;
    VoxelHermiteDataPointer hermitePointer = info.inputValues.at(1).getInlineValue<VoxelHermiteDataPointer>();
    if (hermitePointer && hermitePointer->getSize() == VOXEL_BLOCK_SAMPLES) {
        hermiteContents = hermitePointer->getContents();
    } else {
        hermiteContents = QVector<QRgb>(VOXEL_BLOCK_VOLUME * VoxelHermiteData::EDGE_COUNT);
        fetchFromHeightfield = true;
    }
    
    QByteArray materialContents;
    QVector<SharedObjectPointer> materials;
    VoxelMaterialDataPointer materialPointer = info.inputValues.at(2).getInlineValue<VoxelMaterialDataPointer>();
    if (materialPointer && materialPointer->getSize() == VOXEL_BLOCK_SAMPLES) {
        materialContents = materialPointer->getContents();
        materials = materialPointer->getMaterials();        
    } else {
        materialContents = QByteArray(VOXEL_BLOCK_VOLUME, 0);
        fetchFromHeightfield = true;
    }
    
    float scale = VOXEL_BLOCK_SIZE / info.size;
    if (fetchFromHeightfield) {
        // the first time we touch a voxel block, we have to look up any intersecting heightfield data
        _heightfieldVisitor.init(bounds);
        _data->guide(_heightfieldVisitor);
        if (!_heightfieldVisitor.isEmpty()) {
            QRgb* colorDest = oldColorContents.data();
            QRgb* hermiteDest = hermiteContents.data();
            uchar* materialDest = (uchar*)materialContents.data();
            const Box& heightBounds = _heightfieldVisitor.getHeightBounds();
            float heightStepX = (heightBounds.maximum.x - heightBounds.minimum.x) /
                (_heightfieldVisitor.getContentsWidth() - 1);
            float heightStepY = (heightBounds.maximum.z - heightBounds.minimum.z) /
                (_heightfieldVisitor.getContentsHeight() - 1);
            float lineHeightX = (bounds.minimum.x - heightBounds.minimum.x) / heightStepX;
            float heightY = (bounds.minimum.z - heightBounds.minimum.z) / heightStepY;
            float heightScale = (heightBounds.maximum.y - heightBounds.minimum.y) / EIGHT_BIT_MAXIMUM;
            QHash<int, int> materialMap;
            
            for (int z = 0; z < VOXEL_BLOCK_SAMPLES; z++, heightY += heightStepY) {
                float heightX = lineHeightX;
                for (int x = 0; x < VOXEL_BLOCK_SAMPLES; x++, colorDest++, hermiteDest += VoxelHermiteData::EDGE_COUNT,
                        materialDest++, heightX += heightStepX) {
                    float height = _heightfieldVisitor.getInterpolatedHeight(heightX, heightY);   
                    if (height == 0.0f) {
                        continue;
                    }
                    height = heightBounds.minimum.y + height * heightScale;
                    if (height < bounds.minimum.y) {
                        continue;
                    }
                    height = (height - bounds.minimum.y) * scale;
                    int clampedHeight = qMin((int)height, VOXEL_BLOCK_SIZE);
                    QRgb color = _heightfieldVisitor.getInterpolatedColor(heightX, heightY);
                    uchar* lineMaterialDest = materialDest;
                    uchar material = _heightfieldVisitor.getNearestMaterial(heightX, heightY);
                    if (material != 0) {
                        int& mapping = materialMap[material];
                        if (mapping == 0) {
                            materials.append(_heightfieldVisitor.getMaterials().at(material - 1));
                            mapping = materials.size();
                        }
                        material = mapping;
                    }
                    for (QRgb* lineColorDest = colorDest, *end = lineColorDest + clampedHeight * VOXEL_BLOCK_SAMPLES;
                            lineColorDest != end; lineColorDest += VOXEL_BLOCK_SAMPLES,
                                lineMaterialDest += VOXEL_BLOCK_SAMPLES) {
                        *lineColorDest = color;
                        *lineMaterialDest = material;
                    }
                    hermiteDest[clampedHeight * VOXEL_BLOCK_SAMPLES * VoxelHermiteData::EDGE_COUNT + 1] = packNormal(
                        glm::vec3(0.0f, 1.0f, 0.0f), (height - clampedHeight) * EIGHT_BIT_MAXIMUM);
                }
                colorDest += VOXEL_BLOCK_AREA - VOXEL_BLOCK_SAMPLES;
                hermiteDest += (VOXEL_BLOCK_AREA - VOXEL_BLOCK_SAMPLES) * VoxelHermiteData::EDGE_COUNT;
                materialDest += VOXEL_BLOCK_AREA - VOXEL_BLOCK_SAMPLES;
            }
        }
    }
    QVector<QRgb> colorContents = oldColorContents;
    
    Box overlap = info.getBounds().getIntersection(_spanner->getBounds());
    overlap.minimum = (overlap.minimum - info.minimum) * scale;
    overlap.maximum = (overlap.maximum - info.minimum) * scale;
    int minX = glm::ceil(overlap.minimum.x);
    int minY = glm::ceil(overlap.minimum.y);
    int minZ = glm::ceil(overlap.minimum.z);
    int sizeX = (int)overlap.maximum.x - minX + 1;
    int sizeY = (int)overlap.maximum.y - minY + 1;
    int sizeZ = (int)overlap.maximum.z - minZ + 1;
    
    QRgb rgb = _color.rgba();
    bool flipped = (qAlpha(rgb) == 0);
    float step = 1.0f / scale;
    glm::vec3 position(0.0f, 0.0f, info.minimum.z + minZ * step);
    for (QRgb* destZ = colorContents.data() + minZ * VOXEL_BLOCK_AREA + minY * VOXEL_BLOCK_SAMPLES + minX,
            *endZ = destZ + sizeZ * VOXEL_BLOCK_AREA; destZ != endZ; destZ += VOXEL_BLOCK_AREA, position.z += step) {
        position.y = info.minimum.y + minY * step;
        for (QRgb* destY = destZ, *endY = destY + sizeY * VOXEL_BLOCK_SAMPLES; destY != endY;
                destY += VOXEL_BLOCK_SAMPLES, position.y += step) {
            position.x = info.minimum.x + minX * step;
            for (QRgb* destX = destY, *endX = destX + sizeX; destX != endX; destX++, position.x += step) {
                if (_spanner->contains(position)) {
                    *destX = rgb;
                }
            }
        }
    }
    VoxelColorDataPointer newColorPointer(new VoxelColorData(colorContents, VOXEL_BLOCK_SAMPLES));
    info.outputValues[0] = AttributeValue(info.inputValues.at(0).getAttribute(),
        encodeInline<VoxelColorDataPointer>(newColorPointer));
    
    int hermiteArea = VOXEL_BLOCK_AREA * VoxelHermiteData::EDGE_COUNT;
    int hermiteSamples = VOXEL_BLOCK_SAMPLES * VoxelHermiteData::EDGE_COUNT;
    int hermiteMinX = minX, hermiteMinY = minY, hermiteMinZ = minZ;
    int hermiteSizeX = sizeX, hermiteSizeY = sizeY, hermiteSizeZ = sizeZ;
    if (minX > 0) {
        hermiteMinX--;
        hermiteSizeX++;
    }
    if (minY > 0) {
        hermiteMinY--;
        hermiteSizeY++;
    }
    if (minZ > 0) {
        hermiteMinZ--;
        hermiteSizeZ++;
    }
    QRgb* hermiteDestZ = hermiteContents.data() + hermiteMinZ * hermiteArea + hermiteMinY * hermiteSamples +
        hermiteMinX * VoxelHermiteData::EDGE_COUNT;
    for (int z = hermiteMinZ, hermiteMaxZ = z + hermiteSizeZ - 1; z <= hermiteMaxZ; z++, hermiteDestZ += hermiteArea) {
        QRgb* hermiteDestY = hermiteDestZ;
        for (int y = hermiteMinY, hermiteMaxY = y + hermiteSizeY - 1; y <= hermiteMaxY; y++, hermiteDestY += hermiteSamples) {
            QRgb* hermiteDestX = hermiteDestY;
            for (int x = hermiteMinX, hermiteMaxX = x + hermiteSizeX - 1; x <= hermiteMaxX; x++,
                    hermiteDestX += VoxelHermiteData::EDGE_COUNT) {
                // at each intersected non-terminal edge, we check for a transition and, if one is detected, we assign the
                // crossing and normal values based on intersection with the sphere
                float distance;
                glm::vec3 normal;
                if (x != VOXEL_BLOCK_SIZE) {
                    int offset = z * VOXEL_BLOCK_AREA + y * VOXEL_BLOCK_SAMPLES + x;
                    const QRgb* color = colorContents.constData() + offset;
                    int alpha0 = qAlpha(color[0]);
                    int alpha1 = qAlpha(color[1]);
                    if (alpha0 != alpha1) {
                        if (_spanner->intersects(info.minimum + glm::vec3(x, y, z) * step,
                                info.minimum + glm::vec3(x + 1, y, z) * step, distance, normal)) {
                            const QRgb* oldColor = oldColorContents.constData() + offset;
                            if (qAlpha(oldColor[0]) == alpha0 && qAlpha(oldColor[1]) == alpha1) {
                                int alpha = distance * EIGHT_BIT_MAXIMUM;
                                if (normal.x < 0.0f ? alpha <= qAlpha(hermiteDestX[0]) : alpha >= qAlpha(hermiteDestX[0])) {
                                    hermiteDestX[0] = packNormal(flipped ? -normal : normal, alpha);    
                                }
                            } else {
                                hermiteDestX[0] = packNormal(flipped ? -normal : normal, distance * EIGHT_BIT_MAXIMUM);
                            }
                        }
                    } else {
                        hermiteDestX[0] = 0x0;
                    }
                } else {
                    hermiteDestX[0] = 0x0;
                }
                if (y != VOXEL_BLOCK_SIZE) {
                    int offset = z * VOXEL_BLOCK_AREA + y * VOXEL_BLOCK_SAMPLES + x;
                    const QRgb* color = colorContents.constData() + offset;
                    int alpha0 = qAlpha(color[0]);
                    int alpha2 = qAlpha(color[VOXEL_BLOCK_SAMPLES]);
                    if (alpha0 != alpha2) {
                        if (_spanner->intersects(info.minimum + glm::vec3(x, y, z) * step,
                                info.minimum + glm::vec3(x, y + 1, z) * step, distance, normal)) {
                            const QRgb* oldColor = oldColorContents.constData() + offset;
                            if (qAlpha(oldColor[0]) == alpha0 && qAlpha(oldColor[VOXEL_BLOCK_SAMPLES]) == alpha2) {
                                int alpha = distance * EIGHT_BIT_MAXIMUM;
                                if (normal.y < 0.0f ? alpha <= qAlpha(hermiteDestX[1]) : alpha >= qAlpha(hermiteDestX[1])) {
                                    hermiteDestX[1] = packNormal(flipped ? -normal : normal, alpha);    
                                }
                            } else {
                                hermiteDestX[1] = packNormal(flipped ? -normal : normal, distance * EIGHT_BIT_MAXIMUM);
                            }
                        }
                    } else {
                        hermiteDestX[1] = 0x0;
                    }
                } else {
                    hermiteDestX[1] = 0x0;
                }
                if (z != VOXEL_BLOCK_SIZE) {
                    int offset = z * VOXEL_BLOCK_AREA + y * VOXEL_BLOCK_SAMPLES + x;
                    const QRgb* color = colorContents.constData() + offset;
                    int alpha0 = qAlpha(color[0]);
                    int alpha4 = qAlpha(color[VOXEL_BLOCK_AREA]);
                    if (alpha0 != alpha4) {
                        if (_spanner->intersects(info.minimum + glm::vec3(x, y, z) * step,
                                info.minimum + glm::vec3(x, y, z + 1) * step, distance, normal)) {
                            const QRgb* oldColor = oldColorContents.constData() + offset;
                            if (qAlpha(oldColor[0]) == alpha0 && qAlpha(oldColor[VOXEL_BLOCK_AREA]) == alpha4) {
                                int alpha = distance * EIGHT_BIT_MAXIMUM;
                                if (normal.z < 0.0f ? alpha <= qAlpha(hermiteDestX[2]) : alpha >= qAlpha(hermiteDestX[2])) {
                                    hermiteDestX[2] = packNormal(flipped ? -normal : normal, alpha);    
                                }
                            } else {
                                hermiteDestX[2] = packNormal(flipped ? -normal : normal, distance * EIGHT_BIT_MAXIMUM);
                            }
                        }
                    } else {
                        hermiteDestX[2] = 0x0;
                    }
                } else {
                    hermiteDestX[2] = 0x0;
                }
            }
        }
    }
    VoxelHermiteDataPointer newHermitePointer(new VoxelHermiteData(hermiteContents, VOXEL_BLOCK_SAMPLES));
    info.outputValues[1] = AttributeValue(info.inputValues.at(1).getAttribute(),
        encodeInline<VoxelHermiteDataPointer>(newHermitePointer));
    
    uchar materialIndex = getMaterialIndex(_material, materials, materialContents);
    position.z = info.minimum.z + minZ * step;
    for (uchar* destZ = (uchar*)materialContents.data() + minZ * VOXEL_BLOCK_AREA + minY * VOXEL_BLOCK_SAMPLES + minX,
            *endZ = destZ + sizeZ * VOXEL_BLOCK_AREA; destZ != endZ; destZ += VOXEL_BLOCK_AREA, position.z += step) {
        position.y = info.minimum.y + minY * step;
        for (uchar* destY = destZ, *endY = destY + sizeY * VOXEL_BLOCK_SAMPLES; destY != endY;
                destY += VOXEL_BLOCK_SAMPLES, position.y += step) {
            position.x = info.minimum.x + minX * step;
            for (uchar* destX = destY, *endX = destX + sizeX; destX != endX; destX++, position.x += step) {
                if (_spanner->contains(position)) { 
                    *destX = materialIndex;
                }   
            }
        }
    }
    clearUnusedMaterials(materials, materialContents);
    VoxelMaterialDataPointer newMaterialPointer(new VoxelMaterialData(materialContents, VOXEL_BLOCK_SAMPLES, materials));
    info.outputValues[2] = AttributeValue(_inputs.at(2), encodeInline<VoxelMaterialDataPointer>(newMaterialPointer));
    
    return STOP_RECURSION;
}

class HeightfieldClearVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldClearVisitor(const Box& bounds, float granularity);

    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
};

HeightfieldClearVisitor::HeightfieldClearVisitor(const Box& bounds, float granularity) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute(), QVector<AttributePointer>() <<
            AttributeRegistry::getInstance()->getHeightfieldAttribute() <<
            AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
            AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute()) {
     
    // find the bounds of all voxel nodes intersected
    float nodeSize = VOXEL_BLOCK_SIZE * glm::pow(2.0f, glm::floor(glm::log(granularity) / glm::log(2.0f)));
    _bounds.minimum = glm::floor(bounds.minimum / nodeSize) * nodeSize;
    _bounds.maximum = glm::ceil(bounds.maximum / nodeSize) * nodeSize;
    
    // expand to include edges
    float increment = nodeSize / VOXEL_BLOCK_SIZE;
    _bounds.maximum.x += increment;
    _bounds.maximum.z += increment;
}

int HeightfieldClearVisitor::visit(MetavoxelInfo& info) {
    Box bounds = info.getBounds();
    if (!bounds.intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    HeightfieldHeightDataPointer heightPointer = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    if (!heightPointer) {
        return STOP_RECURSION;
    }
    QByteArray contents(heightPointer->getContents());
    int size = glm::sqrt((float)contents.size());
    float heightScale = size / info.size;
    
    Box overlap = bounds.getIntersection(_bounds);
    int destX = (overlap.minimum.x - info.minimum.x) * heightScale;
    int destY = (overlap.minimum.z - info.minimum.z) * heightScale;
    int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) * heightScale);
    int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) * heightScale);
    char* dest = contents.data() + destY * size + destX;
    
    for (int y = 0; y < destHeight; y++, dest += size) {
        memset(dest, 0, destWidth);
    }
    
    HeightfieldHeightDataPointer newHeightPointer(new HeightfieldHeightData(contents));
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline<HeightfieldHeightDataPointer>(newHeightPointer));
    
    HeightfieldColorDataPointer colorPointer = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
    if (colorPointer) {
        contents = colorPointer->getContents();
        size = glm::sqrt((float)contents.size() / DataBlock::COLOR_BYTES);
        heightScale = size / info.size;
        
        int destX = (overlap.minimum.x - info.minimum.x) * heightScale;
        int destY = (overlap.minimum.z - info.minimum.z) * heightScale;
        int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) * heightScale);
        int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) * heightScale);
        char* dest = contents.data() + (destY * size + destX) * DataBlock::COLOR_BYTES;
        
        for (int y = 0; y < destHeight; y++, dest += size * DataBlock::COLOR_BYTES) {
            memset(dest, 0, destWidth * DataBlock::COLOR_BYTES);
        }
        
        HeightfieldColorDataPointer newColorPointer(new HeightfieldColorData(contents));
        info.outputValues[1] = AttributeValue(_outputs.at(1), encodeInline<HeightfieldColorDataPointer>(newColorPointer));
    }
    
    HeightfieldMaterialDataPointer materialPointer = info.inputValues.at(2).getInlineValue<HeightfieldMaterialDataPointer>();
    if (materialPointer) {
        contents = materialPointer->getContents();
        size = glm::sqrt((float)contents.size());
        heightScale = size / info.size;
        
        int destX = (overlap.minimum.x - info.minimum.x) * heightScale;
        int destY = (overlap.minimum.z - info.minimum.z) * heightScale;
        int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) * heightScale);
        int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) * heightScale);
        char* dest = contents.data() + destY * size + destX;
        
        for (int y = 0; y < destHeight; y++, dest += size) {
            memset(dest, 0, destWidth);
        }
        
        QVector<SharedObjectPointer> materials = materialPointer->getMaterials();
        clearUnusedMaterials(materials, contents);
        HeightfieldMaterialDataPointer newMaterialPointer(new HeightfieldMaterialData(contents, materials));
        info.outputValues[2] = AttributeValue(_outputs.at(2),
            encodeInline<HeightfieldMaterialDataPointer>(newMaterialPointer));
    }
    
    return STOP_RECURSION;
}

void VoxelMaterialSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // expand to fit the entire edit
    Spanner* spanner = static_cast<Spanner*>(this->spanner.data());
    while (!data.getBounds().contains(spanner->getBounds())) {
        data.expand();
    }
    // make sure it's either 100% transparent or 100% opaque
    QColor color = averageColor;
    color.setAlphaF(color.alphaF() > 0.5f ? 1.0f : 0.0f);
    VoxelMaterialSpannerEditVisitor visitor(spanner, material, color);
    data.guide(visitor);
    
    // clear any heightfield data
    HeightfieldClearVisitor clearVisitor(spanner->getBounds(), spanner->getVoxelizationGranularity());
    data.guide(clearVisitor);
}

PaintVoxelMaterialEdit::PaintVoxelMaterialEdit(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& averageColor) :
    MaterialEdit(material, averageColor),
    position(position),
    radius(radius) {
}

class PaintVoxelMaterialEditVisitor : public MetavoxelVisitor {
public:
    
    PaintVoxelMaterialEditVisitor(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& color);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    glm::vec3 _position;
    float _radius;
    SharedObjectPointer _material;
    QColor _color;
    Box _bounds;
};

PaintVoxelMaterialEditVisitor::PaintVoxelMaterialEditVisitor(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& color) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getVoxelColorAttribute() <<
        AttributeRegistry::getInstance()->getVoxelMaterialAttribute(), QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getVoxelColorAttribute() <<
            AttributeRegistry::getInstance()->getVoxelMaterialAttribute()),
    _position(position),
    _radius(radius),
    _material(material),
    _color(color) {
    
    glm::vec3 extents(_radius, _radius, _radius);
    _bounds = Box(_position - extents, _position + extents);
}

int PaintVoxelMaterialEditVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    VoxelColorDataPointer colorPointer = info.inputValues.at(0).getInlineValue<VoxelColorDataPointer>();
    VoxelMaterialDataPointer materialPointer = info.inputValues.at(1).getInlineValue<VoxelMaterialDataPointer>();
    if (!(colorPointer && materialPointer && colorPointer->getSize() == materialPointer->getSize())) {
        return STOP_RECURSION;
    }
    QVector<QRgb> colorContents = colorPointer->getContents();
    QByteArray materialContents = materialPointer->getContents();
    QVector<SharedObjectPointer> materials = materialPointer->getMaterials();
    
    Box overlap = info.getBounds().getIntersection(_bounds);
    int size = colorPointer->getSize();
    int area = size * size;
    float scale = (size - 1.0f) / info.size;
    overlap.minimum = (overlap.minimum - info.minimum) * scale;
    overlap.maximum = (overlap.maximum - info.minimum) * scale;
    int minX = glm::ceil(overlap.minimum.x);
    int minY = glm::ceil(overlap.minimum.y);
    int minZ = glm::ceil(overlap.minimum.z);
    int sizeX = (int)overlap.maximum.x - minX + 1;
    int sizeY = (int)overlap.maximum.y - minY + 1;
    int sizeZ = (int)overlap.maximum.z - minZ + 1;
    
    QRgb rgb = _color.rgba();
    float step = 1.0f / scale;
    glm::vec3 position(0.0f, 0.0f, info.minimum.z + minZ * step);
    uchar materialIndex = getMaterialIndex(_material, materials, materialContents);
    QRgb* colorData = colorContents.data();
    uchar* materialData = (uchar*)materialContents.data();
    for (int destZ = minZ * area + minY * size + minX, endZ = destZ + sizeZ * area; destZ != endZ;
            destZ += area, position.z += step) {
        position.y = info.minimum.y + minY * step;
        for (int destY = destZ, endY = destY + sizeY * size; destY != endY; destY += size, position.y += step) {
            position.x = info.minimum.x + minX * step;
            for (int destX = destY, endX = destX + sizeX; destX != endX; destX++, position.x += step) {
                QRgb& color = colorData[destX];
                if (qAlpha(color) != 0 && glm::distance(position, _position) <= _radius) {
                    color = rgb;
                    materialData[destX] = materialIndex;    
                }
            }
        }
    }
    VoxelColorDataPointer newColorPointer(new VoxelColorData(colorContents, size));
    info.outputValues[0] = AttributeValue(info.inputValues.at(0).getAttribute(),
        encodeInline<VoxelColorDataPointer>(newColorPointer));
    
    clearUnusedMaterials(materials, materialContents);
    VoxelMaterialDataPointer newMaterialPointer(new VoxelMaterialData(materialContents, size, materials));
    info.outputValues[1] = AttributeValue(_inputs.at(1), encodeInline<VoxelMaterialDataPointer>(newMaterialPointer));
    
    return STOP_RECURSION;
}

void PaintVoxelMaterialEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // make sure it's 100% opaque
    QColor color = averageColor;
    color.setAlphaF(1.0f);
    PaintVoxelMaterialEditVisitor visitor(position, radius, material, color);
    data.guide(visitor);
}
