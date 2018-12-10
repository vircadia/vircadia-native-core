//
//  AvatarPackager.cpp
//
//
//  Created by Thijs Wenker on 12/6/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarPackager.h"

#include <QQmlContext>
#include <QStandardPaths>
#include <QUrl>

#include <OffscreenUi.h>
#include "ModelSelector.h"

bool AvatarPackager::open() {
    static const QUrl url{ "hifi/AvatarPackager.qml" };

    const auto packageModelDialogCreated = [=](QQmlContext* context, QObject* newObject) {
        context->setContextProperty("AvatarPackagerCore", this);
    };
    DependencyManager::get<OffscreenUi>()->show(url, "AvatarPackager", packageModelDialogCreated);
    return true;
}

QObject* AvatarPackager::openAvatarProject() {
    // TODO: use QML file browser here, could handle this on QML side instead
    static Setting::Handle<QString> lastModelBrowseLocation("LastModelBrowseLocation",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    const QString filename = QFileDialog::getOpenFileName(nullptr, "Select your model file ...",
        lastModelBrowseLocation.get(),
        "Model files (*.fst *.fbx)");
    QFileInfo fileInfo(filename);

    if (fileInfo.isFile() && fileInfo.completeSuffix().contains(QRegExp("fst|fbx|FST|FBX"))) {
        return AvatarProject::openAvatarProject(fileInfo.absoluteFilePath());
    }
    return nullptr;
}
