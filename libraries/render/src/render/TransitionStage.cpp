#include "TransitionStage.h"

#include <algorithm>

using namespace render;

TransitionStage::Index TransitionStage::addTransition(ItemID itemId, Transition::Type type) {
    Transition transition;
    Index id;
    
    transition.eventType = type;
    transition.itemId = itemId;
    id = _transitions.newElement(transition);
    _activeTransitionIds.push_back(id);

    return id;
}

void TransitionStage::removeTransition(Index index) {
    TransitionIdList::iterator  idIterator = std::find(_activeTransitionIds.begin(), _activeTransitionIds.end(), index);
    if (idIterator != _activeTransitionIds.end()) {
        _activeTransitionIds.erase(idIterator);
    }
    _transitions.freeElement(index);
}
