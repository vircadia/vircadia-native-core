//
//  StreamUtils.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>

#include <glm/gtc/type_ptr.hpp>

#include "StreamUtils.h"

const char* hex_digits = "0123456789abcdef";

void StreamUtil::dump(std::ostream& s, const QByteArray& buffer) {
    int row_size = 32;
    int i = 0;
    while (i < buffer.size()) {
        for(int j = 0; i < buffer.size() && j < row_size; ++j) {
            char byte = buffer[i];
            s << hex_digits[(byte >> 4) & 0x0f] << hex_digits[byte & 0x0f] << " ";
            ++i;
        }
        s << "\n";
    }
}

std::ostream& operator<<(std::ostream& s, const glm::vec3& v) {
    s << "<" << v.x << " " << v.y << " " << v.z << ">";
    return s;
}

std::ostream& operator<<(std::ostream& s, const glm::quat& q) {
    s << "<" << q.x << " " << q.y << " " << q.z << " " << q.w << ">";
    return s;
}

std::ostream& operator<<(std::ostream& s, const glm::mat4& m) {
    s << "[";
    for (int j = 0; j < 4; ++j) {
        s << " " << m[0][j] << " " << m[1][j] << " " << m[2][j] << " " << m[3][j] << ";";
    }
    s << "  ]";
    return s;
}

QDataStream& operator<<(QDataStream& out, const glm::vec3& vector) {
    return out << vector.x << vector.y << vector.z;
}

QDataStream& operator>>(QDataStream& in, glm::vec3& vector) {
    return in >> vector.x >> vector.y >> vector.z;
}

QDataStream& operator<<(QDataStream& out, const glm::quat& quaternion) {
    return out << quaternion.x << quaternion.y << quaternion.z << quaternion.w;
}

QDataStream& operator>>(QDataStream& in, glm::quat& quaternion) {
    return in >> quaternion.x >> quaternion.y >> quaternion.z >> quaternion.w;
}

// less common utils can be enabled with DEBUG
#ifdef DEBUG

std::ostream& operator<<(std::ostream& s, const CollisionInfo& c) {
    s << "{penetration=" << c._penetration 
        << ", contactPoint=" << c._contactPoint
        << ", addedVelocity=" << c._addedVelocity
        << "}";
    return s;
}

std::ostream& operator<<(std::ostream& s, const SphereShape& sphere) {
    s << "{type='sphere', center=" << sphere.getPosition()
        << ", radius=" << sphere.getRadius()
        << "}";
    return s;
}

std::ostream& operator<<(std::ostream& s, const CapsuleShape& capsule) {
    s << "{type='capsule', center=" << capsule.getPosition()
        << ", radius=" << capsule.getRadius()
        << ", length=" << (2.0f * capsule.getHalfHeight())
        << ", begin=" << capsule.getStartPoint()
        << ", end=" << capsule.getEndPoint()
        << "}";
    return s;
}

#endif // DEBUG

#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>

QDebug& operator<<(QDebug& dbg, const glm::vec3& v) {
    dbg.nospace() << "{type='glm::vec3'"
        ", x=" << v.x <<
        ", y=" << v.y <<
        ", z=" << v.z <<
        "}";
    return dbg;
}

QDebug& operator<<(QDebug& dbg, const glm::quat& q) {
    dbg.nospace() << "{type='glm::quat'"
        ", x=" << q.x <<
        ", y=" << q.y <<
        ", z=" << q.z <<
        ", w=" << q.w <<
        "}";
    return dbg;
}

QDebug& operator<<(QDebug& dbg, const glm::mat4& m) {
    dbg.nospace() << "{type='glm::mat4', [";
    for (int j = 0; j < 4; ++j) {
        dbg << ' ' << m[0][j] << ' ' << m[1][j] << ' ' << m[2][j] << ' ' << m[3][j] << ';';
    }
    return dbg << " ]}";
}

#endif // QT_NO_DEBUG_STREAM
