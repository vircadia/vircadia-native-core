//
//  Balls.h
//  hifi
//
//  Created by Philip on 4/25/13.
//
//

#ifndef hifi_Balls_h
#define hifi_Balls_h

#include <glm/glm.hpp>
#include "Util.h"
#include "world.h"
#include "InterfaceConfig.h"


const int NUMBER_SPRINGS = 4;

class Balls {
public:
    Balls(int numberOfBalls);
    
    void simulate(float deltaTime);
    void render();
    
private:
    struct Ball {
        glm::vec3   position, velocity;
        int         links[NUMBER_SPRINGS];
        float       springLength[NUMBER_SPRINGS];
        float       radius;
    } *_balls;
    int _numberOfBalls;
};

#endif
