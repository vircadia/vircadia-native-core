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
#include <QVariantHash>

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

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
// Add support for writing these to qDebug().
QDebug& operator<<(QDebug& s, const glm::vec2& v);
QDebug& operator<<(QDebug& s, const glm::vec3& v);
QDebug& operator<<(QDebug& s, const glm::u8vec3& v);
QDebug& operator<<(QDebug& s, const glm::vec4& v);
QDebug& operator<<(QDebug& s, const glm::quat& q);
QDebug& operator<<(QDebug& s, const glm::mat4& m);
QDebug& operator<<(QDebug& dbg, const QVariantHash& v);
#endif // QT_NO_DEBUG_STREAM

#endif // hifi_StreamUtils_h
