//
//  Transmitter.h
//  interface
//
//  Created by Brad Hefta-Gaub on 5/21/2013
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__transmitter__
#define __interface__transmitter__


class Transmitter {
public:
    void processIncomingData(unsigned char* buffer, int size) { };
    void renderLevels(int x, int y) { };
    void resetLevels(int x = 0, int y = 0) { };
    bool isConnected() const { return false; };
    glm::vec3 getEstimatedRotation() const { return glm::vec3(0, 0, 0); };
};
#endif
