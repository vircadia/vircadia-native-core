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
#include <QTimer>

#include "FBXSerializer.h"
#include <ui/TabletScriptingInterface.h>
#include <graphics/TextureMap.h>
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

AvatarProject* AvatarProject::createAvatarProject(const QString& projectsFolder, const QString& avatarProjectName, const QString& avatarModelPath, const QString& textureFolder) {
    if (!isValidNewProjectName(avatarProjectName)) {
        return nullptr;
    }
    QDir projectDir(projectsFolder + "/" + avatarProjectName);
    if (!projectDir.mkpath(".")) {
        return nullptr;
    }
    QDir projectTexturesDir(projectDir.path() + "/textures");
    if (!projectTexturesDir.mkpath(".")) {
        return nullptr;
    }
    QDir projectScriptsDir(projectDir.path() + "/scripts");
    if (!projectScriptsDir.mkpath(".")) {
        return nullptr;
    }
    const auto fileName = QFileInfo(avatarModelPath).fileName();
    const auto newModelPath = projectDir.absoluteFilePath(fileName);
    const auto newFSTPath = projectDir.absoluteFilePath("avatar.fst");
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
    } catch (const QString& error) {
        qDebug() << "Error reading: " << error;
        return nullptr;
    }
    QStringList textures{};

    auto addTextureToList = [&textures](hfm::Texture texture) mutable {
        if (!texture.filename.isEmpty() && texture.content.isEmpty() && !textures.contains(texture.filename)) {
            textures << texture.filename;
        }
    };

    foreach(const HFMMaterial mat, hfmModel->materials) {
        addTextureToList(mat.normalTexture);
        addTextureToList(mat.albedoTexture);
        addTextureToList(mat.opacityTexture);
        addTextureToList(mat.glossTexture);
        addTextureToList(mat.roughnessTexture);
        addTextureToList(mat.specularTexture);
        addTextureToList(mat.metallicTexture);
        addTextureToList(mat.emissiveTexture);
        addTextureToList(mat.occlusionTexture);
        addTextureToList(mat.scatteringTexture);
        addTextureToList(mat.lightmapTexture);
    }

    QDir textureDir(textureFolder.isEmpty() ? fbxInfo.absoluteDir() : textureFolder);

    for (const auto& texture : textures) {
        QString sourcePath = textureDir.path() + "/" + texture;
        QString targetPath = projectTexturesDir.path() + "/" + texture;

        QFileInfo sourceTexturePath(sourcePath);
        if (sourceTexturePath.exists()) {
            QFile::copy(sourcePath, targetPath);
        }
    }

    auto fst = FST::createFSTFromModel(newFSTPath, newModelPath, *hfmModel);
    fst->setName(avatarProjectName);

    if (!fst->write()) {
        return nullptr;
    }

    return new AvatarProject(fst);
}

QStringList AvatarProject::getScriptPaths(const QDir& scriptsDir) {
    QStringList result{};
    constexpr auto flags = QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Hidden;
    if (!scriptsDir.exists()) {
        return result;
    }

    for (auto& script : scriptsDir.entryInfoList({}, flags)) {
        if (script.fileName().endsWith(".js")) {
            result.push_back("scripts/" + script.fileName());
        }
    }

    return result;
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

    _fst->setScriptPaths(getScriptPaths(QDir(_directory.path() + "/scripts")));
    _fst->write();

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

MarketplaceItemUploader* AvatarProject::upload(bool updateExisting) {
    QUuid itemID;
    if (updateExisting) {
        itemID = _fst->getMarketplaceID();
    }
    auto uploader = new MarketplaceItemUploader(getProjectName(), "Empty description", QFileInfo(getFSTPath()).fileName(), itemID,
                                       _projectFiles);
    connect(uploader, &MarketplaceItemUploader::completed, this, [this, uploader]() {
        if (uploader->getError() == MarketplaceItemUploader::Error::None) {
            _fst->setMarketplaceID(uploader->getMarketplaceID());
            _fst->write();
        }
    });

    return uploader;
}

void AvatarProject::openInInventory() {
    auto tablet = dynamic_cast<TabletProxy*>(
        DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
    tablet->loadQMLSource("hifi/commerce/wallet/Wallet.qml");
    DependencyManager::get<HMDScriptingInterface>()->openTablet();
    auto name = getProjectName();

    // I'm not a fan of this, but it's the only current option.
    QTimer::singleShot(1000, [name]() {
        auto tablet = dynamic_cast<TabletProxy*>(
            DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
        tablet->sendToQml(QVariantMap({ { "method", "updatePurchases" }, { "filterText", name } }));
    });
}
