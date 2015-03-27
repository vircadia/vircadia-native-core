//
//  FSTReader.h
//
//
//  Created by Clement on 3/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FSTReader_h
#define hifi_FSTReader_h

#include <QVariantHash>

static const QString NAME_FIELD = "name";
static const QString FILENAME_FIELD = "filename";
static const QString TEXDIR_FIELD = "texdir";
static const QString LOD_FIELD = "lod";
static const QString JOINT_INDEX_FIELD = "jointIndex";
static const QString SCALE_FIELD = "scale";
static const QString TRANSLATION_X_FIELD = "tx";
static const QString TRANSLATION_Y_FIELD = "ty";
static const QString TRANSLATION_Z_FIELD = "tz";
static const QString JOINT_FIELD = "joint";
static const QString FREE_JOINT_FIELD = "freeJoint";
static const QString BLENDSHAPE_FIELD = "bs";

/// Reads an FST mapping from the supplied data.
QVariantHash readMapping(const QByteArray& data);

/// Writes an FST mapping to a byte array.
QByteArray writeMapping(const QVariantHash& mapping);

#endif // hifi_FSTReader_h