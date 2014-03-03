//
//  globals.h
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#pragma once

#include "svo-viewer-config.h"

#include <QtWidgets/QMainWindow>
#include <QDesktopWidget>
#include <QOpenGLFramebufferObject>
//#include <GLCanvas>

//#include "ui_svoviewer.h"
#include <ViewFrustum.h>
#include <VoxelTree.h>
#include <SharedUtil.h>
//#include <VoxelShader.h>
//#include <VoxelSystem.h>
//#include <PointShader.h>
#include <OctreeRenderer.h>

#include "Camera.h"


extern const GLfloat WHITE_SPECULAR_COLOR[];
extern const GLfloat NO_SPECULAR_COLOR[];

namespace SvoViewerNames // avoid conflicts with voxelSystem namespace objects
{
extern GLubyte cubeFaceIndices[][2*3];

extern GLubyte cubeFaceVtxs[6][4];

extern glm::vec3 faceNormals[6];
}