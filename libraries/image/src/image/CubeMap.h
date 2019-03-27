//
//  CubeMap.h
//  image/src/image
//
//  Created by Olivier Prat on 03/27/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_image_CubeMap_h
#define hifi_image_CubeMap_h

#include <gpu/Forward.h>
#include <glm/vec4.hpp>
#include <vector>
#include <array>

namespace image {

    class CubeMap {
    public:
        
        using Face = std::vector<glm::vec4>;
        using Faces = std::array<Face, 6>;

        CubeMap(int width, int height, int mipCount);

        gpu::uint16 getMipCount() const { return (gpu::uint16)_mips.size(); }
        Faces& editMip(gpu::uint16 mipLevel) { return _mips[mipLevel]; }
        const Faces& getMip(gpu::uint16 mipLevel) const { return _mips[mipLevel]; }

    private:

        int _width;
        int _height;
        std::vector<Faces> _mips;
    };

}

#endif // hifi_image_CubeMap_h
