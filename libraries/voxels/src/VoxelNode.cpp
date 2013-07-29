//
//  VoxelNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cmath>
#include <cstring>
#include <stdio.h>

#include <QDebug>

#include "AABox.h"
#include "OctalCode.h"
#include "SharedUtil.h"
#include "VoxelConstants.h"
#include "VoxelNode.h"
#include "VoxelTree.h"

VoxelNode::VoxelNode() {
    unsigned char* rootCode = new unsigned char[1];
    *rootCode = 0;
    init(rootCode);
}

VoxelNode::VoxelNode(unsigned char * octalCode) {
    init(octalCode);
}

void VoxelNode::init(unsigned char * octalCode) {
    _octalCode = octalCode;
    
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
    _falseColored = false; // assume true color
    _currentColor[0] = _currentColor[1] = _currentColor[2] = _currentColor[3] = 0;
#endif
    _trueColor[0] = _trueColor[1] = _trueColor[2] = _trueColor[3] = 0;
    _density = 0.0f;
    
    // default pointers to child nodes to NULL
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        _children[i] = NULL;
    }
    _childCount = 0;
    _subtreeNodeCount = 1; // that's me
    _subtreeLeafNodeCount = 0; // that's me
    
    _glBufferIndex = GLBUFFER_INDEX_UNKNOWN;
    _voxelSystem = NULL;
    _isDirty = true;
    _shouldRender = false;
    markWithChangedTime();
    calculateAABox();
}

VoxelNode::~VoxelNode() {
    notifyDeleteHooks();

    delete[] _octalCode;
    
    // delete all of this node's children
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_children[i]) {
            delete _children[i];
        }
    }
}

// This method is called by VoxelTree when the subtree below this node
// is known to have changed. It's intended to be used as a place to do
// bookkeeping that a node may need to do when the subtree below it has
// changed. However, you should hopefully make your bookkeeping relatively
// localized, because this method will get called for every node in an
// recursive unwinding case like delete or add voxel
void VoxelNode::handleSubtreeChanged(VoxelTree* myTree) {
    markWithChangedTime();
    
    // here's a good place to do color re-averaging...
    if (myTree->getShouldReaverage()) {
        setColorFromAverageOfChildren();
    }
    
    recalculateSubTreeNodeCount();
}

void VoxelNode::recalculateSubTreeNodeCount() {
    // Assuming the tree below me as changed, I need to recalculate my node count
    _subtreeNodeCount = 1; // that's me
    if (isLeaf()) {
        _subtreeLeafNodeCount = 1;
    } else {
        _subtreeLeafNodeCount = 0;
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            if (_children[i]) {
                _subtreeNodeCount += _children[i]->_subtreeNodeCount;
                _subtreeLeafNodeCount += _children[i]->_subtreeLeafNodeCount;
            }
        }
    }
}


void VoxelNode::setShouldRender(bool shouldRender) {
    // if shouldRender is changing, then consider ourselves dirty
    if (shouldRender != _shouldRender) {
        _shouldRender = shouldRender;
        _isDirty = true;
        markWithChangedTime();
    }
}

void VoxelNode::calculateAABox() {
    
    glm::vec3 corner;
    glm::vec3 size;
    
    // copy corner into box
    copyFirstVertexForCode(_octalCode,(float*)&corner);
    
    // this tells you the "size" of the voxel
    float voxelScale = 1 / powf(2, *_octalCode);
    size = glm::vec3(voxelScale,voxelScale,voxelScale);
    
    _box.setBox(corner,size);
}

void VoxelNode::deleteChildAtIndex(int childIndex) {
    if (_children[childIndex]) {
        delete _children[childIndex];
        _children[childIndex] = NULL;
        _isDirty = true;
        markWithChangedTime();
        _childCount--;
    }
}

// does not delete the node!
VoxelNode* VoxelNode::removeChildAtIndex(int childIndex) {
    VoxelNode* returnedChild = _children[childIndex];
    if (_children[childIndex]) {
        _children[childIndex] = NULL;
        _isDirty = true;
        markWithChangedTime();
        _childCount--;
    }
    return returnedChild;
}

VoxelNode* VoxelNode::addChildAtIndex(int childIndex) {
    if (!_children[childIndex]) {
        _children[childIndex] = new VoxelNode(childOctalCode(_octalCode, childIndex));
        _isDirty = true;
        markWithChangedTime();
        _childCount++;
    }
    return _children[childIndex];
}

// handles staging or deletion of all deep children
void VoxelNode::safeDeepDeleteChildAtIndex(int childIndex) {
    VoxelNode* childToDelete = getChildAtIndex(childIndex);
    if (childToDelete) {
        // If the child is not a leaf, then call ourselves recursively on all the children
        if (!childToDelete->isLeaf()) {
            // delete all it's children
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                childToDelete->safeDeepDeleteChildAtIndex(i);
            }
        }
        deleteChildAtIndex(childIndex);
        _isDirty = true;
        markWithChangedTime();
    }
}

// will average the child colors...
void VoxelNode::setColorFromAverageOfChildren() {
    int colorArray[4] = {0,0,0,0};
    float density = 0.0f;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_children[i] && _children[i]->isColored()) {
            for (int j = 0; j < 3; j++) {
                colorArray[j] += _children[i]->getTrueColor()[j]; // color averaging should always be based on true colors
            }
            colorArray[3]++;
        }
        if (_children[i]) {
            density += _children[i]->getDensity();
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

// Note: !NO_FALSE_COLOR implementations of setFalseColor(), setFalseColored(), and setColor() here.
//       the actual NO_FALSE_COLOR version are inline in the VoxelNode.h
#ifndef NO_FALSE_COLOR // !NO_FALSE_COLOR means, does have false color
void VoxelNode::setFalseColor(colorPart red, colorPart green, colorPart blue) {
    if (_falseColored != true || _currentColor[0] != red || _currentColor[1] != green || _currentColor[2] != blue) {
        _falseColored=true;
        _currentColor[0] = red;
        _currentColor[1] = green;
        _currentColor[2] = blue;
        _currentColor[3] = 1; // XXXBHG - False colors are always considered set
        _isDirty = true;
        markWithChangedTime();
    }
}

void VoxelNode::setFalseColored(bool isFalseColored) {
    if (_falseColored != isFalseColored) {
        // if we were false colored, and are no longer false colored, then swap back
        if (_falseColored && !isFalseColored) {
            memcpy(&_currentColor,&_trueColor,sizeof(nodeColor));
        }
        _falseColored = isFalseColored; 
        _isDirty = true;
        markWithChangedTime();
        _density = 1.0f;       //   If color set, assume leaf, re-averaging will update density if needed.

    }
};


void VoxelNode::setColor(const nodeColor& color) {
    if (_trueColor[0] != color[0] || _trueColor[1] != color[1] || _trueColor[2] != color[2]) {
        memcpy(&_trueColor,&color,sizeof(nodeColor));
        if (!_falseColored) {
            memcpy(&_currentColor,&color,sizeof(nodeColor));
        }
        _isDirty = true;
        markWithChangedTime();
        _density = 1.0f;       //   If color set, assume leaf, re-averaging will update density if needed.
    }
}
#endif

// will detect if children are leaves AND the same color
// and in that case will delete the children and make this node
// a leaf, returns TRUE if all the leaves are collapsed into a 
// single node
bool VoxelNode::collapseIdenticalLeaves() {
    // scan children, verify that they are ALL present and accounted for
    bool allChildrenMatch = true; // assume the best (ottimista)
    int red,green,blue;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        // if no child, child isn't a leaf, or child doesn't have a color
        if (!_children[i] || !_children[i]->isLeaf() || !_children[i]->isColored()) {
            allChildrenMatch=false;
            //qDebug("SADNESS child missing or not colored! i=%d\n",i);
            break;
        } else {
            if (i==0) {
                red   = _children[i]->getColor()[0];
                green = _children[i]->getColor()[1];
                blue  = _children[i]->getColor()[2];
            } else if (red != _children[i]->getColor()[0] || 
                    green != _children[i]->getColor()[1] || blue != _children[i]->getColor()[2]) {
                allChildrenMatch=false;
                break;
            }
        }
    }
    
    
    if (allChildrenMatch) {
        //qDebug("allChildrenMatch: pruning tree\n");
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            delete _children[i]; // delete all the child nodes
            _children[i]=NULL; // set it to NULL
        }
        _childCount = 0;
        nodeColor collapsedColor;
        collapsedColor[0]=red;        
        collapsedColor[1]=green;        
        collapsedColor[2]=blue;        
        collapsedColor[3]=1;    // color is set
        setColor(collapsedColor);
    }
    return allChildrenMatch;
}

void VoxelNode::setRandomColor(int minimumBrightness) {
    nodeColor newColor;
    for (int c = 0; c < 3; c++) {
        newColor[c] = randomColorValue(minimumBrightness);
    }
    
    newColor[3] = 1;
    setColor(newColor);
}

void VoxelNode::printDebugDetails(const char* label) const {
    unsigned char childBits = 0;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_children[i]) {
            setAtBit(childBits,i);            
        }
    }

    qDebug("%s - Voxel at corner=(%f,%f,%f) size=%f\n isLeaf=%s isColored=%s (%d,%d,%d,%d) isDirty=%s shouldRender=%s\n children=", label,
        _box.getCorner().x, _box.getCorner().y, _box.getCorner().z, _box.getSize().x,
        debug::valueOf(isLeaf()), debug::valueOf(isColored()), getColor()[0], getColor()[1], getColor()[2], getColor()[3],
        debug::valueOf(isDirty()), debug::valueOf(getShouldRender()));
        
    outputBits(childBits, false);
    qDebug("\n octalCode=");
    printOctalCode(_octalCode);
}

float VoxelNode::getEnclosingRadius() const {
    return getScale() * sqrtf(3.0f) / 2.0f;
}

bool VoxelNode::isInView(const ViewFrustum& viewFrustum) const {
    AABox box = _box; // use temporary box so we can scale it
    box.scale(TREE_SCALE);
    bool inView = (ViewFrustum::OUTSIDE != viewFrustum.boxInFrustum(box));
    return inView;
}

ViewFrustum::location VoxelNode::inFrustum(const ViewFrustum& viewFrustum) const {
    AABox box = _box; // use temporary box so we can scale it
    box.scale(TREE_SCALE);
    return viewFrustum.boxInFrustum(box);
}

// There are two types of nodes for which we want to "render"
// 1) Leaves that are in the LOD
// 2) Non-leaves are more complicated though... usually you don't want to render them, but if their children
//    wouldn't be rendered, then you do want to render them. But sometimes they have some children that ARE 
//    in the LOD, and others that are not. In this case we want to render the parent, and none of the children.
//
//    Since, if we know the camera position and orientation, we can know which of the corners is the "furthest" 
//    corner. We can use we can use this corner as our "voxel position" to do our distance calculations off of.
//    By doing this, we don't need to test each child voxel's position vs the LOD boundary
bool VoxelNode::calculateShouldRender(const ViewFrustum* viewFrustum, int boundaryLevelAdjust) const {
    bool shouldRender = false;
    if (isColored()) {
        float furthestDistance = furthestDistanceToCamera(*viewFrustum);
        float boundary         = boundaryDistanceForRenderLevel(getLevel() + boundaryLevelAdjust);
        float childBoundary    = boundaryDistanceForRenderLevel(getLevel() + 1 + boundaryLevelAdjust);
        bool  inBoundary       = (furthestDistance <= boundary);
        bool  inChildBoundary  = (furthestDistance <= childBoundary);
        shouldRender = (isLeaf() && inChildBoundary) || (inBoundary && !inChildBoundary);
    }
    return shouldRender;
}

// Calculates the distance to the furthest point of the voxel to the camera
float VoxelNode::furthestDistanceToCamera(const ViewFrustum& viewFrustum) const {
    AABox box = getAABox();
    box.scale(TREE_SCALE);
    glm::vec3 furthestPoint = viewFrustum.getFurthestPointFromCamera(box);
    glm::vec3 temp = viewFrustum.getPosition() - furthestPoint;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float VoxelNode::distanceToCamera(const ViewFrustum& viewFrustum) const {
    glm::vec3 center = _box.getCenter() * (float)TREE_SCALE;
    glm::vec3 temp = viewFrustum.getPosition() - center;
    float distanceToVoxelCenter = sqrtf(glm::dot(temp, temp));
    return distanceToVoxelCenter;
}

float VoxelNode::distanceSquareToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _box.getCenter();
    float distanceSquare = glm::dot(temp, temp);
    return distanceSquare;
}

float VoxelNode::distanceToPoint(const glm::vec3& point) const {
    glm::vec3 temp = point - _box.getCenter();
    float distance = sqrtf(glm::dot(temp, temp));
    return distance;
}

std::vector<VoxelNodeDeleteHook*> VoxelNode::_hooks;

void VoxelNode::addDeleteHook(VoxelNodeDeleteHook* hook) {
    _hooks.push_back(hook);
}

void VoxelNode::removeDeleteHook(VoxelNodeDeleteHook* hook) {
    for (int i = 0; i < _hooks.size(); i++) {
        if (_hooks[i] == hook) {
            _hooks.erase(_hooks.begin() + i);
            return;
        }
    }
}

void VoxelNode::notifyDeleteHooks() {
    for (int i = 0; i < _hooks.size(); i++) {
        _hooks[i]->nodeDeleted(this);
    }
}
