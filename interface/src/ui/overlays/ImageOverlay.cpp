//
//  ImageOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ImageOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <gpu/Context.h>
#include <RegisteredMetaTypes.h>


QString const ImageOverlay::TYPE = "image";
QUrl const ImageOverlay::URL(QString("hifi/overlays/ImageOverlay.qml"));

// ImageOverlay's properties are defined in the QML file specified above.
/**jsdoc
 * These are the properties of an <code>image</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.ImageProperties
 *
 * @property {Rect} bounds - The position and size of the image display area, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value of the image display area = <code>bounds.x</code>.
 *     <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value of the image display area = <code>bounds.y</code>.
 *     <em>Write-only.</em>
 * @property {number} width - Integer width of the image display area = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the image display area = <code>bounds.height</code>. <em>Write-only.</em>
 * @property {string} imageURL - The URL of the image file to display. The image is scaled to fit to the <code>bounds</code>.
 *     <em>Write-only.</em>
 * @property {Vec2} subImage=0,0 - Integer coordinates of the top left pixel to start using image content from.
 *     <em>Write-only.</em>
 * @property {Color} color=0,0,0 - The color to apply over the top of the image to colorize it. <em>Write-only.</em>
 * @property {number} alpha=0.0 - The opacity of the color applied over the top of the image, <code>0.0</code> - 
 *     <code>1.0</code>. <em>Write-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *     <em>Write-only.</em>
 */

ImageOverlay::ImageOverlay() 
    : QmlOverlay(URL) { }

ImageOverlay::ImageOverlay(const ImageOverlay* imageOverlay) :
    QmlOverlay(URL, imageOverlay) { }

ImageOverlay* ImageOverlay::createClone() const {
    return new ImageOverlay(this);
}

