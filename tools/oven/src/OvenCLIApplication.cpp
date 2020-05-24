//
//  OvenCLIApplication.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 2/20/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OvenCLIApplication.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QUrl>

#include <iostream>

#include <image/TextureProcessing.h>
#include <TextureBaker.h>

#include "BakerCLI.h"

static const QString CLI_INPUT_PARAMETER = "i";
static const QString CLI_OUTPUT_PARAMETER = "o";
static const QString CLI_TYPE_PARAMETER = "t";
static const QString CLI_DISABLE_TEXTURE_COMPRESSION_PARAMETER = "disable-texture-compression";

QUrl OvenCLIApplication::_inputUrlParameter;
QUrl OvenCLIApplication::_outputUrlParameter;
QString OvenCLIApplication::_typeParameter;

OvenCLIApplication::OvenCLIApplication(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    BakerCLI* cli = new BakerCLI(this);
    QMetaObject::invokeMethod(cli, "bakeFile", Qt::QueuedConnection, Q_ARG(QUrl, _inputUrlParameter),
                              Q_ARG(QString, _outputUrlParameter.toString()), Q_ARG(QString, _typeParameter));
}

void OvenCLIApplication::parseCommandLine(int argc, char* argv[]) {
    // parse the command line parameters
    QCommandLineParser parser;

    parser.setApplicationDescription("High Fidelity Oven");
    parser.addOptions({
        { CLI_INPUT_PARAMETER, "Path to file that you would like to bake.", "input" },
        { CLI_OUTPUT_PARAMETER, "Path to folder that will be used as output.", "output" },
        { CLI_TYPE_PARAMETER, "Type of asset. [model|material]"/*|js]"*/, "type" },
        { CLI_DISABLE_TEXTURE_COMPRESSION_PARAMETER, "Disable texture compression." }
    });

    auto versionOption = parser.addVersionOption();
    auto helpOption = parser.addHelpOption();

    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << argv[i];
    }

    if (!parser.parse(arguments)) {
        std::cout << parser.errorText().toStdString() << std::endl; // Avoid Qt log spam
        QCoreApplication mockApp(argc, argv); // required for call to showHelp()
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(versionOption)) {
        parser.showVersion();
        Q_UNREACHABLE();
    }
    if (parser.isSet(helpOption)) {
        QCoreApplication mockApp(argc, argv); // required for call to showHelp()
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (!parser.isSet(CLI_INPUT_PARAMETER) || !parser.isSet(CLI_OUTPUT_PARAMETER)) {
        std::cout << "Error: Input and Output not set" << std::endl; // Avoid Qt log spam
        QCoreApplication mockApp(argc, argv); // required for call to showHelp()
        parser.showHelp();
        Q_UNREACHABLE();
    }

    _inputUrlParameter = QDir::fromNativeSeparators(parser.value(CLI_INPUT_PARAMETER));
    _outputUrlParameter = QDir::fromNativeSeparators(parser.value(CLI_OUTPUT_PARAMETER));

    _typeParameter = parser.isSet(CLI_TYPE_PARAMETER) ? parser.value(CLI_TYPE_PARAMETER) : QString();

    if (parser.isSet(CLI_DISABLE_TEXTURE_COMPRESSION_PARAMETER)) {
        qDebug() << "Disabling texture compression";
        TextureBaker::setCompressionEnabled(false);
    }
}
