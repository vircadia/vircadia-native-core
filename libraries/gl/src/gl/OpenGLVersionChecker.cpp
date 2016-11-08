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
#include "Config.h"

#include <mutex>

#include <QtCore/QRegularExpression>
#include <QtCore/QJsonObject>
#include <QtWidgets/QMessageBox>
#include <QtOpenGL/QGLWidget>

#include "GLHelpers.h"

#define MINIMUM_GL_VERSION 0x0401

OpenGLVersionChecker::OpenGLVersionChecker(int& argc, char** argv) :
    QApplication(argc, argv)
{
}

const QGLFormat& getDefaultGLFormat() {
    // Specify an OpenGL 3.3 format using the Core profile.
    // That is, no old-school fixed pipeline functionality
    static QGLFormat glFormat;
    static std::once_flag once;
    std::call_once(once, [] {
        setGLFormatVersion(glFormat);
        glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
        glFormat.setSampleBuffers(false);
        glFormat.setDepth(false);
        glFormat.setStencil(false);
        QGLFormat::setDefaultFormat(glFormat);
    });
    return glFormat;
}


QJsonObject OpenGLVersionChecker::checkVersion(bool& valid, bool& override) {
    valid = true;
    override = false;

    QGLWidget* glWidget = new QGLWidget();
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
    // glWidget->initializeGL();
    glWidget->makeCurrent();
    QJsonObject glData = getGLContextData();
    delete glWidget;

    // Compare against minimum
    // The GL_VERSION string begins with a version number in one of these forms:
    // - major_number.minor_number 
    // - major_number.minor_number.release_number
    // Reference: https://www.opengl.org/sdk/docs/man/docbook4/xhtml/glGetString.xml

    int minimumMajorNumber = (MINIMUM_GL_VERSION >> 16);
    int minimumMinorNumber = (MINIMUM_GL_VERSION & 0xFF);
    int majorNumber = 0;
    int minorNumber = 0;
    const QString version { "version" };
    if (glData.contains(version)) {
        QString glVersion = glData[version].toString();
        QStringList versionParts = glVersion.split(QRegularExpression("[\\.\\s]"));
        if (versionParts.size() >= 2) {
            majorNumber = versionParts[0].toInt();
            minorNumber = versionParts[1].toInt();
            valid = (majorNumber > minimumMajorNumber
                || (majorNumber == minimumMajorNumber && minorNumber >= minimumMinorNumber));
        } else {
            valid = false;
        }
    } else {
        valid = false;
    }

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
