//
//  VoxelTreeElement.cpp
//  libraries/voxels/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <NodeList.h>
#include <PerfStat.h>

#include "VoxelConstants.h"
#include "VoxelTreeElement.h"
#include "VoxelTree.h"

VoxelTreeElement::VoxelTreeElement(unsigned char* octalCode) : 
    OctreeElement(),
    _exteriorOcclusions(OctreeElement::HalfSpace::All),
    _interiorOcclusions(OctreeElement::HalfSpace::None)
{
    init(octalCode);
};

VoxelTreeElement::~VoxelTreeElement() {
    _voxelMemoryUsage -= sizeof(VoxelTreeElement);
}

// This will be called primarily on addChildAt(), which means we're adding a child of our
// own type to our own tree. This means we should initialize that child with any tree and type
// specific settings that our children must have. One example is out VoxelSystem, which
// we know must match ours.
OctreeElement* VoxelTreeElement::createNewElement(unsigned char* octalCode) {
    VoxelTreeElement* newChild = new VoxelTreeElement(octalCode);
    newChild->setVoxelSystem(getVoxelSystem()); // our child is always part of our voxel system NULL ok
    return newChild;
}

void VoxelTreeElement::init(unsigned char* octalCode) {
     setVoxelSystem(NULL);
     setBufferIndex(GLBUFFER_INDEX_UNKNOWN);
    _falseColored = false; // assume true color
    _color[0] = _color[1] = _color[2] = _color[3] = 0;
    _density = 0.0f;
    OctreeElement::init(octalCode);
    _voxelMemoryUsage += sizeof(VoxelTreeElement);
}

bool VoxelTreeElement::requiresSplit() const {
    return isLeaf() && isColored();
}

void VoxelTreeElement::splitChildren() {
    if (requiresSplit()) {
        const nodeColor& ourColor = getColor();

        // for colored leaves, we must add *all* the children
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            addChildAtIndex(i)->setColor(ourColor);
        }
        nodeColor noColor = { 0, 0, 0, 0};
        setColor(noColor); // set our own color to noColor so we are a pure non-leaf
    }
}

OctreeElement::AppendState VoxelTreeElement::appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const {

    OctreeElement::AppendState appendState = packetData->appendColor(getColor()) ? OctreeElement::COMPLETED : OctreeElement::NONE;
    
    /*
    if (appendState == OctreeElement::COMPLETED) {
        qDebug() << "WROTE color for element " << getAACube() << " color: " 
                        << getColor()[0] << ", "
                        << getColor()[1] << ", "
                        << getColor()[2];
    } else {
        qDebug() << "COULD NOT WRITE color for element " << getAACube();
    }
    */
    return appendState;
}


int VoxelTreeElement::readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
            ReadBitstreamToTreeParams& args) {
    const int BYTES_PER_COLOR = 3;
    
    if (bytesLeftToRead < BYTES_PER_COLOR) {
        qDebug() << "UNEXPECTED: readElementDataFromBuffer() only had " << bytesLeftToRead << " bytes. "
                        "Not enough for meaningful data.";
        return bytesLeftToRead;
    }
    

    // pull the color for this child
    nodeColor newColor = { 128, 128, 128, 1};
    if (args.includeColor) {
        memcpy(newColor, data, BYTES_PER_COLOR);
    }
    setColor(newColor);
    return BYTES_PER_COLOR;
}


const uint8_t INDEX_FOR_NULL = 0;
uint8_t VoxelTreeElement::_nextIndex = INDEX_FOR_NULL + 1; // start at 1, 0 is reserved for NULL
std::map<VoxelSystem*, uint8_t> VoxelTreeElement::_mapVoxelSystemPointersToIndex;
std::map<uint8_t, VoxelSystem*> VoxelTreeElement::_mapIndexToVoxelSystemPointers;

VoxelSystem* VoxelTreeElement::getVoxelSystem() const {
    if (_voxelSystemIndex > INDEX_FOR_NULL) {
        if (_mapIndexToVoxelSystemPointers.end() != _mapIndexToVoxelSystemPointers.find(_voxelSystemIndex)) {

            VoxelSystem* voxelSystem = _mapIndexToVoxelSystemPointers[_voxelSystemIndex];
            return voxelSystem;
        }
    }
    return NULL;
}

void VoxelTreeElement::setVoxelSystem(VoxelSystem* voxelSystem) {
    if (!voxelSystem) {
        _voxelSystemIndex = INDEX_FOR_NULL;
    } else {
        uint8_t index;
        if (_mapVoxelSystemPointersToIndex.end() != _mapVoxelSystemPointersToIndex.find(voxelSystem)) {
            index = _mapVoxelSystemPointersToIndex[voxelSystem];
        } else {
            index = _nextIndex;
            _nextIndex++;
            _mapVoxelSystemPointersToIndex[voxelSystem] = index;
            _mapIndexToVoxelSystemPointers[index] = voxelSystem;
        }
        _voxelSystemIndex = index;
    }
}

void VoxelTreeElement::setColor(const nodeColor& color) {
    if (_color[0] != color[0] || _color[1] != color[1] || _color[2] != color[2]) {
        memcpy(&_color,&color,sizeof(nodeColor));
        _isDirty = true;
        if (color[3]) {
            _density = 1.0f; // If color set, assume leaf, re-averaging will update density if needed.
        } else {
            _density = 0.0f;
        }
        markWithChangedTime();
    }
}



// will average the child colors...
void VoxelTreeElement::calculateAverageFromChildren() {
    int colorArray[4] = {0,0,0,0};
    float density = 0.0f;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelTreeElement* childAt = getChildAtIndex(i);
        if (childAt && childAt->isColored()) {
            for (int j = 0; j < 3; j++) {
                colorArray[j] += childAt->getColor()[j]; // color averaging should always be based on true colors
            }
            colorArray[3]++;
        }
        if (childAt) {
            density += childAt->getDensity();
        }
    }
    density /= (float) NUMBER_OF_CHILDREN;
    //
    //  The VISIBLE_ABOVE_DENSITY sets the density of matter above which an averaged color voxel will
    //  be set.  It is an important physical constant in our universe.  A number below 0.5 will cause
    //  things to get 'fatter' at a distance, because upward averaging will make larger voxels out of
    //  less data, which is (probably) going to be preferable because it gives a sense that there is
    //  something out there to go investigate.   A number above 0.5 would cause the world to become
    //  more 'empty' at a distance.  Exactly 0.5 would match the physical world, at least for materials
    //  that are not shiny and have equivalent ambient reflectance.
    //
    const float VISIBLE_ABOVE_DENSITY = 0.10f;
    nodeColor newColor = { 0, 0, 0, 0};
    if (density > VISIBLE_ABOVE_DENSITY) {
        // The density of material in the space of the voxel sets whether it is actually colored
        for (int c = 0; c < 3; c++) {
            // set the average color value
            newColor[c] = colorArray[c] / colorArray[3];
        }
        // set the alpha to 1 to indicate that this isn't transparent
        newColor[3] = 1;
    }
    //  Set the color from the average of the child colors, and update the density
    setColor(newColor);
    setDensity(density);
}

// will detect if children are leaves AND the same color
// and in that case will delete the children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a
// single node
bool VoxelTreeElement::collapseChildren() {
    // scan children, verify that they are ALL present and accounted for
    bool allChildrenMatch = true; // assume the best (ottimista)
    int red = 0;
    int green = 0;
    int blue = 0;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        VoxelTreeElement* childAt = getChildAtIndex(i);
        // if no child, child isn't a leaf, or child doesn't have a color
        if (!childAt || !childAt->isLeaf() || !childAt->isColored()) {
            allChildrenMatch = false;
            break;
        } else {
            if (i==0) {
                red   = childAt->getColor()[0];
                green = childAt->getColor()[1];
                blue  = childAt->getColor()[2];
            } else if (red != childAt->getColor()[0] ||
                    green != childAt->getColor()[1] || blue != childAt->getColor()[2]) {
                allChildrenMatch=false;
                break;
            }
        }
    }


    if (allChildrenMatch) {
        //qDebug("allChildrenMatch: pruning tree\n");
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            OctreeElement* childAt = getChildAtIndex(i);
            delete childAt; // delete all the child nodes
            setChildAtIndex(i, NULL); // set it to NULL
        }
        nodeColor collapsedColor;
        collapsedColor[0]=red;
        collapsedColor[1]=green;
        collapsedColor[2]=blue;
        collapsedColor[3]=1;    // color is set
        setColor(collapsedColor);
    }
    return allChildrenMatch;
}


bool VoxelTreeElement::findSpherePenetration(const glm::vec3& center, float radius,
                                    glm::vec3& penetration, void** penetratedObject) const {
    if (_cube.findSpherePenetration(center, radius, penetration)) {

        // if the caller wants details about the voxel, then return them here...
        if (penetratedObject) {
            VoxelDetail* voxelDetails = new VoxelDetail;
            voxelDetails->x = _cube.getCorner().x;
            voxelDetails->y = _cube.getCorner().y;
            voxelDetails->z = _cube.getCorner().z;
            voxelDetails->s = _cube.getScale();
            voxelDetails->red = getColor()[RED_INDEX];
            voxelDetails->green = getColor()[GREEN_INDEX];
            voxelDetails->blue = getColor()[BLUE_INDEX];

            *penetratedObject = (void*)voxelDetails;
        }
        return true;
    }
    return false;
}

