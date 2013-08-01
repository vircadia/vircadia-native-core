//
//  JurisdictionMap.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__JurisdictionMap__
#define __hifi__JurisdictionMap__

#include <vector>

class VoxelNode; // forward declaration

class JurisdictionMap {
public:
    JurisdictionMap();
    JurisdictionMap(const char* filename);
    JurisdictionMap(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes);
    ~JurisdictionMap();


    bool isMyJurisdiction(VoxelNode* node, int childIndex) const;
    
private:
    void clear();
    void init(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes);

    bool writeToFile(const char* filename);
    bool readFromFile(const char* filename);


    unsigned char* _rootOctalCode;
    std::vector<unsigned char*> _endNodes;
};

#endif /* defined(__hifi__JurisdictionMap__) */


