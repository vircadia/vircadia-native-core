//
//  main.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <thread>

#include <QCommandLineParser>
#include <QtCore/QProcess>
#include <QDebug>
#include <QDir>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSharedMemory>
#include <QTranslator>

#include <BuildInfo.h>
#include <gl/OpenGLVersionChecker.h>
#include <SandboxUtils.h>
#include <SharedUtil.h>
#include <NetworkAccessManager.h>

#include "AddressManager.h"
#include "Application.h"
#include "InterfaceLogging.h"
#include "UserActivityLogger.h"
#include "MainWindow.h"

#ifdef HAS_BUGSPLAT
#include <BugSplat.h>
#include <CrashReporter.h>
#endif

#ifdef Q_OS_WIN
extern "C" {
    typedef int(__stdcall * CHECKMINSPECPROC) ();
}
#endif

int main(int argc, const char* argv[]) {
#if HAS_BUGSPLAT
    static QString BUG_SPLAT_DATABASE = "interface_alpha";
    static QString BUG_SPLAT_APPLICATION_NAME = "Interface";
    CrashReporter crashReporter { BUG_SPLAT_DATABASE, BUG_SPLAT_APPLICATION_NAME, BuildInfo::VERSION };
#endif

    disableQtBearerPoll(); // Fixes wifi ping spikes

    QElapsedTimer startupTime;
    startupTime.start();

    // Set application infos
    QCoreApplication::setApplicationName(BuildInfo::INTERFACE_NAME);
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << argv[i];
    }

    QCommandLineParser parser;
    QCommandLineOption urlOption("url", "", "value");
    QCommandLineOption noUpdaterOption("no-updater", "Do not show auto-updater");
    QCommandLineOption checkMinSpecOption("checkMinSpec", "Check if machine meets minimum specifications");
    QCommandLineOption runServerOption("runServer", "Whether to run the server");
    QCommandLineOption serverContentPathOption("serverContentPath", "Where to find server content", "serverContentPath");
    QCommandLineOption allowMultipleInstancesOption("allowMultipleInstances", "Allow multiple instances to run");
    QCommandLineOption overrideAppLocalDataPathOption("cache", "set test cache <dir>", "dir");
    QCommandLineOption overrideScriptsPathOption(SCRIPTS_SWITCH, "set scripts <path>", "path");
    parser.addOption(urlOption);
    parser.addOption(noUpdaterOption);
    parser.addOption(checkMinSpecOption);
    parser.addOption(runServerOption);
    parser.addOption(serverContentPathOption);
    parser.addOption(overrideAppLocalDataPathOption);
    parser.addOption(overrideScriptsPathOption);
    parser.addOption(allowMultipleInstancesOption);
    parser.parse(arguments);


    const QString& applicationName = getInterfaceSharedMemoryName();
    bool instanceMightBeRunning = true;
#ifdef Q_OS_WIN
    // Try to create a shared memory block - if it can't be created, there is an instance of
    // interface already running. We only do this on Windows for now because of the potential
    // for crashed instances to leave behind shared memory instances on unix.
    QSharedMemory sharedMemory{ applicationName };
    instanceMightBeRunning = !sharedMemory.create(1, QSharedMemory::ReadOnly);
#endif

    // allow multiple interfaces to run if this environment variable is set.
    bool allowMultipleInstances = parser.isSet(allowMultipleInstancesOption) ||
                                  QProcessEnvironment::systemEnvironment().contains("HIFI_ALLOW_MULTIPLE_INSTANCES");
    if (allowMultipleInstances) {
        instanceMightBeRunning = false;
    }
    // this needs to be done here in main, as the mechanism for setting the
    // scripts directory appears not to work.  See the bug report
    // https://highfidelity.fogbugz.com/f/cases/5759/Issues-changing-scripts-directory-in-ScriptsEngine
    if (parser.isSet(overrideScriptsPathOption)) {
        QDir scriptsPath(parser.value(overrideScriptsPathOption));
        if (scriptsPath.exists()) {
            PathUtils::defaultScriptsLocation(scriptsPath.path());
        }
    }

    if (instanceMightBeRunning) {
        // Try to connect and send message to existing interface instance
        QLocalSocket socket;

        socket.connectToServer(applicationName);

        static const int LOCAL_SERVER_TIMEOUT_MS = 500;

        // Try to connect - if we can't connect, interface has probably just gone down
        if (socket.waitForConnected(LOCAL_SERVER_TIMEOUT_MS)) {
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


    // FIXME this method of checking the OpenGL version screws up the `QOpenGLContext::globalShareContext()` value, which in turn
    // leads to crashes when creating the real OpenGL instance.  Disabling for now until we come up with a better way of checking
    // the GL version on the system without resorting to creating a full Qt application
#if 0
    // Check OpenGL version.
    // This is done separately from the main Application so that start-up and shut-down logic within the main Application is
    // not made more complicated than it already is.
    bool overrideGLCheck = false;

    QJsonObject glData;
    {
        OpenGLVersionChecker openGLVersionChecker(argc, const_cast<char**>(argv));
        bool valid = true;
        glData = openGLVersionChecker.checkVersion(valid, overrideGLCheck);
        if (!valid) {
            if (overrideGLCheck) {
                auto glVersion = glData["version"].toString();
                qCWarning(interfaceapp, "Running on insufficient OpenGL version: %s.", glVersion.toStdString().c_str());
            } else {
                qCWarning(interfaceapp, "Early exit due to OpenGL version.");
                return 0;
            }
        }
    }
#endif


    // Debug option to demonstrate that the client's local time does not
    // need to be in sync with any other network node. This forces clock
    // skew for the individual client
    const char* CLOCK_SKEW = "--clockSkew";
    const char* clockSkewOption = getCmdOption(argc, argv, CLOCK_SKEW);
    if (clockSkewOption) {
        qint64 clockSkew = atoll(clockSkewOption);
        usecTimestampNowForceClockSkew(clockSkew);
        qCDebug(interfaceapp) << "clockSkewOption=" << clockSkewOption << "clockSkew=" << clockSkew;
    }

    // Oculus initialization MUST PRECEDE OpenGL context creation.
    // The nature of the Application constructor means this has to be either here,
    // or in the main window ctor, before GL startup.
    Application::initPlugins(arguments);

#ifdef Q_OS_WIN
    // If we're running in steam mode, we need to do an explicit check to ensure we're up to the required min spec
    if (parser.isSet(checkMinSpecOption)) {
        QString appPath;
        {
            char filename[MAX_PATH];
            GetModuleFileName(NULL, filename, MAX_PATH);
            QFileInfo appInfo(filename);
            appPath = appInfo.absolutePath();
        }
        QString openvrDllPath = appPath + "/plugins/openvr.dll";
        HMODULE openvrDll;
        CHECKMINSPECPROC checkMinSpecPtr;
        if ((openvrDll = LoadLibrary(openvrDllPath.toLocal8Bit().data())) &&
            (checkMinSpecPtr = (CHECKMINSPECPROC)GetProcAddress(openvrDll, "CheckMinSpec"))) {
            if (!checkMinSpecPtr()) {
                return -1;
            }
        }
    }
#endif

    int exitCode;
    {
        RunningMarker runningMarker(RUNNING_MARKER_FILENAME);
        bool runningMarkerExisted = runningMarker.fileExists();
        runningMarker.writeRunningMarkerFile();

        bool noUpdater = parser.isSet(noUpdaterOption);
        bool runServer = parser.isSet(runServerOption);
        bool serverContentPathOptionIsSet = parser.isSet(serverContentPathOption);
        QString serverContentPath = serverContentPathOptionIsSet ? parser.value(serverContentPathOption) : QString();
        if (runServer) {
            SandboxUtils::runLocalSandbox(serverContentPath, true, noUpdater);
        }

        // Extend argv to enable WebGL rendering
        std::vector<const char*> argvExtended(&argv[0], &argv[argc]);
        argvExtended.push_back("--ignore-gpu-blacklist");
        int argcExtended = (int)argvExtended.size();

        Application app(argcExtended, const_cast<char**>(argvExtended.data()), startupTime, runningMarkerExisted);

#if 0
        // If we failed the OpenGLVersion check, log it.
        if (overrideGLcheck) {
            auto accountManager = DependencyManager::get<AccountManager>();
            if (accountManager->isLoggedIn()) {
                UserActivityLogger::getInstance().insufficientGLVersion(glData);
            } else {
                QObject::connect(accountManager.data(), &AccountManager::loginComplete, [glData](){
                    static bool loggedInsufficientGL = false;
                    if (!loggedInsufficientGL) {
                        UserActivityLogger::getInstance().insufficientGLVersion(glData);
                        loggedInsufficientGL = true;
                    }
                });
            }
        }
#endif
        

        // Setup local server
        QLocalServer server { &app };

        // We failed to connect to a local server, so we remove any existing servers.
        server.removeServer(applicationName);
        server.listen(applicationName);

        QObject::connect(&server, &QLocalServer::newConnection, &app, &Application::handleLocalServerConnection, Qt::DirectConnection);

#ifdef HAS_BUGSPLAT
        auto accountManager = DependencyManager::get<AccountManager>();
        crashReporter.mpSender.setDefaultUserName(qPrintable(accountManager->getAccountInfo().getUsername()));
        QObject::connect(accountManager.data(), &AccountManager::usernameChanged, &app, [&crashReporter](const QString& newUsername) {
            crashReporter.mpSender.setDefaultUserName(qPrintable(newUsername));
        });

        // BugSplat WILL NOT work with file paths that do not use OS native separators.
        auto logger = app.getLogger();
        auto logPath = QDir::toNativeSeparators(logger->getFilename());
        crashReporter.mpSender.sendAdditionalFile(qPrintable(logPath));

        QMetaObject::Connection connection;
        connection = QObject::connect(logger, &FileLogger::rollingLogFile, &app, [&crashReporter, &connection](QString newFilename) {
            // We only want to add the first rolled log file (the "beginning" of the log) to BugSplat to ensure we don't exceed the 2MB
            // zipped limit, so we disconnect here.
            QObject::disconnect(connection);
            auto rolledLogPath = QDir::toNativeSeparators(newFilename);
            crashReporter.mpSender.sendAdditionalFile(qPrintable(rolledLogPath));
        });
#endif

        printSystemInformation();

        QTranslator translator;
        translator.load("i18n/interface_en");
        app.installTranslator(&translator);
        qCDebug(interfaceapp, "Created QT Application.");
        exitCode = app.exec();
        server.close();
    }

    Application::shutdownPlugins();

    qCDebug(interfaceapp, "Normal exit.");
#if !defined(DEBUG) && !defined(Q_OS_LINUX)
    // HACK: exit immediately (don't handle shutdown callbacks) for Release build
    _exit(exitCode);
#endif
    return exitCode;
}
