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

class VoxelMaterialSpannerEditVisitor : public MetavoxelVisitor {
public:
    
    VoxelMaterialSpannerEditVisitor(Spanner* spanner, const SharedObjectPointer& material, const QColor& color);

    virtual int visit(MetavoxelInfo& info);

private:
    
    Spanner* _spanner;
    SharedObjectPointer _material;
    QColor _color;
    float _blockSize;
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
    QVector<QRgb> oldColorContents;
    VoxelColorDataPointer colorPointer = info.inputValues.at(0).getInlineValue<VoxelColorDataPointer>();
    if (colorPointer && colorPointer->getSize() == VOXEL_BLOCK_SAMPLES) {
        oldColorContents = colorPointer->getContents();
    } else {
        oldColorContents = QVector<QRgb>(VOXEL_BLOCK_VOLUME);
    }
    
    QVector<QRgb> hermiteContents;
    VoxelHermiteDataPointer hermitePointer = info.inputValues.at(1).getInlineValue<VoxelHermiteDataPointer>();
    if (hermitePointer && hermitePointer->getSize() == VOXEL_BLOCK_SAMPLES) {
        hermiteContents = hermitePointer->getContents();
    } else {
        hermiteContents = QVector<QRgb>(VOXEL_BLOCK_VOLUME * VoxelHermiteData::EDGE_COUNT);
    }
    
    QByteArray materialContents;
    QVector<SharedObjectPointer> materials;
    VoxelMaterialDataPointer materialPointer = info.inputValues.at(2).getInlineValue<VoxelMaterialDataPointer>();
    if (materialPointer && materialPointer->getSize() == VOXEL_BLOCK_SAMPLES) {
        materialContents = materialPointer->getContents();
        materials = materialPointer->getMaterials();        
    } else {
        materialContents = QByteArray(VOXEL_BLOCK_VOLUME, 0);
    }
    
    float scale = VOXEL_BLOCK_SIZE / info.size;
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
    
    bool flipped = false;
    float step = 1.0f / scale;
    glm::vec3 position(0.0f, 0.0f, info.minimum.z + minZ * step);
    if (_spanner->hasOwnColors()) {
        for (QRgb* destZ = colorContents.data() + minZ * VOXEL_BLOCK_AREA + minY * VOXEL_BLOCK_SAMPLES + minX,
                *endZ = destZ + sizeZ * VOXEL_BLOCK_AREA; destZ != endZ; destZ += VOXEL_BLOCK_AREA, position.z += step) {
            position.y = info.minimum.y + minY * step;
            for (QRgb* destY = destZ, *endY = destY + sizeY * VOXEL_BLOCK_SAMPLES; destY != endY;
                    destY += VOXEL_BLOCK_SAMPLES, position.y += step) {
                position.x = info.minimum.x + minX * step;
                for (QRgb* destX = destY, *endX = destX + sizeX; destX != endX; destX++, position.x += step) {
                    if (_spanner->contains(position)) {
                        *destX = _spanner->getColorAt(position);
                    }
                }
            }
        }
    } else {
        QRgb rgb = _color.rgba();
        flipped = (qAlpha(rgb) == 0);
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
    }
    
    // if there are no visible colors, we can clear everything
    bool foundOpaque = false;
    for (const QRgb* src = colorContents.constData(), *end = src + colorContents.size(); src != end; src++) {
        if (qAlpha(*src) != 0) {
            foundOpaque = true;
            break;
        }
    }
    if (!foundOpaque) {
        info.outputValues[0] = AttributeValue(_outputs.at(0));
        info.outputValues[1] = AttributeValue(_outputs.at(1));
        info.outputValues[2] = AttributeValue(_outputs.at(2));
        return STOP_RECURSION;
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
    
    if (_spanner->hasOwnMaterials()) {
        QHash<int, int> materialMap;
        position.z = info.minimum.z + minZ * step;
        for (uchar* destZ = (uchar*)materialContents.data() + minZ * VOXEL_BLOCK_AREA + minY * VOXEL_BLOCK_SAMPLES + minX,
                *endZ = destZ + sizeZ * VOXEL_BLOCK_AREA; destZ != endZ; destZ += VOXEL_BLOCK_AREA, position.z += step) {
            position.y = info.minimum.y + minY * step;
            for (uchar* destY = destZ, *endY = destY + sizeY * VOXEL_BLOCK_SAMPLES; destY != endY;
                    destY += VOXEL_BLOCK_SAMPLES, position.y += step) {
                position.x = info.minimum.x + minX * step;
                for (uchar* destX = destY, *endX = destX + sizeX; destX != endX; destX++, position.x += step) {
                    if (_spanner->contains(position)) { 
                        int material = _spanner->getMaterialAt(position);
                        if (material != 0) {
                            int& mapping = materialMap[material];
                            if (mapping == 0) {
                                mapping = getMaterialIndex(_spanner->getMaterials().at(material - 1), materials,
                                    materialContents);
                            }
                            material = mapping;
                        }
                        *destX = material;
                    }   
                }
            }
        }
    } else {
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
    }
    clearUnusedMaterials(materials, materialContents);
    VoxelMaterialDataPointer newMaterialPointer(new VoxelMaterialData(materialContents, VOXEL_BLOCK_SAMPLES, materials));
    info.outputValues[2] = AttributeValue(_inputs.at(2), encodeInline<VoxelMaterialDataPointer>(newMaterialPointer));
    
    return STOP_RECURSION;
}

class HeightfieldClearFetchVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldClearFetchVisitor(const Box& bounds, float granularity);

    const SharedObjectPointer& getSpanner() const { return _spanner; }

    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
    Box _expandedBounds;
    SharedObjectPointer _spanner;
    Box _spannerBounds;
    int _heightfieldWidth;
    int _heightfieldHeight;
};

HeightfieldClearFetchVisitor::HeightfieldClearFetchVisitor(const Box& bounds, float granularity) :
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
    _expandedBounds = _bounds;
    float increment = nodeSize / VOXEL_BLOCK_SIZE;
    _expandedBounds.maximum.x += increment;
    _expandedBounds.maximum.z += increment;
}

int HeightfieldClearFetchVisitor::visit(MetavoxelInfo& info) {
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
    QByteArray contents(heightPointer->getContents());
    int size = glm::sqrt((float)contents.size());
    float heightScale = size / info.size;
    
    Box overlap = bounds.getIntersection(_expandedBounds);
    int srcX = (overlap.minimum.x - info.minimum.x) * heightScale;
    int srcY = (overlap.minimum.z - info.minimum.z) * heightScale;
    int srcWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) * heightScale);
    int srcHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) * heightScale);
    char* src = contents.data() + srcY * size + srcX;
    
    // check for non-zero values
    bool foundNonZero = false;
    for (int y = 0; y < srcHeight; y++, src += (size - srcWidth)) {
        for (char* end = src + srcWidth; src != end; src++) {
            if (*src != 0) {
                foundNonZero = true;
                goto outerBreak;
            }
        }
    }
    outerBreak:
    
    // if everything is zero, we're done
    if (!foundNonZero) {
        return STOP_RECURSION;
    }
    
    // create spanner if necessary
    Heightfield* spanner = static_cast<Heightfield*>(_spanner.data());
    float increment = 1.0f / heightScale;
    if (!spanner) {    
        _spannerBounds.minimum = glm::floor(_bounds.minimum / increment) * increment;
        _spannerBounds.maximum = (glm::ceil(_bounds.maximum / increment) + glm::vec3(1.0f, 0.0f, 1.0f)) * increment;
        _spannerBounds.minimum.y = bounds.minimum.y;
        _spannerBounds.maximum.y = bounds.maximum.y;
        _heightfieldWidth = (int)glm::round((_spannerBounds.maximum.x - _spannerBounds.minimum.x) / increment);
        _heightfieldHeight = (int)glm::round((_spannerBounds.maximum.z - _spannerBounds.minimum.z) / increment);
        int heightfieldArea = _heightfieldWidth * _heightfieldHeight;
        Box innerBounds = _spannerBounds;
        innerBounds.maximum.x -= increment;
        innerBounds.maximum.z -= increment;
        _spanner = spanner = new Heightfield(innerBounds, increment, QByteArray(heightfieldArea, 0),
            QByteArray(heightfieldArea * DataBlock::COLOR_BYTES, 0), QByteArray(heightfieldArea, 0),
            QVector<SharedObjectPointer>());
    }
    
    // copy the inner area
    overlap = bounds.getIntersection(_spannerBounds);
    int destX = (overlap.minimum.x - _spannerBounds.minimum.x) * heightScale;
    int destY = (overlap.minimum.z - _spannerBounds.minimum.z) * heightScale;
    int destWidth = (int)glm::round((overlap.maximum.x - overlap.minimum.x) * heightScale);
    int destHeight = (int)glm::round((overlap.maximum.z - overlap.minimum.z) * heightScale);
    char* dest = spanner->getHeight().data() + destY * _heightfieldWidth + destX;
    srcX = (overlap.minimum.x - info.minimum.x) * heightScale;
    srcY = (overlap.minimum.z - info.minimum.z) * heightScale;
    src = contents.data() + srcY * size + srcX;
    
    for (int y = 0; y < destHeight; y++, dest += _heightfieldWidth, src += size) {
        memcpy(dest, src, destWidth);
    }
    
    // clear the inner area
    Box innerBounds = _spannerBounds;
    innerBounds.minimum.x += increment;
    innerBounds.minimum.z += increment;
    innerBounds.maximum.x -= increment;
    innerBounds.maximum.z -= increment;
    Box innerOverlap = bounds.getIntersection(innerBounds);
    destX = (innerOverlap.minimum.x - info.minimum.x) * heightScale;
    destY = (innerOverlap.minimum.z - info.minimum.z) * heightScale;
    destWidth = glm::ceil((innerOverlap.maximum.x - innerOverlap.minimum.x) * heightScale);
    destHeight = glm::ceil((innerOverlap.maximum.z - innerOverlap.minimum.z) * heightScale);
    dest = contents.data() + destY * size + destX;
    
    for (int y = 0; y < destHeight; y++, dest += size) {
        memset(dest, 0, destWidth);
    }
    
    // see if there are any non-zero values left
    foundNonZero = false;
    dest = contents.data();
    for (char* end = dest + contents.size(); dest != end; dest++) {
        if (*dest != 0) {
            foundNonZero = true;
            break;
        }
    }
    
    // if all is gone, clear the node
    if (foundNonZero) {
        HeightfieldHeightDataPointer newHeightPointer(new HeightfieldHeightData(contents));
        info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline<HeightfieldHeightDataPointer>(newHeightPointer));
        
    } else {
        info.outputValues[0] = AttributeValue(_outputs.at(0));
    }
    
    // allow a border for what we clear in terms of color/material
    innerBounds.minimum.x += increment;
    innerBounds.minimum.z += increment;
    innerBounds.maximum.x -= increment;
    innerBounds.maximum.z -= increment;
    innerOverlap = bounds.getIntersection(innerBounds);
    
    HeightfieldColorDataPointer colorPointer = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
    if (colorPointer) {
        contents = colorPointer->getContents();
        size = glm::sqrt((float)contents.size() / DataBlock::COLOR_BYTES);
        heightScale = size / info.size;
        
        // copy the inner area
        destX = (overlap.minimum.x - _spannerBounds.minimum.x) * heightScale;
        destY = (overlap.minimum.z - _spannerBounds.minimum.z) * heightScale;
        destWidth = (int)glm::round((overlap.maximum.x - overlap.minimum.x) * heightScale);
        destHeight = (int)glm::round((overlap.maximum.z - overlap.minimum.z) * heightScale);
        dest = spanner->getColor().data() + (destY * _heightfieldWidth + destX) * DataBlock::COLOR_BYTES;
        srcX = (overlap.minimum.x - info.minimum.x) * heightScale;
        srcY = (overlap.minimum.z - info.minimum.z) * heightScale;
        src = contents.data() + (srcY * size + srcX) * DataBlock::COLOR_BYTES;
        
        for (int y = 0; y < destHeight; y++, dest += _heightfieldWidth * DataBlock::COLOR_BYTES,
                src += size * DataBlock::COLOR_BYTES) {
            memcpy(dest, src, destWidth * DataBlock::COLOR_BYTES);
        }
        
        if (foundNonZero) {
            destX = (innerOverlap.minimum.x - info.minimum.x) * heightScale;
            destY = (innerOverlap.minimum.z - info.minimum.z) * heightScale;
            destWidth = glm::ceil((innerOverlap.maximum.x - innerOverlap.minimum.x) * heightScale);
            destHeight = glm::ceil((innerOverlap.maximum.z - innerOverlap.minimum.z) * heightScale);
            if (destWidth > 0 && destHeight > 0) {
                dest = contents.data() + (destY * size + destX) * DataBlock::COLOR_BYTES;
                
                for (int y = 0; y < destHeight; y++, dest += size * DataBlock::COLOR_BYTES) {
                    memset(dest, 0, destWidth * DataBlock::COLOR_BYTES);
                }
                
                HeightfieldColorDataPointer newColorPointer(new HeightfieldColorData(contents));
                info.outputValues[1] = AttributeValue(_outputs.at(1),
                    encodeInline<HeightfieldColorDataPointer>(newColorPointer));
            }
        } else {
            info.outputValues[1] = AttributeValue(_outputs.at(1));
        }
    }
    
    HeightfieldMaterialDataPointer materialPointer = info.inputValues.at(2).getInlineValue<HeightfieldMaterialDataPointer>();
    if (materialPointer) {
        contents = materialPointer->getContents();
        QVector<SharedObjectPointer> materials = materialPointer->getMaterials();
        size = glm::sqrt((float)contents.size());
        heightScale = size / info.size;
        
        // copy the inner area
        destX = (overlap.minimum.x - _spannerBounds.minimum.x) * heightScale;
        destY = (overlap.minimum.z - _spannerBounds.minimum.z) * heightScale;
        destWidth = (int)glm::round((overlap.maximum.x - overlap.minimum.x) * heightScale);
        destHeight = (int)glm::round((overlap.maximum.z - overlap.minimum.z) * heightScale);
        uchar* dest = (uchar*)spanner->getMaterial().data() + destY * _heightfieldWidth + destX;
        srcX = (overlap.minimum.x - info.minimum.x) * heightScale;
        srcY = (overlap.minimum.z - info.minimum.z) * heightScale;
        uchar* src = (uchar*)contents.data() + srcY * size + srcX;
        QHash<int, int> materialMap;
        
        for (int y = 0; y < destHeight; y++, dest += _heightfieldWidth, src += size) {
            for (uchar* lineSrc = src, *lineDest = dest, *end = src + destWidth; lineSrc != end; lineSrc++, lineDest++) {
                int material = *lineSrc;
                if (material != 0) {
                    int& mapping = materialMap[material];
                    if (mapping == 0) {
                        mapping = getMaterialIndex(materials.at(material - 1), spanner->getMaterials(),
                            spanner->getMaterial());
                    }
                    material = mapping;
                }
                *lineDest = material;
            }
        }
        
        if (foundNonZero) {
            destX = (innerOverlap.minimum.x - info.minimum.x) * heightScale;
            destY = (innerOverlap.minimum.z - info.minimum.z) * heightScale;
            destWidth = glm::ceil((innerOverlap.maximum.x - innerOverlap.minimum.x) * heightScale);
            destHeight = glm::ceil((innerOverlap.maximum.z - innerOverlap.minimum.z) * heightScale);
            if (destWidth > 0 && destHeight > 0) {
                dest = (uchar*)contents.data() + destY * size + destX;
                
                for (int y = 0; y < destHeight; y++, dest += size) {
                    memset(dest, 0, destWidth);
                }
                
                clearUnusedMaterials(materials, contents);
                HeightfieldMaterialDataPointer newMaterialPointer(new HeightfieldMaterialData(contents, materials));
                info.outputValues[2] = AttributeValue(_outputs.at(2),
                    encodeInline<HeightfieldMaterialDataPointer>(newMaterialPointer));
            }
        } else {
            info.outputValues[2] = AttributeValue(_outputs.at(2));
        }
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
    
    // clear/fetch any heightfield data
    HeightfieldClearFetchVisitor heightfieldVisitor(spanner->getBounds(), spanner->getVoxelizationGranularity());
    data.guide(heightfieldVisitor);
    
    // voxelize the fetched heightfield, if any
    if (heightfieldVisitor.getSpanner()) {
        VoxelMaterialSpannerEditVisitor visitor(static_cast<Spanner*>(heightfieldVisitor.getSpanner().data()),
            material, color);
        data.guide(visitor);
    }
    
    VoxelMaterialSpannerEditVisitor visitor(spanner, material, color);
    data.guide(visitor);
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
