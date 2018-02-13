//
//  BackupHandler.h
//  assignment-client
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

        void loadBackup(QuaZip& zip) {
            data->loadBackup(zip);
        }
        void createBackup(QuaZip& zip) {
            data->createBackup(zip);
        }
        void recoverBackup(QuaZip& zip) {
            data->recoverBackup(zip);
        }
        void deleteBackup(QuaZip& zip) {
            data->deleteBackup(zip);
        }
        void consolidateBackup(QuaZip& zip) {
            data->consolidateBackup(zip);
        }

        std::unique_ptr<T> data;
    };

    std::unique_ptr<Concept> _self;
};

#include <quazip5/quazipfile.h>
class EntitiesBackupHandler {
public:
    EntitiesBackupHandler(QString entitiesFilePath) : _entitiesFilePath(entitiesFilePath) {}

    void loadBackup(QuaZip& zip) {}

    void createBackup(QuaZip& zip) const {
        qDebug() << "Creating a backup from handler";

        QFile entitiesFile { _entitiesFilePath };
        qDebug() << entitiesFile.size();

        if (entitiesFile.open(QIODevice::ReadOnly)) {
            QuaZipFile zipFile { &zip };
            zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("models.json.gz", _entitiesFilePath));
            zipFile.write(entitiesFile.readAll());
            zipFile.close();
            if (zipFile.getZipError() != UNZ_OK) {
                qDebug() << "testCreate(): outFile.close(): " << zipFile.getZipError();
            }
        }
    }

    void recoverBackup(QuaZip& zip) const {}
    void deleteBackup(QuaZip& zip) {}
    void consolidateBackup(QuaZip& zip) const {}

private:
    QString _entitiesFilePath;
};

#endif /* hifi_BackupHandler_h */
