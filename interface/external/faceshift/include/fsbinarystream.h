#pragma once

#ifndef FSBINARYSTREAM_H
#define FSBINARYSTREAM_H

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


/**
 * Define the HAVE_EIGEN preprocessor define, if you are using the Eigen library, it allows you to easily convert our tracked data from and to eigen
 * See fsVector3f and fsQuaternionf for more details
 **/

#ifdef HAVE_EIGEN
#include <Eigen/Core>
#include <Eigen/Geometry>
#endif

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

#include <string>
#include <vector>
#include <stdint.h>

/*******************************************************************************************
  * This first part of the file contains a definition of the datastructures holding the
  * tracking results
  ******************************************************************************************/

namespace fs {

/**
 * A floating point three-vector.
 *
 * To keep these networking classes as simple as possible, we do not implement the
 * vector semantics here, use Eigen for that purpose. The class just holds three named floats,
 * and you have to interpret them yourself.
 **/
struct fsVector3f {
    float x,y,z;

    fsVector3f() {}
#ifdef HAVE_EIGEN
    explicit fsVector3f(const Eigen::Matrix<float,3,1> &v) : x(v[0]), y(v[1]), z(v[2]) {}
   Eigen::Map< Eigen::Matrix<float,3,1> > eigen() const { return Eigen::Map<Eigen::Matrix<float,3,1> >((float*)this); }
#endif
};

/**
 * An integer three-vector.
 **/
struct fsVector3i {
    int32_t x,y,z;

    fsVector3i() {}
#ifdef HAVE_EIGEN
    explicit fsVector3i(const Eigen::Matrix<int32_t,3,1> &v) : x(v[0]), y(v[1]), z(v[2]) {}
    Eigen::Map<Eigen::Matrix<int32_t,3,1> > eigen() const { return Eigen::Map<Eigen::Matrix<int32_t,3,1> >((int32_t*)this); }
#endif
};

/**
 * An integer four-vector.
 **/
struct fsVector4i {
    int32_t x,y,z,w;

    fsVector4i() {}
#ifdef HAVE_EIGEN
    explicit fsVector4i(const Eigen::Matrix<int32_t,4,1> &v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}
    Eigen::Map<Eigen::Matrix<int32_t,4,1,Eigen::DontAlign> > eigen() const { return Eigen::Map<Eigen::Matrix<int32_t,4,1,Eigen::DontAlign> >((int32_t*)this); }
#endif
};

/**
 * Structure holding the data of a quaternion.
 *
 *To keep these networking classes as simple as possible, we do not implement the
 * quaternion semantics here. The class just holds four named floats, and you have to interpret them yourself.
 *
 * If you have Eigen you can just cast this class to an Eigen::Quaternionf and use it.
 *
 *   The quaternion is defined as w+xi+yj+zk
 **/
struct fsQuaternionf {
    float x,y,z,w;

    fsQuaternionf() {}
#ifdef HAVE_EIGEN
    explicit fsQuaternionf(const Eigen::Quaternionf &q) : x(q.x()), y(q.y()), z(q.z()), w(q.w()) {}
    Eigen::Quaternionf eigen() const { return Eigen::Quaternionf(w,x,y,z); }
#endif
};

/**
 * A structure containing the data tracked for a single frame.
 **/
class fsTrackingData {
    public:
    //! time stamp in ms
    double m_timestamp;

    //! flag whether tracking was successful [0,1]
    bool m_trackingSuccessful;

    //! head pose
    fsQuaternionf m_headRotation;
    fsVector3f    m_headTranslation;

    //! eye gaze in degrees
    float m_eyeGazeLeftPitch;
    float m_eyeGazeLeftYaw;
    float m_eyeGazeRightPitch;
    float m_eyeGazeRightYaw;

    //! blendshape coefficients
    std::vector<float> m_coeffs;

    //! marker positions - format specified in faceshift
    std::vector< fsVector3f > m_markers;
};

/**
  * A structure containing vertex information
  */
class fsVertexData {
public:
    //! vertex data
    std::vector<fsVector3f> m_vertices;

#ifdef HAVE_EIGEN
    Eigen::Map<Eigen::Matrix<float,3,Eigen::Dynamic> > eigen() { return Eigen::Map<Eigen::Matrix<float,3,Eigen::Dynamic> >((float*)m_vertices.data(),3,m_vertices.size()); }
#endif
};

/**
  * A strucutre containing mesh information
  */
class fsMeshData {
public:
    //! topology (quads)
    std::vector<fsVector4i> m_quads;

    //! topology (triangles)
    std::vector<fsVector3i> m_tris;

    //! vertex data
    fsVertexData m_vertex_data;

#ifdef HAVE_EIGEN
    Eigen::Map<Eigen::Matrix<int32_t,4,Eigen::Dynamic,Eigen::DontAlign> > quads_eigen() { return Eigen::Map<Eigen::Matrix<int32_t,4,Eigen::Dynamic,Eigen::DontAlign> >((int32_t*)m_quads.data(),4,m_quads.size()); }
    Eigen::Map<Eigen::Matrix<int32_t,3,Eigen::Dynamic> > tris_eigen() { return Eigen::Map<Eigen::Matrix<int32_t,3,Eigen::Dynamic> >((int32_t*)m_tris.data(),3,m_tris.size()); }
    Eigen::Map<Eigen::Matrix<float,3,Eigen::Dynamic> > vertices_eigen() { return m_vertex_data.eigen(); }
#endif

};

/*******************************************************************************************
  * Now follows a definition of datastructures encapsulating the network messages
  ******************************************************************************************/

/** Predeclaration of the message types available in faceshift **/

// Inbound
class fsMsgStartCapturing;
class fsMsgStopCapturing;
class fsMsgCalibrateNeutral;
class fsMsgSendMarkerNames;
class fsMsgSendBlendshapeNames;
class fsMsgSendRig;

// Outbound
class fsMsgTrackingState;
class fsMsgMarkerNames;
class fsMsgBlendshapeNames;
class fsMsgRig;

/**
 * Base class of all message that faceshift is sending.
 * A class can be queried for its type, using the id() function for use in a switch statement, or by using a dynamic_cast.
 **/
class fsMsg {
public:
    virtual ~fsMsg() {}

    enum MessageType {
        // Messages to control faceshift via the network
        // These are sent from the client to faceshift
        MSG_IN_START_TRACKING        = 44344,
        MSG_IN_STOP_TRACKING         = 44444,
        MSG_IN_CALIBRATE_NEUTRAL     = 44544,
        MSG_IN_SEND_MARKER_NAMES     = 44644,
        MSG_IN_SEND_BLENDSHAPE_NAMES = 44744,
        MSG_IN_SEND_RIG              = 44844,
        MSG_IN_HEADPOSE_RELATIVE     = 44944,
        MSG_IN_HEADPOSE_ABSOLUTE     = 44945,

        // Messages containing tracking information
        // These are sent form faceshift to the client application
        MSG_OUT_TRACKING_STATE       = 33433,
        MSG_OUT_MARKER_NAMES         = 33533,
        MSG_OUT_BLENDSHAPE_NAMES     = 33633,
        MSG_OUT_RIG                  = 33733
    };

    virtual MessageType id() const = 0;
};
typedef std::tr1::shared_ptr<fsMsg> fsMsgPtr;


/*************
  * Inbound
  ***********/
class fsMsgStartCapturing : public fsMsg {
public:
    virtual ~fsMsgStartCapturing() {}
    virtual MessageType id() const { return MSG_IN_START_TRACKING; }
};
class fsMsgStopCapturing : public fsMsg {
public:
    virtual ~fsMsgStopCapturing() {}
    virtual MessageType id() const { return MSG_IN_STOP_TRACKING; }
};
class fsMsgCalibrateNeutral : public fsMsg {
public:
    virtual ~fsMsgCalibrateNeutral() {}
    virtual MessageType id() const { return MSG_IN_CALIBRATE_NEUTRAL; }
};
class fsMsgSendMarkerNames : public fsMsg {
public:
    virtual ~fsMsgSendMarkerNames() {}
    virtual MessageType id() const { return MSG_IN_SEND_MARKER_NAMES; }
};
class fsMsgSendBlendshapeNames : public fsMsg {
public:
    virtual ~fsMsgSendBlendshapeNames() {}
    virtual MessageType id() const { return MSG_IN_SEND_BLENDSHAPE_NAMES; }
};
class fsMsgSendRig : public fsMsg {
public:
    virtual ~fsMsgSendRig() {}
    virtual MessageType id() const { return MSG_IN_SEND_RIG; }
};
class fsMsgHeadPoseRelative : public fsMsg {
public:
    virtual ~fsMsgHeadPoseRelative() {}
    virtual MessageType id() const { return MSG_IN_HEADPOSE_RELATIVE; }
};
class fsMsgHeadPoseAbsolute : public fsMsg {
public:
    virtual ~fsMsgHeadPoseAbsolute() {}
    virtual MessageType id() const { return MSG_IN_HEADPOSE_ABSOLUTE; }
};

/*************
  * Outbound
  ***********/
class fsMsgTrackingState : public fsMsg {
public:
    virtual ~fsMsgTrackingState() {}

    /* */ fsTrackingData & tracking_data() /* */ { return m_tracking_data; }
    const fsTrackingData & tracking_data() const { return m_tracking_data; }

    virtual MessageType id() const { return MSG_OUT_TRACKING_STATE; }

private:
    fsTrackingData m_tracking_data;
};
class fsMsgMarkerNames : public fsMsg {
public:
    virtual ~fsMsgMarkerNames() {}

    /* */ std::vector<std::string> & marker_names() /* */ { return m_marker_names; }
    const std::vector<std::string> & marker_names() const { return m_marker_names; }

    virtual MessageType id() const { return MSG_OUT_MARKER_NAMES; }
private:
    std::vector<std::string> m_marker_names;
};
class fsMsgBlendshapeNames : public fsMsg {
public:
    virtual ~fsMsgBlendshapeNames() {}

    /* */ std::vector<std::string> & blendshape_names() /* */ { return m_blendshape_names; }
    const std::vector<std::string> & blendshape_names() const { return m_blendshape_names; }

    virtual MessageType id() const { return MSG_OUT_BLENDSHAPE_NAMES; }
private:
    std::vector<std::string> m_blendshape_names;
};
class fsMsgRig : public fsMsg {
public:
    virtual ~fsMsgRig() {}

    virtual MessageType id() const { return MSG_OUT_RIG; }

    /* */ fsMeshData & mesh() /* */ { return m_mesh; }
    const fsMeshData & mesh() const { return m_mesh; }

    /* */ std::vector<std::string> & blendshape_names() /* */ { return m_blendshape_names; }
    const std::vector<std::string> & blendshape_names() const { return m_blendshape_names; }

    /* */ std::vector<fsVertexData> & blendshapes() /* */ { return m_blendshapes; }
    const std::vector<fsVertexData> & blendshapes() const { return m_blendshapes; }

private:
    //! neutral mesh
    fsMeshData m_mesh;
    //! blendshape names
    std::vector<std::string> m_blendshape_names;
    //! blendshapes
    std::vector<fsVertexData> m_blendshapes;
};
class fsMsgSignal : public fsMsg {
    MessageType m_id;
public:
    explicit fsMsgSignal(MessageType id) : m_id(id) {}
    virtual ~fsMsgSignal() {}
    virtual MessageType id() const { return m_id; }
};

/**
  * Class to parse a faceshift data stream, and to create message to write into such a stream
  *
  * This needs to be connected with your networking methods by calling
  *
  *   void received(int, const char *);
  *
  * whenever new data is available. After adding received data to the parser you can parse faceshift messages using the
  *
  *   std::tr1::shared_ptr<fsMsg> get_message();
  *
  * to get the next message, if a full block of data has been received. This should be iterated until no more messages are in the buffer.
  *
  * You can also use this to encode messages to send back to faceshift. This works by calling the
  *
  *   void encode_message(std::string &msg_out, const fsMsg &msg);
  *
  * methods (actually the specializations existing for each of our message types). This will encode the message into a
  * binary string in msg_out. You then only need to push the resulting string over the network to faceshift.
  *
  * This class does not handle differences in endianness or other strange things that can happen when pushing data over the network.
  * Should you have to adapt this to such a system, then it should be possible to do this by changing only the write_... and read_...
  * functions in the accompanying cpp file, but so far there was no need for it.
  **/
class fsBinaryStream {
public:
    fsBinaryStream();

    /**
      * Use to push data into the parser. Typically called inside of your network receiver routine
      **/
    void received(long int, const char *);
    /**
      * After pushing data, you can try to extract messages from the stream. Process messages until a null pointer is returned.
      **/
    fsMsgPtr get_message();
    /**
      * When an invalid message is received, the valid field is set to false. No attempt is made to recover from the problem, you will have to disconnect.
      **/
    bool valid() const { return m_valid; }
    void clear() { m_start = 0; m_end = 0; m_valid=true; }

    // Inbound
    static void encode_message(std::string &msg_out, const fsMsgTrackingState       &msg);
    static void encode_message(std::string &msg_out, const fsMsgStartCapturing      &msg);
    static void encode_message(std::string &msg_out, const fsMsgStopCapturing       &msg);
    static void encode_message(std::string &msg_out, const fsMsgCalibrateNeutral    &msg);
    static void encode_message(std::string &msg_out, const fsMsgSendMarkerNames     &msg);
    static void encode_message(std::string &msg_out, const fsMsgSendBlendshapeNames &msg);
    static void encode_message(std::string &msg_out, const fsMsgSendRig             &msg);
    static void encode_message(std::string &msg_out, const fsMsgHeadPoseRelative    &msg);
    static void encode_message(std::string &msg_out, const fsMsgHeadPoseAbsolute    &msg);

    // Outbound
    static void encode_message(std::string &msg_out, const fsTrackingData           &msg);
    static void encode_message(std::string &msg_out, const fsMsgMarkerNames         &msg);
    static void encode_message(std::string &msg_out, const fsMsgBlendshapeNames     &msg);
    static void encode_message(std::string &msg_out, const fsMsgRig                 &msg);
    static void encode_message(std::string &msg_out, const fsMsgSignal              &msg); // Generic Signal

private:
    std::string m_buffer;
    long int    m_start;
    long int    m_end;
    bool        m_valid;

};

}


#endif // FSBINARYSTREAM_H
