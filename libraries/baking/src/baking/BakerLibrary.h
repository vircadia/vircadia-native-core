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

// Returns either the given model URL, or, if the model is baked and shouldRebakeOriginals is true,
// the guessed location of the original model
// Returns an empty URL if no bakeable URL found
QUrl getBakeableModelURL(const QUrl& url);

// Assuming the URL is valid, gets the appropriate baker for the given URL, and creates the base directory where the baker's output will later be stored
// Returns an empty pointer if a baker could not be created
std::unique_ptr<ModelBaker> getModelBaker(const QUrl& bakeableModelURL, TextureBakerThreadGetter inputTextureThreadGetter, const QString& contentOutputPath);

#endif hifi_BakerLibrary_h
