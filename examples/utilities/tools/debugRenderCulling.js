//
//  debugRenderOctree.js
//  examples/utilities/tools
//
//  Sam Gateau
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("cookies.js");

var panel = new Panel(10, 300);
var drawOctree = Render.RenderDeferredTask.DrawSceneOctree;
Render.RenderDeferredTask.DrawSceneOctree.enabled = true;
Render.RenderDeferredTask.DrawItemSelection.enabled = true;

panel.newCheckbox("Show Octree Cells",
    function(value) { Render.RenderDeferredTask.DrawSceneOctree.showVisibleCells = value; },
    function() { return (Render.RenderDeferredTask.DrawSceneOctree.showVisibleCells); },
    function(value) { return (value); }
);
panel.newCheckbox("Show Empty Cells",
    function(value) { Render.RenderDeferredTask.DrawSceneOctree.showEmptyCells = value; },
    function() { return (Render.RenderDeferredTask.DrawSceneOctree.showEmptyCells); },
    function(value) { return (value); }
);
panel.newCheckbox("Freeze Frustum",
    function(value) { Render.RenderDeferredTask.FetchSceneSelection.freezeFrustum = value; Render.RenderDeferredTask.CullSceneSelection.freezeFrustum = value; },
    function() { return (Render.RenderDeferredTask.FetchSceneSelection.freezeFrustum); },
    function(value) { return (value); }
);
panel.newCheckbox("Show Inside Items",
    function(value) { Render.RenderDeferredTask.DrawItemSelection.showInsideItems = value; },
    function() { return (Render.RenderDeferredTask.DrawItemSelection.showInsideItems); },
    function(value) { return (value); }
);

panel.newCheckbox("Show Inside Subcell Items",
    function(value) { Render.RenderDeferredTask.DrawItemSelection.showInsideSubcellItems = value; },
    function() { return (Render.RenderDeferredTask.DrawItemSelection.showInsideSubcellItems); },
    function(value) { return (value); }
);

panel.newCheckbox("Show Partial Items",
    function(value) { Render.RenderDeferredTask.DrawItemSelection.showPartialItems = value; },
    function() { return (Render.RenderDeferredTask.DrawItemSelection.showPartialItems); },
    function(value) { return (value); }
);

panel.newCheckbox("Show Partial Subcell Items",
    function(value) { Render.RenderDeferredTask.DrawItemSelection.showPartialSubcellItems = value; },
    function() { return (Render.RenderDeferredTask.DrawItemSelection.showPartialSubcellItems); },
    function(value) { return (value); }
);

/*
panel.newSlider('Cells Free / Allocated', -1, 1, 
    function(value) { value; }, // setter
    function() { return Render.RenderDeferredTask.DrawSceneOctree.numFreeCells; }, // getter
    function(value) { return value; });

this.update = function () {
    var numFree = Render.RenderDeferredTask.DrawSceneOctree.numFreeCells;
    var numAlloc = Render.RenderDeferredTask.DrawSceneOctree.numAllocatedCells;
    var title = [
        ' ' + name,
        numFree + ' / ' + numAlloc
    ].join('\t');

    widget.editTitle({ text: title });
    slider.setMaxValue(numAlloc);
};
*/
function mouseMoveEvent(event) {
    panel.mouseMoveEvent(event);
}

function mousePressEvent(event) {
    panel.mousePressEvent(event);
}

function mouseReleaseEvent(event) {
    panel.mouseReleaseEvent(event);
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

function scriptEnding() {
    panel.destroy();
    Render.RenderDeferredTask.DrawSceneOctree.enabled = false;
    Render.RenderDeferredTask.DrawItemSelection.enabled = false;
}
Script.scriptEnding.connect(scriptEnding);


