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

PaintHeightfieldHeightEdit::PaintHeightfieldHeightEdit(const glm::vec3& position, float radius, float height) :
    position(position),
    radius(radius),
    height(height) {
}

void PaintHeightfieldHeightEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // increase the extents slightly to include neighboring tiles
    const float RADIUS_EXTENSION = 1.1f;
    glm::vec3 extents = glm::vec3(radius, radius, radius) * RADIUS_EXTENSION;
    QVector<SharedObjectPointer> results;
    data.getIntersecting(AttributeRegistry::getInstance()->getSpannersAttribute(),
        Box(position - extents, position + extents), results);
    
    foreach (const SharedObjectPointer& spanner, results) {
        Spanner* newSpanner = static_cast<Spanner*>(spanner.data())->paintHeight(position, radius, height);
        if (newSpanner != spanner) {
            data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), spanner, newSpanner);
        }
    }
}

MaterialEdit::MaterialEdit(const SharedObjectPointer& material, const QColor& averageColor) :
    material(material),
    averageColor(averageColor) {
}

PaintHeightfieldMaterialEdit::PaintHeightfieldMaterialEdit(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& averageColor) :
    MaterialEdit(material, averageColor),
    position(position),
    radius(radius) {
}

void PaintHeightfieldMaterialEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    glm::vec3 extents(radius, radius, radius);
    QVector<SharedObjectPointer> results;
    data.getIntersecting(AttributeRegistry::getInstance()->getSpannersAttribute(),
        Box(position - extents, position + extents), results);
    
    foreach (const SharedObjectPointer& spanner, results) {
        Spanner* newSpanner = static_cast<Spanner*>(spanner.data())->paintMaterial(position, radius, material, averageColor);
        if (newSpanner != spanner) {
            data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), spanner, newSpanner);
        }
    }
}

const int VOXEL_BLOCK_SIZE = 16;
const int VOXEL_BLOCK_SAMPLES = VOXEL_BLOCK_SIZE + 1;
const int VOXEL_BLOCK_AREA = VOXEL_BLOCK_SAMPLES * VOXEL_BLOCK_SAMPLES;
const int VOXEL_BLOCK_VOLUME = VOXEL_BLOCK_AREA * VOXEL_BLOCK_SAMPLES;

HeightfieldMaterialSpannerEdit::HeightfieldMaterialSpannerEdit(const SharedObjectPointer& spanner,
        const SharedObjectPointer& material, const QColor& averageColor) :
    MaterialEdit(material, averageColor),
    spanner(spanner) {
}

class HeightfieldMaterialSpannerEditVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldMaterialSpannerEditVisitor(Spanner* spanner, const SharedObjectPointer& material, const QColor& color);

    virtual int visit(MetavoxelInfo& info);

private:
    
    Spanner* _spanner;
    SharedObjectPointer _material;
    QColor _color;
    float _blockSize;
};

HeightfieldMaterialSpannerEditVisitor::HeightfieldMaterialSpannerEditVisitor(Spanner* spanner,
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

int HeightfieldMaterialSpannerEditVisitor::visit(MetavoxelInfo& info) {
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
    const int EIGHT_BIT_MAXIMUM = 255;
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

void HeightfieldMaterialSpannerEdit::apply(MetavoxelData& data, const WeakSharedObjectHash& objects) const {
    // make sure the color is either 100% transparent or 100% opaque
    QColor color = averageColor;
    color.setAlphaF(color.alphaF() > 0.5f ? 1.0f : 0.0f);
    
    QVector<SharedObjectPointer> results;
    data.getIntersecting(AttributeRegistry::getInstance()->getSpannersAttribute(),
        static_cast<Spanner*>(spanner.data())->getBounds(), results);
    
    foreach (const SharedObjectPointer& result, results) {
        Spanner* newResult = static_cast<Spanner*>(result.data())->setMaterial(spanner, material, color);
        if (newResult != result) {
            data.replace(AttributeRegistry::getInstance()->getSpannersAttribute(), result, newResult);
        }
    }
}

