#include "TransitionStage.h"

#include <algorithm>

using namespace render;

std::string TransitionStage::_name("Transition");

TransitionStage::Index TransitionStage::addTransition(ItemID itemId, Transition::Type type, ItemID boundId) {
    Transition transition;
    Index id;
    
    transition.eventType = type;
    transition.itemId = itemId;
    transition.boundItemId = boundId;
    id = _transitions.newElement(transition);
    _activeTransitionIds.push_back(id);

    return id;
}

void TransitionStage::removeTransition(Index index) {
    TransitionIdList::iterator  idIterator = std::find(_activeTransitionIds.begin(), _activeTransitionIds.end(), index);
    if (idIterator != _activeTransitionIds.end()) {
        _activeTransitionIds.erase(idIterator);
    }
    if (!_transitions.isElementFreed(index)) {
        _transitions.freeElement(index);
    }
}

TransitionStageSetup::TransitionStageSetup() {
}

void TransitionStageSetup::run(const RenderContextPointer& renderContext) {
    auto stage = renderContext->_scene->getStage(TransitionStage::getName());
    if (!stage) {
        stage = std::make_shared<TransitionStage>();
        renderContext->_scene->resetStage(TransitionStage::getName(), stage);
    }
}

