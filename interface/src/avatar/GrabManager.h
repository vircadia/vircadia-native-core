//
//  GrabManager.h
//  interface/src/avatar/
//
//  Created by Seth Alves on 2018-12-4.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AvatarData.h>
#include <EntityTreeRenderer.h>
#include "AvatarManager.h"

class GrabManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void simulateGrabs();

};
