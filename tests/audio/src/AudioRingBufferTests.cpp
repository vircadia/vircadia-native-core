//
//  AudioRingBufferTests.cpp
//  tests/audio/src
//
//  Created by Yixin Wang on 6/24/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioRingBufferTests.h"

#include "SharedUtil.h"

void AudioRingBufferTests::assertBufferSize(const AudioRingBuffer& buffer, int samples) {
    if (buffer.samplesAvailable() != samples) {
        qDebug("Unexpected num samples available! Exptected: %d  Actual: %d\n", samples, buffer.samplesAvailable());
    }
}

void AudioRingBufferTests::runAllTests() {

    int16_t writeData[10000];
    for (int i = 0; i < 10000; i++) { writeData[i] = i; }
    int writeIndexAt;

    int16_t readData[10000];
    int readIndexAt;


    AudioRingBuffer ringBuffer(10, false, 10); // makes buffer of 100 int16_t samples
    for (int T = 0; T < 300; T++) {

        writeIndexAt = 0;
        readIndexAt = 0;

        // write 73 samples, 73 samples in buffer
        writeIndexAt += ringBuffer.writeSamples(&writeData[writeIndexAt], 73);
        assertBufferSize(ringBuffer, 73);

        // read 43 samples, 30 samples in buffer
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 43);
        assertBufferSize(ringBuffer, 30);

        // write 70 samples, 100 samples in buffer (full)
        writeIndexAt += ringBuffer.writeSamples(&writeData[writeIndexAt], 70);
        assertBufferSize(ringBuffer, 100);

        // read 100 samples, 0 samples in buffer (empty)
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 100);
        assertBufferSize(ringBuffer, 0);


        // verify 143 samples of read data
        for (int i = 0; i < 143; i++) {
            if (readData[i] != i) {
                qDebug("first readData[%d] incorrect!  Expcted: %d  Actual: %d", i, i, readData[i]);
                return;
            }
        }


        writeIndexAt = 0;
        readIndexAt = 0;

        // write 59 samples, 59 samples in buffer
        writeIndexAt += ringBuffer.writeSamples(&writeData[writeIndexAt], 59);
        assertBufferSize(ringBuffer, 59);

        // write 99 samples, 100 samples in buffer
        writeIndexAt += ringBuffer.writeSamples(&writeData[writeIndexAt], 99);
        assertBufferSize(ringBuffer, 100);

        // read 100 samples, 0 samples in buffer
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 100);
        assertBufferSize(ringBuffer, 0);

        // verify 100 samples of read data
        for (int i = 0; i < 100; i++) {
            readData[i] = writeIndexAt - 100 + i;
        }




        writeIndexAt = 0;
        readIndexAt = 0;

        // write 77 samples, 77 samples in buffer
        writeIndexAt += ringBuffer.writeSamples(&writeData[writeIndexAt], 77);
        assertBufferSize(ringBuffer, 77);

        // write 24 samples, 100 samples in buffer (overwrote one sample: "0")
        writeIndexAt += ringBuffer.writeSamples(&writeData[writeIndexAt], 24);
        assertBufferSize(ringBuffer, 100);

        // write 29 silent samples, 100 samples in buffer, make sure non were added
        int samplesWritten;
        if ((samplesWritten = ringBuffer.addSilentSamples(29)) != 0) {
            qDebug("addSilentSamples(29) incorrect!  Expected: 0  Actual: %d", samplesWritten);
            return;
        }
        assertBufferSize(ringBuffer, 100);

        // read 3 samples, 97 samples in buffer (expect to read "1", "2", "3")
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 3);
        for (int i = 0; i < 3; i++) {
            if (readData[i] != i + 1) {
                qDebug("Second readData[%d] incorrect!  Expcted: %d  Actual: %d", i, i + 1, readData[i]);
                return;
            }
        }
        assertBufferSize(ringBuffer, 97);

        // write 4 silent samples, 100 samples in buffer
        if ((samplesWritten = ringBuffer.addSilentSamples(4)) != 3) {
            qDebug("addSilentSamples(4) incorrect!  Exptected: 3  Actual: %d", samplesWritten);
            return;
        }
        assertBufferSize(ringBuffer, 100);

        // read back 97 samples (the non-silent samples), 3 samples in buffer (expect to read "4" thru "100")
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 97);
        for (int i = 3; i < 100; i++) {
            if (readData[i] != i + 1) {
                qDebug("third readData[%d] incorrect!  Expcted: %d  Actual: %d", i, i + 1, readData[i]);
                return;
            }
        }
        assertBufferSize(ringBuffer, 3);

        // read back 3 silent samples, 0 samples in buffer
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 3);
        for (int i = 100; i < 103; i++) {
            if (readData[i] != 0) {
                qDebug("Fourth readData[%d] incorrect!  Expcted: %d  Actual: %d", i, 0, readData[i]);
                return;
            }
        }
        assertBufferSize(ringBuffer, 0);
    }

    qDebug() << "PASSED";
}
