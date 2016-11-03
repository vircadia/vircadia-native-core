//
//  main.cpp
//  tests/render-utils/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QDir>
#include <ByteCountCoding.h>

#include <ShapeEntityItem.h>
#include <EntityItemProperties.h>
#include <Octree.h>
#include <PathUtils.h>

const QString& getTestResourceDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../")) + "/";
        qDebug() << "Qml Test Path: " << dir;
    }
    return dir;
}

class StopWatch {
public:
    void start() {
        Q_ASSERT(_start == 0);
        _start = usecTimestampNow();
    }

    void stop() {
        Q_ASSERT(_start != 0);
        _last = usecTimestampNow() - _start;
        _start = 0;
        _total += _last;
        _count++;
    }

    quint64 getLast() {
        return _last;
    }

    quint64 getTotal() {
        return _total;
    }

    float getAverage() {
        return (float)_total / (float)_count;
    }

    void reset() {
        _last = _start = _total = _count = 0;
    }

private:
    size_t _count{ 0 };
    quint64 _total{ 0 };
    quint64 _start{ 0 };
    quint64 _last{ 0 };
};

template <typename T>
void testByteCountCodedStable(const T& value) {
    ByteCountCoded<T> coder((T)value);
    auto encoded = coder.encode();
    #ifndef QT_NO_DEBUG
    auto originalEncodedSize = encoded.size();
    #endif
    for (int i = 0; i < 10; ++i) {
        encoded.append(qrand());
    }
    ByteCountCoded<T> decoder;
    decoder.decode(encoded);
    Q_ASSERT(decoder.data == coder.data);
    #ifndef QT_NO_DEBUG
    auto consumed = decoder.decode(encoded.data(), encoded.size());
    Q_ASSERT(consumed == (unsigned int)originalEncodedSize);
    #endif

}

template <typename T>
void testByteCountCoded() {
    testByteCountCodedStable<T>(0);
    testByteCountCodedStable<T>(1);
    testByteCountCodedStable<T>(std::numeric_limits<T>::max() >> 1);
    testByteCountCodedStable<T>(std::numeric_limits<T>::max());
}

void testPropertyFlags(uint32_t value) {
    EntityPropertyFlags original;
    original.clear();
    auto enumSize = sizeof(EntityPropertyList);
    for (size_t i = 0; i < enumSize * 8; ++i) {
        original.setHasProperty((EntityPropertyList)i);
    }
    QByteArray encoded = original.encode();
    #ifndef QT_NO_DEBUG
    int originalSize = encoded.size();
    #endif
    for (size_t i = 0; i < enumSize; ++i) {
        encoded.append(qrand());
    }

    EntityPropertyFlags decodeOld, decodeNew;
    {
        decodeOld.decode(encoded);
        Q_ASSERT(decodeOld == original);
    }

    {
        #ifndef QT_NO_DEBUG
        int decodeSize = (int)decodeNew.decode((const uint8_t*)encoded.data(), (int)encoded.size());
        Q_ASSERT(originalSize == decodeSize);
        Q_ASSERT(decodeNew == original);
        #endif
    }
}

void testPropertyFlags() {
    testPropertyFlags(0);
    testPropertyFlags(1);
    testPropertyFlags(1 << 16);
    testPropertyFlags(0xFFFF);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    {
        auto start = usecTimestampNow();
        for (int i = 0; i < 1000; ++i) {
            testPropertyFlags();
            testByteCountCoded<quint8>();
            testByteCountCoded<quint16>();
            testByteCountCoded<quint32>();
            testByteCountCoded<quint64>();
        }
        auto duration = usecTimestampNow() - start;
        qDebug() << duration;

    }
    DependencyManager::set<NodeList>(NodeType::Unassigned);

    QFile file(getTestResourceDir() + "packet.bin");
    if (!file.open(QIODevice::ReadOnly)) return -1;
    QByteArray packet = file.readAll();
    EntityItemPointer item = ShapeEntityItem::boxFactory(EntityItemID(), EntityItemProperties());
    ReadBitstreamToTreeParams params;
    params.bitstreamVersion = 33;

    auto start = usecTimestampNow();
    for (int i = 0; i < 1000; ++i) {
        item->readEntityDataFromBuffer(reinterpret_cast<const unsigned char*>(packet.constData()), packet.size(), params);
    }
    float duration = (usecTimestampNow() - start);
    qDebug() << (duration / 1000.0f);
    return 0;
}

#include "main.moc"
