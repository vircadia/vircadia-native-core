//
//  HighlightStage.h

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_HighlightStage_h
#define hifi_render_utils_HighlightStage_h

#include "Stage.h"
#include "Engine.h"
#include "IndexedContainer.h"
#include "HighlightStyle.h"

namespace render {

    // Highlight stage to set up HighlightStyle-related effects
    class HighlightStage : public Stage {
    public:

        class Highlight {
        public:

            Highlight(const std::string& selectionName, const HighlightStyle& style) : _selectionName{ selectionName }, _style{ style } { }

            std::string _selectionName;
            HighlightStyle _style;

        };

        static const std::string& getName() { return _name; }

        using Index = render::indexed_container::Index;
        static const Index INVALID_INDEX{ render::indexed_container::INVALID_INDEX };
        using HighlightIdList = render::indexed_container::Indices;

        static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }

        bool checkHighlightId(Index index) const { return _highlights.checkIndex(index); }

        const Highlight& getHighlight(Index index) const { return _highlights.get(index); }
        Highlight& editHighlight(Index index) { return _highlights.edit(index); }

        Index addHighlight(const std::string& selectionName, const HighlightStyle& style = HighlightStyle());
        Index getHighlightIdBySelection(const std::string& selectionName) const;
        void removeHighlight(Index index);

        HighlightIdList::iterator begin() { return _activeHighlightIds.begin(); }
        HighlightIdList::iterator end() { return _activeHighlightIds.end(); }

    private:

        using Highlights = render::indexed_container::IndexedVector<Highlight>;

        static std::string _name;

        Highlights _highlights;
        HighlightIdList _activeHighlightIds;
    };
    using HighlightStagePointer = std::shared_ptr<HighlightStage>;

    class HighlightStageSetup {
    public:
        using JobModel = render::Job::Model<HighlightStageSetup>;

        HighlightStageSetup();
        void run(const RenderContextPointer& renderContext);

    protected:
    };

}
#endif // hifi_render_utils_HighlightStage_h
