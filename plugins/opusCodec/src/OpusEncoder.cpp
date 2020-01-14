
#include <PerfStat.h>
#include <QtCore/QLoggingCategory>

#include "OpusEncoder.h"
#include "opus/opus.h"

static QLoggingCategory encoder("AthenaOpusEncoder");

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



AthenaOpusEncoder::AthenaOpusEncoder(int sampleRate, int numChannels)
{
    _opus_sample_rate = sampleRate;
    _opus_channels    = numChannels;

    int error;

    _encoder = opus_encoder_create(sampleRate, numChannels, DEFAULT_APPLICATION, &error);

    if ( error != OPUS_OK ) {
        qCCritical(encoder) << "Failed to initialize Opus encoder: " << error_to_string(error);
        _encoder = nullptr;
        return;
    }

    setBitrate(DEFAULT_BITRATE);
    setComplexity(DEFAULT_COMPLEXITY);
    setApplication(DEFAULT_APPLICATION);
    setSignal(DEFAULT_SIGNAL);

    qCDebug(encoder) << "Opus encoder initialized, sampleRate = " << sampleRate << "; numChannels = " << numChannels;
}

AthenaOpusEncoder::~AthenaOpusEncoder()
{
    opus_encoder_destroy(_encoder);
}



void AthenaOpusEncoder::encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) {

    PerformanceTimer perfTimer("AthenaOpusEncoder::encode");
    assert(_encoder);

    encodedBuffer.resize( decodedBuffer.size() );
    int frame_size = decodedBuffer.length()/ _opus_channels / static_cast<int>(sizeof(opus_int16));

    int bytes = opus_encode(_encoder, reinterpret_cast<const opus_int16*>(decodedBuffer.constData()), frame_size, reinterpret_cast<unsigned char*>(encodedBuffer.data()), encodedBuffer.size() );

    if ( bytes >= 0 ) {
        //qCDebug(encoder) << "Encoded " << decodedBuffer.length() << " bytes into " << bytes << " opus bytes";
        encodedBuffer.resize(bytes);
    } else {
        encodedBuffer.resize(0);

        qCWarning(encoder) << "Error when encoding " << decodedBuffer.length() << " bytes of audio: " << error_to_string(bytes);
    }

}

int AthenaOpusEncoder::getComplexity() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_COMPLEXITY(&ret));
    return ret;
}

void AthenaOpusEncoder::setComplexity(int complexity)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(complexity));

    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting complexity to " << complexity << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getBitrate() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_BITRATE(&ret));
    return ret;
}

void AthenaOpusEncoder::setBitrate(int bitrate)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(bitrate));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting bitrate to " << bitrate << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getVBR() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_VBR(&ret));
    return ret;
}

void AthenaOpusEncoder::setVBR(int vbr)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_VBR(vbr));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting VBR to " << vbr << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getVBRConstraint() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_VBR_CONSTRAINT(&ret));
    return ret;
}

void AthenaOpusEncoder::setVBRConstraint(int vbr_const)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_VBR_CONSTRAINT(vbr_const));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting VBR constraint to " << vbr_const << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getMaxBandwidth() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_MAX_BANDWIDTH(&ret));
    return ret;
}

void AthenaOpusEncoder::setMaxBandwidth(int maxbw)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_MAX_BANDWIDTH(maxbw));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting max bandwidth to " << maxbw << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getBandwidth() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_BANDWIDTH(&ret));
    return ret;
}

void AthenaOpusEncoder::setBandwidth(int bw)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_BANDWIDTH(bw));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting bandwidth to " << bw << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getSignal() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_SIGNAL(&ret));
    return ret;
}

void AthenaOpusEncoder::setSignal(int signal)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(signal));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting signal to " << signal << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getApplication() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_APPLICATION(&ret));
    return ret;
}

void AthenaOpusEncoder::setApplication(int app)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_APPLICATION(app));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting application to " << app << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getLookahead() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_LOOKAHEAD(&ret));
    return ret;
}

int AthenaOpusEncoder::getInbandFEC() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_INBAND_FEC(&ret));
    return ret;
}

void AthenaOpusEncoder::setInbandFEC(int fec)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_INBAND_FEC(fec));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting inband FEC to " << fec << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getExpectedPacketLossPercent() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_PACKET_LOSS_PERC(&ret));
    return ret;
}

void AthenaOpusEncoder::setExpectedPacketLossPercent(int perc)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_PACKET_LOSS_PERC(perc));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting loss percent to " << perc << ": " << error_to_string(ret);
}

int AthenaOpusEncoder::getDTX() const
{
    assert(_encoder);
    int ret;
    opus_encoder_ctl(_encoder, OPUS_GET_DTX(&ret));
    return ret;
}

void AthenaOpusEncoder::setDTX(int dtx)
{
    assert(_encoder);
    int ret = opus_encoder_ctl(_encoder, OPUS_SET_DTX(dtx));
    if ( ret != OPUS_OK )
        qCWarning(encoder) << "Error when setting DTX to " << dtx << ": " << error_to_string(ret);
}


