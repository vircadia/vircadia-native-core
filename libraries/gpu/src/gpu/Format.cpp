//
//  Created by Bradley Austin Davis on 2015/09/20
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Format.h"

using namespace gpu;

const Element Element::COLOR_RGBA_32{ VEC4, NUINT8, RGBA };
const Element Element::COLOR_SRGBA_32{ VEC4, NUINT8, SRGBA };
const Element Element::COLOR_R11G11B10{ SCALAR, FLOAT, R11G11B10 };
const Element Element::VEC4F_COLOR_RGBA{ VEC4, FLOAT, RGBA };
const Element Element::VEC2F_UV{ VEC2, FLOAT, UV };
const Element Element::VEC2F_XY{ VEC2, FLOAT, XY };
const Element Element::VEC3F_XYZ{ VEC3, FLOAT, XYZ };
const Element Element::VEC4F_XYZW{ VEC4, FLOAT, XYZW };
const Element Element::INDEX_UINT16{ SCALAR, UINT16, INDEX };
const Element Element::PART_DRAWCALL{ VEC4, UINT32, PART };

