//
//  main.cpp
//  animation-server/src
//
//  Created by Brad Hefta-Gaub on 05/16/2013.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimationServer.h"

int main(int argc, char * argv[]) {
    AnimationServer animationServer(argc, argv);
    return animationServer.exec();
}
