//
//  ModelPackager.cpp
//
//
//  Created by Clement on 3/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelSelector.h"
#include "ModelPackager.h"

void ModelPackager::package() {
    ModelPackager packager;
    if (packager.selectModel()) {
        packager.editProperties();
        packager.zipModel();
    }
}

bool ModelPackager::selectModel() {
    ModelSelector selector;
    return selector.exec() == QDialog::Accepted;
}

void ModelPackager::editProperties() {
    
}

void ModelPackager::zipModel() {
    
}


