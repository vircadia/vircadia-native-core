//
//  Created by Bradley Austin Davis on 2016/09/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef _GLIHelpers_H_
#define _GLIHelpers_H_

#include <qglobal.h>
#include <QtCore/QString>

// Work around for a bug in the MSVC compiler that chokes when you use GLI and Qt headers together.
#define gli glm

#ifdef Q_OS_MAC
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wempty-body"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <gli/gli.hpp>

#ifdef Q_OS_MAC
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <gpu/Forward.h>

gpu::TexturePointer processTexture(const QString& file);

#endif
