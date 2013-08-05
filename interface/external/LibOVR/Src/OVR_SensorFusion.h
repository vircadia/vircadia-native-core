/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_SensorFusion.h
Content     :   Methods that determine head orientation from sensor data over time
Created     :   October 9, 2012
Authors     :   Michael Antonov, Steve LaValle

Copyright   :   Copyright 2012 Oculus VR, Inc. All Rights reserved.

Use of this software is subject to the terms of the Oculus license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

*************************************************************************************/

#ifndef OVR_SensorFusion_h
#define OVR_SensorFusion_h

#include "OVR_Device.h"
#include "OVR_SensorFilter.h"

namespace OVR {

//-------------------------------------------------------------------------------------
// ***** SensorFusion

// SensorFusion class accumulates Sensor notification messages to keep track of
// orientation, which involves integrating the gyro and doing correction with gravity.
// Orientation is reported as a quaternion, from which users can obtain either the
// rotation matrix or Euler angles.
//
// The class can operate in two ways:
//  - By user manually passing MessageBodyFrame messages to the OnMessage() function. 
//  - By attaching SensorFusion to a SensorDevice, in which case it will
//    automatically handle notifications from that device.

class SensorFusion : public NewOverrideBase
{
public:
    SensorFusion(SensorDevice* sensor = 0);
    ~SensorFusion();
    
    // Attaches this SensorFusion to a sensor device, from which it will receive
    // notification messages. If a sensor is attached, manual message notification
    // is not necessary. Calling this function also resets SensorFusion state.
    bool        AttachToSensor(SensorDevice* sensor);

    // Returns true if this Sensor fusion object is attached to a sensor.
    bool        IsAttachedToSensor() const { return Handler.IsHandlerInstalled(); }

    void        SetGravityEnabled(bool enableGravity) { EnableGravity = enableGravity; }
   
    bool        IsGravityEnabled() const { return EnableGravity;}

    void        SetYawCorrectionEnabled(bool enableYawCorrection) { EnableYawCorrection = enableYawCorrection; }
   
    // Yaw correction is set up to work
    bool        IsYawCorrectionEnabled() const { return EnableYawCorrection;}

    // Yaw correction is currently working (forcing a corrective yaw rotation)
    bool        IsYawCorrectionInProgress() const { return YawCorrectionInProgress;}

    // Store the calibration matrix for the magnetometer
    void        SetMagCalibration(const Matrix4f& m)
    {
        MagCalibrationMatrix = m;
        MagCalibrated = true;
    }

    // True only if the mag has calibration values stored
    bool        HasMagCalibration() const { return MagCalibrated;}
    
    // Force the mag into the uncalibrated state
    void        ClearMagCalibration() 
    { 
        MagCalibrated = false;
        MagReady = false;
    }

    // Set the magnetometer's reference orientation for use in yaw correction
    void        SetMagReference(const Quatf& q);
    // Default to current HMD orientation
    void        SetMagReference() { SetMagReference(Q); }

    bool        HasMagReference() const { return MagReferenced; }

    void        ClearMagReference() 
    { 
        MagReferenced = false; 
        MagReady = false;
    }

    bool        IsMagReady() const { return MagReady; }

    void        SetMagRefDistance(const float d) { MagRefDistance = d; }

    // Notifies SensorFusion object about a new BodyFrame message from a sensor.
    // Should be called by user if not attaching to a sensor.
    void        OnMessage(const MessageBodyFrame& msg)
    {
        OVR_ASSERT(!IsAttachedToSensor());
        handleMessage(msg);
    }

    // Obtain the current accumulated orientation.
    Quatf       GetOrientation() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return Q;
    }    
    Quatf       GetPredictedOrientation() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return QP;
    }    
    // Obtain the last absolute acceleration reading, in m/s^2.
    Vector3f    GetAcceleration() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return A;
    }
    
    // Obtain the last angular velocity reading, in rad/s.
    Vector3f    GetAngularVelocity() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return AngV;
    }
    // Obtain the last magnetometer reading, in Gauss
    Vector3f    GetMagnetometer() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return Mag;
    }
    // Obtain the raw magnetometer reading, in Gauss (uncalibrated!)
    Vector3f    GetRawMagnetometer() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return RawMag;
    }

    float       GetMagRefYaw() const
    {
        return MagRefYaw;
    }
    // For later
    //Vector3f    GetGravity() const;

    // Resets the current orientation
    void        Reset()
    {
        MagReferenced = false;

        Lock::Locker lockScope(Handler.GetHandlerLock());
        Q  = Quatf();
        QP = Quatf();

        Stage = 0;
    }

    // Configuration

    // Gain used to correct gyro with accel. Default value is appropriate for typical use.
    float       GetAccelGain() const   { return Gain; }
    void        SetAccelGain(float ag) { Gain = ag; }

    // Multiplier for yaw rotation (turning); setting this higher than 1 (the default) can allow the game
    // to be played without auxillary rotation controls, possibly making it more immersive. Whether this is more
    // or less likely to cause motion sickness is unknown.
    float       GetYawMultiplier() const  { return YawMult; }
    void        SetYawMultiplier(float y) { YawMult = y; }

    void        SetDelegateMessageHandler(MessageHandler* handler)
    { pDelegate = handler; }

	// Prediction functions.
    // Prediction delta specifes how much prediction should be applied in seconds; it should in
    // general be under the average rendering latency. Call GetPredictedOrientation() to get
    // predicted orientation.
    float       GetPredictionDelta() const                  { return PredictionDT; }
    void        SetPrediction(float dt, bool enable = true) { PredictionDT = dt; EnablePrediction = enable; }
	void		SetPredictionEnabled(bool enable = true)    { EnablePrediction = enable; }    
	bool		IsPredictionEnabled()                       { return EnablePrediction; }

    // Methods for magnetometer calibration
    float       AngleDifference(float theta1, float theta2);
    Vector3f    CalculateSphereCenter(Vector3f p1, Vector3f p2,
                                      Vector3f p3, Vector3f p4);


private:
    SensorFusion* getThis()  { return this; }

    // Internal handler for messages; bypasses error checking.
    void handleMessage(const MessageBodyFrame& msg);

    class BodyFrameHandler : public MessageHandler
    {
        SensorFusion* pFusion;
    public:
        BodyFrameHandler(SensorFusion* fusion) : pFusion(fusion) { }
        ~BodyFrameHandler();

        virtual void OnMessage(const Message& msg);
        virtual bool SupportsMessageType(MessageType type) const;
    };   

    Quatf             Q;
    Vector3f          A;    
    Vector3f          AngV;
    Vector3f          Mag;
    Vector3f          RawMag;
    unsigned int      Stage;
    BodyFrameHandler  Handler;
    MessageHandler*   pDelegate;
    float             Gain;
    float             YawMult;
    volatile bool     EnableGravity;

    bool              EnablePrediction;
    float             PredictionDT;
    Quatf             QP;

    SensorFilter      FMag;
    SensorFilter      FAccW;
    SensorFilter      FAngV;

    int               TiltCondCount;
    float             TiltErrorAngle;
    Vector3f          TiltErrorAxis;

    bool              EnableYawCorrection;
    Matrix4f          MagCalibrationMatrix;
    bool              MagCalibrated;
    int               MagCondCount;
    bool              MagReferenced;
    float             MagRefDistance;
    bool              MagReady;
    Quatf             MagRefQ;
    Vector3f          MagRefM;
    float             MagRefYaw;
    float             YawErrorAngle;
    int               YawErrorCount;
    bool              YawCorrectionInProgress;

};


} // namespace OVR

#endif
