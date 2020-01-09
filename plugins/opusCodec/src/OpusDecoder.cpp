#include <PerfStat.h>
#include <QtCore/QLoggingCategory>
#include <AudioConstants.h>



#include "OpusDecoder.h"

static QLoggingCategory decoder("AthenaOpusDecoder");

static QString error_to_string(int error) {
    switch (error) {
    case OPUS_OK:
        return "OK";
    case OPUS_BAD_ARG:
        return "One or more invalid/out of range arguments.";
    case OPUS_BUFFER_TOO_SMALL:
        return "The mode struct passed is invalid.";
    case OPUS_INTERNAL_ERROR:
        return "An internal error was detected.";
    case OPUS_INVALID_PACKET:
        return "The compressed data passed is corrupted.";
    case OPUS_UNIMPLEMENTED:
        return "Invalid/unsupported request number.";
    case OPUS_INVALID_STATE:
        return "An encoder or decoder structure is invalid or already freed.";
    default:
        return QString("Unknown error code: %i").arg(error);
    }
}


AthenaOpusDecoder::AthenaOpusDecoder(int sampleRate, int numChannels)
{
    int error;

    _opus_sample_rate = sampleRate;
    _opus_num_channels = numChannels;

    _decoder = opus_decoder_create(sampleRate, numChannels, &error);

    if ( error != OPUS_OK ) {
        qCCritical(decoder) << "Failed to initialize Opus encoder: " << error_to_string(error);
        _decoder = nullptr;
        return;
    }


    qCDebug(decoder) << "Opus decoder initialized, sampleRate = " << sampleRate << "; numChannels = " << numChannels;
}

AthenaOpusDecoder::~AthenaOpusDecoder()
{
    if ( _decoder )
        opus_decoder_destroy(_decoder);

}

void AthenaOpusDecoder::decode(const QByteArray &encodedBuffer, QByteArray &decodedBuffer)
{
    assert(_decoder);

    PerformanceTimer perfTimer("AthenaOpusDecoder::decode");


    int buffer_size = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * static_cast<int>(sizeof(int16_t)) * _opus_num_channels;

    decodedBuffer.resize( buffer_size );
    int frame_size = decodedBuffer.size() / _opus_num_channels / static_cast<int>(sizeof( opus_int16 ));

    qCDebug(decoder) << "Opus decode: encodedBytes = " << encodedBuffer.length() << "; decodedBufferBytes = " << decodedBuffer.size() << "; frameCount = " << frame_size;
    int frames = opus_decode( _decoder, reinterpret_cast<const unsigned char*>(encodedBuffer.data()), encodedBuffer.length(), reinterpret_cast<opus_int16*>(decodedBuffer.data()), frame_size, 0 );

    if ( frames >= 0 ) {
        qCDebug(decoder) << "Decoded " << frames << " Opus frames";
        decodedBuffer.resize( frames *  static_cast<int>(sizeof( opus_int16 )) * _opus_num_channels );
    } else {
        qCCritical(decoder) << "Failed to decode audio: " << error_to_string(frames);
    }

}

void AthenaOpusDecoder::lostFrame(QByteArray &decodedBuffer)
{
    assert(_decoder);

    PerformanceTimer perfTimer("AthenaOpusDecoder::lostFrame");

    int buffer_size = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * static_cast<int>(sizeof(int16_t)) * _opus_num_channels;
    decodedBuffer.resize( buffer_size );
    int frame_size = decodedBuffer.size() / _opus_num_channels / static_cast<int>(sizeof( opus_int16 ));

    int frames = opus_decode( _decoder, nullptr, 0, reinterpret_cast<opus_int16*>(decodedBuffer.data()), frame_size, 1 );

    if ( frames >= 0 ) {
        qCDebug(decoder) << "Produced " << frames << " opus frames from a lost frame";
        decodedBuffer.resize( frames *  static_cast<int>(sizeof( opus_int16 )) * _opus_num_channels );
    } else {
        qCCritical(decoder) << "Failed to decode lost frame: " << error_to_string(frames);
    }
}


