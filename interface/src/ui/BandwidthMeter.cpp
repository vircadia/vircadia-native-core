//
//  BandwidthMeter.cpp
//  interface/src/ui
//
//  Created by Tobias Schwinger on 6/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstdio>

#include "BandwidthMeter.h"
#include "InterfaceConfig.h"

#include "Util.h"

namespace { // .cpp-local

    int const AREA_WIDTH = -280;            // Width of the area used. Aligned to the right when negative.
    int const AREA_HEIGHT =  -40;            // Height of the area used. Aligned to the bottom when negative.
    int const BORDER_DISTANCE_HORIZ = -10;  // Distance to edge of screen (use negative value when width is negative).
    int const BORDER_DISTANCE_VERT = -15;    // Distance to edge of screen (use negative value when height is negative).

    int SPACING_VERT_BARS = 2;              // Vertical distance between input and output bar
    int SPACING_RIGHT_CAPTION_IN_OUT = 4;   // IN/OUT <--> |######## :         |
    int SPACING_LEFT_CAPTION_UNIT = 4;      //             |######## :         | <--> UNIT
    int PADDING_HORIZ_VALUE = 2;            //             |<-->X.XX<:->#      |
    
    unsigned const COLOR_TEXT      = 0xedededff; // ^      ^     ^   ^         ^       ^
    unsigned const COLOR_FRAME     = 0xe0e0e0b0; //        |         |         |
    unsigned const COLOR_INDICATOR = 0xc0c0c0b0; //                  |
    
    char const* CAPTION_IN = "In";
    char const* CAPTION_OUT = "Out";
    char const* CAPTION_UNIT = "Mbps";

    double const UNIT_SCALE = 8000.0 / (1024.0 * 1024.0);  // Bytes/ms -> Mbps
    int const INITIAL_SCALE_MAXIMUM_INDEX = 250; // / 9: exponent, % 9: mantissa - 2, 0 o--o 2 * 10^-10
    int const MIN_METER_SCALE = 10; // 10Mbps
    int const NUMBER_OF_MARKERS = 10;
}

BandwidthMeter::ChannelInfo BandwidthMeter::_CHANNELS[] = {
    { "Audio"   , "Kbps", 8000.0 / 1024.0, 0x33cc99ff },
    { "Avatars" , "Kbps", 8000.0 / 1024.0, 0xffef40c0 },
    { "Voxels"  , "Kbps", 8000.0 / 1024.0, 0xd0d0d0a0 },
    { "Metavoxels", "Kbps", 8000.0 / 1024.0, 0xd0d0d0a0 }
};

BandwidthMeter::BandwidthMeter() :
    _textRenderer(TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, -1, QFont::Bold, false)),
    _scaleMaxIndex(INITIAL_SCALE_MAXIMUM_INDEX) {

    _channels = static_cast<ChannelInfo*>( malloc(sizeof(_CHANNELS)) );
    memcpy(_channels, _CHANNELS, sizeof(_CHANNELS));
}

BandwidthMeter::~BandwidthMeter() {

    free(_channels);
}

BandwidthMeter::Stream::Stream(float msToAverage) : _value(0.0f), _msToAverage(msToAverage) {
    _prevTime.start();
}

void BandwidthMeter::Stream::updateValue(double amount) {

    // Determine elapsed time
    double dt = (double)_prevTime.nsecsElapsed() / 1000000.0; // ns to ms

    // Ignore this value when timer imprecision yields dt = 0
    if (dt == 0.0) {
        return;
    }

    _prevTime.start();

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
    for (size_t i = 0; i < N_CHANNELS; ++i) {

        totalIn += inputStream(ChannelIndex(i)).getValue();
        totalOut += outputStream(ChannelIndex(i)).getValue();
    }
    totalIn *= UNIT_SCALE;
    totalOut *= UNIT_SCALE;
    float totalMax = glm::max(totalIn, totalOut);

    // Get font / caption metrics
    QFontMetrics const& fontMetrics = _textRenderer->metrics();
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
    glTranslatef((float)barX, (float)y, 0.0f);

    // Render captions
    setColorRGBA(COLOR_TEXT);
    _textRenderer->draw(barWidth + SPACING_LEFT_CAPTION_UNIT, textYcenteredLine, CAPTION_UNIT);
    _textRenderer->draw(-labelWidthIn - SPACING_RIGHT_CAPTION_IN_OUT, textYupperLine, CAPTION_IN);
    _textRenderer->draw(-labelWidthOut - SPACING_RIGHT_CAPTION_IN_OUT, textYlowerLine, CAPTION_OUT);

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
            _scaleMaxIndex = glm::max(0, _scaleMaxIndex - 1);
            commit = true;
        } else if (totalMax > scaleMax) {
            _scaleMaxIndex += 1;
            commit = true;
        }
    } while (commit);

    step = scaleMax / NUMBER_OF_MARKERS;
    if (scaleMax < MIN_METER_SCALE) {
        scaleMax = MIN_METER_SCALE;
    }
    
    // Render scale indicators
    setColorRGBA(COLOR_INDICATOR);
    for (int j = NUMBER_OF_MARKERS; --j > 0;) {
        renderVerticalLine((barWidth * j) / NUMBER_OF_MARKERS, 0, h);
    }

    // Render bars
    int xIn = 0, xOut = 0;
    for (size_t i = 0; i < N_CHANNELS; ++i) {

        ChannelIndex chIdx = ChannelIndex(i);
        int wIn = (int)(barWidth * inputStream(chIdx).getValue() * UNIT_SCALE / scaleMax);
        int wOut = (int)(barWidth * outputStream(chIdx).getValue() * UNIT_SCALE / scaleMax);

        setColorRGBA(channelInfo(chIdx).colorRGBA);

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
    setColorRGBA(COLOR_TEXT);
    sprintf(fmtBuf, "%0.1f", totalIn);
    _textRenderer->draw(glm::max(xIn - fontMetrics.width(fmtBuf) - PADDING_HORIZ_VALUE,
                                PADDING_HORIZ_VALUE),
                       textYupperLine, fmtBuf);
    sprintf(fmtBuf, "%0.1f", totalOut);
    _textRenderer->draw(glm::max(xOut - fontMetrics.width(fmtBuf) - PADDING_HORIZ_VALUE,
                                PADDING_HORIZ_VALUE),
                       textYlowerLine, fmtBuf);

    glPopMatrix();

    // After rendering, indicate that no data has been sent/received since the last feed.
    // This way, the meters fall when not continuously fed.
    for (size_t i = 0; i < N_CHANNELS; ++i) {
        inputStream(ChannelIndex(i)).updateValue(0);
        outputStream(ChannelIndex(i)).updateValue(0);
    }
}


