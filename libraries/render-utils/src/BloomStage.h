//
//  BloomStage.h

//  Created by Sam Gondelman on 8/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_BloomStage_h
#define hifi_render_utils_BloomStage_h

#include <graphics/Stage.h>
#include <set>
#include <unordered_map>
#include <render/IndexedContainer.h>
#include <render/Stage.h>

#include <render/Forward.h>
#include <render/DrawTask.h>
#include <graphics/Bloom.h>

// Bloom stage to set up bloom-related rendering tasks
class BloomStage : public render::Stage {
public:
    static std::string _stageName;
    static const std::string& getName() { return _stageName; }

    using Index = render::indexed_container::Index;
    static const Index INVALID_INDEX;
    static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }
    
    using BloomPointer = graphics::BloomPointer;
    using Blooms = render::indexed_container::IndexedPointerVector<graphics::Bloom>;
    using BloomMap = std::unordered_map<BloomPointer, Index>;

    using BloomIndices = std::vector<Index>;

    Index findBloom(const BloomPointer& bloom) const;
    Index addBloom(const BloomPointer& bloom);

    BloomPointer removeBloom(Index index);
    
    bool checkBloomId(Index index) const { return _blooms.checkIndex(index); }

    Index getNumBlooms() const { return _blooms.getNumElements(); }
    Index getNumFreeBlooms() const { return _blooms.getNumFreeIndices(); }
    Index getNumAllocatedBlooms() const { return _blooms.getNumAllocatedIndices(); }

    BloomPointer getBloom(Index bloomId) const {
        return _blooms.get(bloomId);
    }

    Blooms _blooms;
    BloomMap _bloomMap;

    class Frame {
    public:
        Frame() {}
        
        void clear() { _blooms.clear(); }

        void pushBloom(BloomStage::Index index) { _blooms.emplace_back(index); }

        BloomStage::BloomIndices _blooms;
    };
    
    Frame _currentFrame;
};
using BloomStagePointer = std::shared_ptr<BloomStage>;

class BloomStageSetup {
public:
    using JobModel = render::Job::Model<BloomStageSetup>;

    BloomStageSetup();
    void run(const render::RenderContextPointer& renderContext);

protected:
};

class FetchBloomConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float bloomIntensity MEMBER bloomIntensity WRITE setBloomIntensity NOTIFY dirty);
    Q_PROPERTY(float bloomThreshold MEMBER bloomThreshold WRITE setBloomThreshold NOTIFY dirty);
    Q_PROPERTY(float bloomSize MEMBER bloomSize WRITE setBloomSize NOTIFY dirty);

public:
    FetchBloomConfig() : render::Job::Config() {}

    float bloomIntensity { graphics::Bloom::INITIAL_BLOOM_INTENSITY };
    float bloomThreshold { graphics::Bloom::INITIAL_BLOOM_THRESHOLD };
    float bloomSize { graphics::Bloom::INITIAL_BLOOM_SIZE };

public slots:
    void setBloomIntensity(const float value) { bloomIntensity = value; emit dirty(); }
    void setBloomThreshold(const float value) { bloomThreshold = value; emit dirty(); }
    void setBloomSize(const float value) { bloomSize = value; emit dirty(); }

signals:
    void dirty();
};

class FetchBloomStage {
public:
    using Config = FetchBloomConfig;
    using JobModel = render::Job::ModelO<FetchBloomStage, graphics::BloomPointer, Config>;

    FetchBloomStage();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, graphics::BloomPointer& bloom);

private:
    graphics::BloomPointer _bloom;
};
#endif
