//
//  TextOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextOverlay.h"

#include <QQuickItem>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GLMHelpers.h>
#include <OffscreenUi.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>
#include <TextureCache.h>
#include <ViewFrustum.h>

#include "Application.h"
#include "text/FontFamilies.h"

QString const TextOverlay::TYPE = "text";
QUrl const TextOverlay::URL(QString("hifi/overlays/TextOverlay.qml"));

// TextOverlay's properties are defined in the QML file specified above.
/**jsdoc
 * These are the properties of a <code>text</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.TextProperties
 *
 * @property {Rect} bounds - The position and size of the rectangle, in pixels. <em>Write-only.</em>
 * @property {number} x - Integer left, x-coordinate value = <code>bounds.x</code>. <em>Write-only.</em>
 * @property {number} y - Integer top, y-coordinate value = <code>bounds.y</code>. <em>Write-only.</em>
 * @property {number} width - Integer width of the rectangle = <code>bounds.width</code>. <em>Write-only.</em>
 * @property {number} height - Integer height of the rectangle = <code>bounds.height</code>. <em>Write-only.</em>
 *
 * @property {number} margin=0 - Sets the <code>leftMargin</code> and <code>topMargin</code> values, in pixels.
 *     <em>Write-only.</em>
 * @property {number} leftMargin=0 - The left margin's size, in pixels. <em>Write-only.</em>
 * @property {number} topMargin=0 - The top margin's size, in pixels. <em>Write-only.</em>
 * @property {string} text="" - The text to display. Text does not automatically wrap; use <code>\n</code> for a line break. Text
 *     is clipped to the <code>bounds</code>. <em>Write-only.</em>
 * @property {number} font.size=18 - The size of the text, in pixels. <em>Write-only.</em>
 * @property {number} lineHeight=18 - The height of a line of text, in pixels. <em>Write-only.</em>
 * @property {Color} color=255,255,255 - The color of the text. Synonym: <code>textColor</code>. <em>Write-only.</em>
 * @property {number} alpha=1.0 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>. <em>Write-only.</em>
 * @property {Color} backgroundColor=0,0,0 - The color of the background rectangle. <em>Write-only.</em>
 * @property {number} backgroundAlpha=0.7 - The opacity of the background rectangle. <em>Write-only.</em>
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 *     <em>Write-only.</em>
 */

TextOverlay::TextOverlay() : QmlOverlay(URL) { }

TextOverlay::TextOverlay(const TextOverlay* textOverlay) 
    : QmlOverlay(URL, textOverlay) {
}

TextOverlay::~TextOverlay() { }

TextOverlay* TextOverlay::createClone() const {
    return new TextOverlay(this);
}

QSizeF TextOverlay::textSize(const QString& text) const {
    int lines = 1;
    foreach(QChar c, text) {
        if (c == QChar('\n')) {
            ++lines;
        }
    }
    QFont font(SANS_FONT_FAMILY);
    font.setPixelSize(18);
    QFontMetrics fm(font);
    QSizeF result = QSizeF(fm.width(text), 18 * lines);
    return result; 
}


void TextOverlay::setTopMargin(float margin) {}
void TextOverlay::setLeftMargin(float margin) {}
void TextOverlay::setFontSize(float size) {}
void TextOverlay::setText(const QString& text) {}
