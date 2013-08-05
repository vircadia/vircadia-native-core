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
    enum
    {
        MagMaxReferences = 80
    };

public:
    SensorFusion(SensorDevice* sensor = 0);
    ~SensorFusion();
    
    // Attaches this SensorFusion to a sensor device, from which it will receive
    // notification messages. If a sensor is attached, manual message notification
    // is not necessary. Calling this function also resets SensorFusion state.
    bool        AttachToSensor(SensorDevice* sensor);

    // Returns true if this Sensor fusion object is attached to a sensor.
    bool        IsAttachedToSensor() const              { return Handler.IsHandlerInstalled(); }

    void        SetGravityEnabled(bool enableGravity)   { EnableGravity = enableGravity; }
   
    bool        IsGravityEnabled() const                { return EnableGravity;}

	void        SetYawCorrectionEnabled(bool enableYawCorrection) { EnableYawCorrection = enableYawCorrection; }
   
    // Yaw correction is set up to work
    bool        IsYawCorrectionEnabled() const          { return EnableYawCorrection;}

    // Yaw correction is currently working (forcing a corrective yaw rotation)
    bool        IsYawCorrectionInProgress() const       { return YawCorrectionInProgress;}

    // Store the calibration matrix for the magnetometer
    void        SetMagCalibration(const Matrix4f& m)
    {
        MagCalibrationMatrix = m;
        MagCalibrated = true;
    }

    // True only if the mag has calibration values stored
    bool        HasMagCalibration() const        { return MagCalibrated;}
  
    // Force the mag into the uncalibrated state
    void        ClearMagCalibration()            { MagCalibrated = false; }

	// These refer to reference points that associate mag readings with orientations
	void        ClearMagReferences()             { MagNumReferences = 0; }
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

    // Use a predictive filter to estimate the future orientation
	Quatf       GetPredictedOrientation(float pdt); // Specify lookahead time in ms
	Quatf       GetPredictedOrientation() { return GetPredictedOrientation(PredictionDT); }

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
        return RawMag;
    }
    // Obtain the filtered magnetometer reading, in Gauss
    Vector3f    GetFilteredMagnetometer() const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return FRawMag.Mean();
    }
    // Obtain the calibrated magnetometer reading (direction and field strength)
    Vector3f    GetCalibratedMagnetometer() const
    {
        OVR_ASSERT(MagCalibrated);
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return CalMag;
    }

    Vector3f    GetCalibratedMagValue(const Vector3f& rawMag) const;

    float       GetMagRefYaw() const
    {
        return MagRefYaw;
    }

	float       GetYawErrorAngle() const
	{
		return YawErrorAngle;
	}
    // For later
    //Vector3f    GetGravity() const;

    // Resets the current orientation
    void        Reset();

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

private:
    SensorFusion* getThis()  { return this; }

    // Internal handler for messages; bypasses error checking.
    void handleMessage(const MessageBodyFrame& msg);

    // Set the magnetometer's reference orientation for use in yaw correction
    // The supplied mag is an uncalibrated value
    void        SetMagReference(const Quatf& q, const Vector3f& rawMag);
    // Default to current HMD orientation
    void        SetMagReference()                { SetMagReference(Q, RawMag); }

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
	Quatf			  QUncorrected;
    Vector3f          A;    
    Vector3f          AngV;
    Vector3f          CalMag;
    Vector3f          RawMag;
    unsigned int      Stage;
	float             RunningTime;
	float             DeltaT;
    BodyFrameHandler  Handler;
    MessageHandler*   pDelegate;
    float             Gain;
    float             YawMult;
    volatile bool     EnableGravity;

    bool              EnablePrediction;
    float             PredictionDT;
	float             PredictionTimeIncrement;

    SensorFilter      FRawMag;
    SensorFilter      FAccW;
    SensorFilter      FAngV;

    int               TiltCondCount;
    float             TiltErrorAngle;
    Vector3f          TiltErrorAxis;

    bool              EnableYawCorrection;
    Matrix4f          MagCalibrationMatrix;
    bool              MagCalibrated;
    int               MagCondCount;
    float             MagRefDistance;
    Quatf             MagRefQ;
    Vector3f          MagRefM;
    float             MagRefYaw;
    bool              MagHasNearbyReference;
    Quatf             MagRefTableQ[MagMaxReferences];
    Vector3f          MagRefTableM[MagMaxReferences];
    float             MagRefTableYaw[MagMaxReferences];
    int               MagNumReferences;
    float             YawErrorAngle;
    int               YawErrorCount;
    bool              YawCorrectionInProgress;
	bool			  YawCorrectionActivated;

};


} // namespace OVR

#endif
