#include <QtGlobal>
#include <QtTest/QtTest>
#include <QtCore/QTemporaryFile>
#include <QtCore/QString>

#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

#include <recording/Clip.h>
#include <recording/Frame.h>

#include "Constants.h"

using namespace recording;
FrameType TEST_FRAME_TYPE { Frame::TYPE_INVALID };

void testFrameTypeRegistration() {
    TEST_FRAME_TYPE = Frame::registerFrameType(TEST_NAME);
    QVERIFY(TEST_FRAME_TYPE != Frame::TYPE_INVALID);
    QVERIFY(TEST_FRAME_TYPE != Frame::TYPE_HEADER);

    auto forwardMap = recording::Frame::getFrameTypes();
    QVERIFY(forwardMap.count(TEST_NAME) == 1);
    QVERIFY(forwardMap[TEST_NAME] == TEST_FRAME_TYPE);
    QVERIFY(forwardMap[HEADER_NAME] == recording::Frame::TYPE_HEADER);

    auto backMap = recording::Frame::getFrameTypeNames();
    QVERIFY(backMap.count(TEST_FRAME_TYPE) == 1);
    QVERIFY(backMap[TEST_FRAME_TYPE] == TEST_NAME);
    auto typeHeader = recording::Frame::TYPE_HEADER;
    QVERIFY(backMap[typeHeader] == HEADER_NAME);
}

void testFilePersist() {
    QTemporaryFile file;
    QString fileName;
    if (file.open()) {
        fileName = file.fileName();
        file.close();
    }
    auto readClip = Clip::fromFile(fileName);
    QVERIFY(Clip::Pointer() == readClip);
    auto writeClip = Clip::newClip();
    writeClip->addFrame(std::make_shared<Frame>(TEST_FRAME_TYPE, 5.0f, QByteArray()));
    QVERIFY(writeClip->frameCount() == 1);
    QVERIFY(writeClip->duration() == 5.0f);

    Clip::toFile(fileName, writeClip);
    readClip = Clip::fromFile(fileName);
    QVERIFY(readClip != Clip::Pointer());
    QVERIFY(readClip->frameCount() == 1);
    QVERIFY(readClip->duration() == 5.0f);
    readClip->seek(0);
    writeClip->seek(0);

    size_t count = 0;
    for (auto readFrame = readClip->nextFrame(), writeFrame = writeClip->nextFrame(); readFrame && writeFrame;
        readFrame = readClip->nextFrame(), writeFrame = writeClip->nextFrame(), ++count) {
        QVERIFY(readFrame->type == writeFrame->type);
        QVERIFY(readFrame->timeOffset == writeFrame->timeOffset);
        QVERIFY(readFrame->data == writeFrame->data);
    }
    QVERIFY(readClip->frameCount() == count);


    writeClip = Clip::newClip();
    writeClip->addFrame(std::make_shared<Frame>(TEST_FRAME_TYPE, 5.0f, QByteArray()));
    // Simulate an unknown frametype
    writeClip->addFrame(std::make_shared<Frame>(Frame::TYPE_INVALID - 1, 10.0f, QByteArray()));
    QVERIFY(writeClip->frameCount() == 2);
    QVERIFY(writeClip->duration() == 10.0f);
    Clip::toFile(fileName, writeClip);

    // Verify that the read version of the clip ignores the unknown frame type
    readClip = Clip::fromFile(fileName);
    QVERIFY(readClip != Clip::Pointer());
    QVERIFY(readClip->frameCount() == 1);
    QVERIFY(readClip->duration() == 5.0f);
}

void testClipOrdering() {
    auto writeClip = Clip::newClip();
    // simulate our of order addition of frames
    writeClip->addFrame(std::make_shared<Frame>(TEST_FRAME_TYPE, 10.0f, QByteArray()));
    writeClip->addFrame(std::make_shared<Frame>(TEST_FRAME_TYPE, 5.0f, QByteArray()));
    QVERIFY(writeClip->frameCount() == 2);
    QVERIFY(writeClip->duration() == 10.0f);

    QVERIFY(std::numeric_limits<float>::max() == writeClip->position());
    writeClip->seek(0);
    QVERIFY(5.0f == writeClip->position());
    float lastFrameTimeOffset { 0 };
    for (auto writeFrame = writeClip->nextFrame(); writeFrame; writeFrame = writeClip->nextFrame()) {
        QVERIFY(writeClip->position() >= lastFrameTimeOffset);
    }
    Q_UNUSED(lastFrameTimeOffset); // FIXME - Unix build not yet upgraded to Qt 5.5.1 we can remove this once it is
}

#ifdef Q_OS_WIN32
void myMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg) {
    OutputDebugStringA(msg.toLocal8Bit().toStdString().c_str());
    OutputDebugStringA("\n");
}
#endif

int main(int, const char**) {
#ifdef Q_OS_WIN32
    qInstallMessageHandler(myMessageHandler);
#endif
    testFrameTypeRegistration();
    testFilePersist();
    testClipOrdering();
}