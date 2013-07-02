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

#define CHILD_COORD_X_IS_1 0x1
#define CHILD_COORD_Y_IS_1 0x2
#define ALPHA_CHANNEL_RANGE_FLOAT 256.f
#define ALPHA_CHANNEL_BIT_OFFSET 24
#define RED_CHANNEL_BIT_OFFSET 16
#define GREEN_CHANNEL_BIT_OFFSET 8

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
    uint8_t _minimumNeighbourhoodAplha;
    
    // 0    x ->  1
    //   +---+---+
    // y | 0 | 1 | <- child index
    // | +---+---+
    // v | 2 | 3 |
    //   +---+---+
    // 1
    PixelQuadTreeNode* _children[4];
    
    PixelQuadTreeNode(PixelQuadTreeCoordinates coord, SquarePixelMap* pixelMap);
    ~PixelQuadTreeNode() {
        for (int i = 0; i < 4; i++) {
            delete _children[i];
        }
    }

private:
    void updateChildCoordinates(int i, PixelQuadTreeCoordinates& childCoord) {
        childCoord.x = _coord.x;
        childCoord.y = _coord.y;
        
        if (i & CHILD_COORD_X_IS_1) {
            childCoord.x += childCoord.size;
        }
        if (i & CHILD_COORD_Y_IS_1) {
            childCoord.y += childCoord.size;
        }
    }
    
    bool hasAllChildrenSameColor() {
        return false; //turn off import voxel grouping
        
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

PixelQuadTreeNode::PixelQuadTreeNode(PixelQuadTreeCoordinates coord, SquarePixelMap* pixelMap) : _coord(coord), _minimumNeighbourhoodAplha(-1) {
    for (int i = 0; i < 4; i++) {
        _children[i] = NULL;
    }
    
    if (_coord.size == 1) {
        _color = pixelMap->getPixelAt(_coord.x, _coord.y);
        
        _minimumNeighbourhoodAplha = std::min<uint8_t>(pixelMap->getAlphaAt(_coord.x + 1, _coord.y), _minimumNeighbourhoodAplha);
        _minimumNeighbourhoodAplha = std::min<uint8_t>(pixelMap->getAlphaAt(_coord.x - 1, _coord.y), _minimumNeighbourhoodAplha);
        _minimumNeighbourhoodAplha = std::min<uint8_t>(pixelMap->getAlphaAt(_coord.x, _coord.y + 1), _minimumNeighbourhoodAplha);
        _minimumNeighbourhoodAplha = std::min<uint8_t>(pixelMap->getAlphaAt(_coord.x, _coord.y - 1), _minimumNeighbourhoodAplha);
        
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
            
            _minimumNeighbourhoodAplha = _children[0]->_minimumNeighbourhoodAplha;
            
            for (int i = 0; i < 4; i++) {
                _minimumNeighbourhoodAplha = std::min<uint8_t>(_children[i]->_minimumNeighbourhoodAplha, _minimumNeighbourhoodAplha);
                delete _children[i];
                _children[i] = NULL;
            }
        } else {
            _allChildrenHasSameColor = false;
        }
    }
}

SquarePixelMap::SquarePixelMap(const uint32_t* pixels, int dimension) : _rootPixelQuadTreeNode(NULL) {
    _data = new SquarePixelMapData();
    _data->dimension = dimension;
    _data->reference_counter = 1;
    
    size_t pixels_size = dimension * dimension;
    _data->pixels = new uint32_t[pixels_size];
    memcpy((void*)_data->pixels, (void*)pixels, sizeof(uint32_t) * pixels_size);
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

void SquarePixelMap::addVoxelsToVoxelTree(VoxelTree* voxelTree) {
    this->generateRootPixelQuadTreeNode();
    this->createVoxelsFromPixelQuadTreeToVoxelTree(_rootPixelQuadTreeNode, voxelTree);
}

int SquarePixelMap::dimension() {
    return _data->dimension;
}
                
uint32_t SquarePixelMap::getPixelAt(unsigned int x, unsigned int y) {
    return _data->pixels[x + y * _data->dimension];
}

uint8_t SquarePixelMap::getAlphaAt(int x, int y) {
    int max_coord = this->dimension() - 1;
    
    if (x < 0 || y < 0 || x > max_coord || y > max_coord) {
        return -1;
    }
    
    return this->getPixelAt(x, y) >> ALPHA_CHANNEL_BIT_OFFSET;
}

void SquarePixelMap::generateRootPixelQuadTreeNode() {
    delete _rootPixelQuadTreeNode;

    PixelQuadTreeCoordinates rootNodeCoord = PixelQuadTreeCoordinates();
    rootNodeCoord.size = 1 << numberOfBitsForSize(_data->dimension);
    rootNodeCoord.x = rootNodeCoord.y = 0;
    
    _rootPixelQuadTreeNode = new PixelQuadTreeNode(rootNodeCoord, this);
}

void SquarePixelMap::createVoxelsFromPixelQuadTreeToVoxelTree(PixelQuadTreeNode* pixelQuadTreeNode, VoxelTree* voxelTree) {
    if (pixelQuadTreeNode->_allChildrenHasSameColor) {
        VoxelDetail voxel = this->getVoxelDetail(pixelQuadTreeNode);
        
        unsigned char minimumNeighbourhoodAplha = std::max<int>(0, pixelQuadTreeNode->_minimumNeighbourhoodAplha - 1);
        
        float minimumNeighbourhoodY = voxel.s * (floor(minimumNeighbourhoodAplha / (ALPHA_CHANNEL_RANGE_FLOAT * voxel.s)) + 0.5);
        
        do {
            voxelTree->createVoxel(voxel.x, voxel.y, voxel.z, voxel.s, voxel.red, voxel.green, voxel.blue, true);
        } while ((voxel.y -= voxel.s) > minimumNeighbourhoodY);
    } else {
        for (int i = 0; i < 4; i++) {
            PixelQuadTreeNode* child = pixelQuadTreeNode->_children[i];
            if (child) {
                this->createVoxelsFromPixelQuadTreeToVoxelTree(child, voxelTree);
            }
        }
    }
}

VoxelDetail SquarePixelMap::getVoxelDetail(PixelQuadTreeNode* pixelQuadTreeNode) {
    VoxelDetail voxel = VoxelDetail();
    
    uint32_t color = pixelQuadTreeNode->_color;
    unsigned char alpha = std::max<int>(0, (color >> ALPHA_CHANNEL_BIT_OFFSET) - 1);
    
    voxel.red = color >> RED_CHANNEL_BIT_OFFSET;
    voxel.green = color >> GREEN_CHANNEL_BIT_OFFSET;
    voxel.blue = color;
    
    
    float rootSize = _rootPixelQuadTreeNode->_coord.size;
    
    voxel.s = pixelQuadTreeNode->_coord.size / rootSize;
    voxel.y = voxel.s * (floor(alpha / (ALPHA_CHANNEL_RANGE_FLOAT * voxel.s)) + 0.5);
    voxel.x = pixelQuadTreeNode->_coord.x / rootSize + voxel.s / 2;
    voxel.z = pixelQuadTreeNode->_coord.y / rootSize + voxel.s / 2;
    
    return voxel;
}
