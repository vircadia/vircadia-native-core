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

class AvatarData;

class HandData {
public:
    HandData(AvatarData* owningAvatar);
    
    // These methods return the positions in Leap-relative space.
    // To convert to world coordinates, use Hand::leapPositionToWorldPosition.
    const std::vector<glm::vec3>& getFingerTips() const { return _fingerTips; }
    const std::vector<glm::vec3>& getFingerRoots() const { return _fingerRoots; }
    const std::vector<glm::vec3>& getHandPositions() const { return _handPositions; }
    const std::vector<glm::vec3>& getHandNormals() const { return _handNormals; }
    void setFingerTips(const std::vector<glm::vec3>& fingerTips) { _fingerTips = fingerTips; }
    void setFingerRoots(const std::vector<glm::vec3>& fingerRoots) { _fingerRoots = fingerRoots; }
    void setHandPositions(const std::vector<glm::vec3>& handPositons) { _handPositions = handPositons; }
    void setHandNormals(const std::vector<glm::vec3>& handNormals) { _handNormals = handNormals; }
    
    friend class AvatarData;
protected:
    std::vector<glm::vec3> _fingerTips;
    std::vector<glm::vec3> _fingerRoots;
    std::vector<glm::vec3> _handPositions;
    std::vector<glm::vec3> _handNormals;
    AvatarData* _owningAvatarData;
private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HandData(const HandData&);
    HandData& operator= (const HandData&);
};

#endif /* defined(__hifi__HandData__) */
