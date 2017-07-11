//
//  TransitionStage.h

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_TransitionStage_h
#define hifi_render_TransitionStage_h

#include "Stage.h"
#include "IndexedContainer.h"
#include "Engine.h"
#include "Transition.h"

namespace render {

	// Transition stage to set up Transition-related effects
	class TransitionStage : public render::Stage {
	public:

        static const std::string& getName() { return _name; }

		using Index = indexed_container::Index;
		static const Index INVALID_INDEX{ indexed_container::INVALID_INDEX };
        using TransitionIdList = indexed_container::Indices;

        static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }

		bool checkTransitionId(Index index) const { return _transitions.checkIndex(index); }

        const Transition& getTransition(Index TransitionId) const { return _transitions.get(TransitionId); }

        Transition& editTransition(Index TransitionId) { return _transitions.edit(TransitionId); }

        Index addTransition(ItemID itemId, Transition::Type type, ItemID boundId);
        void removeTransition(Index index);

        TransitionIdList::iterator begin() { return _activeTransitionIds.begin(); }
        TransitionIdList::iterator end() { return _activeTransitionIds.end(); }

    private:

        using Transitions = indexed_container::IndexedVector<Transition>;

        static std::string _name;

		Transitions _transitions;
        TransitionIdList _activeTransitionIds;
	};
	using TransitionStagePointer = std::shared_ptr<TransitionStage>;

    class TransitionStageSetup {
    public:
        using JobModel = render::Job::Model<TransitionStageSetup>;

        TransitionStageSetup();
        void run(const RenderContextPointer& renderContext);

    protected:
    };

}

#endif // hifi_render_TransitionStage_h
