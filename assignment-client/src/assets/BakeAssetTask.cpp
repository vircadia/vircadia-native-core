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

#include <QtCore/QThread>

#include <FBXBaker.h>
#include <PathUtils.h>
#include <JSBaker.h>

BakeAssetTask::BakeAssetTask(const AssetHash& assetHash, const AssetPath& assetPath, const QString& filePath) :
    _assetHash(assetHash),
    _assetPath(assetPath),
    _filePath(filePath)
{

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
    _isBaking.store(true);

    qRegisterMetaType<QVector<QString> >("QVector<QString>");
    TextureBakerThreadGetter fn = []() -> QThread* { return QThread::currentThread();  };

    QString tempOutputDir;

    if (_assetPath.endsWith(".fbx")) {
        tempOutputDir = PathUtils::generateTemporaryDir();
        _baker = std::unique_ptr<FBXBaker> {
            new FBXBaker(QUrl("file:///" + _filePath), fn, tempOutputDir)
        };
    } else if (_assetPath.endsWith(".js", Qt::CaseInsensitive)) {
        _baker = std::unique_ptr<JSBaker>{
            new JSBaker(QUrl("file:///" + _filePath), PathUtils::generateTemporaryDir())
        };
    } else {
        tempOutputDir = PathUtils::generateTemporaryDir();
        _baker = std::unique_ptr<TextureBaker> {
            new TextureBaker(QUrl("file:///" + _filePath), image::TextureUsage::CUBE_TEXTURE,
                             tempOutputDir)
        };
    }

    QEventLoop loop;
    connect(_baker.get(), &Baker::finished, &loop, &QEventLoop::quit);
    connect(_baker.get(), &Baker::aborted, &loop, &QEventLoop::quit);
    QMetaObject::invokeMethod(_baker.get(), "bake", Qt::QueuedConnection);
    loop.exec();

    if (_baker->wasAborted()) {
        qDebug() << "Aborted baking: " << _assetHash << _assetPath;

        _wasAborted.store(true);

        cleanupTempFiles(tempOutputDir, _baker->getOutputFiles());

        emit bakeAborted(_assetHash, _assetPath);
    } else if (_baker->hasErrors()) {
        qDebug() << "Failed to bake: " << _assetHash << _assetPath << _baker->getErrors();

        auto errors = _baker->getErrors().join('\n'); // Join error list into a single string for convenience

        _didFinish.store(true);

        cleanupTempFiles(tempOutputDir, _baker->getOutputFiles());

        emit bakeFailed(_assetHash, _assetPath, errors);
    } else {
        auto vectorOutputFiles = QVector<QString>::fromStdVector(_baker->getOutputFiles());

        qDebug() << "Finished baking: " << _assetHash << _assetPath << vectorOutputFiles;

        _didFinish.store(true);

        emit bakeComplete(_assetHash, _assetPath, tempOutputDir, vectorOutputFiles);
    }
}

void BakeAssetTask::abort() {
    if (_baker) {
        _baker->abort();
    }
}
