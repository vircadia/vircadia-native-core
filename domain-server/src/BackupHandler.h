//
//  BackupHandler.h
//  domain-server/src
//
//  Created by Clement Brisset on 2/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BackupHandler_h
#define hifi_BackupHandler_h

#include <memory>

#include <QDebug>

#include <quazip5/quazip.h>

class BackupHandler {
public:
    template <typename T>
    BackupHandler(T* x) : _self(new Model<T>(x)) {}

    void loadBackup(QuaZip& zip) {
        _self->loadBackup(zip);
    }
    void createBackup(QuaZip& zip) {
        _self->createBackup(zip);
    }
    void recoverBackup(QuaZip& zip) {
        _self->recoverBackup(zip);
    }
    void deleteBackup(QuaZip& zip) {
        _self->deleteBackup(zip);
    }
    void consolidateBackup(QuaZip& zip) {
        _self->consolidateBackup(zip);
    }

private:
    struct Concept {
        virtual ~Concept() = default;

        virtual void loadBackup(QuaZip& zip) = 0;
        virtual void createBackup(QuaZip& zip) = 0;
        virtual void recoverBackup(QuaZip& zip) = 0;
        virtual void deleteBackup(QuaZip& zip) = 0;
        virtual void consolidateBackup(QuaZip& zip) = 0;
    };

    template <typename T>
    struct Model : Concept {
        Model(T* x) : data(x) {}

        void loadBackup(QuaZip& zip) override {
            data->loadBackup(zip);
        }
        void createBackup(QuaZip& zip) override {
            data->createBackup(zip);
        }
        void recoverBackup(QuaZip& zip) override {
            data->recoverBackup(zip);
        }
        void deleteBackup(QuaZip& zip) override {
            data->deleteBackup(zip);
        }
        void consolidateBackup(QuaZip& zip) override {
            data->consolidateBackup(zip);
        }

        std::unique_ptr<T> data;
    };

    std::unique_ptr<Concept> _self;
};

#include <quazip5/quazipfile.h>
class EntitiesBackupHandler {
public:
    EntitiesBackupHandler(QString entitiesFilePath, QString entitiesReplacementFilePath)
        : _entitiesFilePath(entitiesFilePath)
        , _entitiesReplacementFilePath(entitiesReplacementFilePath) {}

    void loadBackup(QuaZip& zip) {}

    // Create a skeleton backup
    void createBackup(QuaZip& zip) {
        QFile entitiesFile { _entitiesFilePath };

        if (entitiesFile.open(QIODevice::ReadOnly)) {
            QuaZipFile zipFile { &zip };
            zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("models.json.gz", _entitiesFilePath));
            zipFile.write(entitiesFile.readAll());
            zipFile.close();
            if (zipFile.getZipError() != UNZ_OK) {
                qCritical() << "Failed to zip models.json.gz: " << zipFile.getZipError();
            }
        }
    }

    // Recover from a full backup
    void recoverBackup(QuaZip& zip) {
        if (!zip.setCurrentFile("models.json.gz")) {
            qWarning() << "Failed to find models.json.gz while recovering backup";
            return;
        }
        QuaZipFile zipFile { &zip };
        if (!zipFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open models.json.gz in backup";
            return;
        }
        auto data = zipFile.readAll();

        QFile entitiesFile { _entitiesReplacementFilePath };

        if (entitiesFile.open(QIODevice::WriteOnly)) {
            entitiesFile.write(data);
        }

        zipFile.close();

        if (zipFile.getZipError() != UNZ_OK) {
            qCritical() << "Failed to zip models.json.gz: " << zipFile.getZipError();
        }
    }

    // Delete a skeleton backup
    void deleteBackup(QuaZip& zip) {
    }

    // Create a full backup
    void consolidateBackup(QuaZip& zip) {
    }

private:
    QString _entitiesFilePath;
    QString _entitiesReplacementFilePath;
};

#endif /* hifi_BackupHandler_h */
