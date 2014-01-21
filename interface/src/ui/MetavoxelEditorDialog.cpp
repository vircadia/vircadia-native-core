//
//  MetavoxelEditorDialog.cpp
//  interface
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#include "Application.h"
#include "MetavoxelEditorDialog.h"

MetavoxelEditorDialog::MetavoxelEditorDialog() :
    QDialog(Application::getInstance()->getGLWidget()) {
    
    setWindowTitle("Metavoxel Editor");
    setAttribute(Qt::WA_DeleteOnClose);
    
    show();
}
