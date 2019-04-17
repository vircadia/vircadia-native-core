//
//  BakeAssetTask.cpp
//  assignment-client/src/assets
//
//  Created by Stephen Birarda on 9/18/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BakeAssetTask.h"

#include <mutex>

#include <QtCore/QThread>
#include <QCoreApplication>

#include <PathUtils.h>

static const int OVEN_STATUS_CODE_SUCCESS { 0 };
static const int OVEN_STATUS_CODE_FAIL { 1 };
static const int OVEN_STATUS_CODE_ABORT { 2 };

std::once_flag registerMetaTypesFlag;

BakeAssetTask::BakeAssetTask(const AssetUtils::AssetHash& assetHash, const AssetUtils::AssetPath& assetPath, const QString& filePath) :
    _assetHash(assetHash),
    _assetPath(assetPath),
    _filePath(filePath)
{

    std::call_once(registerMetaTypesFlag, []() {
        qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    });
}

void BakeAssetTask::run() {
    if (_isBaking.exchange(true)) {
        qWarning() << "Tried to start bake asset task while already baking";
        return;
    }

    // Make a new temporary directory for the Oven to work in
    QString tempOutputDir = PathUtils::generateTemporaryDir();
    QString tempOutputDirName = QDir(tempOutputDir).dirName();
    if (tempOutputDir.isEmpty()) {
        QString errors = "Could not create temporary working directory";
        emit bakeFailed(_assetHash, _assetPath, errors);
        PathUtils::deleteMyTemporaryDir(tempOutputDirName);
        return;
    }

    // Copy file to bake the temporary dir and give a name the oven can work with
    auto assetName = _assetPath.split("/").last();
    auto tempAssetPath = tempOutputDir + "/" + assetName;
    auto sucess = QFile::copy(_filePath, tempAssetPath);
    if (!sucess) {
        QString errors = "Couldn't copy file to bake to temporary directory";
        emit bakeFailed(_assetHash, _assetPath, errors);
        PathUtils::deleteMyTemporaryDir(tempOutputDirName);
        return;
    }

    auto base = QFileInfo(QCoreApplication::applicationFilePath()).absoluteDir();
    QString path = base.absolutePath() + "/oven";
    QString extension = _assetPath.mid(_assetPath.lastIndexOf('.') + 1);
    QStringList args {
        "-i", tempAssetPath,
        "-o", tempOutputDir,
        "-t", extension,
    };

    _ovenProcess.reset(new QProcess());

    QEventLoop loop;

    connect(_ovenProcess.get(), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [&loop, this, tempOutputDir, tempAssetPath, tempOutputDirName](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Baking process finished: " << exitCode << exitStatus;

        if (exitStatus == QProcess::CrashExit) {
            PathUtils::deleteMyTemporaryDir(tempOutputDirName);
            if (_wasAborted) {
                emit bakeAborted(_assetHash, _assetPath);
            } else {
                QString errors = "Fatal error occurred while baking";
                emit bakeFailed(_assetHash, _assetPath, errors);
            }
        } else if (exitCode == OVEN_STATUS_CODE_SUCCESS) {
            emit bakeComplete(_assetHash, _assetPath, tempOutputDir);
        } else if (exitStatus == QProcess::NormalExit && exitCode == OVEN_STATUS_CODE_ABORT) {
            _wasAborted.store(true);
            PathUtils::deleteMyTemporaryDir(tempOutputDirName);
            emit bakeAborted(_assetHash, _assetPath);
        } else {
            QString errors;
            if (exitCode == OVEN_STATUS_CODE_FAIL) {
                QDir outputDir = tempOutputDir;
                auto errorFilePath = outputDir.absoluteFilePath("errors.txt");
                QFile errorFile { errorFilePath };
                if (errorFile.open(QIODevice::ReadOnly)) {
                    errors = errorFile.readAll();
                    errorFile.close();
                } else {
                    errors = "Unknown error occurred while baking";
                }
            }
            PathUtils::deleteMyTemporaryDir(tempOutputDirName);
            emit bakeFailed(_assetHash, _assetPath, errors);
        }

        loop.quit();
    });

    qDebug() << "Starting oven for " << _assetPath;
    _ovenProcess->start(path, args, QIODevice::ReadOnly);
    qDebug() << "Running:" << path << args;
    if (!_ovenProcess->waitForStarted()) {
        PathUtils::deleteMyTemporaryDir(tempOutputDirName);

        QString errors = "Oven process failed to start";
        emit bakeFailed(_assetHash, _assetPath, errors);
        return;
    }

    _isBaking = true;

    loop.exec();
}

void BakeAssetTask::abort() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "abort");
        return;
    }
    qDebug() << "Aborting BakeAssetTask for" << _assetHash;
    if (_ovenProcess->state() != QProcess::NotRunning) {
        qDebug() << "Teminating oven process for" << _assetHash;
        _wasAborted = true;
        _ovenProcess->terminate();
    }
}
