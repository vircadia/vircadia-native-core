//
//  HandData.h
//  hifi
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__HandData__
#define __hifi__HandData__

#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#define MAX_AVATAR_LEAP_BALLS 10

class AvatarData;

class HandData {
public:
    HandData(AvatarData* owningAvatar);
    
    const std::vector<glm::vec3>& getFingerPositions() const { return _fingerPositions; }
    void setFingerPositions(const std::vector<glm::vec3>& fingerPositions) { _fingerPositions = fingerPositions; }
    
    friend class AvatarData;
protected:
    std::vector<glm::vec3> _fingerPositions;
    AvatarData* _owningAvatarData;
private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HandData(const HandData&);
    HandData& operator= (const HandData&);
};

#endif /* defined(__hifi__HandData__) */
