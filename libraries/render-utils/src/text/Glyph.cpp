#include "Glyph.h"
#include <StreamHelpers.h>

// We adjust bounds because offset is the bottom left corner of the font but the top left corner of a QRect
QRectF Glyph::bounds() const {
    return glmToRect(offset, size).translated(0.0f, -size.y);
}

QRectF Glyph::textureBounds() const {
    return glmToRect(texOffset, texSize);
}

void Glyph::read(QIODevice& in) {
    uint16_t charcode;
    readStream(in, charcode);
    c = charcode;
    readStream(in, texOffset);
    readStream(in, size);
    readStream(in, offset);
    readStream(in, d);
    // texSize is divided by the image size later
    texSize = size;
}
