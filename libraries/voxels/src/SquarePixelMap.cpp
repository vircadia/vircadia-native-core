//
//  SquarePixelMap.cpp
//  hifi
//
//  Created by Tomáš Horáček on 6/25/13.
//
//

#include "SquarePixelMap.h"
#include <string.h>
#include <stdlib.h>


unsigned int numberOfBitsForSize(unsigned int size) {
    if (size == 0) {
        return 0;
    }
    
    size--;
    
    unsigned int ans = 1;
    while (size >>= 1) {
        ans++;
    }
    return ans;
}

struct PixelQuadTreeCoordinates {
    unsigned int x;
    unsigned int y;
    unsigned int size;
};

class PixelQuadTreeNode {
public:
    PixelQuadTreeCoordinates _coord;
    uint32_t _color; // undefined value for _allChildrenHasSameColor = false
    bool _allChildrenHasSameColor;
    
    // 0    x ->  1
    //   +---+---+
    // y | 0 | 1 | <- child index
    // | +---+---+
    // v | 2 | 3 |
    //   +---+---+
    // 1
    PixelQuadTreeNode *_children[4];
    
    PixelQuadTreeNode(PixelQuadTreeCoordinates coord, SquarePixelMap *pixelMap);
    ~PixelQuadTreeNode() {
        for (int i = 0; i < 4; i++) {
            delete _children[i];
        }
    }

private:
    void updateChildCoordinates(int i, PixelQuadTreeCoordinates &childCoord) {
        childCoord.x = _coord.x;
        childCoord.y = _coord.y;
        
        if (i & 0x1) {
            childCoord.x += childCoord.size;
        }
        if (i & 0x2) {
            childCoord.y += childCoord.size;
        }
    }
    
    bool hasAllChildrenSameColor() {
        for (int i = 1; i < 4; i++) {
            if (!_children[i]->_allChildrenHasSameColor) {
                return false;
            }
        }
        
        uint32_t firstColor = _children[0]->_color;
        
        for (int i = 1; i < 4; i++) {
            if (firstColor != _children[i]->_color) {
                return false;
            }
        }
        return true;
    }
};

PixelQuadTreeNode::PixelQuadTreeNode(PixelQuadTreeCoordinates coord, SquarePixelMap *pixelMap) : _coord(coord) {
    for (int i = 0; i < 4; i++) {
        _children[i] = NULL;
    }
    
    if (_coord.size == 1) {
        _color = pixelMap->getPixelAt(_coord.x, _coord.y);
        _allChildrenHasSameColor = true;
    } else {
        PixelQuadTreeCoordinates childCoord = PixelQuadTreeCoordinates();
        childCoord.size = _coord.size / 2;
        
        for (int i = 0; i < 4; i++) {
            this->updateChildCoordinates(i, childCoord);
            
            
            if (childCoord.x < pixelMap->dimension() &&
                childCoord.y < pixelMap->dimension()) {
                
                _children[i] = new PixelQuadTreeNode(childCoord, pixelMap);
            }
        }
        
        if (this->hasAllChildrenSameColor()) {
            _allChildrenHasSameColor = true;
            _color = _children[0]->_color;
            
            for (int i = 0; i < 4; i++) {
                delete _children[i];
                _children[i] = NULL;
            }
        } else {
            _allChildrenHasSameColor = false;
        }
    }
}

SquarePixelMap::SquarePixelMap(const uint32_t *pixels, int dimension) : _rootPixelQuadTreeNode(NULL) {
    _data = new SquarePixelMapData();
    _data->dimension = dimension;
    _data->reference_counter = 1;
    
    size_t pixels_size = dimension * dimension;
    _data->pixels = new uint32_t[pixels_size];
    memcpy((void *)_data->pixels, (void *)pixels, sizeof(uint32_t) * pixels_size);
}

SquarePixelMap::SquarePixelMap(const SquarePixelMap& other) {
    this->_data = other._data;
    this->_data->reference_counter++;
}

SquarePixelMap::~SquarePixelMap() {
    delete _rootPixelQuadTreeNode;
    
    if (--_data->reference_counter == 0) {
        delete _data->pixels;
        delete _data;
    }
}

void SquarePixelMap::addVoxelsToVoxelTree(VoxelTree *voxelTree) {
    this->generateRootPixelQuadTreeNode();
    this->createVoxelsFromPixelQuadTreeToVoxelTree(_rootPixelQuadTreeNode, voxelTree);
}

int SquarePixelMap::dimension() {
    return _data->dimension;
}
                
uint32_t SquarePixelMap::getPixelAt(unsigned int x, unsigned int y) {
    return _data->pixels[x + y * _data->dimension];
}

void SquarePixelMap::generateRootPixelQuadTreeNode() {
    delete _rootPixelQuadTreeNode;

    PixelQuadTreeCoordinates rootNodeCoord = PixelQuadTreeCoordinates();
    rootNodeCoord.size = 1 << numberOfBitsForSize(_data->dimension);
    rootNodeCoord.x = rootNodeCoord.y = 0;
    
    _rootPixelQuadTreeNode = new PixelQuadTreeNode(rootNodeCoord, this);
}

void SquarePixelMap::createVoxelsFromPixelQuadTreeToVoxelTree(PixelQuadTreeNode *pixelQuadTreeNode, VoxelTree *voxelTree) {
    if (pixelQuadTreeNode->_allChildrenHasSameColor) {
        VoxelDetail voxel = this->getVoxelDetail(pixelQuadTreeNode);
        
        voxelTree->createVoxel(voxel.x, voxel.y, voxel.z, voxel.s, voxel.red, voxel.green, voxel.blue, true);
    } else {
        for (int i = 0; i < 4; i++) {
            PixelQuadTreeNode *child = pixelQuadTreeNode->_children[i];
            if (child) {
                this->createVoxelsFromPixelQuadTreeToVoxelTree(child, voxelTree);
            }
        }
    }
}

VoxelDetail SquarePixelMap::getVoxelDetail(PixelQuadTreeNode *pixelQuadTreeNode) {
    VoxelDetail voxel = VoxelDetail();
    
    uint32_t color = pixelQuadTreeNode->_color;
    unsigned char alpha = color >> 24;
    
    voxel.red = color >> 16;
    voxel.green = color >> 8;
    voxel.blue = color;
    
    
    float rootSize = _rootPixelQuadTreeNode->_coord.size;
    
    voxel.s = pixelQuadTreeNode->_coord.size / rootSize;
    voxel.y = voxel.s * (floor(alpha / (256.f * voxel.s)) + 0.5);
    voxel.x = pixelQuadTreeNode->_coord.x / rootSize + voxel.s / 2;
    voxel.z = pixelQuadTreeNode->_coord.y / rootSize + voxel.s / 2;
    
    return voxel;
}
