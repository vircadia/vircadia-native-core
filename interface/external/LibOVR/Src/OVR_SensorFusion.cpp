/************************************************************************************

Filename    :   OVR_SensorFusion.cpp
Content     :   Methods that determine head orientation from sensor data over time
Created     :   October 9, 2012
Authors     :   Michael Antonov, Steve LaValle

Copyright   :   Copyright 2012 Oculus VR, Inc. All Rights reserved.

Use of this software is subject to the terms of the Oculus license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

*************************************************************************************/

#include "OVR_SensorFusion.h"
#include "Kernel/OVR_Log.h"
#include "Kernel/OVR_System.h"

namespace OVR {

//-------------------------------------------------------------------------------------
// ***** Sensor Fusion

SensorFusion::SensorFusion(SensorDevice* sensor)
  : Handler(getThis()), pDelegate(0),
    Gain(0.05f), YawMult(1), EnableGravity(true), Stage(0), 
	EnablePrediction(false), PredictionDT(0.03f),
    FMag(10), FAccW(20), FAngV(20),
    TiltCondCount(0), TiltErrorAngle(0), 
    TiltErrorAxis(0,1,0),
    MagCondCount(0), MagReady(false), MagCalibrated(false), MagReferenced(false), 
    MagRefQ(0, 0, 0, 1), MagRefM(0), MagRefYaw(0), YawErrorAngle(0), MagRefDistance(0.15f),
    YawErrorCount(0), YawCorrectionInProgress(false), EnableYawCorrection(false)
{
   if (sensor)
       AttachToSensor(sensor);
   MagCalibrationMatrix.SetIdentity();
}

SensorFusion::~SensorFusion()
{
}


bool SensorFusion::AttachToSensor(SensorDevice* sensor)
{
    
    if (sensor != NULL)
    {
        MessageHandler* pCurrentHandler = sensor->GetMessageHandler();

        if (pCurrentHandler == &Handler)
        {
            Reset();
            return true;
        }

        if (pCurrentHandler != NULL)
        {
            OVR_DEBUG_LOG(
                ("SensorFusion::AttachToSensor failed - sensor %p already has handler", sensor));
            return false;
        }
    }

    if (Handler.IsHandlerInstalled())
    {
        Handler.RemoveHandlerFromDevices();
    }

    if (sensor != NULL)
    {
        sensor->SetMessageHandler(&Handler);
    }

    Reset();
    return true;
}




void SensorFusion::handleMessage(const MessageBodyFrame& msg)
{
    if (msg.Type != Message_BodyFrame)
        return;
  
    // Put the sensor readings into convenient local variables
    float deltaT       = msg.TimeDelta;
    Vector3f angVel    = msg.RotationRate; 
    Vector3f rawAccel  = msg.Acceleration;
    Vector3f mag       = msg.MagneticField;

    // Set variables accessible through the class API
    AngV = msg.RotationRate;
    AngV.y *= YawMult;  // Warning: If YawMult != 1, then AngV is not true angular velocity
    A = rawAccel;

    // Allow external access to uncalibrated magnetometer values
    RawMag = mag;  

    // Apply the calibration parameters to raw mag
    if (HasMagCalibration())
    {
        mag.x += MagCalibrationMatrix.M[0][3];
        mag.y += MagCalibrationMatrix.M[1][3];
        mag.z += MagCalibrationMatrix.M[2][3];
    }

    // Provide external access to calibrated mag values
    // (if the mag is not calibrated, then the raw value is returned)
    Mag = mag;

    float angVelLength = angVel.Length();
    float accLength    = rawAccel.Length();


    // Acceleration in the world frame (Q is current HMD orientation)
    Vector3f accWorld  = Q.Rotate(rawAccel);

    // Keep track of time
    Stage++;
    float currentTime  = Stage * deltaT; // Assumes uniform time spacing

    // Insert current sensor data into filter history
    FMag.AddElement(mag);
    FAccW.AddElement(accWorld);
    FAngV.AddElement(angVel);

    // Update orientation Q based on gyro outputs.  This technique is
    // based on direct properties of the angular velocity vector:
    // Its direction is the current rotation axis, and its magnitude
    // is the rotation rate (rad/sec) about that axis.  Our sensor
    // sampling rate is so fast that we need not worry about integral
    // approximation error (not yet, anyway).
    if (angVelLength > 0.0f)
    {
        Vector3f     rotAxis      = angVel / angVelLength;  
        float        halfRotAngle = angVelLength * deltaT * 0.5f;
        float        sinHRA       = sin(halfRotAngle);
        Quatf        deltaQ(rotAxis.x*sinHRA, rotAxis.y*sinHRA, rotAxis.z*sinHRA, cos(halfRotAngle));

        Q =  Q * deltaQ;

        // This is a simple predictive filter based only on extrapolating the smoothed, current angular velocity.
        // Note that both QP (the predicted future orientation) and Q (the current orientation) are both maintained.
        QP = Q;
        if (EnablePrediction)
        {
            Vector3f angVelF  = FAngV.SavitzkyGolaySmooth8();
            float    angVelFL = angVelF.Length();
            
            if (angVelFL > 0.001f)
            {
                Vector3f    rotAxisP = angVelF / angVelFL;  
                float       halfRotAngleP = angVelFL * PredictionDT * 0.5f;
                float       sinaHRAP  = sin(halfRotAngleP);
                Quatf       deltaQP(rotAxisP.x*sinaHRAP, rotAxisP.y*sinaHRAP,
                                    rotAxisP.z*sinaHRAP, cos(halfRotAngleP));
                QP = Q * deltaQP;
            }
        }
    }
    
    // The quaternion magnitude may slowly drift due to numerical error,
    // so it is periodically normalized.
    if (Stage % 5000 == 0)
        Q.Normalize();
    
    // Perform tilt correction using the accelerometer data. This enables 
    // drift errors in pitch and roll to be corrected. Note that yaw cannot be corrected
    // because the rotation axis is parallel to the gravity vector.
    if (EnableGravity)
    {
        // Correcting for tilt error by using accelerometer data
        const float  gravityEpsilon = 0.4f;
        const float  angVelEpsilon  = 0.1f; // Relatively slow rotation
        const int    tiltPeriod     = 50;   // Req'd time steps of stability
        const float  maxTiltError   = 0.05f;
        const float  minTiltError   = 0.01f;

        // This condition estimates whether the only measured acceleration is due to gravity 
        // (the Rift is not linearly accelerating).  It is often wrong, but tends to average
        // out well over time.
        if ((fabs(accLength - 9.81f) < gravityEpsilon) &&
            (angVelLength < angVelEpsilon))
            TiltCondCount++;
        else
            TiltCondCount = 0;
    
        // After stable measurements have been taken over a sufficiently long period,
        // estimate the amount of tilt error and calculate the tilt axis for later correction.
        if (TiltCondCount >= tiltPeriod)
        {   // Update TiltErrorEstimate
            TiltCondCount = 0;
            // Use an average value to reduce noice (could alternatively use an LPF)
            Vector3f accWMean = FAccW.Mean();
            // Project the acceleration vector into the XZ plane
            Vector3f xzAcc = Vector3f(accWMean.x, 0.0f, accWMean.z);
            // The unit normal of xzAcc will be the rotation axis for tilt correction
            Vector3f tiltAxis = Vector3f(xzAcc.z, 0.0f, -xzAcc.x).Normalized();
            Vector3f yUp = Vector3f(0.0f, 1.0f, 0.0f);
            // This is the amount of rotation
            float    tiltAngle = yUp.Angle(accWMean);
            // Record values if the tilt error is intolerable
            if (tiltAngle > maxTiltError) 
            {
                TiltErrorAngle = tiltAngle;
                TiltErrorAxis = tiltAxis;
            }
        }

        // This part performs the actual tilt correction as needed
        if (TiltErrorAngle > minTiltError) 
        {
            if ((TiltErrorAngle > 0.4f)&&(Stage < 2000))
            {   // Tilt completely to correct orientation
                Q = Quatf(TiltErrorAxis, -TiltErrorAngle) * Q;
                TiltErrorAngle = 0.0f;
            }
            else 
            {
                //LogText("Performing tilt correction  -  Angle: %f   Axis: %f %f %f\n",
                //        TiltErrorAngle,TiltErrorAxis.x,TiltErrorAxis.y,TiltErrorAxis.z);
                //float deltaTiltAngle = -Gain*TiltErrorAngle*0.005f;
                // This uses agressive correction steps while your head is moving fast
                float deltaTiltAngle = -Gain*TiltErrorAngle*0.005f*(5.0f*angVelLength+1.0f);
                // Incrementally "untilt" by a small step size
                Q = Quatf(TiltErrorAxis, deltaTiltAngle) * Q;
                TiltErrorAngle += deltaTiltAngle;
            }
        }
    }

    // Yaw drift correction based on magnetometer data.  This corrects the part of the drift
    // that the accelerometer cannot handle.
    // This will only work if the magnetometer has been enabled, calibrated, and a reference
    // point has been set.
    const float maxAngVelLength = 3.0f;
    const int   magWindow = 5;
    const float yawErrorMax = 0.1f;
    const float yawErrorMin = 0.01f;
    const int   yawErrorCountLimit = 50;
    const float yawRotationStep = 0.00002f;

    if (angVelLength < maxAngVelLength)
        MagCondCount++;
    else
        MagCondCount = 0;

    if (EnableYawCorrection && MagReady && (currentTime > 2.0f) && (MagCondCount >= magWindow) &&
        (Q.Distance(MagRefQ) < MagRefDistance))
    {
        // Use rotational invariance to bring reference mag value into global frame
        Vector3f grefmag = MagRefQ.Rotate(MagRefM);
        // Bring current (averaged) mag reading into global frame
        Vector3f gmag = Q.Rotate(FMag.Mean());
        // Calculate the reference yaw in the global frame
        float gryaw = atan2(grefmag.x,grefmag.z);
        // Calculate the current yaw in the global frame
        float gyaw = atan2(gmag.x,gmag.z);
        //LogText("Yaw error estimate: %f\n",YawErrorAngle);
        // The difference between reference and current yaws is the perceived error
        YawErrorAngle = AngleDifference(gyaw,gryaw);
        // If the perceived error is large, keep count
        if ((fabs(YawErrorAngle) > yawErrorMax) && (!YawCorrectionInProgress))
            YawErrorCount++;
        // After enough iterations of high perceived error, start the correction process
        if (YawErrorCount > yawErrorCountLimit)
            YawCorrectionInProgress = true;
        // If the perceived error becomes small, turn off the yaw correction
        if ((fabs(YawErrorAngle) < yawErrorMin) && YawCorrectionInProgress) 
        {
            YawCorrectionInProgress = false;
            YawErrorCount = 0;
        }
        // Perform the actual yaw correction, due to previously detected, large yaw error
        if (YawCorrectionInProgress) 
        {
            int sign = (YawErrorAngle > 0.0f) ? 1 : -1;
            // Incrementally "unyaw" by a small step size
            Q = Quatf(Vector3f(0.0f,1.0f,0.0f), -yawRotationStep * sign) * Q;
        }
    }
}


void SensorFusion::SetMagReference(const Quatf& q) 
{
        MagRefQ = q;
        MagRefM = FMag.Mean();

        float pitch, roll, yaw;
        Q.GetEulerAngles<Axis_X, Axis_Z, Axis_Y>(&pitch, &roll, &yaw);
        MagRefYaw = yaw;
        if (MagCalibrated)
            MagReady = true;
}


float SensorFusion::AngleDifference(float theta1, float theta2) 
{
    float x = theta1 - theta2;
    if (x > Math<float>::Pi)
        return x - Math<float>::TwoPi;
    if (x < -Math<float>::Pi)
        return x + Math<float>::TwoPi;
    return x;
}


SensorFusion::BodyFrameHandler::~BodyFrameHandler()
{
    RemoveHandlerFromDevices();
}

void SensorFusion::BodyFrameHandler::OnMessage(const Message& msg)
{
    if (msg.Type == Message_BodyFrame)
        pFusion->handleMessage(static_cast<const MessageBodyFrame&>(msg));
    if (pFusion->pDelegate)
        pFusion->pDelegate->OnMessage(msg);
}

bool SensorFusion::BodyFrameHandler::SupportsMessageType(MessageType type) const
{
    return (type == Message_BodyFrame);
}


} // namespace OVR

