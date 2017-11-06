#include "OutlineStyleStage.h"

using namespace render;

std::string OutlineStyleStage::_name("OutlineStyle");

OutlineStyleStage::Index OutlineStyleStage::addOutline(const std::string& selectionName, const OutlineStyle& style) {
    Outline outline{ selectionName, style };
    Index id;

    id = _outlines.newElement(outline);
    _activeOutlineIds.push_back(id);

    return id;
}

void OutlineStyleStage::removeOutline(Index index) {
    OutlineIdList::iterator  idIterator = std::find(_activeOutlineIds.begin(), _activeOutlineIds.end(), index);
    if (idIterator != _activeOutlineIds.end()) {
        _activeOutlineIds.erase(idIterator);
    }
    if (!_outlines.isElementFreed(index)) {
        _outlines.freeElement(index);
    }
}

Index OutlineStyleStage::getOutlineIdBySelection(const std::string& selectionName) const {
    for (auto outlineId : _activeOutlineIds) {
        const auto& outline = _outlines.get(outlineId);
        if (outline._selectionName == selectionName) {
            return outlineId;
        }
    }
    return INVALID_INDEX;
}

OutlineStyleStageSetup::OutlineStyleStageSetup() {
}

void OutlineStyleStageSetup::run(const render::RenderContextPointer& renderContext) {
    if (!renderContext->_scene->getStage(OutlineStyleStage::getName())) {
        auto stage = std::make_shared<OutlineStyleStage>();
        renderContext->_scene->resetStage(OutlineStyleStage::getName(), stage);
    }
}

