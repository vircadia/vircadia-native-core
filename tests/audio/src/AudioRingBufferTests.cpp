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

// Adds an implicit cast to make sure that actual and expected are of the same type.
//  QCOMPARE(<unsigned int>, <int>) => cryptic linker error.
//  (since QTest::qCompare is defined for (T, T, ...), but not (T, U, ...))
//
#define QCOMPARE_WITH_CAST(actual, expected) \
QCOMPARE(actual, static_cast<decltype(actual)>(expected))

QTEST_MAIN(AudioRingBufferTests)

void AudioRingBufferTests::assertBufferSize(const AudioRingBuffer& buffer, int samples) {
    QCOMPARE(buffer.samplesAvailable(), samples);
}

void AudioRingBufferTests::runAllTests() {

    int16_t writeData[10000];
    for (int i = 0; i < 10000; i++) { writeData[i] = i; }
    int writeIndexAt;

    int16_t readData[10000];
    int readIndexAt;


    AudioRingBuffer ringBuffer(10, 10); // makes buffer of 100 int16_t samples
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
            QCOMPARE(readData[i], (int16_t)i);
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
        int samplesWritten = ringBuffer.addSilentSamples(29);
        QCOMPARE(samplesWritten, 0);
        assertBufferSize(ringBuffer, 100);

        // read 3 samples, 97 samples in buffer (expect to read "1", "2", "3")
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 3);
        for (int i = 0; i < 3; i++) {
            QCOMPARE(readData[i], static_cast<int16_t>(i + 1));
        }
        assertBufferSize(ringBuffer, 97);

        // write 4 silent samples, 100 samples in buffer
        QCOMPARE(ringBuffer.addSilentSamples(4), 3);
        assertBufferSize(ringBuffer, 100);

        // read back 97 samples (the non-silent samples), 3 samples in buffer (expect to read "4" thru "100")
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 97);
        for (int i = 3; i < 100; i++) {
            QCOMPARE(readData[i], static_cast<int16_t>(i + 1));
        }
        assertBufferSize(ringBuffer, 3);

        // read back 3 silent samples, 0 samples in buffer
        readIndexAt += ringBuffer.readSamples(&readData[readIndexAt], 3);
        for (int i = 100; i < 103; i++) {
            QCOMPARE(readData[i], static_cast<int16_t>(0));
        }
        assertBufferSize(ringBuffer, 0);
    }
}
