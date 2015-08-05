//
//  3DConnexion.cpp
//  hifi
//
//  Created by MarcelEdward Verhagen on 09-06-15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef I3D_MOUSE_PARAMS_H
#define I3D_MOUSE_PARAMS_H

// Parameters for the 3D mouse based on the SDK from 3Dconnexion

class I3dMouseSensor {
public:
    enum Speed {
       SPEED_LOW = 0,
       SPEED_MID,
       SPEED_HIGH
    };

    virtual bool IsPanZoom() const = 0;
    virtual bool IsRotate() const  = 0;
    virtual Speed GetSpeed() const  = 0;

    virtual void SetPanZoom(bool isPanZoom) = 0;
    virtual void SetRotate(bool isRotate) = 0;
    virtual void SetSpeed(Speed speed) = 0;

protected:
    virtual ~I3dMouseSensor() {}
};

class I3dMouseNavigation {
public:
   enum Pivot {
      PIVOT_MANUAL = 0,
      PIVOT_AUTO,
      PIVOT_AUTO_OVERRIDE
   };

   enum Navigation {
      NAVIGATION_OBJECT_MODE = 0,
      NAVIGATION_CAMERA_MODE,
      NAVIGATION_FLY_MODE,
      NAVIGATION_WALK_MODE,
      NAVIGATION_HELICOPTER_MODE
   };

   enum PivotVisibility {
      PIVOT_HIDE = 0,
      PIVOT_SHOW,
      PIVOT_SHOW_MOVING
   };

    virtual Navigation GetNavigationMode() const  = 0;
    virtual Pivot GetPivotMode() const  = 0;
    virtual PivotVisibility GetPivotVisibility() const = 0;
    virtual bool IsLockHorizon() const = 0;

    virtual void SetLockHorizon(bool bOn) = 0;
    virtual void SetNavigationMode(Navigation navigation) = 0;
    virtual void SetPivotMode(Pivot pivot) = 0;
    virtual void SetPivotVisibility(PivotVisibility visibility) = 0;

protected:
    virtual ~I3dMouseNavigation(){}
};

class I3dMouseParam : public I3dMouseSensor, public I3dMouseNavigation {
public:
    virtual ~I3dMouseParam() {}
};

#endif
