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

#include <QtConcurrent>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "Gzip.h"

#include "Oven.h"

#include "DomainBaker.h"

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
    auto jsonDocument = QJsonDocument::fromJson(fileContents);

    // grab the entities object from the root JSON object
    _entities = jsonDocument.object()[ENTITIES_OBJECT_KEY].toArray();

    if (_entities.isEmpty()) {
        // add an error to our list stating that the models file was empty

        // return to stop processing
        return;
    }
}

const QString ENTITY_MODEL_URL_KEY = "modelURL";
const QString ENTITY_SKYBOX_KEY = "skybox";
const QString ENTITY_SKYBOX_URL_KEY = "url";
const QString ENTITY_KEYLIGHT_KEY = "keyLight";
const QString ENTITY_KEYLIGHT_AMBIENT_URL_KEY = "ambientURL";

void DomainBaker::enumerateEntities() {
    qDebug() << "Enumerating" << _entities.size() << "entities from domain";

    for (auto it = _entities.begin(); it != _entities.end(); ++it) {
        // make sure this is a JSON object
        if (it->isObject()) {
            auto entity = it->toObject();

            // check if this is an entity with a model URL or is a skybox texture
            if (entity.contains(ENTITY_MODEL_URL_KEY)) {
                // grab a QUrl for the model URL
                QUrl modelURL { entity[ENTITY_MODEL_URL_KEY].toString() };

                // check if the file pointed to by this URL is a bakeable model, by comparing extensions
                auto modelFileName = modelURL.fileName();

                static const QString BAKEABLE_MODEL_EXTENSION { ".fbx" };
                static const QString BAKED_MODEL_EXTENSION = ".baked.fbx";

                bool isBakedFBX = modelFileName.endsWith(BAKED_MODEL_EXTENSION, Qt::CaseInsensitive);
                bool isUnbakedFBX = modelFileName.endsWith(BAKEABLE_MODEL_EXTENSION, Qt::CaseInsensitive) && !isBakedFBX;

                if (isUnbakedFBX || (_shouldRebakeOriginals && isBakedFBX)) {

                    if (isBakedFBX) {
                        // grab a URL to the original, that we assume is stored a directory up, in the "original" folder
                        // with just the fbx extension
                        qDebug() << "Re-baking original for" << modelURL;

                        auto originalFileName = modelFileName;
                        originalFileName.replace(".baked", "");
                        modelURL = modelURL.resolved("../original/" + originalFileName);

                        qDebug() << "Original must be present at" << modelURL;
                    } else {
                        // grab a clean version of the URL without a query or fragment
                        modelURL = modelURL.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);
                    }

                    // setup an FBXBaker for this URL, as long as we don't already have one
                    if (!_modelBakers.contains(modelURL)) {
                        QSharedPointer<FBXBaker> baker {
                            new FBXBaker(modelURL, _contentOutputPath, []() -> QThread* {
                                return qApp->getNextWorkerThread();
                            }), &FBXBaker::deleteLater
                        };

                        // make sure our handler is called when the baker is done
                        connect(baker.data(), &Baker::finished, this, &DomainBaker::handleFinishedModelBaker);

                        // insert it into our bakers hash so we hold a strong pointer to it
                        _modelBakers.insert(modelURL, baker);

                        // move the baker to the baker thread
                        // and kickoff the bake
                        baker->moveToThread(qApp->getFBXBakerThread());
                        QMetaObject::invokeMethod(baker.data(), "bake");

                        // keep track of the total number of baking entities
                        ++_totalNumberOfSubBakes;
                    }

                    // add this QJsonValueRef to our multi hash so that we can easily re-write
                    // the model URL to the baked version once the baker is complete
                    _entitiesNeedingRewrite.insert(modelURL, *it);
                }
            } else {
//                // We check now to see if we have either a texture for a skybox or a keylight, or both.
//                if (entity.contains(ENTITY_SKYBOX_KEY)) {
//                    auto skyboxObject = entity[ENTITY_SKYBOX_KEY].toObject();
//                    if (skyboxObject.contains(ENTITY_SKYBOX_URL_KEY)) {
//                        // we have a URL to a skybox, grab it
//                        QUrl skyboxURL { skyboxObject[ENTITY_SKYBOX_URL_KEY].toString() };
//
//                        // setup a bake of the skybox
//                        bakeSkybox(skyboxURL, *it);
//                    }
//                }
//
//                if (entity.contains(ENTITY_KEYLIGHT_KEY)) {
//                    auto keyLightObject = entity[ENTITY_KEYLIGHT_KEY].toObject();
//                    if (keyLightObject.contains(ENTITY_KEYLIGHT_AMBIENT_URL_KEY)) {
//                        // we have a URL to a skybox, grab it
//                        QUrl skyboxURL { keyLightObject[ENTITY_KEYLIGHT_AMBIENT_URL_KEY].toString() };
//
//                        // setup a bake of the skybox
//                        bakeSkybox(skyboxURL, *it);
//                    }
//                }
            }
        }
    }

    // emit progress now to say we're just starting
    emit bakeProgress(0, _totalNumberOfSubBakes);
}

void DomainBaker::bakeSkybox(QUrl skyboxURL, QJsonValueRef entity) {

    auto skyboxFileName = skyboxURL.fileName();

    static const QStringList BAKEABLE_SKYBOX_EXTENSIONS {
        ".jpg", ".png", ".gif", ".bmp", ".pbm", ".pgm", ".ppm", ".xbm", ".xpm", ".svg"
    };
    auto completeLowerExtension = skyboxFileName.mid(skyboxFileName.indexOf('.')).toLower();

    if (BAKEABLE_SKYBOX_EXTENSIONS.contains(completeLowerExtension)) {
        // grab a clean version of the URL without a query or fragment
        skyboxURL = skyboxURL.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);

        // setup a texture baker for this URL, as long as we aren't baking a skybox already
        if (!_skyboxBakers.contains(skyboxURL)) {
            // setup a baker for this skybox

            QSharedPointer<TextureBaker> skyboxBaker {
                new TextureBaker(skyboxURL, image::TextureUsage::CUBE_TEXTURE, _contentOutputPath),
                &TextureBaker::deleteLater
            };

            // make sure our handler is called when the skybox baker is done
            connect(skyboxBaker.data(), &TextureBaker::finished, this, &DomainBaker::handleFinishedSkyboxBaker);

            // insert it into our bakers hash so we hold a strong pointer to it
            _skyboxBakers.insert(skyboxURL, skyboxBaker);

            // move the baker to a worker thread and kickoff the bake
            skyboxBaker->moveToThread(qApp->getNextWorkerThread());
            QMetaObject::invokeMethod(skyboxBaker.data(), "bake");

            // keep track of the total number of baking entities
            ++_totalNumberOfSubBakes;
        }

        // add this QJsonValueRef to our multi hash so that it can re-write the skybox URL
        // to the baked version once the baker is complete
        _entitiesNeedingRewrite.insert(skyboxURL, entity);
    }
}

void DomainBaker::handleFinishedModelBaker() {
    auto baker = qobject_cast<FBXBaker*>(sender());

    if (baker) {
        if (!baker->hasErrors()) {
            // this FBXBaker is done and everything went according to plan
            qDebug() << "Re-writing entity references to" << baker->getFBXUrl();

            // enumerate the QJsonRef values for the URL of this FBX from our multi hash of
            // entity objects needing a URL re-write
            for (QJsonValueRef entityValue : _entitiesNeedingRewrite.values(baker->getFBXUrl())) {

                // convert the entity QJsonValueRef to a QJsonObject so we can modify its URL
                auto entity = entityValue.toObject();

                // grab the old URL
                QUrl oldModelURL { entity[ENTITY_MODEL_URL_KEY].toString() };

                // setup a new URL using the prefix we were passed
                QUrl newModelURL = _destinationPath.resolved(baker->getBakedFBXRelativePath());

                // copy the fragment and query, and user info from the old model URL
                newModelURL.setQuery(oldModelURL.query());
                newModelURL.setFragment(oldModelURL.fragment());
                newModelURL.setUserInfo(oldModelURL.userInfo());

                // set the new model URL as the value in our temp QJsonObject
                entity[ENTITY_MODEL_URL_KEY] = newModelURL.toString();

                // check if the entity also had an animation at the same URL
                // in which case it should be replaced with our baked model URL too
                const QString ENTITY_ANIMATION_KEY = "animation";
                const QString ENTITIY_ANIMATION_URL_KEY = "url";

                if (entity.contains(ENTITY_ANIMATION_KEY)) {
                    auto animationObject = entity[ENTITY_ANIMATION_KEY].toObject();

                    if (animationObject.contains(ENTITIY_ANIMATION_URL_KEY)) {
                        // grab the old animation URL
                        QUrl oldAnimationURL { animationObject[ENTITIY_ANIMATION_URL_KEY].toString() };

                        // check if its stripped down version matches our stripped down model URL
                        if (oldAnimationURL.matches(oldModelURL, QUrl::RemoveQuery | QUrl::RemoveFragment)) {
                            // the animation URL matched the old model URL, so make the animation URL point to the baked FBX
                            // with its original query and fragment
                            auto newAnimationURL = _destinationPath.resolved(baker->getBakedFBXRelativePath());
                            newAnimationURL.setQuery(oldAnimationURL.query());
                            newAnimationURL.setFragment(oldAnimationURL.fragment());
                            newAnimationURL.setUserInfo(oldAnimationURL.userInfo());

                            animationObject[ENTITIY_ANIMATION_URL_KEY] = newAnimationURL.toString();

                            // replace the animation object in the entity object
                            entity[ENTITY_ANIMATION_KEY] = animationObject;
                        }
                    }
                }
                
                // replace our temp object with the value referenced by our QJsonValueRef
                entityValue = entity;
            }
        } else {
            // this model failed to bake - this doesn't fail the entire bake but we need to add
            // the errors from the model to our warnings
            _warningList << baker->getErrors();
        }

        // remove the baked URL from the multi hash of entities needing a re-write
        _entitiesNeedingRewrite.remove(baker->getFBXUrl());

        // drop our shared pointer to this baker so that it gets cleaned up
        _modelBakers.remove(baker->getFBXUrl());

        // emit progress to tell listeners how many models we have baked
        emit bakeProgress(++_completedSubBakes, _totalNumberOfSubBakes);

        // check if this was the last model we needed to re-write and if we are done now
        checkIfRewritingComplete();
    }
}

void DomainBaker::handleFinishedSkyboxBaker() {
    auto baker = qobject_cast<TextureBaker*>(sender());

    if (baker) {
        if (!baker->hasErrors()) {
            // this FBXBaker is done and everything went according to plan
            qDebug() << "Re-writing entity references to" << baker->getTextureURL();

            // enumerate the QJsonRef values for the URL of this FBX from our multi hash of
            // entity objects needing a URL re-write
            for (QJsonValueRef entityValue : _entitiesNeedingRewrite.values(baker->getTextureURL())) {
                // convert the entity QJsonValueRef to a QJsonObject so we can modify its URL
                auto entity = entityValue.toObject();

                if (entity.contains(ENTITY_SKYBOX_KEY)) {
                    auto skyboxObject = entity[ENTITY_SKYBOX_KEY].toObject();

                    if (skyboxObject.contains(ENTITY_SKYBOX_URL_KEY)) {
                        if (rewriteSkyboxURL(skyboxObject[ENTITY_SKYBOX_URL_KEY], baker)) {
                            // we re-wrote the URL, replace the skybox object referenced by the entity object
                            entity[ENTITY_SKYBOX_KEY] = skyboxObject;
                        }
                    }
                }

                if (entity.contains(ENTITY_KEYLIGHT_KEY)) {
                    auto ambientObject = entity[ENTITY_KEYLIGHT_KEY].toObject();

                    if (ambientObject.contains(ENTITY_KEYLIGHT_AMBIENT_URL_KEY)) {
                        if (rewriteSkyboxURL(ambientObject[ENTITY_KEYLIGHT_AMBIENT_URL_KEY], baker)) {
                            // we re-wrote the URL, replace the ambient object referenced by the entity object
                            entity[ENTITY_KEYLIGHT_KEY] = ambientObject;
                        }
                    }
                }

                // replace our temp object with the value referenced by our QJsonValueRef
                entityValue = entity;
            }
        } else {
            // this skybox failed to bake - this doesn't fail the entire bake but we need to add the errors from
            // the model to our warnings
            _warningList << baker->getWarnings();
        }

        // remove the baked URL from the multi hash of entities needing a re-write
        _entitiesNeedingRewrite.remove(baker->getTextureURL());

        // drop our shared pointer to this baker so that it gets cleaned up
        _skyboxBakers.remove(baker->getTextureURL());

         // emit progress to tell listeners how many models we have baked
         emit bakeProgress(++_completedSubBakes, _totalNumberOfSubBakes);

         // check if this was the last model we needed to re-write and if we are done now
         checkIfRewritingComplete();
    }
}

bool DomainBaker::rewriteSkyboxURL(QJsonValueRef urlValue, TextureBaker* baker) {
    // grab the old skybox URL
    QUrl oldSkyboxURL { urlValue.toString() };

    if (oldSkyboxURL.matches(baker->getTextureURL(), QUrl::RemoveQuery | QUrl::RemoveFragment)) {
        // change the URL to point to the baked texture with its original query and fragment

        auto newSkyboxURL = _destinationPath.resolved(baker->getBakedTextureFileName());
        newSkyboxURL.setQuery(oldSkyboxURL.query());
        newSkyboxURL.setFragment(oldSkyboxURL.fragment());
        newSkyboxURL.setUserInfo(oldSkyboxURL.userInfo());

        urlValue = newSkyboxURL.toString();

        return true;
    } else {
        return false;
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
    QJsonDocument entitiesDocument;

    QJsonObject rootObject;
    rootObject[ENTITIES_OBJECT_KEY] = _entities;

    entitiesDocument.setObject(rootObject);

    // turn that QJsonDocument into a byte array ready for compression
    QByteArray jsonByteArray = entitiesDocument.toJson();

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

    qDebug() << "Exported entities file with baked model URLs to" << bakedEntitiesFilePath;
}

