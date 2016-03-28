//
//  main.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSettings>
#include <QSharedMemory>
#include <QTranslator>

#include <gl/OpenGLVersionChecker.h>
#include <SharedUtil.h>

#include "AddressManager.h"
#include "Application.h"
#include "InterfaceLogging.h"
#include "UserActivityLogger.h"
#include "MainWindow.h"

#ifdef HAS_BUGSPLAT
#include <BuildInfo.h>
#include <BugSplat.h>
#endif

int main(int argc, const char* argv[]) {
    disableQtBearerPoll(); // Fixes wifi ping spikes

#if HAS_BUGSPLAT
    // Prevent other threads from hijacking the Exception filter, and allocate 4MB up-front that may be useful in
    // low-memory scenarios.
    static const DWORD BUG_SPLAT_FLAGS = MDSF_PREVENTHIJACKING | MDSF_USEGUARDMEMORY;
    static const char* BUG_SPLAT_DATABASE = "interface_alpha";
    static const char* BUG_SPLAT_APPLICATION_NAME = "Interface";
    MiniDmpSender mpSender { BUG_SPLAT_DATABASE, BUG_SPLAT_APPLICATION_NAME, qPrintable(BuildInfo::VERSION),
                             nullptr, BUG_SPLAT_FLAGS };
#endif
    
    QString applicationName = "High Fidelity Interface - " + qgetenv("USERNAME");

    bool instanceMightBeRunning = true;

#ifdef Q_OS_WIN
    // Try to create a shared memory block - if it can't be created, there is an instance of
    // interface already running. We only do this on Windows for now because of the potential
    // for crashed instances to leave behind shared memory instances on unix.
    QSharedMemory sharedMemory { applicationName };
    instanceMightBeRunning = !sharedMemory.create(1, QSharedMemory::ReadOnly);
#endif

    if (instanceMightBeRunning) {
        // Try to connect and send message to existing interface instance
        QLocalSocket socket;

        socket.connectToServer(applicationName);

        static const int LOCAL_SERVER_TIMEOUT_MS = 500;

        // Try to connect - if we can't connect, interface has probably just gone down
        if (socket.waitForConnected(LOCAL_SERVER_TIMEOUT_MS)) {

            QStringList arguments;
            for (int i = 0; i < argc; ++i) {
                arguments << argv[i];
            }

            QCommandLineParser parser;
            QCommandLineOption urlOption("url", "", "value");
            parser.addOption(urlOption);
            parser.process(arguments);

            if (parser.isSet(urlOption)) {
                QUrl url = QUrl(parser.value(urlOption));
                if (url.isValid() && url.scheme() == HIFI_URL_SCHEME) {
                    qDebug() << "Writing URL to local socket";
                    socket.write(url.toString().toUtf8());
                    if (!socket.waitForBytesWritten(5000)) {
                        qDebug() << "Error writing URL to local socket";
                    }
                }
            }

            socket.close();

            qDebug() << "Interface instance appears to be running, exiting";

            return EXIT_SUCCESS;
        }

#ifdef Q_OS_WIN
        return EXIT_SUCCESS;
#endif
    }

    // Check OpenGL version.
    // This is done separately from the main Application so that start-up and shut-down logic within the main Application is
    // not made more complicated than it already is.
    bool override = false;
    QString glVersion;
    {
        OpenGLVersionChecker openGLVersionChecker(argc, const_cast<char**>(argv));
        bool valid = true;
        glVersion = openGLVersionChecker.checkVersion(valid, override);
        if (!valid) {
            if (override) {
                qCDebug(interfaceapp, "Running on insufficient OpenGL version: %s.", glVersion.toStdString().c_str());
            } else {
                qCDebug(interfaceapp, "Early exit due to OpenGL version.");
                return 0;
            }
        }
    }

    QElapsedTimer startupTime;
    startupTime.start();

    // Debug option to demonstrate that the client's local time does not
    // need to be in sync with any other network node. This forces clock
    // skew for the individual client
    const char* CLOCK_SKEW = "--clockSkew";
    const char* clockSkewOption = getCmdOption(argc, argv, CLOCK_SKEW);
    if (clockSkewOption) {
        int clockSkew = atoi(clockSkewOption);
        usecTimestampNowForceClockSkew(clockSkew);
        qCDebug(interfaceapp, "clockSkewOption=%s clockSkew=%d", clockSkewOption, clockSkew);
    }

    // Oculus initialization MUST PRECEDE OpenGL context creation.
    // The nature of the Application constructor means this has to be either here,
    // or in the main window ctor, before GL startup.
    Application::initPlugins();

    int exitCode;
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        Application app(argc, const_cast<char**>(argv), startupTime);

        // If we failed the OpenGLVersion check, log it.
        if (override) {
            auto& accountManager = AccountManager::getInstance();
            if (accountManager.isLoggedIn()) {
                UserActivityLogger::getInstance().insufficientGLVersion(glVersion);
            } else {
                QObject::connect(&AccountManager::getInstance(), &AccountManager::loginComplete, [glVersion](){
                    static bool loggedInsufficientGL = false;
                    if (!loggedInsufficientGL) {
                        UserActivityLogger::getInstance().insufficientGLVersion(glVersion);
                        loggedInsufficientGL = true;
                    }
                });
            }
        }

        // Setup local server
        QLocalServer server { &app };

        // We failed to connect to a local server, so we remove any existing servers.
        server.removeServer(applicationName);
        server.listen(applicationName);

        QObject::connect(&server, &QLocalServer::newConnection, &app, &Application::handleLocalServerConnection);

#ifdef HAS_BUGSPLAT
        AccountManager& accountManager = AccountManager::getInstance();
        mpSender.setDefaultUserName(qPrintable(accountManager.getAccountInfo().getUsername()));
        QObject::connect(&accountManager, &AccountManager::usernameChanged, &app, [&mpSender](const QString& newUsername) {
            mpSender.setDefaultUserName(qPrintable(newUsername));
        });

        // BugSplat WILL NOT work with file paths that do not use OS native separators.
        auto logPath = QDir::toNativeSeparators(app.getLogger()->getFilename());
        mpSender.sendAdditionalFile(qPrintable(logPath));
#endif

        QTranslator translator;
        translator.load("i18n/interface_en");
        app.installTranslator(&translator);

        qCDebug(interfaceapp, "Created QT Application.");
        exitCode = app.exec();
        server.close();
    }

    Application::shutdownPlugins();

    qCDebug(interfaceapp, "Normal exit.");
#ifndef DEBUG
    // HACK: exit immediately (don't handle shutdown callbacks) for Release build
    _exit(exitCode);
#endif
    return exitCode;
}
