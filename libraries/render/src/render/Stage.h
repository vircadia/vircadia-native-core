//
//  Stage.h
//  render/src/render
//
//  Created by Sam Gateau on 6/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Stage_h
#define hifi_render_Stage_h

#include <memory>
#include <map>
#include <string>

namespace render {

    class Stage {
    public:
        using Name = std::string;

        Stage();
        virtual ~Stage();

    protected:
        Name _name;
    };

    using StagePointer = std::shared_ptr<Stage>;

    using StageMap = std::map<const Stage::Name, StagePointer>;

}

#endif // hifi_render_Stage_h
