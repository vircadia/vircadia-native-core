//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterleavedStereoDisplayPlugin.h"

#include <QApplication>
#include <QDesktopWidget>

#include <GlWindow.h>
#include <ViewFrustum.h>
#include <MatrixStack.h>

#include <gpu/GLBackend.h>

const QString InterleavedStereoDisplayPlugin::NAME("Interleaved Stereo Display");

const QString & InterleavedStereoDisplayPlugin::getName() const {
    return NAME;
}

InterleavedStereoDisplayPlugin::InterleavedStereoDisplayPlugin() {
}

void InterleavedStereoDisplayPlugin::customizeContext() {
    StereoDisplayPlugin::customizeContext();
    // Set up the stencil buffers?  Or use a custom shader?

}
