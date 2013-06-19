//
//  UVCCameraControl.hpp
//  UVCCameraControl
//
//  Created by Philip Rosedale on 6/18/13.
//  Copyright (c) 2013 Phobos Lab. All rights reserved.
//

#ifndef UVCCameraControl_UVCCameraControl_hpp
#define UVCCameraControl_UVCCameraControl_hpp

#ifdef __cplusplus
extern "C" {
#endif
    
void configureCamera(int vendorID, int productID, int autoExposure, float exposure, float contrast, float saturation,
    float sharpness, int autoWhiteBalance, float whiteBalance);
    
#ifdef __cplusplus
}
#endif

#endif
