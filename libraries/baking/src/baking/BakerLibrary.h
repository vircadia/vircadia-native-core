//
//  ModelBaker.h
//  libraries/baking/src/baking
//
//  Created by Sabrina Shanman on 2019/02/14.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BakerLibrary_h
#define hifi_BakerLibrary_h

#include <QUrl>

#include "../ModelBaker.h"

// Returns either the given model URL if valid, or an empty URL
QUrl getBakeableModelURL(const QUrl& url);

bool isModelBaked(const QUrl& bakeableModelURL);

// Assuming the URL is valid, gets the appropriate baker for the given URL, and creates the base directory where the baker's output will later be stored
// Returns an empty pointer if a baker could not be created
std::unique_ptr<ModelBaker> getModelBaker(const QUrl& bakeableModelURL, const QString& contentOutputPath, const QUrl& destinationPath);

// Similar to getModelBaker, but gives control over where the output folders will be
std::unique_ptr<ModelBaker> getModelBakerWithOutputDirectories(const QUrl& bakeableModelURL, const QUrl& destinationPath, const QString& bakedOutputDirectory, const QString& originalOutputDirectory);

#endif // hifi_BakerLibrary_h
