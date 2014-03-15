// ==========================================================================
// Copyright (C) 2012 faceshift AG, and/or its licensors.  All rights reserved.
//
// the software is free to use and provided "as is", without warranty of any kind.
// faceshift AG does not make and hereby disclaims any express or implied
// warranties including, but not limited to, the warranties of
// non-infringement, merchantability or fitness for a particular purpose,
// or arising from a course of dealing, usage, or trade practice. in no
// event will faceshift AG and/or its licensors be liable for any lost
// revenues, data, or profits, or special, direct, indirect, or
// consequential damages, even if faceshift AG and/or its licensors has
// been advised of the possibility or probability of such damages.
// ==========================================================================

#include "fsbinarystream.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define FSNETWORKVERSION 1

#ifdef FS_INTERNAL
#include <common/log.hpp>
#else
#define LOG_RELEASE_ERROR(...)		{ printf("ERROR:   %20s:%6d", __FILE__, __LINE__); printf(__VA_ARGS__); }
#define LOG_RELEASE_WARNING(...) 	{ printf("WARNING: %20s:%6d", __FILE__, __LINE__); printf(__VA_ARGS__); }
#define LOG_RELEASE_INFO(...)		{ printf("INFO:    %20s:%6d", __FILE__, __LINE__); printf(__VA_ARGS__); }
#endif


namespace fs {

// Ids of the submessages for the tracking state
enum BlockId {
   BLOCKID_INFO            = 101,
   BLOCKID_POSE            = 102,
   BLOCKID_BLENDSHAPES     = 103,
   BLOCKID_EYES            = 104,
   BLOCKID_MARKERS         = 105
};


typedef long int Size;

struct BlockHeader {
    uint16_t id;
    uint16_t version;
    uint32_t size;
    BlockHeader(uint16_t _id=0,
                uint32_t _size=0,
                uint16_t _version=FSNETWORKVERSION
            ) : id(_id), version(_version), size(_size) {}
};

// Interprets the data at the position start in buffer as a T and increments start by sizeof(T)
// It should be sufficient to change/overload this function when you are on a wierd endian system
template<class T> bool read_pod(T &value, const std::string &buffer, Size &start) {
    if(start+sizeof(T) > buffer.size()) return false;
    value = *(const T*)(&buffer[start]);
    start += sizeof(T);
    return true;
}
bool read_pod(std::string &value, const std::string &buffer, Size &start) {
    uint16_t len = 0;
    if(!read_pod(len, buffer, start)) return false;
    if(start+len>Size(buffer.size())) return false; // check whether we have enough data available
    value.resize(len);
    memcpy(&(value[0]), &buffer[start], len);
    start+=len;
    return true;
}
template<class T> bool read_vector(std::vector<T> & values, const std::string & buffer, Size & start) {
    uint32_t len = 0;
    if( !read_pod(len, buffer, start)) return false;
    if( start+len*sizeof(T) > buffer.size() ) return false;
    values.resize(len);
    for(uint32_t i = 0; i < len; ++i) {
        read_pod(values[i],buffer,start);
    }
    return true;
}
template<class T> bool read_small_vector(std::vector<T> & values, const std::string & buffer, Size & start) {
    uint16_t len = 0;
    if( !read_pod(len, buffer, start)) return false;
    if( start+len*sizeof(T) > buffer.size() ) return false;
    values.resize(len);
    bool success = true;
    for(uint16_t i = 0; i < len; ++i) {
        success &= read_pod(values[i],buffer,start);
    }
    return success;
}

// Adds the bitpattern of the data to the end of the buffer.
// It should be sufficient to change/overload this function when you are on a wierd endian system
template <class T>
void write_pod(std::string &buffer, const T &value) {
    Size start = buffer.size();
    buffer.resize(start + sizeof(T));
    *(T*)(&buffer[start]) = value;
    start += sizeof(T);
}
// special write function for strings
void write_pod(std::string &buffer, const std::string &value) {
    uint16_t len = uint16_t(value.size()); write_pod(buffer, len);
    buffer.append(value);
}
template<class T> void write_vector(std::string & buffer, const std::vector<T> & values) {
    uint32_t len = values.size();
    write_pod(buffer,len);
    for(uint32_t i = 0; i < len; ++i)
        write_pod(buffer,values[i]);
}
template<class T> void write_small_vector(std::string & buffer, const std::vector<T> & values) {
    uint16_t len = values.size();
    write_pod(buffer,len);
    for(uint16_t i = 0; i < len; ++i)
        write_pod(buffer,values[i]);
}
void update_msg_size(std::string &buffer, Size start) {
    *(uint32_t*)(&buffer[start+4]) = buffer.size() - sizeof(BlockHeader) - start;
}
void update_msg_size(std::string &buffer) {
    *(uint32_t*)(&buffer[4]) = buffer.size() - sizeof(BlockHeader);
}

static void skipHeader( Size &start) {
    start += sizeof(BlockHeader);
}

//! returns whether @param data contains enough data to read the block header
static bool headerAvailable(BlockHeader &header, const std::string &buffer, Size &start, const Size &end) {
    if (end-start >= Size(sizeof(BlockHeader))) {
        header = *(BlockHeader*)(&buffer[start]);
        return true;
    } else {
        return false;
    }
}

//! returns whether @param data contains data for a full block
static bool blockAvailable(const std::string &buffer, Size &start, const Size &end) {
    BlockHeader header;
    if (!headerAvailable(header, buffer, start, end)) return false;
    return end-start >= Size(sizeof(header)+header.size);
}

fsBinaryStream::fsBinaryStream() : m_buffer(), m_start(0), m_end(0), m_valid(true) { m_buffer.resize(64*1024); } // Use a 64kb buffer by default

void fsBinaryStream::received(long int sz, const char *data) {

    long int new_end = m_end + sz;
    if (new_end > Size(m_buffer.size()) && m_start>0) {
        // If newly received block is too large to fit into the buffer, but we already have processed data from the start of the buffer, then
        // move memory to the front of the buffer
        // The buffer only grows, such that it is always large enough to contain the largest message seen so far.
        if (m_end>m_start) memmove(&m_buffer[0], &m_buffer[0] + m_start, m_end - m_start);
        m_end   = m_end - m_start;
        m_start = 0;
        new_end = m_end + sz;
    }

    if (new_end > Size(m_buffer.size())) m_buffer.resize((new_end * 15 / 10)); // HIFI: to get 1.5 without warnings

    memcpy(&m_buffer[0] + m_end, data, sz);
    m_end   += sz;

}

static bool decodeInfo(fsTrackingData & _trackingData, const std::string &buffer, Size &start) {
    bool success = true;
    success &= read_pod<double>(_trackingData.m_timestamp, buffer, start);
    unsigned char tracking_successfull = 0;
    success &= read_pod<unsigned char>( tracking_successfull, buffer, start );
    _trackingData.m_trackingSuccessful = bool(tracking_successfull != 0); // HIFI: get rid of windows warning
    return success;
}

static bool decodePose(fsTrackingData & _trackingData, const std::string &buffer, Size &start) {
    bool success = true;
    success &= read_pod(_trackingData.m_headRotation.x, buffer, start);
    success &= read_pod(_trackingData.m_headRotation.y, buffer, start);
    success &= read_pod(_trackingData.m_headRotation.z, buffer, start);
    success &= read_pod(_trackingData.m_headRotation.w, buffer, start);
    success &= read_pod(_trackingData.m_headTranslation.x, buffer, start);
    success &= read_pod(_trackingData.m_headTranslation.y, buffer, start);
    success &= read_pod(_trackingData.m_headTranslation.z, buffer, start);
    return success;
}

static bool decodeBlendshapes(fsTrackingData & _trackingData, const std::string &buffer, Size &start) {
    return read_vector(_trackingData.m_coeffs, buffer, start);
}

static bool decodeEyeGaze(fsTrackingData & _trackingData, const std::string &buffer, Size &start) {
    bool success = true;
    success &= read_pod(_trackingData.m_eyeGazeLeftPitch , buffer, start);
    success &= read_pod(_trackingData.m_eyeGazeLeftYaw   , buffer, start);
    success &= read_pod(_trackingData.m_eyeGazeRightPitch, buffer, start);
    success &= read_pod(_trackingData.m_eyeGazeRightYaw  , buffer, start);
    return success;
}

static bool decodeMarkers(fsTrackingData & _trackingData, const std::string &buffer, Size &start) {
    return read_small_vector( _trackingData.m_markers, buffer, start );
}

static bool decodeMarkerNames(fsMsgMarkerNames &_msg, const std::string &buffer, Size &start) {
    return read_small_vector(_msg.marker_names(), buffer, start);
}
static bool decodeBlendshapeNames(fsMsgBlendshapeNames &_msg, const std::string &buffer, Size &start) {
    return read_small_vector(_msg.blendshape_names(), buffer, start);
}
static bool decodeRig(fsMsgRig &_msg, const std::string &buffer, Size &start) {
    bool success = true;
    success &= read_vector(_msg.mesh().m_quads,buffer,start);                 // read quads
    success &= read_vector(_msg.mesh().m_tris,buffer,start);                  // read triangles
    success &= read_vector(_msg.mesh().m_vertex_data.m_vertices,buffer,start);// read neutral vertices
    success &= read_small_vector(_msg.blendshape_names(),buffer,start);       // read names
    uint16_t bsize = 0;
    success &= read_pod(bsize,buffer,start);
    _msg.blendshapes().resize(bsize);
    for(uint16_t i = 0;i < bsize; i++)
        success &= read_vector(_msg.blendshapes()[i].m_vertices,buffer,start);                  // read blendshapes
    return success;
}

bool is_valid_msg(int id) {
    switch(id) {
    case fsMsg::MSG_IN_START_TRACKING       :
    case fsMsg::MSG_IN_STOP_TRACKING        :
    case fsMsg::MSG_IN_CALIBRATE_NEUTRAL    :
    case fsMsg::MSG_IN_SEND_MARKER_NAMES    :
    case fsMsg::MSG_IN_SEND_BLENDSHAPE_NAMES:
    case fsMsg::MSG_IN_SEND_RIG             :
    case fsMsg::MSG_IN_HEADPOSE_RELATIVE    :
    case fsMsg::MSG_IN_HEADPOSE_ABSOLUTE    :
    case fsMsg::MSG_OUT_TRACKING_STATE      :
    case fsMsg::MSG_OUT_MARKER_NAMES        :
    case fsMsg::MSG_OUT_BLENDSHAPE_NAMES    :
    case fsMsg::MSG_OUT_RIG                 : return true;
    default:
        LOG_RELEASE_ERROR("Invalid Message ID %d", id);
        return false;
    }
}

fsMsgPtr fsBinaryStream::get_message() {
    BlockHeader super_block;
    if( !headerAvailable(super_block, m_buffer, m_start, m_end) ) return fsMsgPtr();
    if (!is_valid_msg(super_block.id)) { LOG_RELEASE_ERROR("Invalid superblock id"); m_valid = false; return fsMsgPtr(); }
    if( !blockAvailable(              m_buffer, m_start, m_end) ) return fsMsgPtr();
    skipHeader(m_start);
    long super_block_data_start = m_start;
    switch (super_block.id) {
    case fsMsg::MSG_IN_START_TRACKING: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgStartCapturing() );
    }; break;
    case fsMsg::MSG_IN_STOP_TRACKING: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgStopCapturing() );
    }; break;
    case fsMsg::MSG_IN_CALIBRATE_NEUTRAL: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgCalibrateNeutral() );
    }; break;
    case fsMsg::MSG_IN_SEND_MARKER_NAMES: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgSendMarkerNames() );
    }; break;
    case fsMsg::MSG_IN_SEND_BLENDSHAPE_NAMES: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgSendBlendshapeNames() );
    }; break;
    case fsMsg::MSG_IN_SEND_RIG: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgSendRig() );
    }; break;
    case fsMsg::MSG_IN_HEADPOSE_RELATIVE: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgHeadPoseRelative() );
    }; break;
    case fsMsg::MSG_IN_HEADPOSE_ABSOLUTE: {
        if (super_block.size > 0) { LOG_RELEASE_ERROR("Expected Size to be 0, not %d", super_block.size); m_valid = false; return fsMsgPtr(); }
        return fsMsgPtr(new fsMsgHeadPoseAbsolute() );
    }; break;
    case fsMsg::MSG_OUT_MARKER_NAMES: {
        std::tr1::shared_ptr< fsMsgMarkerNames > msg(new fsMsgMarkerNames());
        if( !decodeMarkerNames(*msg, m_buffer, m_start )) { LOG_RELEASE_ERROR("Could not decode marker names"); m_valid = false; return fsMsgPtr(); }
        uint64_t actual_size = m_start-super_block_data_start;
        if( actual_size != super_block.size ) { LOG_RELEASE_ERROR("Block was promised to be of size %d, not %d", super_block.size, actual_size); m_valid = false; return fsMsgPtr(); }
        return msg;
    }; break;
    case fsMsg::MSG_OUT_BLENDSHAPE_NAMES: {
        std::tr1::shared_ptr< fsMsgBlendshapeNames > msg(new fsMsgBlendshapeNames() );
        if( !decodeBlendshapeNames(*msg, m_buffer, m_start) ) { LOG_RELEASE_ERROR("Could not decode blendshape names"); m_valid = false; return fsMsgPtr(); }
        uint64_t actual_size = m_start-super_block_data_start;
        if( actual_size != super_block.size ) { LOG_RELEASE_ERROR("Block was promised to be of size %d, not %d", super_block.size, actual_size); m_valid = false; return fsMsgPtr(); }
        return msg;
    }; break;
    case fsMsg::MSG_OUT_TRACKING_STATE: {
        BlockHeader sub_block;
        uint16_t num_blocks = 0;
        if( !read_pod(num_blocks, m_buffer, m_start) ) { LOG_RELEASE_ERROR("Could not read num_blocks"); m_valid = false; return fsMsgPtr(); }
        std::tr1::shared_ptr<fsMsgTrackingState> msg = std::tr1::shared_ptr<fsMsgTrackingState>(new fsMsgTrackingState());
        for(int i = 0; i < num_blocks; i++) {
            if( !headerAvailable(sub_block, m_buffer, m_start, m_end) ) { LOG_RELEASE_ERROR("could not read sub-header %d", i); m_valid = false; return fsMsgPtr(); }
            if( !blockAvailable(            m_buffer, m_start, m_end) ) { LOG_RELEASE_ERROR("could not read sub-block %d",  i); m_valid = false; return fsMsgPtr(); }
            skipHeader(m_start);
            long sub_block_data_start = m_start;
            bool success = true;
            switch(sub_block.id) {
            case BLOCKID_INFO:        success &= decodeInfo(       msg->tracking_data(), m_buffer, m_start); break;
            case BLOCKID_POSE:        success &= decodePose(       msg->tracking_data(), m_buffer, m_start); break;
            case BLOCKID_BLENDSHAPES: success &= decodeBlendshapes(msg->tracking_data(), m_buffer, m_start); break;
            case BLOCKID_EYES:        success &= decodeEyeGaze(    msg->tracking_data(), m_buffer, m_start); break;
            case BLOCKID_MARKERS:     success &= decodeMarkers(    msg->tracking_data(), m_buffer, m_start); break;
            default:
                LOG_RELEASE_ERROR("Unexpected subblock id %d", sub_block.id);
                m_valid = false; return msg;
                break;
            }
            if(!success) {
                LOG_RELEASE_ERROR("Could not decode subblock with id %d", sub_block.id);
                m_valid = false; return fsMsgPtr();
            }
            uint64_t actual_size =  m_start-sub_block_data_start;
            if( actual_size != sub_block.size ) {
                LOG_RELEASE_ERROR("Unexpected number of bytes consumed %d instead of %d for subblock %d id:%d", actual_size, sub_block.size, i, sub_block.id);
                m_valid = false; return fsMsgPtr();
            }
        }
        uint64_t actual_size =  m_start-super_block_data_start;
        if( actual_size != super_block.size ) {
            LOG_RELEASE_ERROR("Unexpected number of bytes consumed %d instead of %d", actual_size, super_block.size);
            m_valid = false; return fsMsgPtr();
        }
        return msg;
    }; break;
    case fsMsg::MSG_OUT_RIG: {
        std::tr1::shared_ptr< fsMsgRig > msg(new fsMsgRig() );
        if( !decodeRig(*msg, m_buffer, m_start)                ) { LOG_RELEASE_ERROR("Could not decode rig"); m_valid = false; return fsMsgPtr(); }
        if( m_start-super_block_data_start != super_block.size ) { LOG_RELEASE_ERROR("Could not decode rig unexpected size"); m_valid = false; return fsMsgPtr(); }
        return msg;
    }; break;
    default: {
        LOG_RELEASE_ERROR("Unexpected superblock id %d", super_block.id);
        m_valid = false; return fsMsgPtr();
    }; break;
    }
    return fsMsgPtr();
}

static void encodeInfo(std::string &buffer, const fsTrackingData & _trackingData) {
    BlockHeader header(BLOCKID_INFO, sizeof(double) + 1);
    write_pod(buffer, header);

    write_pod(buffer, _trackingData.m_timestamp);
    unsigned char tracking_successfull = _trackingData.m_trackingSuccessful;
    write_pod( buffer, tracking_successfull );
}

static void encodePose(std::string &buffer, const fsTrackingData & _trackingData) {
    BlockHeader header(BLOCKID_POSE, sizeof(float)*7);
    write_pod(buffer, header);

    write_pod(buffer, _trackingData.m_headRotation.x);
    write_pod(buffer, _trackingData.m_headRotation.y);
    write_pod(buffer, _trackingData.m_headRotation.z);
    write_pod(buffer, _trackingData.m_headRotation.w);
    write_pod(buffer, _trackingData.m_headTranslation.x);
    write_pod(buffer, _trackingData.m_headTranslation.y);
    write_pod(buffer, _trackingData.m_headTranslation.z);
}

static void encodeBlendshapes(std::string &buffer, const fsTrackingData & _trackingData) {
    uint32_t num_parameters = _trackingData.m_coeffs.size();
    BlockHeader header(BLOCKID_BLENDSHAPES, sizeof(uint32_t) + sizeof(float)*num_parameters);
    write_pod(buffer, header);
    write_pod(buffer, num_parameters);
    for(uint32_t i = 0; i < num_parameters; i++)
        write_pod(buffer, _trackingData.m_coeffs[i]);
}

static void encodeEyeGaze(std::string &buffer, const fsTrackingData & _trackingData) {
    BlockHeader header(BLOCKID_EYES, sizeof(float)*4);
    write_pod(buffer, header);
    write_pod(buffer, _trackingData.m_eyeGazeLeftPitch );
    write_pod(buffer, _trackingData.m_eyeGazeLeftYaw   );
    write_pod(buffer, _trackingData.m_eyeGazeRightPitch);
    write_pod(buffer, _trackingData.m_eyeGazeRightYaw  );
}

static void encodeMarkers(std::string &buffer, const fsTrackingData & _trackingData) {
    uint16_t numMarkers = _trackingData.m_markers.size();
    BlockHeader header(BLOCKID_MARKERS, sizeof(uint16_t) + sizeof(float)*3*numMarkers);
    write_pod(buffer, header);
    write_pod(buffer, numMarkers);
    for(int i = 0; i < numMarkers; i++) {
        write_pod(buffer, _trackingData.m_markers[i].x);
        write_pod(buffer, _trackingData.m_markers[i].y);
        write_pod(buffer, _trackingData.m_markers[i].z);
    }
}

// Inbound
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgTrackingState       &msg) {
    encode_message(msg_out, msg.tracking_data());
}

void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgStartCapturing      &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgStopCapturing       &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgCalibrateNeutral    &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgSendMarkerNames     &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgSendBlendshapeNames &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgSendRig             &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgHeadPoseRelative    &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgHeadPoseAbsolute    &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}

// Outbound
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgSignal              &msg) {
    BlockHeader header(msg.id());
    write_pod(msg_out, header);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsTrackingData           &tracking_data) {
    Size start = msg_out.size();

    BlockHeader header(fsMsg::MSG_OUT_TRACKING_STATE);
    write_pod(msg_out, header);

    uint16_t N_blocks = 5;
    write_pod(msg_out, N_blocks);
    encodeInfo(       msg_out, tracking_data);
    encodePose(       msg_out, tracking_data);
    encodeBlendshapes(msg_out, tracking_data);
    encodeEyeGaze(    msg_out, tracking_data);
    encodeMarkers(    msg_out, tracking_data);

    update_msg_size(msg_out, start);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgMarkerNames         &msg) {
    Size start = msg_out.size();

    BlockHeader header(msg.id());
    write_pod(msg_out, header);

    write_small_vector(msg_out,msg.marker_names());

    update_msg_size(msg_out, start);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgBlendshapeNames     &msg) {
    Size start = msg_out.size();

    BlockHeader header(msg.id());
    write_pod(msg_out, header);

    write_small_vector(msg_out,msg.blendshape_names());

    update_msg_size(msg_out, start);
}
void fsBinaryStream::encode_message(std::string &msg_out, const fsMsgRig                 &msg) {
    Size start = msg_out.size();

    BlockHeader header(msg.id());
    write_pod(msg_out, header);

    write_vector(msg_out, msg.mesh().m_quads); // write quads
    write_vector(msg_out, msg.mesh().m_tris);// write triangles
    write_vector(msg_out, msg.mesh().m_vertex_data.m_vertices);// write neutral vertices
    write_small_vector(msg_out, msg.blendshape_names());// write names
    write_pod(msg_out,uint16_t(msg.blendshapes().size()));
    for(uint16_t i = 0;i < uint16_t(msg.blendshapes().size()); i++)
        write_vector(msg_out, msg.blendshapes()[i].m_vertices); // write blendshapes

    update_msg_size(msg_out, start);
}
}
