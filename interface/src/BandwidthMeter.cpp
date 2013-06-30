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

namespace { // .cpp-local

    int const AREA_WIDTH = -400;            // Width of the area used. Aligned to the right when negative.
    int const AREA_HEIGHT =  32;            // Height of the area used. Aligned to the bottom when negative.
    int const BORDER_DISTANCE_HORIZ = -20;  // Distance to edge of screen (use negative value when width is negative).
    int const BORDER_DISTANCE_VERT = 20;    // Distance to edge of screen (use negative value when height is negative).

    char const* CAPTION_IN = "IN";
    char const* CAPTION_OUT = "OUT";
    char const* CAPTION_UNIT = "Mbps";

    double const UNIT_SCALE = 1000.0 / (1024.0 * 1024.0);  // Bytes/ms -> Mbps
    int const INITIAL_SCALE_MAXIMUM_INDEX = 250; // / 9: exponent, % 9: mantissa - 2, 0 o--o 2 * 10^-10

    int SPACING_RIGHT_CAPTION_IN_OUT = 4;
    int SPACING_LEFT_CAPTION_UNIT = 6;

    int SPACING_VERT_BARS = 2;

    unsigned const COLOR_CAPTIONS = 0xa0a0a0e0;
    unsigned const COLOR_FRAME = 0xe0e0e0b0;
    unsigned const COLOR_INDICATOR = 0xc0c0c0b0;

}

BandwidthMeter::ChannelInfo BandwidthMeter::_DEFAULT_CHANNELS[] = {
    { "Audio"   , "Kbps", 1000.0 / 1024.0, 0x40ff40d0 },
    { "Avatars" , "Kbps", 1000.0 / 1024.0, 0xffef40c0 },
    { "Voxels"  , "Kbps", 1000.0 / 1024.0, 0xd0d0d0a0 }
};

BandwidthMeter::BandwidthMeter() :
    _textRenderer(SANS_FONT_FAMILY, -1, -1, false, TextRenderer::SHADOW_EFFECT),
    _scaleMaxIndex(INITIAL_SCALE_MAXIMUM_INDEX) {

    memcpy(_channels, _DEFAULT_CHANNELS, sizeof(_DEFAULT_CHANNELS));
}

BandwidthMeter::Stream::Stream(float msToAverage) :
    _value(0.0f),
    _msToAverage(msToAverage) {

    gettimeofday(& _prevTime, NULL);
}

void BandwidthMeter::Stream::updateValue(double amount) {

    // Determine elapsed time
    timeval now;
    gettimeofday(& now, NULL);
    double dt = diffclock(& _prevTime, & now);
    memcpy(& _prevTime, & now, sizeof(timeval));

    // Compute approximate average
    _value = glm::mix(_value, amount / dt,
                      glm::clamp(dt / _msToAverage, 0.0, 1.0));
}

void BandwidthMeter::setColorRGBA(unsigned c) {

    glColor4ub(GLubyte( c >> 24), 
               GLubyte((c >> 16) & 0xff),
               GLubyte((c >>  8) & 0xff),
               GLubyte( c        & 0xff));
}

void BandwidthMeter::renderBox(int x, int y, int w, int h) {

    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
}

void BandwidthMeter::renderVerticalLine(int x, int y, int h) {

    glBegin(GL_LINES);
    glVertex2i(x, y);
    glVertex2i(x, y + h);
    glEnd();
}

inline int BandwidthMeter::centered(int subject, int object) {
    return (object - subject) / 2;
}

bool BandwidthMeter::isWithinArea(int x, int y, int screenWidth, int screenHeight) {

    int minX = BORDER_DISTANCE_HORIZ + (AREA_WIDTH >= 0 ? 0 : screenWidth + AREA_WIDTH);
    int minY = BORDER_DISTANCE_VERT + (AREA_HEIGHT >= 0 ? 0 : screenHeight + AREA_HEIGHT);

    return x >= minX && x < minX + glm::abs(AREA_WIDTH) && 
           y >= minY && y < minY + glm::abs(AREA_HEIGHT);
}

void BandwidthMeter::render(int screenWidth, int screenHeight) {

    int x = BORDER_DISTANCE_HORIZ + (AREA_WIDTH >= 0 ? 0 : screenWidth + AREA_WIDTH);
    int y = BORDER_DISTANCE_VERT + (AREA_HEIGHT >= 0 ? 0 : screenHeight + AREA_HEIGHT);
    int w = glm::abs(AREA_WIDTH), h = glm::abs(AREA_HEIGHT);

    // Determine total
    float totalIn = 0.0f, totalOut = 0.0f;
    for (int i = 0; i < N_CHANNELS; ++i) {

        totalIn += inputStream(i).getValue();
        totalOut += outputStream(i).getValue();
    }
    totalIn *= UNIT_SCALE;
    totalOut *= UNIT_SCALE;
    float totalMax = glm::max(totalIn, totalOut);

    // Get font / caption metrics
    QFontMetrics const& fontMetrics = _textRenderer.metrics();
    int fontDescent = fontMetrics.descent(); 
    int labelWidthIn = fontMetrics.width(CAPTION_IN);
    int labelWidthOut = fontMetrics.width(CAPTION_OUT);
    int labelWidthInOut = glm::max(labelWidthIn, labelWidthOut);
    int labelHeight = fontMetrics.ascent() + fontDescent;
    int labelWidthUnit = fontMetrics.width(CAPTION_UNIT);
    int labelsWidth = labelWidthInOut + SPACING_RIGHT_CAPTION_IN_OUT + SPACING_LEFT_CAPTION_UNIT + labelWidthUnit;

    // Calculate coordinates and dimensions
    int barX = x + labelWidthInOut + SPACING_RIGHT_CAPTION_IN_OUT;
    int barWidth = w - labelsWidth;
    int barHeight = (h - SPACING_VERT_BARS) / 2;
    int textYcenteredLine = h - centered(labelHeight, h) - fontDescent;
    int textYupperLine = barHeight - centered(labelHeight, barHeight) - fontDescent;
    int textYlowerLine = h - centered(labelHeight, barHeight) - fontDescent;

    // Center of coordinate system -> upper left of bar
    glPushMatrix();
    glTranslatef(float(barX), float(y), 0.0f);

    // Render captions
    setColorRGBA(COLOR_CAPTIONS);
    _textRenderer.draw(barWidth + SPACING_LEFT_CAPTION_UNIT, textYcenteredLine, CAPTION_UNIT);
    _textRenderer.draw(-labelWidthIn - SPACING_RIGHT_CAPTION_IN_OUT, textYupperLine, CAPTION_IN);
    _textRenderer.draw(-labelWidthOut - SPACING_RIGHT_CAPTION_IN_OUT, textYlowerLine, CAPTION_OUT);

    // Render vertical lines for the frame
    setColorRGBA(COLOR_FRAME);
    renderVerticalLine(0, 0, h);
    renderVerticalLine(barWidth, 0, h);

    // Adjust scale
    int steps;
    double step, scaleMax;
    bool commit = false;
    do {
        steps = (_scaleMaxIndex % 9) + 2;
        step = pow(10.0, (_scaleMaxIndex / 9) - 10);
        scaleMax = step * steps;
        if (commit) {
//            printLog("Bandwidth meter scale: %d\n", _scaleMaxIndex);
            break;
        }
        if (totalMax < scaleMax * 0.5) {
            _scaleMaxIndex = glm::max(0, _scaleMaxIndex-1);
            commit = true;
        } else if (totalMax > scaleMax) {
            _scaleMaxIndex += 1;
            commit = true;
        }
    } while (commit);

    // Render scale indicators
    setColorRGBA(COLOR_INDICATOR);
    for (int j = int((scaleMax + step - 0.000001) / step); --j > 0;) {
        renderVerticalLine(int(barWidth * j * step / scaleMax), 0, h);
    }

    // Render bars
    int xIn = 0, xOut = 0;
    for (int i = 0; i < N_CHANNELS; ++i) {

        int wIn = int(barWidth * inputStream(i).getValue() * UNIT_SCALE / scaleMax);
        int wOut = int(barWidth * outputStream(i).getValue() * UNIT_SCALE / scaleMax);

        setColorRGBA(channelInfo(i).colorRGBA);

        if (wIn > 0) {
            renderBox(xIn, 0, wIn, barHeight);
        }
        xIn += wIn;

        if (wOut > 0) {
            renderBox(xOut, h - barHeight, wOut, barHeight);
        }
        xOut += wOut;
    }

    // Render numbers
    char fmtBuf[8];
    setColorRGBA(COLOR_CAPTIONS);
    sprintf(fmtBuf, "%0.2f", totalIn);
    _textRenderer.draw(glm::max(xIn - fontMetrics.width(fmtBuf), 0), textYupperLine, fmtBuf);
    sprintf(fmtBuf, "%0.2f", totalOut);
    _textRenderer.draw(glm::max(xOut - fontMetrics.width(fmtBuf), 0), textYlowerLine, fmtBuf);

    glPopMatrix();
}


