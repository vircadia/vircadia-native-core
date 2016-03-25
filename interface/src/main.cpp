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
#include <QSysInfo>
#include <QTranslator>

#include <gl/OpenGLVersionChecker.h>
#include <SharedUtil.h>

#include "AddressManager.h"
#include "Application.h"
#include "InterfaceLogging.h"
#include "MainWindow.h"
#include <QtCore/QProcess>

#ifdef HAS_BUGSPLAT
#include <BuildInfo.h>
#include <BugSplat.h>
#endif


#ifdef Q_OS_WIN
#include <CPUID.h>
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
    {
        OpenGLVersionChecker openGLVersionChecker(argc, const_cast<char**>(argv));
        if (!openGLVersionChecker.isValidVersion()) {
            qCDebug(interfaceapp, "Early exit due to OpenGL version.");
            return 0;
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
        {
            // Write system information to log
            qDebug() << "Build Information";
            qDebug().noquote() << "\tBuild ABI: " << QSysInfo::buildAbi();
            qDebug().noquote() << "\tBuild CPU Architecture: " << QSysInfo::buildCpuArchitecture();

            qDebug().noquote() << "System Information";
            qDebug().noquote() << "\tProduct Name: " << QSysInfo::prettyProductName();
            qDebug().noquote() << "\tCPU Architecture: " << QSysInfo::currentCpuArchitecture();
            qDebug().noquote() << "\tKernel Type: " << QSysInfo::kernelType();
            qDebug().noquote() << "\tKernel Version: " << QSysInfo::kernelVersion();

            auto macVersion = QSysInfo::macVersion();
            if (macVersion != QSysInfo::MV_None) {
                qDebug() << "\tMac Version: " << macVersion;
            }

            auto windowsVersion = QSysInfo::windowsVersion();
            if (windowsVersion != QSysInfo::WV_None) {
                qDebug() << "\tWindows Version: " << windowsVersion;
            }

#ifdef Q_OS_WIN
            SYSTEM_INFO si;
            GetNativeSystemInfo(&si);

            qDebug() << "SYSTEM_INFO";
            qDebug().noquote() << "\tOEM ID: " << si.dwOemId;
            qDebug().noquote() << "\tProcessor Architecture: " << si.wProcessorArchitecture;
            qDebug().noquote() << "\tProcessor Type: " << si.dwProcessorType;
            qDebug().noquote() << "\tProcessor Level: " << si.wProcessorLevel;
            qDebug().noquote() << "\tProcessor Revision: "
                << QString("0x%1").arg(si.wProcessorRevision, 4, 16, QChar('0'));
            qDebug().noquote() << "\tNumber of Processors: " << si.dwNumberOfProcessors;
            qDebug().noquote() << "\tPage size: " << si.dwPageSize << " Bytes";
            qDebug().noquote() << "\tMin Application Address: "
                               << QString("0x%1").arg(qulonglong(si.lpMinimumApplicationAddress), 16, 16, QChar('0'));
            qDebug().noquote() << "\tMax Application Address: "
                               << QString("0x%1").arg(qulonglong(si.lpMaximumApplicationAddress), 16, 16, QChar('0'));

            const double BYTES_TO_MEGABYTE = 1.0 / (1024 * 1024);

            qDebug() << "MEMORYSTATUSEX";
            MEMORYSTATUSEX ms;
            ms.dwLength = sizeof(ms);
            if (GlobalMemoryStatusEx(&ms)) {
                qDebug().noquote() << QString("\tCurrent System Memory Usage: %1%").arg(ms.dwMemoryLoad);
                qDebug().noquote() << QString("\tAvail Physical Memory: %1 MB").arg(ms.ullAvailPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
                qDebug().noquote() << QString("\tTotal Physical Memory: %1 MB").arg(ms.ullTotalPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
                qDebug().noquote() << QString("\tAvail in Page File:    %1 MB").arg(ms.ullAvailPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
                qDebug().noquote() << QString("\tTotal in Page File:    %1 MB").arg(ms.ullTotalPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
                qDebug().noquote() << QString("\tAvail Virtual Memory:  %1 MB").arg(ms.ullAvailVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
                qDebug().noquote() << QString("\tTotal Virtual Memory:  %1 MB").arg(ms.ullTotalVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
            } else {
                qDebug() << "\tFailed to retrieve memory status: " << GetLastError();
            }

            qDebug() << "CPUID";

            auto printSupported = [](QString isaFeatureName, bool isSupported) {
                qDebug().nospace().noquote() << "\t[" << (isSupported ? "x" : " ") << "] " << isaFeatureName;
            };

            qDebug() << "\tCPU Vendor: " << CPUID::Vendor().c_str();
            qDebug() << "\tCPU Brand:  " << CPUID::Brand().c_str();

            printSupported("3DNOW", CPUID::_3DNOW());
            printSupported("3DNOWEXT", CPUID::_3DNOWEXT());
            printSupported("ABM", CPUID::ABM());
            printSupported("ADX", CPUID::ADX());
            printSupported("AES", CPUID::AES());
            printSupported("AVX", CPUID::AVX());
            printSupported("AVX2", CPUID::AVX2());
            printSupported("AVX512CD", CPUID::AVX512CD());
            printSupported("AVX512ER", CPUID::AVX512ER());
            printSupported("AVX512F", CPUID::AVX512F());
            printSupported("AVX512PF", CPUID::AVX512PF());
            printSupported("BMI1", CPUID::BMI1());
            printSupported("BMI2", CPUID::BMI2());
            printSupported("CLFSH", CPUID::CLFSH());
            printSupported("CMPXCHG16B", CPUID::CMPXCHG16B());
            printSupported("CX8", CPUID::CX8());
            printSupported("ERMS", CPUID::ERMS());
            printSupported("F16C", CPUID::F16C());
            printSupported("FMA", CPUID::FMA());
            printSupported("FSGSBASE", CPUID::FSGSBASE());
            printSupported("FXSR", CPUID::FXSR());
            printSupported("HLE", CPUID::HLE());
            printSupported("INVPCID", CPUID::INVPCID());
            printSupported("LAHF", CPUID::LAHF());
            printSupported("LZCNT", CPUID::LZCNT());
            printSupported("MMX", CPUID::MMX());
            printSupported("MMXEXT", CPUID::MMXEXT());
            printSupported("MONITOR", CPUID::MONITOR());
            printSupported("MOVBE", CPUID::MOVBE());
            printSupported("MSR", CPUID::MSR());
            printSupported("OSXSAVE", CPUID::OSXSAVE());
            printSupported("PCLMULQDQ", CPUID::PCLMULQDQ());
            printSupported("POPCNT", CPUID::POPCNT());
            printSupported("PREFETCHWT1", CPUID::PREFETCHWT1());
            printSupported("RDRAND", CPUID::RDRAND());
            printSupported("RDSEED", CPUID::RDSEED());
            printSupported("RDTSCP", CPUID::RDTSCP());
            printSupported("RTM", CPUID::RTM());
            printSupported("SEP", CPUID::SEP());
            printSupported("SHA", CPUID::SHA());
            printSupported("SSE", CPUID::SSE());
            printSupported("SSE2", CPUID::SSE2());
            printSupported("SSE3", CPUID::SSE3());
            printSupported("SSE4.1", CPUID::SSE41());
            printSupported("SSE4.2", CPUID::SSE42());
            printSupported("SSE4a", CPUID::SSE4a());
            printSupported("SSSE3", CPUID::SSSE3());
            printSupported("SYSCALL", CPUID::SYSCALL());
            printSupported("TBM", CPUID::TBM());
            printSupported("XOP", CPUID::XOP());
            printSupported("XSAVE", CPUID::XSAVE());
#endif

            qDebug() << "Environment Variables";
            auto envVariables = QProcessEnvironment::systemEnvironment().toStringList();
            for (auto& env : envVariables) {
                qDebug().noquote().nospace() << "\t" << env;
            }
        }

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
