//
//  SketchUp.h
//  hifi
//
//  Created by Stojce Slavkovski on 6/27/13.
//
//

#ifndef __hifi__SketchUp__
#define __hifi__SketchUp__

#include <iostream>
#include "VoxelTree.h"

namespace voxelImport {
    class SketchUp {
    public:
        static bool importModel(const char* modelFileName, VoxelTree tree, float voxelSize, float modelVoxelSize);
    };
}
#endif /* defined(__hifi__SketchUp__) */
