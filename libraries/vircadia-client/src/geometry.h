//
//  geometry.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 23 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_GEOMETRY_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_GEOMETRY_H

/// @brief Defines the layout of vectors.
struct vircadia_vector {
    /// @brief The x coordinate of the vector.
    float x;
    /// @brief The y coordinate of the vector.
    float y;
    /// @brief The z coordinate of the vector.
    float z;
};

/// @brief Defines the layout of quaternions.
struct vircadia_quaternion {
    /// @brief The x component of the quaternion.
    float x;
    /// @brief The y component of the quaternion.
    float y;
    /// @brief The z component of the quaternion.
    float z;
    /// @brief The w component of the quaternion.
    float w;
};

/// @brief Defines the layout of bounding box data.
///
/// Used in vircadia_set_my_avatar_bounding_box(),
/// vircadia_get_avatar_bounding_box()
struct vircadia_bounds {
    /// @brief The dimensions/size of the bounding box.
    vircadia_vector dimensions;
    /// @brief The offset of the bounding box from avatar position.
    vircadia_vector offset;
};

/// @brief Defines the layout of position-rotation pairs (vantage
/// points).
struct vircadia_vantage {
    /// @brief The position component.
    vircadia_vector position;
    /// @brief The rotation component.
    vircadia_quaternion rotation;
};

/// @brief Defines the layout linear transformation data.
struct vircadia_transform {
    /// @brief The translation and rotation pair.
    vircadia_vantage vantage;
    /// @brief The scale.
    float scale;
};

#endif /* end of include guard */
