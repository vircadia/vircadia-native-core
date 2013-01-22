//
//  markers.h
//  interface
//
//  Created by Kenneth Keiter on 12/11/12.
//  Copyright (c) 2012 Rosedale Lab. All rights reserved.
//

#ifndef __interface__markers__
#define __interface__markers__

#include <iostream>
#include <vector.h>
#include <opencv2/opencv.hpp>
#include <GLUT/glut.h>
#include <CVBlob/blob.h>
#include <CVBlob/BlobResult.h>
#include <pthread.h>

#define CAMERA_INDEX CV_CAP_ANY
#define CV_BLOB_SIZE_MIN 40
#define CV_COLOR_TOLERANCE 30

typedef struct MarkerPositionEstimate{
    double distance;
    double angle;
    CvPoint blob0_center;
    CvPoint blob1_center;
} MarkerPositionEstimate;

typedef struct MarkerColorRange{
    CvScalar base; // Initial color
    CvScalar from; // Min color
    CvScalar to;   // Max color
    unsigned int tolerance;
} MarkerColorRange;

// Cute little lock/unlock macro.
#define synchronized(lock) \
for (pthread_mutex_t * i_ = &lock; i_; \
i_ = NULL, pthread_mutex_unlock(i_)) \
for (pthread_mutex_lock(i_); i_; i_ = NULL)

class MarkerCapture{
public:
    double fps;
    bool color_acquired = false;
    MarkerColorRange target_color;
    pthread_mutex_t frame_mutex;
    
    MarkerCapture(int source_index);
    ~MarkerCapture();
    
    int init_capture();
    void tick();
    void end_capture();
    
    void acquire_color(CvScalar color);
    void frame_updated(void (*callback)(MarkerCapture* inst, IplImage* image, IplImage* thresh_image));
    void position_updated(void (*callback)(MarkerCapture* inst, MarkerPositionEstimate position));
    void glDrawIplImage(IplImage *img, int x, int, GLfloat xZoom, GLfloat yZoom);
    void ipl_to_texture(IplImage *img, GLuint *t);
    
private:
    CvCapture* camera;
    IplImage* current_frame;
    int capture_source_index;
    bool capture_ready = false;
    int time;
    
    void (*position_update_callback)(MarkerCapture* inst, MarkerPositionEstimate position);
    void (*frame_update_callback)(MarkerCapture* inst, IplImage* image, IplImage* thresh_image);
    
    MarkerColorRange color_to_range(CvScalar color, int tolerance);
    char* IplImage_to_buffer(IplImage *img);
    IplImage* apply_threshold(IplImage* img, MarkerColorRange color_range);
    void IplDeinterlace(IplImage* src);
    CvPoint blob_center(CBlob blob);
    CBlobResult detect_blobs(IplImage* img, int min_size);
    double distance(CBlob blob1, CBlob blob2);
    double angle(CBlob blob1, CBlob blob2);
};

#endif /* defined(__interface__markers__) */
