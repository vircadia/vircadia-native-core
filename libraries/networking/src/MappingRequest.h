//
//  MappingRequest.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_MappingRequest_h
#define hifi_MappingRequest_h

#include <QtCore/QObject>

#include "AssetUtils.h"
#include "AssetClient.h"

class MappingRequest : public QObject {
    Q_OBJECT
public:
    enum Error {
        NoError,
        NotFound,
        NetworkError,
        PermissionDenied,
        InvalidPath,
        InvalidHash,
        UnknownError
    };

    virtual ~MappingRequest();

    Q_INVOKABLE void start();
    Error getError() const { return _error; }
    Q_INVOKABLE QString getErrorString() const;

protected:
    Error _error { NoError };
    MessageID _mappingRequestID { AssetClient::INVALID_MESSAGE_ID };

private:
    virtual void doStart() = 0;
};


class GetMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    GetMappingRequest(const AssetPath& path);

    AssetHash getHash() const { return _hash;  }

signals:
    void finished(GetMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPath _path;
    AssetHash _hash;
};

class SetMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    SetMappingRequest(const AssetPath& path, const AssetHash& hash);

    AssetPath getPath() const { return _path;  }
    AssetHash getHash() const { return _hash;  }

signals:
    void finished(SetMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPath _path;
    AssetHash _hash;
};

class DeleteMappingsRequest : public MappingRequest {
    Q_OBJECT
public:
    DeleteMappingsRequest(const AssetPathList& path);

signals:
    void finished(DeleteMappingsRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPathList _paths;
};

class RenameMappingRequest : public MappingRequest {
    Q_OBJECT
public:
    RenameMappingRequest(const AssetPath& oldPath, const AssetPath& newPath);

signals:
    void finished(RenameMappingRequest* thisRequest);

private:
    virtual void doStart() override;

    AssetPath _oldPath;
    AssetPath _newPath;
};

class GetAllMappingsRequest : public MappingRequest {
    Q_OBJECT
public:
    GetAllMappingsRequest();

    AssetMapping getMappings() const { return _mappings;  }

signals:
    void finished(GetAllMappingsRequest* thisRequest);

private:
    virtual void doStart() override;
    
    std::map<AssetPath, AssetHash> _mappings;
};


#endif // hifi_MappingRequest_h
