//
//  OpenGLVersionChecker.cpp
//  libraries/gl/src/gl
//
//  Created by David Rowe on 28 Jan 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OpenGLVersionChecker.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QJsonObject>

#include "Config.h"
#include "GLWidget.h"
#include "GLHelpers.h"

OpenGLVersionChecker::OpenGLVersionChecker(int& argc, char** argv) :
    QApplication(argc, argv)
{
}

QJsonObject OpenGLVersionChecker::checkVersion(bool& valid, bool& override) {
    valid = true;
    override = false;

    GLWidget* glWidget = new GLWidget();
    valid = glWidget->isValid();
    // Inform user if no OpenGL support
    if (!valid) {
        QMessageBox messageBox;
        messageBox.setWindowTitle("Missing OpenGL Support");
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setText(QString().sprintf("Your system does not support OpenGL, Interface cannot run."));
        messageBox.setInformativeText("Press OK to exit.");
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.setDefaultButton(QMessageBox::Ok);
        messageBox.exec();
        return QJsonObject();
    }
    
    // Retrieve OpenGL version
    glWidget->initializeGL();
    QJsonObject glData = getGLContextData();
    delete glWidget;

    // Compare against minimum
    // The GL_VERSION string begins with a version number in one of these forms:
    // - major_number.minor_number 
    // - major_number.minor_number.release_number
    // Reference: https://www.opengl.org/sdk/docs/man/docbook4/xhtml/glGetString.xml
    const QString version { "version" };
    QString glVersion = glData[version].toString();
    QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
    int majorNumber = versionParts[0].toInt();
    int minorNumber = versionParts[1].toInt();
    int minimumMajorNumber = GPU_CORE_MINIMUM / 100;
    int minimumMinorNumber = (GPU_CORE_MINIMUM - minimumMajorNumber * 100) / 10;
    valid = (majorNumber > minimumMajorNumber
        || (majorNumber == minimumMajorNumber && minorNumber >= minimumMinorNumber));

    // Prompt user if below minimum
    if (!valid) {
        QMessageBox messageBox;
        messageBox.setWindowTitle("OpenGL Version Too Low");
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setText(QString().sprintf("Your OpenGL version of %i.%i is lower than the minimum of %i.%i.",
            majorNumber, minorNumber, minimumMajorNumber, minimumMinorNumber));
        messageBox.setInformativeText("Press OK to exit; Ignore to continue.");
        messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Ignore);
        messageBox.setDefaultButton(QMessageBox::Ok);
        override = messageBox.exec() == QMessageBox::Ignore;
    }

    return glData;
}
