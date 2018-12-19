//
//  AvatarProject.cpp
//
//
//  Created by Thijs Wenker on 12/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarProject.h"

#include <FSTReader.h>

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>
#include <QQmlEngine>

#include <ui/TabletScriptingInterface.h>
#include "scripting/HMDScriptingInterface.h"

AvatarProject* AvatarProject::openAvatarProject(const QString& path) {
    const auto pathToLower = path.toLower();
    if (pathToLower.endsWith(".fst")) {
        QFile file{ path };
        if (!file.open(QIODevice::ReadOnly)) {
            return nullptr;
        }
        auto project = new AvatarProject(path, file.readAll());
        QQmlEngine::setObjectOwnership(project, QQmlEngine::CppOwnership);
        return project;
    }

    if (pathToLower.endsWith(".fbx")) {
        // TODO: Create FST here:
    }

    return nullptr;
}

AvatarProject::AvatarProject(const QString& fstPath, const QByteArray& data) :
    _fstPath(fstPath), _fst(fstPath, FSTReader::readMapping(data)) {
    _fstFilename = QFileInfo(_fstPath).fileName();
    qDebug() << "Pointers: " << this << &_fst;

    _directory = QFileInfo(_fstPath).absoluteDir();

    //_projectFiles = _directory.entryList();
    refreshProjectFiles();

    auto fileInfo = QFileInfo(_fstPath);
    _projectPath = fileInfo.absoluteDir().absolutePath();
}

void AvatarProject::appendDirectory(QString prefix, QDir dir) {
    auto flags = QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Hidden;
    for (auto& entry : dir.entryInfoList({}, flags)) {
        if (entry.isFile()) {
            //_projectFiles.append(prefix + "/" + entry.fileName());
            _projectFiles.append(entry.absoluteFilePath());
        } else if (entry.isDir()) {
            appendDirectory(prefix + dir.dirName() + "/", entry.absoluteFilePath());
        }
    }
}

void AvatarProject::refreshProjectFiles() {
    _projectFiles.clear();
    appendDirectory("", _directory);
}

MarketplaceItemUploader* AvatarProject::upload() {
    return new MarketplaceItemUploader("test_avatar", "blank description", _fstFilename, QUuid(), _projectFiles);
}

void AvatarProject::openInInventory() {
    auto tablet = dynamic_cast<TabletProxy*>(
        DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
    tablet->loadQMLSource("hifi/commerce/wallet/Wallet.qml");
    DependencyManager::get<HMDScriptingInterface>()->openTablet();
    tablet->sendToQml(QVariantMap({
        { "method", "updatePurchases" },
        { "filterText", "filtertext" } }));
}
