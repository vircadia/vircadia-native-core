#include "HighlightStage.h"

using namespace render;

std::string HighlightStage::_name("Highlight");

HighlightStage::Index HighlightStage::addHighlight(const std::string& selectionName, const HighlightStyle& style) {
    Highlight outline{ selectionName, style };
    Index id;

    id = _highlights.newElement(outline);
    _activeHighlightIds.push_back(id);

    return id;
}

void HighlightStage::removeHighlight(Index index) {
    HighlightIdList::iterator  idIterator = std::find(_activeHighlightIds.begin(), _activeHighlightIds.end(), index);
    if (idIterator != _activeHighlightIds.end()) {
        _activeHighlightIds.erase(idIterator);
    }
    if (!_highlights.isElementFreed(index)) {
        _highlights.freeElement(index);
    }
}

Index HighlightStage::getHighlightIdBySelection(const std::string& selectionName) const {
    for (auto outlineId : _activeHighlightIds) {
        const auto& outline = _highlights.get(outlineId);
        if (outline._selectionName == selectionName) {
            return outlineId;
        }
    }
    return INVALID_INDEX;
}

HighlightStageSetup::HighlightStageSetup() {
}

void HighlightStageSetup::run(const render::RenderContextPointer& renderContext) {
    if (!renderContext->_scene->getStage(HighlightStage::getName())) {
        auto stage = std::make_shared<HighlightStage>();
        renderContext->_scene->resetStage(HighlightStage::getName(), stage);
    }
}

