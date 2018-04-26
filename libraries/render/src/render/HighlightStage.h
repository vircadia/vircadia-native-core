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
        static const Index INVALID_INDEX;
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
        const HighlightIdList&  getActiveHighlightIds() const { return _activeHighlightIds; }

    private:

        using Highlights = render::indexed_container::IndexedVector<Highlight>;

        static std::string _name;

        Highlights _highlights;
        HighlightIdList _activeHighlightIds;
    };
    using HighlightStagePointer = std::shared_ptr<HighlightStage>;

    class HighlightStageConfig : public render::Job::Config {
        Q_OBJECT
            Q_PROPERTY(QString selectionName READ getSelectionName WRITE setSelectionName NOTIFY dirty)
            Q_PROPERTY(bool isOutlineSmooth READ isOutlineSmooth WRITE setOutlineSmooth NOTIFY dirty)
            Q_PROPERTY(float colorR READ getColorRed WRITE setColorRed NOTIFY dirty)
            Q_PROPERTY(float colorG READ getColorGreen WRITE setColorGreen NOTIFY dirty)
            Q_PROPERTY(float colorB READ getColorBlue WRITE setColorBlue NOTIFY dirty)
            Q_PROPERTY(float outlineWidth READ getOutlineWidth WRITE setOutlineWidth NOTIFY dirty)
            Q_PROPERTY(float outlineIntensity READ getOutlineIntensity WRITE setOutlineIntensity NOTIFY dirty)
            Q_PROPERTY(float unoccludedFillOpacity READ getUnoccludedFillOpacity WRITE setUnoccludedFillOpacity NOTIFY dirty)
            Q_PROPERTY(float occludedFillOpacity READ getOccludedFillOpacity WRITE setOccludedFillOpacity NOTIFY dirty)

    public:

        using SelectionStyles = std::map<std::string, HighlightStyle>;

        QString getSelectionName() const { return QString(_selectionName.c_str()); }
        void setSelectionName(const QString& name);

        bool isOutlineSmooth() const { return getStyle()._isOutlineSmooth; }
        void setOutlineSmooth(bool isSmooth);

        float getColorRed() const { return getStyle()._outlineUnoccluded.color.r; }
        void setColorRed(float value);

        float getColorGreen() const { return getStyle()._outlineUnoccluded.color.g; }
        void setColorGreen(float value);

        float getColorBlue() const { return getStyle()._outlineUnoccluded.color.b; }
        void setColorBlue(float value);

        float getOutlineWidth() const { return getStyle()._outlineWidth; }
        void setOutlineWidth(float value);

        float getOutlineIntensity() const { return getStyle()._outlineUnoccluded.alpha; }
        void setOutlineIntensity(float value);

        float getUnoccludedFillOpacity() const { return getStyle()._fillUnoccluded.alpha; }
        void setUnoccludedFillOpacity(float value);

        float getOccludedFillOpacity() const { return getStyle()._fillOccluded.alpha; }
        void setOccludedFillOpacity(float value);

        std::string _selectionName{ "contextOverlayHighlightList" };
        mutable SelectionStyles _styles;

        const HighlightStyle& getStyle() const;
        HighlightStyle& editStyle();

    signals:
        void dirty();
    };

    class HighlightStageSetup {
    public:
        using Config = HighlightStageConfig;
        using JobModel = render::Job::Model<HighlightStageSetup, Config>;

        HighlightStageSetup();

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext);

    protected:

        HighlightStageConfig::SelectionStyles _styles;
    };

}
#endif // hifi_render_utils_HighlightStage_h
