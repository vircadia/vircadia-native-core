//
//  Balls.h
//  hifi
//
//  Created by Philip on 4/25/13.
//
//

#ifndef hifi_Balls_h
#define hifi_Balls_h

const int NUMBER_SPRINGS = 4;

class Balls {
public:
    Balls(int numberOfBalls);
    
    void simulate(float deltaTime);
    void render();
    
    void setColor(const glm::vec3& c) { _color = c; };
    void moveOrigin(const glm::vec3& newOrigin);
    
private:
    struct Ball {
        glm::vec3   position, targetPosition, velocity;
        int         links[NUMBER_SPRINGS];
        float       springLength[NUMBER_SPRINGS];
        float       radius;
    } *_balls;
    int _numberOfBalls;
    glm::vec3 _origin;
    glm::vec3 _color;
};

#endif
