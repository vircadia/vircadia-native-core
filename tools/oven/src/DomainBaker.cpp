//
//  DomainBaker.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/12/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainBaker.h"

#include <QtConcurrent>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>

#include "Gzip.h"
#include "Oven.h"
#include "baking/BakerLibrary.h"

DomainBaker::DomainBaker(const QUrl& localModelFileURL, const QString& domainName,
                         const QString& baseOutputPath, const QUrl& destinationPath,
                         bool shouldRebakeOriginals) :
    _localEntitiesFileURL(localModelFileURL),
    _domainName(domainName),
    _baseOutputPath(baseOutputPath),
    _shouldRebakeOriginals(shouldRebakeOriginals)
{
    // make sure the destination path has a trailing slash
    if (!destinationPath.toString().endsWith('/')) {
        _destinationPath = destinationPath.toString() + '/';
    } else {
        _destinationPath = destinationPath;
    }
}

void DomainBaker::bake() {
    setupOutputFolder();

    if (hasErrors()) {
        return;
    }

    loadLocalFile();

    if (hasErrors()) {
        return;
    }

    enumerateEntities();

    if (hasErrors()) {
        return;
    }

    // in case we've baked and re-written all of our entities already, check if we're done
    checkIfRewritingComplete();
}

void DomainBaker::setupOutputFolder() {
    // in order to avoid overwriting previous bakes, we create a special output folder with the domain name and timestamp

    // first, construct the directory name
    auto domainPrefix = !_domainName.isEmpty() ? _domainName + "-" : "";
    auto timeNow = QDateTime::currentDateTime();

    static const QString FOLDER_TIMESTAMP_FORMAT = "yyyyMMdd-hhmmss";
    QString outputDirectoryName = domainPrefix + timeNow.toString(FOLDER_TIMESTAMP_FORMAT);

    //  make sure we can create that directory
    QDir outputDir { _baseOutputPath };

    if (!outputDir.mkpath(outputDirectoryName)) {
        // add an error to specify that the output directory could not be created
        handleError("Could not create output folder");

        return;
    }

    // store the unique output path so we can re-use it when saving baked models
    outputDir.cd(outputDirectoryName);
    _uniqueOutputPath = outputDir.absolutePath();

    // add a content folder inside the unique output folder
    static const QString CONTENT_OUTPUT_FOLDER_NAME = "content";
    if (!outputDir.mkpath(CONTENT_OUTPUT_FOLDER_NAME)) {
        // add an error to specify that the content output directory could not be created
        handleError("Could not create content folder");
        return;
    }

    _contentOutputPath = outputDir.absoluteFilePath(CONTENT_OUTPUT_FOLDER_NAME);
}

const QString ENTITIES_OBJECT_KEY = "Entities";

void DomainBaker::loadLocalFile() {
    // load up the local entities file
    QFile entitiesFile { _localEntitiesFileURL.toLocalFile() };

    // first make a copy of the local entities file in our output folder
    if (!entitiesFile.copy(_uniqueOutputPath + "/" + "original-" + _localEntitiesFileURL.fileName())) {
        // add an error to our list to specify that the file could not be copied
        handleError("Could not make a copy of entities file");

        // return to stop processing
        return;
    }

    if (!entitiesFile.open(QIODevice::ReadOnly)) {
        // add an error to our list to specify that the file could not be read
        handleError("Could not open entities file");

        // return to stop processing
        return;
    }

    // grab a byte array from the file
    auto fileContents = entitiesFile.readAll();

    // check if we need to inflate a gzipped models file or if this was already decompressed
    static const QString GZIPPED_ENTITIES_FILE_SUFFIX = "gz";
    if (QFileInfo(_localEntitiesFileURL.toLocalFile()).suffix() == "gz") {
        // this was a gzipped models file that we need to decompress
        QByteArray uncompressedContents;
        gunzip(fileContents, uncompressedContents);
        fileContents = uncompressedContents;
    }

    // read the file contents to a JSON document
    _json = QJsonDocument::fromJson(fileContents);

    // grab the entities object from the root JSON object
    _entities = _json.object()[ENTITIES_OBJECT_KEY].toArray();

    if (_entities.isEmpty()) {
        // add an error to our list stating that the models file was empty

        // return to stop processing
        return;
    }
}

void DomainBaker::addModelBaker(const QString& property, const QString& url, const QJsonValueRef& jsonRef) {
    // grab a QUrl for the model URL
    QUrl bakeableModelURL = getBakeableModelURL(url);
    if (!bakeableModelURL.isEmpty() && (_shouldRebakeOriginals || !isModelBaked(bakeableModelURL))) {
        // setup a ModelBaker for this URL, as long as we don't already have one
        bool haveBaker = _modelBakers.contains(bakeableModelURL);
        if (!haveBaker) {
            QSharedPointer<ModelBaker> baker = QSharedPointer<ModelBaker>(getModelBaker(bakeableModelURL, _contentOutputPath).release(), &Baker::deleteLater);
            if (baker) {
                // Hold on to the old url userinfo/query/fragment data so ModelBaker::getFullOutputMappingURL retains that data from the original model URL
                // Note: The ModelBaker currently doesn't store this in the FST because the equal signs mess up FST parsing.
                //       There is a small chance this could break a server workflow relying on the old behavior.
                //       Url suffix is still propagated to the baked URL if the input URL is an FST.
                //       Url suffix has always been stripped from the URL when loading the original model file to be baked.
                baker->setOutputURLSuffix(url);

                // make sure our handler is called when the baker is done
                connect(baker.data(), &Baker::finished, this, &DomainBaker::handleFinishedModelBaker);

                // insert it into our bakers hash so we hold a strong pointer to it
                _modelBakers.insert(bakeableModelURL, baker);
                haveBaker = true;

                // move the baker to the baker thread
                // and kickoff the bake
                baker->moveToThread(Oven::instance().getNextWorkerThread());
                QMetaObject::invokeMethod(baker.data(), "bake", Qt::QueuedConnection);

                // keep track of the total number of baking entities
                ++_totalNumberOfSubBakes;
            }
        }

        if (haveBaker) {
            // add this QJsonValueRef to our multi hash so that we can easily re-write
            // the model URL to the baked version once the baker is complete
            _entitiesNeedingRewrite.insert(bakeableModelURL, { property, jsonRef });
        }
    }
}

void DomainBaker::addTextureBaker(const QString& property, const QString& url, image::TextureUsage::Type type, const QJsonValueRef& jsonRef) {
    QString cleanURL = QUrl(url).adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment).toDisplayString();
    auto idx = cleanURL.lastIndexOf('.');
    auto extension = idx >= 0 ? cleanURL.mid(idx + 1).toLower() : "";

    if (QImageReader::supportedImageFormats().contains(extension.toLatin1())) {
        // grab a clean version of the URL without a query or fragment
        QUrl textureURL = QUrl(url).adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);
        TextureKey key = { textureURL, type };

        // setup a texture baker for this URL, as long as we aren't baking a texture already
        if (!_textureBakers.contains(key)) {
            auto baseTextureFileName = _textureFileNamer.createBaseTextureFileName(textureURL.fileName(), type);

            // setup a baker for this texture
            QSharedPointer<TextureBaker> textureBaker {
                new TextureBaker(textureURL, type, _contentOutputPath, baseTextureFileName),
                &TextureBaker::deleteLater
            };

            // make sure our handler is called when the texture baker is done
            connect(textureBaker.data(), &TextureBaker::finished, this, &DomainBaker::handleFinishedTextureBaker);

            // insert it into our bakers hash so we hold a strong pointer to it
            _textureBakers.insert(key, textureBaker);

            // move the baker to a worker thread and kickoff the bake
            textureBaker->moveToThread(Oven::instance().getNextWorkerThread());
            QMetaObject::invokeMethod(textureBaker.data(), "bake", Qt::QueuedConnection);

            // keep track of the total number of baking entities
            ++_totalNumberOfSubBakes;
        }

        // add this QJsonValueRef to our multi hash so that it can re-write the texture URL
        // to the baked version once the baker is complete
        // it doesn't really matter what this key is as long as it's consistent
        _entitiesNeedingRewrite.insert(textureURL.toDisplayString() + "^" + QString::number(type), { property, jsonRef });
    } else {
        qDebug() << "Texture extension not supported: " << extension;
    }
}

void DomainBaker::addScriptBaker(const QString& property, const QString& url, const QJsonValueRef& jsonRef) {
    // grab a clean version of the URL without a query or fragment
    QUrl scriptURL = QUrl(url).adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);

    // setup a script baker for this URL, as long as we aren't baking a script already
    if (!_scriptBakers.contains(scriptURL)) {

        // setup a baker for this script
        QSharedPointer<JSBaker> scriptBaker {
            new JSBaker(scriptURL, _contentOutputPath),
            &JSBaker::deleteLater
        };

        // make sure our handler is called when the script baker is done
        connect(scriptBaker.data(), &JSBaker::finished, this, &DomainBaker::handleFinishedScriptBaker);

        // insert it into our bakers hash so we hold a strong pointer to it
        _scriptBakers.insert(scriptURL, scriptBaker);

        // move the baker to a worker thread and kickoff the bake
        scriptBaker->moveToThread(Oven::instance().getNextWorkerThread());
        QMetaObject::invokeMethod(scriptBaker.data(), "bake", Qt::QueuedConnection);

        // keep track of the total number of baking entities
        ++_totalNumberOfSubBakes;
    }

    // add this QJsonValueRef to our multi hash so that it can re-write the script URL
    // to the baked version once the baker is complete
    _entitiesNeedingRewrite.insert(scriptURL, { property, jsonRef });
}

void DomainBaker::addMaterialBaker(const QString& property, const QString& data, bool isURL, const QJsonValueRef& jsonRef, QUrl destinationPath) {
    // grab a clean version of the URL without a query or fragment
    QString materialData;
    if (isURL) {
        materialData = QUrl(data).adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment).toDisplayString();
    } else {
        materialData = data;
    }

    // setup a material baker for this URL, as long as we aren't baking a material already
    if (!_materialBakers.contains(materialData)) {

        // setup a baker for this material
        QSharedPointer<MaterialBaker> materialBaker {
            new MaterialBaker(materialData, isURL, _contentOutputPath, destinationPath),
            &MaterialBaker::deleteLater
        };

        // make sure our handler is called when the material baker is done
        connect(materialBaker.data(), &MaterialBaker::finished, this, &DomainBaker::handleFinishedMaterialBaker);

        // insert it into our bakers hash so we hold a strong pointer to it
        _materialBakers.insert(materialData, materialBaker);

        // move the baker to a worker thread and kickoff the bake
        materialBaker->moveToThread(Oven::instance().getNextWorkerThread());
        QMetaObject::invokeMethod(materialBaker.data(), "bake", Qt::QueuedConnection);

        // keep track of the total number of baking entities
        ++_totalNumberOfSubBakes;
    }

    // add this QJsonValueRef to our multi hash so that it can re-write the material URL
    // to the baked version once the baker is complete
    _entitiesNeedingRewrite.insert(materialData, { property, jsonRef });
}

// All the Entity Properties that can be baked
// ***************************************************************************************

const QString TYPE_KEY = "type";

// Models
const QString MODEL_URL_KEY = "modelURL";
const QString COMPOUND_SHAPE_URL_KEY = "compoundShapeURL";
const QString GRAB_KEY = "grab";
const QString EQUIPPABLE_INDICATOR_URL_KEY = "equippableIndicatorURL";
const QString ANIMATION_KEY = "animation";
const QString ANIMATION_URL_KEY = "url";

// Textures
const QString TEXTURES_KEY = "textures";
const QString IMAGE_URL_KEY = "imageURL";
const QString X_TEXTURE_URL_KEY = "xTextureURL";
const QString Y_TEXTURE_URL_KEY = "yTextureURL";
const QString Z_TEXTURE_URL_KEY = "zTextureURL";
const QString AMBIENT_LIGHT_KEY = "ambientLight";
const QString AMBIENT_URL_KEY = "ambientURL";
const QString SKYBOX_KEY = "skybox";
const QString SKYBOX_URL_KEY = "url";

// Scripts
const QString SCRIPT_KEY = "script";
const QString SERVER_SCRIPTS_KEY = "serverScripts";

// Materials
const QString MATERIAL_URL_KEY = "materialURL";
const QString MATERIAL_DATA_KEY = "materialData";

// ***************************************************************************************

void DomainBaker::enumerateEntities() {
    qDebug() << "Enumerating" << _entities.size() << "entities from domain";

    for (auto it = _entities.begin(); it != _entities.end(); ++it) {
        // make sure this is a JSON object
        if (it->isObject()) {
            auto entity = it->toObject();

            // Models
            if (entity.contains(MODEL_URL_KEY)) {
                addModelBaker(MODEL_URL_KEY, entity[MODEL_URL_KEY].toString(), *it);
            }
            if (entity.contains(COMPOUND_SHAPE_URL_KEY)) {
                // TODO: Support collision model baking
                // Do not combine mesh parts, otherwise the collision behavior will be different
                // combineParts is currently only used by OBJBaker (mesh-combining functionality ought to be moved to the asset engine at some point), and is also used by OBJBaker to determine if the material library should be loaded (should be separate flag)
                // TODO: this could be optimized so that we don't do the full baking pass for collision shapes,
                // but we have to handle the case where it's also used as a modelURL somewhere
                //addModelBaker(COMPOUND_SHAPE_URL_KEY, entity[COMPOUND_SHAPE_URL_KEY].toString(), *it);
            }
            if (entity.contains(ANIMATION_KEY)) {
                auto animationObject = entity[ANIMATION_KEY].toObject();
                if (animationObject.contains(ANIMATION_URL_KEY)) {
                    addModelBaker(ANIMATION_KEY + "." + ANIMATION_URL_KEY, animationObject[ANIMATION_URL_KEY].toString(), *it);
                }
            }
            if (entity.contains(GRAB_KEY)) {
                auto grabObject = entity[GRAB_KEY].toObject();
                if (grabObject.contains(EQUIPPABLE_INDICATOR_URL_KEY)) {
                    addModelBaker(GRAB_KEY + "." + EQUIPPABLE_INDICATOR_URL_KEY, grabObject[EQUIPPABLE_INDICATOR_URL_KEY].toString(), *it);
                }
            }

            // Textures
            if (entity.contains(TEXTURES_KEY)) {
                if (entity.contains(TYPE_KEY)) {
                    QString type = entity[TYPE_KEY].toString();
                    // TODO: handle textures for model entities
                    if (type == "ParticleEffect" || type == "PolyLine") {
                        addTextureBaker(TEXTURES_KEY, entity[TEXTURES_KEY].toString(), image::TextureUsage::DEFAULT_TEXTURE, *it);
                    }
                }
            }
            if (entity.contains(IMAGE_URL_KEY)) {
                addTextureBaker(IMAGE_URL_KEY, entity[IMAGE_URL_KEY].toString(), image::TextureUsage::DEFAULT_TEXTURE, *it);
            }
            if (entity.contains(X_TEXTURE_URL_KEY)) {
                addTextureBaker(X_TEXTURE_URL_KEY, entity[X_TEXTURE_URL_KEY].toString(), image::TextureUsage::DEFAULT_TEXTURE, *it);
            }
            if (entity.contains(Y_TEXTURE_URL_KEY)) {
                addTextureBaker(Y_TEXTURE_URL_KEY, entity[Y_TEXTURE_URL_KEY].toString(), image::TextureUsage::DEFAULT_TEXTURE, *it);
            }
            if (entity.contains(Z_TEXTURE_URL_KEY)) {
                addTextureBaker(Z_TEXTURE_URL_KEY, entity[Z_TEXTURE_URL_KEY].toString(), image::TextureUsage::DEFAULT_TEXTURE, *it);
            }
            if (entity.contains(AMBIENT_LIGHT_KEY)) {
                auto ambientLight = entity[AMBIENT_LIGHT_KEY].toObject();
                if (ambientLight.contains(AMBIENT_URL_KEY)) {
                    addTextureBaker(AMBIENT_LIGHT_KEY + "." + AMBIENT_URL_KEY, ambientLight[AMBIENT_URL_KEY].toString(), image::TextureUsage::AMBIENT_TEXTURE, *it);
                }
            }
            if (entity.contains(SKYBOX_KEY)) {
                auto skybox = entity[SKYBOX_KEY].toObject();
                if (skybox.contains(SKYBOX_URL_KEY)) {
                    addTextureBaker(SKYBOX_KEY + "." + SKYBOX_URL_KEY, skybox[SKYBOX_URL_KEY].toString(), image::TextureUsage::SKY_TEXTURE, *it);
                }
            }

            // FIXME: disabled for now because it breaks some scripts
            /*
            // Scripts
            if (entity.contains(SCRIPT_KEY)) {
                addScriptBaker(SCRIPT_KEY, entity[SCRIPT_KEY].toString(), *it);
            }
            if (entity.contains(SERVER_SCRIPTS_KEY)) {
                // TODO: serverScripts can be multiple scripts, need to handle that
            }
            */

            // Materials
            if (entity.contains(MATERIAL_URL_KEY)) {
                QString materialURL = entity[MATERIAL_URL_KEY].toString();
                if (!materialURL.startsWith("materialData")) {
                    addMaterialBaker(MATERIAL_URL_KEY, materialURL, true, *it);
                }
            }
            if (entity.contains(MATERIAL_DATA_KEY)) {
                addMaterialBaker(MATERIAL_DATA_KEY, entity[MATERIAL_DATA_KEY].toString(), false, *it, _destinationPath);
            }
        }
    }

    // emit progress now to say we're just starting
    emit bakeProgress(0, _totalNumberOfSubBakes);
}

void DomainBaker::handleFinishedModelBaker() {
    auto baker = qobject_cast<ModelBaker*>(sender());

    if (baker) {
        if (!baker->hasErrors()) {
            // this ModelBaker is done and everything went according to plan
            qDebug() << "Re-writing entity references to" << baker->getModelURL();

            // setup a new URL using the prefix we were passed
            auto relativeMappingFilePath = QDir(_contentOutputPath).relativeFilePath(baker->getFullOutputMappingURL().toString());
            if (relativeMappingFilePath.startsWith("/")) {
                relativeMappingFilePath = relativeMappingFilePath.right(relativeMappingFilePath.length() - 1);
            }

            QUrl newURL = _destinationPath.resolved(relativeMappingFilePath);

            // enumerate the QJsonRef values for the URL of this model from our multi hash of
            // entity objects needing a URL re-write
            for (auto propertyEntityPair : _entitiesNeedingRewrite.values(baker->getOriginalInputModelURL())) {
                QString property = propertyEntityPair.first;
                // convert the entity QJsonValueRef to a QJsonObject so we can modify its URL
                auto entity = propertyEntityPair.second.toObject();

                if (!property.contains(".")) {
                    // grab the old URL
                    QUrl oldURL = entity[property].toString();

                    // set the new URL as the value in our temp QJsonObject
                    // The fragment, query, and user info from the original model URL should now be present on the filename in the FST file
                    entity[property] = newURL.toString();
                } else {
                    // Group property
                    QStringList propertySplit = property.split(".");
                    assert(propertySplit.length() == 2);
                    // grab the old URL
                    auto oldObject = entity[propertySplit[0]].toObject();
                    QUrl oldURL = oldObject[propertySplit[1]].toString();

                    // copy the fragment and query, and user info from the old model URL
                    newURL.setQuery(oldURL.query());
                    newURL.setFragment(oldURL.fragment());
                    newURL.setUserInfo(oldURL.userInfo());

                    // set the new URL as the value in our temp QJsonObject
                    oldObject[propertySplit[1]] = newURL.toString();
                    entity[propertySplit[0]] = oldObject;
                }

                // replace our temp object with the value referenced by our QJsonValueRef
                propertyEntityPair.second = entity;
            }
        } else {
            // this model failed to bake - this doesn't fail the entire bake but we need to add
            // the errors from the model to our warnings
            _warningList << baker->getErrors();
        }

        // remove the baked URL from the multi hash of entities needing a re-write
        _entitiesNeedingRewrite.remove(baker->getOriginalInputModelURL());

        // drop our shared pointer to this baker so that it gets cleaned up
        _modelBakers.remove(baker->getOriginalInputModelURL());

        // emit progress to tell listeners how many models we have baked
        emit bakeProgress(++_completedSubBakes, _totalNumberOfSubBakes);

        // check if this was the last model we needed to re-write and if we are done now
        checkIfRewritingComplete();
    }
}

void DomainBaker::handleFinishedTextureBaker() {
    auto baker = qobject_cast<TextureBaker*>(sender());

    if (baker) {
        QUrl rewriteKey = baker->getTextureURL().toDisplayString() + "^" + QString::number(baker->getTextureType());

        if (!baker->hasErrors()) {
            // this TextureBaker is done and everything went according to plan
            qDebug() << "Re-writing entity references to" << baker->getTextureURL() << "with usage" << baker->getTextureType();

            // setup a new URL using the prefix we were passed
            auto relativeTextureFilePath = QDir(_contentOutputPath).relativeFilePath(baker->getMetaTextureFileName());
            if (relativeTextureFilePath.startsWith("/")) {
                relativeTextureFilePath = relativeTextureFilePath.right(relativeTextureFilePath.length() - 1);
            }
            auto newURL = _destinationPath.resolved(relativeTextureFilePath);

            // enumerate the QJsonRef values for the URL of this texture from our multi hash of
            // entity objects needing a URL re-write
            for (auto propertyEntityPair : _entitiesNeedingRewrite.values(rewriteKey)) {
                QString property = propertyEntityPair.first;
                // convert the entity QJsonValueRef to a QJsonObject so we can modify its URL
                auto entity = propertyEntityPair.second.toObject();

                if (!property.contains(".")) {
                    // grab the old URL
                    QUrl oldURL = entity[property].toString();

                    // copy the fragment and query, and user info from the old texture URL
                    newURL.setQuery(oldURL.query());
                    newURL.setFragment(oldURL.fragment());
                    newURL.setUserInfo(oldURL.userInfo());

                    // set the new URL as the value in our temp QJsonObject
                    entity[property] = newURL.toString();
                } else {
                    // Group property
                    QStringList propertySplit = property.split(".");
                    assert(propertySplit.length() == 2);
                    // grab the old URL
                    auto oldObject = entity[propertySplit[0]].toObject();
                    QUrl oldURL = oldObject[propertySplit[1]].toString();

                    // copy the fragment and query, and user info from the old texture URL
                    newURL.setQuery(oldURL.query());
                    newURL.setFragment(oldURL.fragment());
                    newURL.setUserInfo(oldURL.userInfo());

                    // set the new URL as the value in our temp QJsonObject
                    oldObject[propertySplit[1]] = newURL.toString();
                    entity[propertySplit[0]] = oldObject;
                }

                // replace our temp object with the value referenced by our QJsonValueRef
                propertyEntityPair.second = entity;
            }
        } else {
            // this texture failed to bake - this doesn't fail the entire bake but we need to add the errors from
            // the texture to our warnings
            _warningList << baker->getWarnings();
        }

        // remove the baked URL from the multi hash of entities needing a re-write
        _entitiesNeedingRewrite.remove(rewriteKey);

        // drop our shared pointer to this baker so that it gets cleaned up
        _textureBakers.remove({ baker->getTextureURL(), baker->getTextureType() });

        // emit progress to tell listeners how many textures we have baked
        emit bakeProgress(++_completedSubBakes, _totalNumberOfSubBakes);

        // check if this was the last texture we needed to re-write and if we are done now
        checkIfRewritingComplete();
    }
}

void DomainBaker::handleFinishedScriptBaker() {
    auto baker = qobject_cast<JSBaker*>(sender());

    if (baker) {
        if (!baker->hasErrors()) {
            // this JSBaker is done and everything went according to plan
            qDebug() << "Re-writing entity references to" << baker->getJSPath();

            // setup a new URL using the prefix we were passed
            auto relativeScriptFilePath = QDir(_contentOutputPath).relativeFilePath(baker->getBakedJSFilePath());
            if (relativeScriptFilePath.startsWith("/")) {
                relativeScriptFilePath = relativeScriptFilePath.right(relativeScriptFilePath.length() - 1);
            }
            auto newURL = _destinationPath.resolved(relativeScriptFilePath);

            // enumerate the QJsonRef values for the URL of this script from our multi hash of
            // entity objects needing a URL re-write
            for (auto propertyEntityPair : _entitiesNeedingRewrite.values(baker->getJSPath())) {
                QString property = propertyEntityPair.first;
                // convert the entity QJsonValueRef to a QJsonObject so we can modify its URL
                auto entity = propertyEntityPair.second.toObject();

                if (!property.contains(".")) {
                    // grab the old URL
                    QUrl oldURL = entity[property].toString();

                    // copy the fragment and query, and user info from the old script URL
                    newURL.setQuery(oldURL.query());
                    newURL.setFragment(oldURL.fragment());
                    newURL.setUserInfo(oldURL.userInfo());

                    // set the new URL as the value in our temp QJsonObject
                    entity[property] = newURL.toString();
                } else {
                    // Group property
                    QStringList propertySplit = property.split(".");
                    assert(propertySplit.length() == 2);
                    // grab the old URL
                    auto oldObject = entity[propertySplit[0]].toObject();
                    QUrl oldURL = oldObject[propertySplit[1]].toString();

                    // copy the fragment and query, and user info from the old script URL
                    newURL.setQuery(oldURL.query());
                    newURL.setFragment(oldURL.fragment());
                    newURL.setUserInfo(oldURL.userInfo());

                    // set the new URL as the value in our temp QJsonObject
                    oldObject[propertySplit[1]] = newURL.toString();
                    entity[propertySplit[0]] = oldObject;
                }

                // replace our temp object with the value referenced by our QJsonValueRef
                propertyEntityPair.second = entity;
            }
        } else {
            // this script failed to bake - this doesn't fail the entire bake but we need to add
            // the errors from the script to our warnings
            _warningList << baker->getErrors();
        }

        // remove the baked URL from the multi hash of entities needing a re-write
        _entitiesNeedingRewrite.remove(baker->getJSPath());

        // drop our shared pointer to this baker so that it gets cleaned up
        _scriptBakers.remove(baker->getJSPath());

        // emit progress to tell listeners how many scripts we have baked
        emit bakeProgress(++_completedSubBakes, _totalNumberOfSubBakes);

        // check if this was the last script we needed to re-write and if we are done now
        checkIfRewritingComplete();
    }
}

void DomainBaker::handleFinishedMaterialBaker() {
    auto baker = qobject_cast<MaterialBaker*>(sender());

    if (baker) {
        if (!baker->hasErrors()) {
            // this MaterialBaker is done and everything went according to plan
            qDebug() << "Re-writing entity references to" << baker->getMaterialData();

            QString newDataOrURL;
            if (baker->isURL()) {
                // setup a new URL using the prefix we were passed
                auto relativeMaterialFilePath = QDir(_contentOutputPath).relativeFilePath(baker->getBakedMaterialData());
                if (relativeMaterialFilePath.startsWith("/")) {
                    relativeMaterialFilePath = relativeMaterialFilePath.right(relativeMaterialFilePath.length() - 1);
                }
                newDataOrURL = _destinationPath.resolved(relativeMaterialFilePath).toDisplayString();
            } else {
                newDataOrURL = baker->getBakedMaterialData();
            }

            // enumerate the QJsonRef values for the URL of this material from our multi hash of
            // entity objects needing a URL re-write
            for (auto propertyEntityPair : _entitiesNeedingRewrite.values(baker->getMaterialData())) {
                QString property = propertyEntityPair.first;
                // convert the entity QJsonValueRef to a QJsonObject so we can modify its URL
                auto entity = propertyEntityPair.second.toObject();

                if (!property.contains(".")) {
                    // grab the old URL
                    QUrl oldURL = entity[property].toString();

                    // copy the fragment and query, and user info from the old material data
                    if (baker->isURL()) {
                        QUrl newURL = newDataOrURL;
                        newURL.setQuery(oldURL.query());
                        newURL.setFragment(oldURL.fragment());
                        newURL.setUserInfo(oldURL.userInfo());

                        // set the new URL as the value in our temp QJsonObject
                        entity[property] = newURL.toString();
                    } else {
                        entity[property] = newDataOrURL;
                    }
                } else {
                    // Group property
                    QStringList propertySplit = property.split(".");
                    assert(propertySplit.length() == 2);
                    // grab the old URL
                    auto oldObject = entity[propertySplit[0]].toObject();
                    QUrl oldURL = oldObject[propertySplit[1]].toString();

                    // copy the fragment and query, and user info from the old material data
                    if (baker->isURL()) {
                        QUrl newURL = newDataOrURL;
                        newURL.setQuery(oldURL.query());
                        newURL.setFragment(oldURL.fragment());
                        newURL.setUserInfo(oldURL.userInfo());

                        // set the new URL as the value in our temp QJsonObject
                        oldObject[propertySplit[1]] = newURL.toString();
                        entity[propertySplit[0]] = oldObject;
                    } else {
                        oldObject[propertySplit[1]] = newDataOrURL;
                        entity[propertySplit[0]] = oldObject;
                    }
                }

                // replace our temp object with the value referenced by our QJsonValueRef
                propertyEntityPair.second = entity;
            }
        } else {
            // this material failed to bake - this doesn't fail the entire bake but we need to add
            // the errors from the material to our warnings
            _warningList << baker->getErrors();
        }

        // remove the baked URL from the multi hash of entities needing a re-write
        _entitiesNeedingRewrite.remove(baker->getMaterialData());

        // drop our shared pointer to this baker so that it gets cleaned up
        _materialBakers.remove(baker->getMaterialData());

        // emit progress to tell listeners how many materials we have baked
        emit bakeProgress(++_completedSubBakes, _totalNumberOfSubBakes);

        // check if this was the last material we needed to re-write and if we are done now
        checkIfRewritingComplete();
    }
}

void DomainBaker::checkIfRewritingComplete() {
    if (_entitiesNeedingRewrite.isEmpty()) {
        writeNewEntitiesFile();

        if (hasErrors()) {
            return;
        }

        // we've now written out our new models file - time to say that we are finished up
        emit finished();
    }
}

void DomainBaker::writeNewEntitiesFile() {
    // we've enumerated all of our entities and re-written all the URLs we'll be able to re-write
    // time to write out a main models.json.gz file

    // first setup a document with the entities array below the entities key
    QJsonObject json = _json.object();
    json[ENTITIES_OBJECT_KEY] = _entities;

    // turn that QJsonDocument into a byte array ready for compression
    QByteArray jsonByteArray = QJsonDocument(json).toJson();

    // compress the json byte array using gzip
    QByteArray compressedJson;
    gzip(jsonByteArray, compressedJson);

    // write the gzipped json to a new models file
    static const QString MODELS_FILE_NAME = "models.json.gz";

    auto bakedEntitiesFilePath = QDir(_uniqueOutputPath).filePath(MODELS_FILE_NAME);
    QFile compressedEntitiesFile { bakedEntitiesFilePath };

    if (!compressedEntitiesFile.open(QIODevice::WriteOnly)
        || (compressedEntitiesFile.write(compressedJson) == -1)) {

        // add an error to our list to state that the output models file could not be created or could not be written to
        handleError("Failed to export baked entities file");

        return;
    }

    qDebug() << "Exported baked entities file to" << bakedEntitiesFilePath;
}
