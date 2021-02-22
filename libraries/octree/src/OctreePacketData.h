//
//  OctreePacketData.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 11/19/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  TO DO:
//    *  add stats tracking for number of unique colors and consecutive identical colors.
//          (as research for color dictionaries and RLE)
//
//    *  further testing of compression to determine optimal configuration for performance and compression
//
//    *  improve semantics for "reshuffle" - current approach will work for now and with compression
//       but wouldn't work with RLE because the colors in the levels would get reordered and RLE would need
//       to be recalculated
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreePacketData_h
#define hifi_OctreePacketData_h

#include <atomic>

#include <QByteArray>
#include <QString>
#include <QUuid>

#include <SharedUtil.h>
#include <ShapeInfo.h>
#include <NLPacket.h>
#include <udt/PacketHeaders.h>

#include "MaterialMappingMode.h"
#include "BillboardMode.h"
#include "RenderLayer.h"
#include "PrimitiveMode.h"
#include "WebInputMode.h"
#include "PulseMode.h"
#include "GizmoType.h"
#include "TextEffect.h"
#include "TextAlignment.h"

#include "OctreeConstants.h"
#include "OctreeElement.h"

using AtomicUIntStat = std::atomic<uintmax_t>;

typedef unsigned char OCTREE_PACKET_FLAGS;
typedef uint16_t OCTREE_PACKET_SEQUENCE;
const uint16_t MAX_OCTREE_PACKET_SEQUENCE = 65535;
typedef quint64 OCTREE_PACKET_SENT_TIME;
typedef uint16_t OCTREE_PACKET_INTERNAL_SECTION_SIZE;
const int MAX_OCTREE_PACKET_SIZE = udt::MAX_PACKET_SIZE;

const unsigned int OCTREE_PACKET_EXTRA_HEADERS_SIZE = sizeof(OCTREE_PACKET_FLAGS)
                + sizeof(OCTREE_PACKET_SEQUENCE) + sizeof(OCTREE_PACKET_SENT_TIME);

const unsigned int MAX_OCTREE_PACKET_DATA_SIZE =
    udt::MAX_PACKET_SIZE - (NLPacket::MAX_PACKET_HEADER_SIZE + OCTREE_PACKET_EXTRA_HEADERS_SIZE);
const unsigned int MAX_OCTREE_UNCOMRESSED_PACKET_SIZE = MAX_OCTREE_PACKET_DATA_SIZE;

const unsigned int MINIMUM_ATTEMPT_MORE_PACKING = sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE) + 40;
const unsigned int COMPRESS_PADDING = 15;
const int REASONABLE_NUMBER_OF_PACKING_ATTEMPTS = 5;

const int PACKET_IS_COLOR_BIT = 0;
const int PACKET_IS_COMPRESSED_BIT = 1;

/// An opaque key used when starting, ending, and discarding encoding/packing levels of OctreePacketData
class LevelDetails {
    LevelDetails(int startIndex, int bytesOfOctalCodes, int bytesOfBitmasks, int bytesOfColor, int bytesReservedAtStart) :
        _startIndex(startIndex),
        _bytesOfOctalCodes(bytesOfOctalCodes),
        _bytesOfBitmasks(bytesOfBitmasks),
        _bytesOfColor(bytesOfColor),
        _bytesReservedAtStart(bytesReservedAtStart) {
    }
    
    friend class OctreePacketData;

private:
    int _startIndex;
    int _bytesOfOctalCodes;
    int _bytesOfBitmasks;
    int _bytesOfColor;
    int _bytesReservedAtStart;
};

/// Handles packing of the data portion of PacketType_OCTREE_DATA messages. 
class OctreePacketData {
public:
    OctreePacketData(bool enableCompression = false, int maxFinalizedSize = MAX_OCTREE_PACKET_DATA_SIZE);
    ~OctreePacketData();

    /// change compression and target size settings
    void changeSettings(bool enableCompression = false, unsigned int targetSize = MAX_OCTREE_PACKET_DATA_SIZE);

    /// reset completely, all data is discarded
    void reset();
    
    /// call to begin encoding a subtree starting at this point, this will append the octcode to the uncompressed stream
    /// at this point. May fail if new datastream is too long. In failure case the stream remains in it's previous state.
    bool startSubTree(const unsigned char* octcode = NULL);
    
    // call to indicate that the current subtree is complete and changes should be committed.
    void endSubTree();

    // call rollback the current subtree and restore the stream to the state prior to starting the subtree encoding
    void discardSubTree();

    /// starts a level marker. returns an opaque key which can be used to discard the level
    LevelDetails startLevel();

    /// discards all content back to a previous marker key
    void discardLevel(LevelDetails key);
    
    /// ends a level, and performs any expensive finalization. may fail if finalization creates a stream which is too large
    /// if the finalization would fail, the packet will automatically discard the previous level.
    bool endLevel(LevelDetails key);

    /// appends a bitmask to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendBitMask(unsigned char bitmask);

    /// updates the value of a bitmask from a previously appended portion of the uncompressed stream, might fail if the new 
    /// bitmask would cause packet to be less compressed, or if offset was out of range.
    bool updatePriorBitMask(int offset, unsigned char bitmask); 

    /// reserves space in the stream for a future bitmask, may fail if new data stream is too long to fit in packet
    bool reserveBitMask();

    /// reserves space in the stream for a future number of bytes, may fail if new data stream is too long to fit in packet.
    /// The caller must call releaseReservedBytes() before attempting to fill the bytes.
    bool reserveBytes(int numberOfBytes);

    /// releases previously reserved space in the stream.
    bool releaseReservedBitMask();

    /// releases previously reserved space in the stream.
    bool releaseReservedBytes(int numberOfBytes);

    /// updates the uncompressed content of the stream starting at byte offset with replacementBytes for length.
    /// Might fail if the new bytes would cause packet to be less compressed, or if offset and length was out of range.
    bool updatePriorBytes(int offset, const unsigned char* replacementBytes, int length);

    /// appends a color to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendColor(colorPart red, colorPart green, colorPart blue);

    /// appends a color to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const nodeColor& color);

    /// appends a unsigned 8 bit int to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(uint8_t value);

    /// appends a unsigned 16 bit int to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(uint16_t value);

    /// appends a unsigned 32 bit int to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(uint32_t value);

    /// appends a unsigned 64 bit int to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(quint64 value);

    /// appends a float value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(float value);

    /// appends a vec2 to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const glm::vec2& value);

    /// appends a non-position vector to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const glm::vec3& value);

    /// appends a color to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const glm::u8vec3& value);

    /// appends a QVector of vec3s to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QVector<glm::vec3>& value);

    /// appends a QVector of quats to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QVector<glm::quat>& value);

    /// appends a QVector of floats to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QVector<float>& value);

    /// appends a QVector of bools to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QVector<bool>& value);

    /// appends a QVector of QUuids to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QVector<QUuid>& value);

    /// appends a packed quat to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const glm::quat& value);

    /// appends a bool value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(bool value);

    /// appends a string value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QString& string);

    /// appends a uuid value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QUuid& uuid);

    /// appends a QByteArray value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QByteArray& bytes);

    /// appends an AACube value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const AACube& aaCube);

    /// appends an QRect value to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendValue(const QRect& rect);

    /// appends a position to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendPosition(const glm::vec3& value);

    /// appends raw bytes, might fail if byte would cause packet to be too large
    bool appendRawData(const unsigned char* data, int length);
    bool appendRawData(QByteArray data);

    /// returns a byte offset from beginning of the uncompressed stream based on offset from end.
    /// Positive offsetFromEnd returns that many bytes before the end of uncompressed stream
    int getUncompressedByteOffset(int offsetFromEnd = 0) const { return _bytesInUse - offsetFromEnd; }

    /// get access to the finalized data (it may be compressed or rewritten into optimal form)
    const unsigned char* getFinalizedData();
    /// get size of the finalized data (it may be compressed or rewritten into optimal form)
    int getFinalizedSize();

    /// get pointer to the uncompressed stream buffer at the byteOffset
    const unsigned char* getUncompressedData(int byteOffset = 0) { return &_uncompressed[byteOffset]; }

    /// the size of the packet in uncompressed form
    int getUncompressedSize() { return _bytesInUse; }

    /// update the size of the packet in uncompressed form
    void setUncompressedSize(int newSize) { _bytesInUse = newSize; }

    /// has some content been written to the packet
    bool hasContent() const { return (_bytesInUse > 0); }

    /// load finalized content to allow access to decoded content for parsing
    void loadFinalizedContent(const unsigned char* data, int length);
    
    /// returns whether or not zlib compression enabled on finalization
    bool isCompressed() const { return _enableCompression; }
    
    /// returns the target uncompressed size
    unsigned int getTargetSize() const { return _targetSize; }

    /// the number of bytes in the packet currently reserved
    int getReservedBytes() { return _bytesReserved; }

    int getBytesAvailable() { return _bytesAvailable; }

    /// displays contents for debugging
    void debugContent();
    void debugBytes();
    
    static quint64 getCompressContentTime() { return _compressContentTime; } /// total time spent compressing content
    static quint64 getCompressContentCalls() { return _compressContentCalls; } /// total calls to compress content
    static quint64 getTotalBytesOfOctalCodes() { return _totalBytesOfOctalCodes; }  /// total bytes for octal codes
    static quint64 getTotalBytesOfBitMasks() { return _totalBytesOfBitMasks; }  /// total bytes of bitmasks
    static quint64 getTotalBytesOfColor() { return _totalBytesOfColor; } /// total bytes of color
    
    static int unpackDataFromBytes(const unsigned char* dataBytes, float& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, bool& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, quint64& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, uint32_t& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, uint16_t& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, uint8_t& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, glm::quat& result) { int bytes = unpackOrientationQuatFromBytes(dataBytes, result); return bytes; }
    static int unpackDataFromBytes(const unsigned char* dataBytes, ShapeType& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, MaterialMappingMode& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, BillboardMode& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, RenderLayer& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, PrimitiveMode& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, WebInputMode& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, PulseMode& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, GizmoType& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, TextEffect& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, TextAlignment& result) { memcpy(&result, dataBytes, sizeof(result)); return sizeof(result); }
    static int unpackDataFromBytes(const unsigned char* dataBytes, glm::vec2& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, glm::vec3& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, glm::u8vec3& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QString& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QUuid& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QVector<glm::vec3>& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QVector<glm::quat>& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QVector<float>& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QVector<bool>& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QVector<QUuid>& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QByteArray& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, AACube& result);
    static int unpackDataFromBytes(const unsigned char* dataBytes, QRect& result);

private:
    /// appends raw bytes, might fail if byte would cause packet to be too large
    bool append(const unsigned char* data, int length);
    
    /// append a single byte, might fail if byte would cause packet to be too large
    bool append(unsigned char byte);

    unsigned int _targetSize;
    bool _enableCompression;
    
    QByteArray _uncompressedByteArray;
    unsigned char* _uncompressed { nullptr };
    int _bytesInUse;
    int _bytesAvailable;
    int _subTreeAt;
    int _bytesReserved;
    int _subTreeBytesReserved; // the number of reserved bytes at start of a subtree

    bool compressContent();
    
    QByteArray _compressedByteArray;
    unsigned char* _compressed { nullptr };
    int _compressedBytes;
    int _bytesInUseLastCheck;
    bool _dirty;

    // statistics...
    int _bytesOfOctalCodes;
    int _bytesOfBitMasks;
    int _bytesOfColor;
    int _bytesOfValues;
    int _bytesOfPositions;
    int _bytesOfRawData;

    int _bytesOfOctalCodesCurrentSubTree;

    static bool _debug;

    static AtomicUIntStat _compressContentTime;
    static AtomicUIntStat _compressContentCalls;

    static AtomicUIntStat _totalBytesOfOctalCodes;
    static AtomicUIntStat _totalBytesOfBitMasks;
    static AtomicUIntStat _totalBytesOfColor;
    static AtomicUIntStat _totalBytesOfValues;
    static AtomicUIntStat _totalBytesOfPositions;
    static AtomicUIntStat _totalBytesOfRawData;
};

#endif // hifi_OctreePacketData_h
