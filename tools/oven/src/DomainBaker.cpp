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

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "Gzip.h"

#include "DomainBaker.h"

DomainBaker::DomainBaker(const QUrl& localModelFileURL, const QString& domainName, const QString& baseOutputPath) :
    _localEntitiesFileURL(localModelFileURL),
    _domainName(domainName),
    _baseOutputPath(baseOutputPath)
{
    
}

void DomainBaker::start() {
    setupOutputFolder();
    loadLocalFile();
    enumerateEntities();
}

void DomainBaker::setupOutputFolder() {
    // in order to avoid overwriting previous bakes, we create a special output folder with the domain name and timestamp

    // first, construct the directory name
    auto domainPrefix = !_domainName.isEmpty() ? _domainName + "-" : "";
    auto timeNow = QDateTime::currentDateTime();

    static const QString FOLDER_TIMESTAMP_FORMAT = "yyyyMMdd-hhmmss";
    QString outputDirectoryName = domainPrefix + timeNow.toString(FOLDER_TIMESTAMP_FORMAT);

    //  make sure we can create that directory
    QDir baseDir { _baseOutputPath };

    if (!baseDir.mkpath(outputDirectoryName)) {

        // add an error to specify that the output directory could not be created

        return;
    }

    // store the unique output path so we can re-use it when saving baked models
    baseDir.cd(outputDirectoryName);
    _uniqueOutputPath = baseDir.absolutePath();
}

void DomainBaker::loadLocalFile() {
    // load up the local entities file
    QFile modelsFile { _localEntitiesFileURL.toLocalFile() };

    if (!modelsFile.open(QIODevice::ReadOnly)) {
        // add an error to our list to specify that the file could not be read

        // return to stop processing
        return;
    }

    // grab a byte array from the file
    auto fileContents = modelsFile.readAll();

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
    _entities = jsonDocument.object()["Entities"].toArray();

    if (_entities.isEmpty()) {
        // add an error to our list stating that the models file was empty

        // return to stop processing
        return;
    }
}

void DomainBaker::enumerateEntities() {
    qDebug() << "Enumerating" << _entities.size() << "entities from domain";

    foreach(QJsonValue entityValue, _entities) {
        // make sure this is a JSON object
        if (entityValue.isObject()) {
            auto entity = entityValue.toObject();

            // check if this is an entity with a model URL
            static const QString ENTITY_MODEL_URL_KEY = "modelURL";
            if (entity.contains(ENTITY_MODEL_URL_KEY)) {
                // grab a QUrl for the model URL
                auto modelURL = QUrl(entity[ENTITY_MODEL_URL_KEY].toString());

                // check if the file pointed to by this URL is a bakeable model, by comparing extensions
                auto modelFileName = modelURL.fileName();

                static const QStringList BAKEABLE_MODEL_EXTENSIONS { ".fbx" };
                auto completeLowerExtension = modelFileName.mid(modelFileName.indexOf('.')).toLower();

                if (BAKEABLE_MODEL_EXTENSIONS.contains(completeLowerExtension)) {
                    // grab a clean version of the URL without a query or fragment
                    modelURL.setFragment("");
                    modelURL.setQuery("");

                    // setup an FBXBaker for this URL, as long as we don't already have one
                    if (!_bakers.contains(modelURL)) {
                        QSharedPointer<FBXBaker> baker { new FBXBaker(modelURL, _uniqueOutputPath) };

                        // start the baker
                        baker->start();

                        // insert it into our bakers hash so we hold a strong pointer to it
                        _bakers.insert(modelURL, baker);
                    }
                }
            }
        }
    }
}
