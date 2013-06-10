//
//  ViewFrustum.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//
//  Simple view frustum class.
//
//

#ifndef __hifi__ViewFrustum__
#define __hifi__ViewFrustum__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Plane.h"
#include "AABox.h"

const float DEFAULT_KEYHOLE_RADIUS = 2.0f;

class ViewFrustum {
public:
    // setters for camera attributes
    void setPosition        (const glm::vec3& p) { _position = p; };
    void setOrientation     (const glm::quat& orientationAsQuaternion);

    // getters for camera attributes
    const glm::vec3& getPosition()    const { return _position;    };
    const glm::quat& getOrientation() const { return _orientation; };
    const glm::vec3& getDirection()   const { return _direction;   };
    const glm::vec3& getUp()          const { return _up;          };
    const glm::vec3& getRight()       const { return _right;       };

    // setters for lens attributes
    void setFieldOfView          ( float f )          { _fieldOfView          = f; };
    void setAspectRatio          ( float a )          { _aspectRatio          = a; };
    void setNearClip             ( float n )          { _nearClip             = n; };
    void setFarClip              ( float f )          { _farClip              = f; };
    void setEyeOffsetPosition    (const glm::vec3& p) { _eyeOffsetPosition    = p; };
    void setEyeOffsetOrientation (const glm::quat& o) { _eyeOffsetOrientation = o; };

    // getters for lens attributes
    float getFieldOfView()                     const { return _fieldOfView;         };
    float getAspectRatio()                     const { return _aspectRatio;         };
    float getNearClip()                        const { return _nearClip;            };
    float getFarClip()                         const { return _farClip;             };
    const glm::vec3& getEyeOffsetPosition()    const { return _eyeOffsetPosition;   };
    const glm::quat& getEyeOffsetOrientation() const { return _eyeOffsetOrientation;};

    const glm::vec3& getOffsetPosition()    const { return _offsetPosition; };
    const glm::vec3& getOffsetDirection()   const { return _offsetDirection;};
    const glm::vec3& getOffsetUp()          const { return _offsetUp;       };
    const glm::vec3& getOffsetRight()       const { return _offsetRight;    };

    const glm::vec3& getFarTopLeft()        const { return _farTopLeft;     };  
    const glm::vec3& getFarTopRight()       const { return _farTopRight;    };
    const glm::vec3& getFarBottomLeft()     const { return _farBottomLeft;  };
    const glm::vec3& getFarBottomRight()    const { return _farBottomRight; };

    const glm::vec3& getNearTopLeft()       const { return _nearTopLeft;    };  
    const glm::vec3& getNearTopRight()      const { return _nearTopRight;   };
    const glm::vec3& getNearBottomLeft()    const { return _nearBottomLeft; };
    const glm::vec3& getNearBottomRight()   const { return _nearBottomRight;};

    // get/set for keyhole attribute
    void  setKeyholeRadius(float keyholdRadius)       { _keyholeRadius = keyholdRadius; };
    float getKeyholeRadius()                    const { return _keyholeRadius;          };

    void calculate();

    ViewFrustum();

    typedef enum {OUTSIDE, INTERSECT, INSIDE} location;

    ViewFrustum::location pointInFrustum(const glm::vec3& point) const;
    ViewFrustum::location sphereInFrustum(const glm::vec3& center, float radius) const;
    ViewFrustum::location boxInFrustum(const AABox& box) const;
    
    // some frustum comparisons
    bool matches(const ViewFrustum& compareTo, bool debug = false) const;
    bool matches(const ViewFrustum* compareTo, bool debug = false) const { return matches(*compareTo, debug); };

    void computePickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const;

    void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& near, float& far,
                               glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;

    void printDebugDetails() const;
    
private:

    // Used for keyhole calculations
    ViewFrustum::location pointInSphere(const glm::vec3& point, const glm::vec3& center, float radius) const;
    ViewFrustum::location sphereInSphere(const glm::vec3& centerA, float radiusA, const glm::vec3& centerB, float radiusB) const;
    ViewFrustum::location boxInSphere(const AABox& box, const glm::vec3& center, float radius) const;
    
    // camera location/orientation attributes
    glm::vec3   _position;
    glm::quat   _orientation;
    
    // calculated for orientation
    glm::vec3   _direction;
    glm::vec3   _up;
    glm::vec3   _right;
    
    // Lens attributes
    float       _fieldOfView;
    float       _aspectRatio;
    float       _nearClip;
    float       _farClip;
    glm::vec3   _eyeOffsetPosition;
    glm::quat   _eyeOffsetOrientation;
    
    // keyhole attributes
    float       _keyholeRadius;

    
    // Calculated values
    glm::vec3   _offsetPosition;
    glm::vec3   _offsetDirection;
    glm::vec3   _offsetUp;
    glm::vec3   _offsetRight;
    glm::vec3   _farTopLeft;
    glm::vec3   _farTopRight;
    glm::vec3   _farBottomLeft;
    glm::vec3   _farBottomRight;
    glm::vec3   _nearTopLeft;
    glm::vec3   _nearTopRight;
    glm::vec3   _nearBottomLeft;
    glm::vec3   _nearBottomRight;
    enum { TOP_PLANE = 0, BOTTOM_PLANE, LEFT_PLANE, RIGHT_PLANE, NEAR_PLANE, FAR_PLANE };
    Plane _planes[6]; // How will this be used?
    
    const char* debugPlaneName (int plane) const;
    
};


#endif /* defined(__hifi__ViewFrustum__) */
