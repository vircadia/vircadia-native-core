//
//  Hair.cpp
//  interface/src
//
//  Created by Philip on June 26, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Creates single flexible vertlet-integrated strands that can be used for hair/fur/grass

#include "Hair.h"

#include "Util.h"
#include "world.h"


Hair::Hair() {
    qDebug() << "Creating Hair";
 }

void Hair::simulate(float deltaTime) {
}

void Hair::render() {
    //
    //  Before calling this function, translate/rotate to the origin of the owning object
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(1.0f, 15, 15);
    glPopMatrix();
}


