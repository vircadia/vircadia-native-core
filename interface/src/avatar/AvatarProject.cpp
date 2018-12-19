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
#include <QDebug>
#include <QQmlEngine>
#include "FBXSerializer.h"

#include <ui/TabletScriptingInterface.h>
#include "scripting/HMDScriptingInterface.h"

AvatarProject* AvatarProject::openAvatarProject(const QString& path) {
    if (!path.toLower().endsWith(".fst")) {
        return nullptr;
    }
    QFile file{ path };
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    const auto project = new AvatarProject(path, file.readAll());
    QQmlEngine::setObjectOwnership(project, QQmlEngine::CppOwnership);
    return project;
}

AvatarProject* AvatarProject::createAvatarProject(const QString& avatarProjectName, const QString& avatarModelPath) {
    if (!isValidNewProjectName(avatarProjectName)) {
        return nullptr;
    }
    QDir dir(getDefaultProjectsPath() + "/" + avatarProjectName);
    if (!dir.mkpath(".")) {
        return nullptr;
    }
    const auto fileName = QFileInfo(avatarModelPath).fileName();
    const auto newModelPath = dir.absoluteFilePath(fileName);
    const auto newFSTPath = dir.absoluteFilePath("avatar.fst");
    QFile::copy(avatarModelPath, newModelPath);

    QFileInfo fbxInfo(newModelPath);
    QFile fbx(fbxInfo.filePath());
    if (!fbxInfo.exists() || !fbxInfo.isFile() || !fbx.open(QIODevice::ReadOnly)) {
        // TODO: Can't open model FBX (throw error here)
        return nullptr;
    }

    std::shared_ptr<hfm::Model> hfmModel;


    try {
        qDebug() << "Reading FBX file : " << fbxInfo.filePath();
        const QByteArray fbxContents = fbx.readAll();
        hfmModel = FBXSerializer().read(fbxContents, QVariantHash(), fbxInfo.filePath());
    }
    catch (const QString& error) {
        qDebug() << "Error reading: " << error;
        return nullptr;
    }
    //TODO: copy/fix textures here:



    FST* fst = FST::createFSTFromModel(newFSTPath, newModelPath, *hfmModel);

    fst->setName(avatarProjectName);

    if (!fst->write()) {
        return nullptr;
    }

    return new AvatarProject(fst);
}

bool AvatarProject::isValidNewProjectName(const QString& projectName) {
    QDir dir(getDefaultProjectsPath() + "/" + projectName);
    return !dir.exists();
}

AvatarProject::AvatarProject(const QString& fstPath, const QByteArray& data) :
    AvatarProject::AvatarProject(new FST(fstPath, FSTReader::readMapping(data))) {

}
AvatarProject::AvatarProject(FST* fst) {
    _fst = fst;
    auto fileInfo = QFileInfo(getFSTPath());
    _directory = fileInfo.absoluteDir();

    //_projectFiles = _directory.entryList();
    refreshProjectFiles();

    _projectPath = fileInfo.absoluteDir().absolutePath();
}

void AvatarProject::appendDirectory(QString prefix, QDir dir) {
    constexpr auto flags = QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Hidden;
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
    return new MarketplaceItemUploader("test_avatar", "blank description", QFileInfo(getFSTPath()).fileName(), QUuid(), _projectFiles);
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
