//
//  ModelPackager.h
//
//
//  Created by Clement on 3/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelPackager_h
#define hifi_ModelPackager_h

#include <memory>

#include <QFileInfo>
#include <QMultiHash>

#include "ui/ModelsBrowser.h"

namespace hfm {
    class Model;
};

class ModelPackager : public QObject {
public:
    static bool package();
    
private:
    bool selectModel();
    
    bool loadModel();
    bool editProperties();
    bool zipModel();
    
    void populateBasicMapping(QVariantHash& mapping, QString filename, const hfm::Model& hfmModel);
    
    void listTextures();
    bool copyTextures(const QString& oldDir, const QDir& newDir);
    
    QFileInfo _modelFile;
    QFileInfo _fbxInfo;
    QString _texDir;
    QString _scriptDir;

    QMultiHash<QString, QVariant> _mapping;
    std::shared_ptr<hfm::Model> _hfmModel;
    QStringList _textures;
    QStringList _scripts;
};




#endif // hifi_ModelPackager_h
