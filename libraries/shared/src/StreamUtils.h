//
//  StreamUtils.h
//
//  Created by Andrew Meadows on 2014.02.21
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __tests__StreamUtils__
#define __tests__StreamUtils__

#include <iostream>

#include <QByteArray>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>


namespace StreamUtil {
    // dump the buffer, 32 bytes per row, each byte in hex, separated by whitespace
    void dump(std::ostream& s, const QByteArray& buffer);
}

std::ostream& operator<<(std::ostream& s, const glm::vec3& v);
std::ostream& operator<<(std::ostream& s, const glm::quat& q);
std::ostream& operator<<(std::ostream& s, const glm::mat4& m);

// less common utils can be enabled with DEBUG
#ifdef DEBUG
#include "CollisionInfo.h"
#include "SphereShape.h"
#include "CapsuleShape.h"
std::ostream& operator<<(std::ostream& s, const CollisionInfo& c);
std::ostream& operator<<(std::ostream& s, const SphereShape& shape);
std::ostream& operator<<(std::ostream& s, const CapsuleShape& capsule);
#endif // DEBUG


#endif // __tests__StreamUtils__
