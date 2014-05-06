//
//  StreamUtils.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StreamUtils_h
#define hifi_StreamUtils_h

#include <iostream>

#include <QByteArray>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class QDataStream;

namespace StreamUtil {
    // dump the buffer, 32 bytes per row, each byte in hex, separated by whitespace
    void dump(std::ostream& s, const QByteArray& buffer);
}

std::ostream& operator<<(std::ostream& s, const glm::vec3& v);
std::ostream& operator<<(std::ostream& s, const glm::quat& q);
std::ostream& operator<<(std::ostream& s, const glm::mat4& m);

QDataStream& operator<<(QDataStream& out, const glm::vec3& vector);
QDataStream& operator>>(QDataStream& in, glm::vec3& vector);

QDataStream& operator<<(QDataStream& out, const glm::quat& quaternion);
QDataStream& operator>>(QDataStream& in, glm::quat& quaternion);

// less common utils can be enabled with DEBUG
#ifdef DEBUG
#include "CollisionInfo.h"
#include "SphereShape.h"
#include "CapsuleShape.h"
std::ostream& operator<<(std::ostream& s, const CollisionInfo& c);
std::ostream& operator<<(std::ostream& s, const SphereShape& shape);
std::ostream& operator<<(std::ostream& s, const CapsuleShape& capsule);
#endif // DEBUG


#endif // hifi_StreamUtils_h
