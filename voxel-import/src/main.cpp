//
//  main.cpp
//  skpimporter
//
//  Created by Stojce Slavkovski on 6/14/13.
//  Copyright (c) 2013 High Fidelity. All rights reserved.
//
	
#include "SketchUp.h"
#include "VoxelTree.h"

int main(int argc, const char * argv[])
{
    VoxelTree tree;
    if (voxelImport::SketchUp::importModel("model.skp", &tree, 0.1f / 512, 10)) {
        tree.writeToSVOFile("voxels.svo");
    }
    
    return 0;
}

