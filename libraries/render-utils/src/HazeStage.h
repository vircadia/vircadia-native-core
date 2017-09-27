//
//  HazeStage.h

//  Created by Nissim Hadar on 9/26/2017.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_HazeStage_h
#define hifi_render_utils_HazeStage_h

#include <model/Stage.h>
#include <set>
#include <unordered_map>
#include <render/IndexedContainer.h>
#include <render/Stage.h>

#include "LightingModel.h"

// Haze stage to set up haze-related rendering tasks
class HazeStage : public render::Stage {
public:
    static std::string _stageName;
    static const std::string& getName() { return _stageName; }

    using Index = render::indexed_container::Index;
    static const Index INVALID_INDEX { render::indexed_container::INVALID_INDEX };
    static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }
    
    using HazePointer = model::SunSkyStagePointer;
    using Hazes = render::indexed_container::IndexedPointerVector<model::SunSkyStage>;
    using HazeMap = std::unordered_map<HazePointer, Index>;

    using HazeIndices = std::vector<Index>;


    Index findHaze(const HazePointer& haze) const;
    Index addHaze(const HazePointer& haze);

    HazePointer removeHaze(Index index);
    
    bool checkHazeId(Index index) const { return _hazes.checkIndex(index); }

    Index getNumHazes() const { return _hazes.getNumElements(); }
    Index getNumFreeHazes() const { return _hazes.getNumFreeIndices(); }
    Index getNumAllocatedHazes() const { return _hazes.getNumAllocatedIndices(); }

    HazePointer getHaze(Index hazeId) const {
        return _hazes.get(hazeId);
    }

    Hazes _hazes;
    HazeMap _hazeMap;

    class Frame {
    public:
        Frame() {}
        
        void clear() { _hazes.clear(); }

        void pushHaze(HazeStage::Index index) { _hazes.emplace_back(index); }

        HazeStage::HazeIndices _hazes;
    };
    
    Frame _currentFrame;
};
using HazeStagePointer = std::shared_ptr<HazeStage>;

class HazeStageSetup {
public:
    using JobModel = render::Job::Model<HazeStageSetup>;

    HazeStageSetup();
    void run(const render::RenderContextPointer& renderContext);

protected:
};

#endif
