//
//  DomainBaker.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/12/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainBaker_h
#define hifi_DomainBaker_h

#include <QtCore/QJsonArray>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QThread>

#include "Baker.h"
#include "ModelBaker.h"
#include "TextureBaker.h"
#include "JSBaker.h"
#include "MaterialBaker.h"

class DomainBaker : public Baker {
    Q_OBJECT
public:
    // This is a real bummer, but the FBX SDK is not thread safe - even with separate FBXManager objects.
    // This means that we need to put all of the FBX importing/exporting from the same process on the same thread.
    // That means you must pass a usable running QThread when constructing a domain baker.
    DomainBaker(const QUrl& localEntitiesFileURL, const QString& domainName,
                const QString& baseOutputPath, const QUrl& destinationPath);

signals:
    void allModelsFinished();
    void bakeProgress(int baked, int total);

private slots:
    virtual void bake() override;
    void handleFinishedModelBaker();
    void handleFinishedTextureBaker();
    void handleFinishedScriptBaker();
    void handleFinishedMaterialBaker();

private:
    void setupOutputFolder();
    void loadLocalFile();
    void enumerateEntities();
    void checkIfRewritingComplete();
    void writeNewEntitiesFile();

    QUrl _localEntitiesFileURL;
    QString _domainName;
    QString _baseOutputPath;
    QString _uniqueOutputPath;
    QString _contentOutputPath;
    QString _bakedOutputPath;
    QString _originalOutputPath;
    QUrl _destinationPath;

    QJsonArray _entities;

    QHash<QUrl, QSharedPointer<ModelBaker>> _modelBakers;
    QHash<QUrl, QSharedPointer<TextureBaker>> _textureBakers;
    QHash<QUrl, QSharedPointer<JSBaker>> _scriptBakers;
    QHash<QUrl, QSharedPointer<MaterialBaker>> _materialBakers;
    
    QMultiHash<QUrl, std::pair<QString, QJsonValueRef>> _entitiesNeedingRewrite;

    int _totalNumberOfSubBakes { 0 };
    int _completedSubBakes { 0 };

    void addModelBaker(const QString& property, const QString& url, QJsonValueRef& jsonRef);
    void addTextureBaker(const QString& property, const QString& url, image::TextureUsage::Type type, QJsonValueRef& jsonRef);
    void addScriptBaker(const QString& property, const QString& url, QJsonValueRef& jsonRef);
    void addMaterialBaker(const QString& property, const QString& data, bool isURL, QJsonValueRef& jsonRef);
};

#endif // hifi_DomainBaker_h
