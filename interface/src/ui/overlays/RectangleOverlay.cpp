//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RectangleOverlay.h"

QString const RectangleOverlay::TYPE = "rectangle";
QUrl const RectangleOverlay::URL(QString("hifi/overlays/RectangleOverlay.qml"));

// RectangleOverlay's properties are defined in the QML file specified above.
/**jsdoc
 * These are the properties of a <code>rectangle</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.RectangleProperties
 *
 * @property {Rect} bounds - The position and size of the rectangle, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 *
 * @property {Color} color=0,0,0 - The color of the overlay. <em>Write-only.</em>
 * @property {number} alpha=1.0 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. <em>Write-only.</em>
 * @property {number} borderWidth=1 - Integer width of the border, in pixels. The border is drawn within the rectangle's bounds.
 *     It is not drawn unless either <code>borderColor</code> or <code>borderAlpha</code> are specified. <em>Write-only.</em>
 * @property {number} radius=0 - Integer corner radius, in pixels. <em>Write-only.</em>
 * @property {Color} borderColor=0,0,0 - The color of the border. <em>Write-only.</em>
 * @property {number} borderAlpha=1.0 - The opacity of the border, <code>0.0</code> - <code>1.0</code>.
 *     <em>Write-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *     <em>Write-only.</em>
 */

RectangleOverlay::RectangleOverlay() : QmlOverlay(URL) {}

RectangleOverlay::RectangleOverlay(const RectangleOverlay* rectangleOverlay) 
    : QmlOverlay(URL, rectangleOverlay) { }

RectangleOverlay* RectangleOverlay::createClone() const {
    return new RectangleOverlay(this);
}
