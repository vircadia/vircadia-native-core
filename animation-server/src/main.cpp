//
//  main.cpp
//  Animation Server
//
//  Created by Brad Hefta-Gaub on 05/16/2013
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "AnimationServer.h"

int main(int argc, char * argv[]) {
    AnimationServer animationServer(argc, argv);
    return animationServer.exec();
}
