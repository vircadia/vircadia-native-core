//
//  OpenGLInfo.cpp
//  interface/src
//
//  Created by David Rowe on 28 Jan 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OpenGLInfo.h"

#include <QMessageBox>
#include <QRegularExpression>

#include "GLCanvas.h"

OpenGLInfo::OpenGLInfo(int& argc, char** argv) :
    QApplication(argc, argv)
{
}

bool OpenGLInfo::isValidVersion() {
    bool valid = true;

    // Retrieve OpenGL version
    GLCanvas* glCanvas = new GLCanvas();
    glCanvas->initializeGL();
    QString glVersion = QString((const char*)glGetString(GL_VERSION));
    delete glCanvas;

    // Compare against minimum
    // The GL_VERSION string begins with a version number in one of these forms:
    // - major_number.minor_number 
    // - major_number.minor_number.release_number
    // Reference: https://www.opengl.org/sdk/docs/man/docbook4/xhtml/glGetString.xml
    const int MINIMUM_OPENGL_MAJOR_VERSION = 4;
    const int MINIMUM_OPENGL_MINOR_VERSION = 1;
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = versionParts[0].toInt();
    int minorNumber = versionParts[1].toInt();
    valid = (majorNumber > MINIMUM_OPENGL_MAJOR_VERSION
        || (majorNumber == MINIMUM_OPENGL_MAJOR_VERSION && minorNumber >= MINIMUM_OPENGL_MINOR_VERSION));

    // Prompt user if below minimum
    if (!valid) {
        QMessageBox messageBox;
        messageBox.setWindowTitle("OpenGL Version Too Low");
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setText(QString().sprintf("Your OpenGL version of %i.%i is lower than the minimum of %i.%i.",
            majorNumber, minorNumber, MINIMUM_OPENGL_MAJOR_VERSION, MINIMUM_OPENGL_MINOR_VERSION));
        messageBox.setInformativeText("Press OK to exit; Ignore to continue.");
        messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Ignore);
        messageBox.setDefaultButton(QMessageBox::Ok);
        valid = messageBox.exec() == QMessageBox::Ignore;
    }

    return valid;
}
