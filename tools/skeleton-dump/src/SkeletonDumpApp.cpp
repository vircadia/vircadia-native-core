//
//  SkeletonDumpApp.cpp
//  tools/skeleton-dump/src
//
//  Created by Anthony Thibault on 11/4/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SkeletonDumpApp.h"
#include <QCommandLineParser>
#include <QFile>
#include <FBXReader.h>
#include <AnimSkeleton.h>

SkeletonDumpApp::SkeletonDumpApp(int argc, char* argv[]) : QCoreApplication(argc, argv) {

    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity FBX Skeleton Analyzer");
    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    const QCommandLineOption inputFilenameOption("i", "input file", "filename.fbx");
    parser.addOption(inputFilenameOption);

    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        _returnCode = 1;
        return;
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        return;
    }

    bool verbose = parser.isSet(verboseOutput);

    QString inputFilename;
    if (parser.isSet(inputFilenameOption)) {
        inputFilename = parser.value(inputFilenameOption);
    }

    QFile file(inputFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file " << inputFilename;
        _returnCode = 2;
        return;
    }
    QByteArray blob = file.readAll();
    std::unique_ptr<FBXGeometry> fbxGeometry(readFBX(blob, QVariantHash()));
    std::unique_ptr<AnimSkeleton> skeleton(new AnimSkeleton(*fbxGeometry));
    skeleton->dump(verbose);
}

SkeletonDumpApp::~SkeletonDumpApp() {
}
