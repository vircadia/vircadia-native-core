//
//  BandwidthMeter.h
//  interface
//
//  Created by Tobias Schwinger on 6/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "BandwidthMeter.h"
#include "InterfaceConfig.h"

#include "Log.h"
#include "Util.h"

// --- Configuration

// Layout:
//
//  +--- unit label width (e)
//  |  +-- unit label horiz. spacing (f)
//  V  V
// |  | |
//      +--------+  \                               \  Channel
// Unit +-------+    | Total channel height (a)     /   height (d)
//      |           /        ] Channel spacing (b)
//      +----+
// Unit +------+
//      |
//     ...  
//

namespace { // .cpp-local

    float const CHANNEL_SPACING = 0.125f;            // (b) fractional in respect to total channel height
    float const STREAM_SPACING = 0.0625f;            // (c) fractional in respect to total channel height
    float const LABEL_CHAR_WIDTH = 0.28f;            // (e) fractional in respect to total channel height
    float const LABEL_HORIZ_SPACING = 0.25f;         // (f) fractional in respect to total channel height

    unsigned const FRAME_COLOR = 0xe0e0e0b0;
    unsigned const INDICATOR_COLOR = 0xc0c0c0b0;
    unsigned const VALUE_COLOR = 0xa0a0a0e0;
    float const INDICATOR_BASE = 10.0f;
    float const INDICATOR_LOG_BASE = log(INDICATOR_BASE);
}

BandwidthMeter::ChannelInfo BandwidthMeter::_DEFAULT_CHANNELS[] = {
    { "Audio"   , "Kbps",    1.0 * 1024.0,  85.0, 0x40ff40d0 },
    { "Avatars" , "KBps",    1.0 * 1024.0,  20.0, 0xffef40c0 },
    { "Voxels"  , "Mbps", 1024.0 * 1024.0,   0.5, 0xd0d0d0a0 }
};

// ---

BandwidthMeter::BandwidthMeter() :
    _textRenderer(SANS_FONT_FAMILY, -1, -1, false, TextRenderer::SHADOW_EFFECT) {

    memcpy(_channels, _DEFAULT_CHANNELS, sizeof(_DEFAULT_CHANNELS));
}

BandwidthMeter::Stream::Stream(float secondsToAverage) :
    _value(0.0f),
    _secsToAverage(secondsToAverage) {

    gettimeofday(& _prevTime, NULL);
}

void BandwidthMeter::Stream::updateValue(double amount) {

    // Determine elapsed time
    timeval now;
    gettimeofday(& now, NULL);
    double dt = diffclock(& _prevTime, & now) / 1000.0;
    memcpy(& _prevTime, & now, sizeof(timeval));

    // Compute approximate average
    _value = glm::mix(_value, amount / dt,
                      glm::clamp(dt / _secsToAverage, 0.0, 1.0));
}

static void setColorRGBA(unsigned c) {

    glColor4ub(GLubyte( c >> 24), 
               GLubyte((c >> 16) & 0xff),
               GLubyte((c >>  8) & 0xff),
               GLubyte( c        & 0xff));
}

static void renderBox(float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(w, 0.0f);
    glVertex2f(w, h);
    glVertex2f(0.0f, h);
    glEnd();
}

void BandwidthMeter::render(int x, int y, unsigned w, unsigned h) {

    float channelTotalHeight = float(h) / N_CHANNELS;
    float channelSpacing = CHANNEL_SPACING * channelTotalHeight;
    float channelHeight = channelTotalHeight - channelSpacing;
    float streamSpacing = STREAM_SPACING * channelTotalHeight;
    float streamHeight = (channelHeight - streamSpacing) * 0.5;

    QFontMetrics const& fontMetrics = _textRenderer.metrics();
    int fontDescent = fontMetrics.descent();
    int labelRenderWidthCaption = 0;
    int labelRenderWidthUnit = 0;
    for (int i = 0; i < N_CHANNELS; ++i) {
        labelRenderWidthCaption = glm::max(labelRenderWidthCaption, fontMetrics.width(_channels[i].caption));
        labelRenderWidthUnit = glm::max(labelRenderWidthUnit, fontMetrics.width(_channels[i].unitCaption));
    }
    int labelRenderWidth = glm::max(labelRenderWidthCaption, labelRenderWidthUnit);
    int labelRenderHeight = fontMetrics.lineSpacing();
    float labelCharWSpaces = float(labelRenderWidth) / float(fontMetrics.width("M"));
    float labelWidth = channelTotalHeight * LABEL_CHAR_WIDTH * labelCharWSpaces;
    float labelHeight = (channelHeight - streamSpacing) * 0.5f;

    float labelScaleX = labelWidth / float(labelRenderWidth);
    float labelScaleY = labelHeight / float(labelRenderHeight);
    float labelHorizSpacing = channelTotalHeight * LABEL_HORIZ_SPACING;


    glPushMatrix();

    int xMin = x + int(labelWidth + labelHorizSpacing);

    // Render vertical lines for the frame
    setColorRGBA(FRAME_COLOR);
    glBegin(GL_LINES);
    glVertex2i(xMin - 1, y);
    glVertex2i(xMin - 1, y + h);
    glVertex2i(x + w - 1, y);
    glVertex2i(x + w - 1, y + h);
    glEnd();

    // Set coordinate center to right edge of bars / top
    glTranslatef(float(xMin), float(y) + channelSpacing * 0.5f, 0.0f);

    // Determine maximum horizontal length for bars
    float xMax = float(w) - labelWidth - labelHorizSpacing;

    char fmtBuf[64];

    for (int i = 0; i < N_CHANNELS; ++i) {
        // ...each channel

        ChannelInfo& c = channelInfo(i);
        float scaleStep = powf(INDICATOR_BASE, ceil(logf(c.unitsMax) / INDICATOR_LOG_BASE) - 1.0f);

        float unitsIn = inputStream(i).getValue() / c.unitScale;
        if (unitsIn > c.unitsMax) {
            c.unitsMax += scaleStep;
        }
        float barPosIn = xMax * fmin(unitsIn / c.unitsMax, 1.0f);
        float unitsOut = outputStream(i).getValue() / c.unitScale;
        float barPosOut = xMax * fmin(unitsOut / c.unitsMax, 1.0f);

        // Render scale indicators
        setColorRGBA(INDICATOR_COLOR);
        for (int j = int((c.unitsMax + scaleStep - 1) / scaleStep); --j > 0;) {
            float xPix = xMax * float(j) * scaleStep / c.unitsMax;

            glBegin(GL_LINES);
            glVertex2f(xPix, 0);
            glVertex2f(xPix, channelHeight);
            glEnd();
        }

        // Render captions
        setColorRGBA(c.colorRGBA);
        glPushMatrix();
            glTranslatef(-labelHorizSpacing, channelHeight * 0.5f, 0.0f);
            glScalef(labelScaleX, labelScaleY, 1.0f);
            _textRenderer.draw(-labelRenderWidth, -fontDescent, c.caption);
            _textRenderer.draw(-fontMetrics.width(c.unitCaption), labelRenderHeight - fontDescent, c.unitCaption);
        glPopMatrix();

        // Render input bar
        renderBox(barPosIn, streamHeight);

        // Render output value
        glPushMatrix();
            setColorRGBA(VALUE_COLOR);
            glTranslatef(0.0f, streamHeight, 0.0f);
            glScalef(labelScaleX, labelScaleY, 1.0f);

            sprintf(fmtBuf, "%0.2f in", unitsIn);
            _textRenderer.draw(glm::max(int(barPosIn / labelScaleX) - fontMetrics.width(fmtBuf), 0), -fontDescent, fmtBuf);
        glPopMatrix();

        // Advance to next stream
        glTranslatef(0.0f, streamHeight + streamSpacing, 0.0f);

        // Render output bar
        setColorRGBA(c.colorRGBA);
        renderBox(barPosOut, streamHeight);

        // Render output value
        glPushMatrix();
            setColorRGBA(VALUE_COLOR);
            glTranslatef(0.0f, streamHeight, 0.0f);
            glScalef(labelScaleX, labelScaleY, 1.0f);

            sprintf(fmtBuf, "%0.2f out", unitsOut);
            _textRenderer.draw(glm::max(int(barPosOut / labelScaleX) - fontMetrics.width(fmtBuf), 0), -fontDescent, fmtBuf);
        glPopMatrix();

        // Advance to next channel
        glTranslatef(0.0f, streamHeight + channelSpacing, 0.0f);
    }

    glPopMatrix();
}


