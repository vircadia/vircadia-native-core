//
//  Oven.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QCommandLineParser>

#include <image/Image.h>
#include <SettingInterface.h>

#include "ui/OvenMainWindow.h"
#include "Oven.h"
#include "BakerCLI.h"

static const QString OUTPUT_FOLDER = "/Users/birarda/code/hifi/lod/test-oven/export";

static const QString CLI_INPUT_PARAMETER = "i";
static const QString CLI_OUTPUT_PARAMETER = "o";

Oven::Oven(int argc, char* argv[]) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName("High Fidelity");
    QCoreApplication::setApplicationName("Oven");

    // init the settings interface so we can save and load settings
    Setting::init();

    // parse the command line parameters
    QCommandLineParser parser;
   
    parser.addOptions({
        { CLI_INPUT_PARAMETER, "Path to file that you would like to bake.", "input" },
        { CLI_OUTPUT_PARAMETER, "Path to folder that will be used as output.", "output" }
    });
    parser.addHelpOption();
    parser.process(*this);

    // enable compression in image library, except for cube maps
    image::setColorTexturesCompressionEnabled(true);
    image::setGrayscaleTexturesCompressionEnabled(true);
    image::setNormalTexturesCompressionEnabled(true);
    image::setCubeTexturesCompressionEnabled(true);

    // setup our worker threads
    setupWorkerThreads(QThread::idealThreadCount() - 1);

    // Autodesk's SDK means that we need a single thread for all FBX importing/exporting in the same process
    // setup the FBX Baker thread
    setupFBXBakerThread();

    // check if we were passed any command line arguments that would tell us just to run without the GUI
    if (parser.isSet(CLI_INPUT_PARAMETER) || parser.isSet(CLI_OUTPUT_PARAMETER)) {
        if (parser.isSet(CLI_INPUT_PARAMETER) && parser.isSet(CLI_OUTPUT_PARAMETER)) {
            BakerCLI* cli = new BakerCLI(this);
            QUrl inputUrl(QDir::fromNativeSeparators(parser.value(CLI_INPUT_PARAMETER)));
            QUrl outputUrl(QDir::fromNativeSeparators(parser.value(CLI_OUTPUT_PARAMETER)));
            cli->bakeFile(inputUrl, outputUrl.toString());
        } else {
            parser.showHelp();
            QApplication::quit();
        } 
    } else {
        // setup the GUI
        _mainWindow = new OvenMainWindow;
        _mainWindow->show();
    }
}

Oven::~Oven() {
    // cleanup the worker threads
    for (auto i = 0; i < _workerThreads.size(); ++i) {
        _workerThreads[i]->quit();
        _workerThreads[i]->wait();
    }

    // cleanup the FBX Baker thread
    _fbxBakerThread->quit();
    _fbxBakerThread->wait();
}

void Oven::setupWorkerThreads(int numWorkerThreads) {
    for (auto i = 0; i < numWorkerThreads; ++i) {
        // setup a worker thread yet and add it to our concurrent vector
        auto newThread = new QThread(this);
        newThread->setObjectName("Oven Worker Thread " + QString::number(i + 1));

        _workerThreads.push_back(newThread);
    }
}

void Oven::setupFBXBakerThread() {
    // we're being asked for the FBX baker thread, but we don't have one yet
    // so set that up now
    _fbxBakerThread = new QThread(this);
    _fbxBakerThread->setObjectName("Oven FBX Baker Thread");
}

QThread* Oven::getFBXBakerThread() {
    if (!_fbxBakerThread->isRunning()) {
        // start the FBX baker thread if it isn't running yet
        _fbxBakerThread->start();
    }

    return _fbxBakerThread;
}

QThread* Oven::getNextWorkerThread() {
    // Here we replicate some of the functionality of QThreadPool by giving callers an available worker thread to use.
    // We can't use QThreadPool because we want to put QObjects with signals/slots on these threads.
    // So instead we setup our own list of threads, up to one less than the ideal thread count
    // (for the FBX Baker Thread to have room), and cycle through them to hand a usable running thread back to our callers.

    auto nextIndex = ++_nextWorkerThreadIndex;
    auto nextThread = _workerThreads[nextIndex % _workerThreads.size()];

    // start the thread if it isn't running yet
    if (!nextThread->isRunning()) {
        nextThread->start();
    }

    return nextThread;
}

