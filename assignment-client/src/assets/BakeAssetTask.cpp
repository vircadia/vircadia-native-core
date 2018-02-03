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

void cleanupTempFiles(QString tempOutputDir, std::vector<QString> files) {
    for (const auto& filename : files) {
        QFile f { filename };
        if (!f.remove()) {
            qDebug() << "Failed to remove:" << filename;
        }
    }
    if (!tempOutputDir.isEmpty()) {
        QDir dir { tempOutputDir };
        if (!dir.rmdir(".")) {
            qDebug() << "Failed to remove temporary directory:" << tempOutputDir;
        }
    }
};

void BakeAssetTask::run() {
    if (_isBaking.exchange(true)) {
        qWarning() << "Tried to start bake asset task while already baking";
        return;
    }

    QString tempOutputDir = PathUtils::generateTemporaryDir();
    auto base = QFileInfo(QCoreApplication::applicationFilePath()).absoluteDir();
    QString path = base.absolutePath() + "/oven";
    QString extension = _assetPath.mid(_assetPath.lastIndexOf('.') + 1);
    QStringList args {
        "-i", _filePath,
        "-o", tempOutputDir,
        "-t", extension,
    };

    _ovenProcess.reset(new QProcess());

    connect(_ovenProcess.get(), static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, tempOutputDir](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Baking process finished: " << exitCode << exitStatus;

        if (exitStatus == QProcess::CrashExit) {
            if (_wasAborted) {
                emit bakeAborted(_assetHash, _assetPath);
            } else {
                QString errors = "Fatal error occurred while baking";
                emit bakeFailed(_assetHash, _assetPath, errors);
            }
        } else if (exitCode == OVEN_STATUS_CODE_SUCCESS) {
            QDir outputDir = tempOutputDir;
            auto files = outputDir.entryInfoList(QDir::Files);
            QVector<QString> outputFiles;
            for (auto& file : files) {
                outputFiles.push_back(file.absoluteFilePath());
            }

            emit bakeComplete(_assetHash, _assetPath, tempOutputDir, outputFiles);
        } else if (exitStatus == QProcess::NormalExit && exitCode == OVEN_STATUS_CODE_ABORT) {
            _wasAborted.store(true);
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
            emit bakeFailed(_assetHash, _assetPath, errors);
        }

    });

    qDebug() << "Starting oven for " << _assetPath;
    _ovenProcess->start(path, args, QIODevice::ReadOnly);
    if (!_ovenProcess->waitForStarted(-1)) {
        QString errors = "Oven process failed to start";
        emit bakeFailed(_assetHash, _assetPath, errors);
        return;
    }
    _ovenProcess->waitForFinished();
}

void BakeAssetTask::abort() {
    if (!_wasAborted.exchange(true)) {
        _ovenProcess->terminate();
    }
}
