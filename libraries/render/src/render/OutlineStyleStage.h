//
//  OutlineStyleStage.h

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_outlinestage_h
#define hifi_render_utils_outlinestage_h

#include "Stage.h"
#include "Engine.h"
#include "IndexedContainer.h"
#include "OutlineStyle.h"

namespace render {

    // OutlineStyle stage to set up OutlineStyle-related effects
    class OutlineStyleStage : public Stage {
    public:

        class Outline {
        public:

            Outline(const std::string& selectionName, const OutlineStyle& style) : _selectionName{ selectionName }, _style{ style } { }

            std::string _selectionName;
            OutlineStyle _style;

        };

        static const std::string& getName() { return _name; }

        using Index = render::indexed_container::Index;
        static const Index INVALID_INDEX{ render::indexed_container::INVALID_INDEX };
        using OutlineIdList = render::indexed_container::Indices;

        static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }

        bool checkOutlineId(Index index) const { return _outlines.checkIndex(index); }

        const Outline& getOutline(Index OutlineStyleId) const { return _outlines.get(OutlineStyleId); }
        Outline& editOutline(Index OutlineStyleId) { return _outlines.edit(OutlineStyleId); }

        Index addOutline(const std::string& selectionName, const OutlineStyle& style = OutlineStyle());
        Index getOutlineIdBySelection(const std::string& selectionName) const;
        void removeOutline(Index index);

        OutlineIdList::iterator begin() { return _activeOutlineIds.begin(); }
        OutlineIdList::iterator end() { return _activeOutlineIds.end(); }

    private:

        using Outlines = render::indexed_container::IndexedVector<Outline>;

        static std::string _name;

        Outlines _outlines;
        OutlineIdList _activeOutlineIds;
    };
    using OutlineStyleStagePointer = std::shared_ptr<OutlineStyleStage>;

    class OutlineStyleStageSetup {
    public:
        using JobModel = render::Job::Model<OutlineStyleStageSetup>;

        OutlineStyleStageSetup();
        void run(const RenderContextPointer& renderContext);

    protected:
    };

}
#endif // hifi_render_utils_outlinestage_h
