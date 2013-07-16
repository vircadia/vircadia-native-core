//
//  Balls.h
//  hifi
//
//  Created by Philip on 4/25/13.
//
//

#ifndef hifi_Physics_h
#define hifi_Physics_h

void applyStaticFriction(float deltaTime, glm::vec3& velocity, float maxVelocity, float strength);
void applyDamping(float deltaTime, glm::vec3& velocity, float linearStrength, float squaredStrength);

#endif
