//
//  markers.cpp
//  interface
//
//  Created by Kenneth Keiter <ken@kenkeiter.com> on 12/11/12.
//  Copyright (c) 2012 Rosedale Lab. All rights reserved.
//

#include "markers.h"
#include <vector.h>
#include <time.h>
#include <math.h>
#include <GLUT/glut.h>
#include <opencv2/opencv.hpp>
#include <CVBlob/blob.h>
#include <CVBlob/BlobResult.h>


MarkerCapture::MarkerCapture(int source_index){
    capture_source_index = source_index;
    capture_ready = false;
    if (pthread_mutex_init(&frame_mutex, NULL) != 0){
        printf("Frame lock mutext init failed. Exiting.\n");
        exit(-1);
    }
}

MarkerCapture::~MarkerCapture(){
    pthread_mutex_destroy(&frame_mutex);
}

/* Begin capture of camera data. Should be called exactly once. */
int MarkerCapture::init_capture(){
    camera = cvCaptureFromCAM(capture_source_index);
    if(!camera){
        return -1;
    }else{
        time = clock();
        capture_ready = true;
        return 0;
    }
}

/* Begin tracking targets of a given CvScalar, color. */
void MarkerCapture::acquire_color(CvScalar color){
    target_color = color_to_range(color, CV_COLOR_TOLERANCE);
    color_acquired = true;
}

/* Fetch a frame (if available) and process it, calling appropriate 
 callbacks when data becomes available. */
void MarkerCapture::tick(){
    IplImage *thresh_frame = NULL;
    CBlobResult blobs;
    
    // Acquire the lock, update the current frame.
    pthread_mutex_lock(&frame_mutex);
    current_frame = cvCloneImage(cvQueryFrame(camera));
    if(color_acquired && current_frame){
        thresh_frame = apply_threshold(current_frame, target_color);
    }else{
        // create a suplicant.
        thresh_frame = cvCreateImage(cvGetSize(current_frame),IPL_DEPTH_8U,1);
    }
    pthread_mutex_unlock(&frame_mutex);
    // Lock released. Done messing with buffers.
    
    if(frame_update_callback){
        (*frame_update_callback)(this, current_frame, thresh_frame);
    }
    if(color_acquired){
        blobs = detect_blobs(thresh_frame, CV_BLOB_SIZE_MIN);
        if(blobs.GetNumBlobs() >= 2){ // need 2 or more blobs for positional fix.
            MarkerPositionEstimate position;
            // fetch the two largest blobs, by area.
            CBlob blob0, blob1;
            blobs.GetNthBlob(CBlobGetArea(), 0, blob0);
            blobs.GetNthBlob(CBlobGetArea(), 1, blob1);
            // perform positional calculations
            position.distance = distance(blob0, blob1);
            position.angle = angle(blob0, blob1);
            position.blob0_center = blob_center(blob0);
            position.blob1_center = blob_center(blob1);
            // call the update handler.
            if(position_update_callback){
                (*position_update_callback)(this, position);
            }
        }
        blobs.ClearBlobs();
    }
    
    pthread_mutex_lock(&frame_mutex);
    cvReleaseImage(&current_frame);
    cvReleaseImage(&thresh_frame);
    pthread_mutex_unlock(&frame_mutex);
    
    int curr_time = clock();
    fps = CLOCKS_PER_SEC/(double)(curr_time - time);
    time = curr_time;
}

/* Provide a callback to handle new frames. */
void MarkerCapture::frame_updated(void (*callback)(MarkerCapture* inst, IplImage* image, IplImage* thresh_image)){
    frame_update_callback = callback;
}

/* Provide a callback to handle marker position updates. */
void MarkerCapture::position_updated(void (*callback)(MarkerCapture* inst, MarkerPositionEstimate position)){
    position_update_callback = callback;
}

void MarkerCapture::end_capture(){
    
}

/* Convert an IplImage to an OpenGL texture */
void MarkerCapture::ipl_to_texture(IplImage *img, GLuint *t){
    glGenTextures(1, t);
    glBindTexture(GL_TEXTURE_2D, *t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    pthread_mutex_lock(&frame_mutex);
    char* buffer = IplImage_to_buffer(img);
    pthread_mutex_unlock(&frame_mutex);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->width, img->height, 0, GL_BGR, GL_UNSIGNED_BYTE, buffer);
    delete[] buffer;
}

/* Given an IplImage, draw it using glDrawPixels. */
void MarkerCapture::glDrawIplImage(IplImage *img, int x = 0, int y = 0, GLfloat xZoom = 1.0, GLfloat yZoom = 1.0){
    GLint format, type;
    switch(img->nChannels){
        case 1:
            format = GL_LUMINANCE;
            break;
        case 3:
            format = GL_BGR;
            break;
        case 4:
            format = GL_BGRA;
        default:
            format = GL_BGR;
    }
    switch(img->depth){
        case IPL_DEPTH_8U:
            type = GL_UNSIGNED_BYTE;
            break;
        case IPL_DEPTH_8S:
            type = GL_BYTE;
            break;
        case IPL_DEPTH_16U:
            type = GL_UNSIGNED_SHORT;
            break;
        case IPL_DEPTH_16S:
            type = GL_SHORT;
            break;
        case IPL_DEPTH_32S:
            type = GL_INT;
            break;
        case IPL_DEPTH_32F:
            type = GL_FLOAT;
            break;
        case IPL_DEPTH_64F:
            /* not supported by opengl */
        default:
           type = GL_UNSIGNED_BYTE; 
    }
    glRasterPos2i(x, y);
    glPixelZoom(xZoom, yZoom);
    
    // Acquire frame lock while we copy the buffer.
    char* buffer = IplImage_to_buffer(img);
    
    glDrawPixels(img->width, img->height, format, type, buffer);
    delete[] buffer;
}

/*******
 PRIVATE
 *******/

MarkerColorRange MarkerCapture::color_to_range(CvScalar color, int tolerance){
    MarkerColorRange range;
    range.base = color;
    range.tolerance = tolerance;
    float r, g, b;
    // note that color min is clamped to 0 if v - tolerance <= 0;
    range.from = cvScalar(((b = color.val[0] - tolerance) >= 0) ? b : 0,
                          ((g = color.val[1] - tolerance) >= 0) ? g : 0,
                          ((r = color.val[2] - tolerance) >= 0) ? r : 0);
    
    // note that color max are clamped to 0 if v - tolerance <= 0;
    range.to = cvScalar(((b = color.val[0] + tolerance) <= 255) ? b : 255,
                        ((g = color.val[1] + tolerance) <= 255) ? g : 255,
                        ((r = color.val[2] + tolerance) <= 255) ? r : 255);
    
    return range;
}

/* Find the center of a given blob. */
CvPoint MarkerCapture::blob_center(CBlob blob){
    CvPoint point;
    point.x = blob.GetBoundingBox().x + (blob.GetBoundingBox().width / 2);
    point.y = blob.GetBoundingBox().y + (blob.GetBoundingBox().height / 2);
    return point;
}

/* Convert an IplImage to an allocated buffer, removing any byte alignment. */
char* MarkerCapture::IplImage_to_buffer(IplImage *img){
    char* buffer = new char[img->width*img->height*img->nChannels];
    char * data  = (char *)img->imageData;
    for(int i=0; i<img->height; i++){
        memcpy(&buffer[i*img->width*img->nChannels], &(data[i*img->widthStep]), img->width*img->nChannels);
    }
    return buffer;
}

/* Given an IplImage, convert it to HSV, and select only colors within the proper range. */
IplImage* MarkerCapture::apply_threshold(IplImage* img, MarkerColorRange color_range){
    // convert to HSV
    // IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
    // cvCvtColor(img, imgHSV, CV_BGR2HSV);
    
    IplImage* imgThresh = cvCreateImage(cvGetSize(img), 8, 1);
    cvInRangeS(img, color_range.from, color_range.to, imgThresh); // match the color range.
    
    return imgThresh;
}

/* Detect blobs larger than min_size in a given IplImage. */
CBlobResult MarkerCapture::detect_blobs(IplImage *img, int min_size = 10){
    // find white blobs in thresholded image
    CBlobResult blobs = CBlobResult(img, NULL, 0);
    // exclude ones smaller than min_size.
    blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, min_size);
    return blobs;
}

double MarkerCapture::distance(CBlob blob1, CBlob blob2){
    // calculate centers, return the distance between them.
    CvPoint blob1_p = blob_center(blob1);
    CvPoint blob2_p = blob_center(blob2);
    return sqrt(pow(abs(blob2_p.x - blob1_p.x), 2) + pow(abs(blob2_p.y - blob1_p.y), 2));
}

/* Get the angle, in degrees, between two blobs */
double MarkerCapture::angle(CBlob blob1, CBlob blob2){
    CvPoint blob1_p = blob_center(blob1);
    CvPoint blob2_p = blob_center(blob2);
    double delta_x = fabs(blob2_p.x - blob1_p.x);
    double delta_y = fabs(blob2_p.y - blob1_p.y);
    double abs_angle = atan2(delta_y, delta_x) * 180 / M_PI;
    if(blob1_p.x < blob2_p.x && blob1_p.y > blob2_p.y){ // blob1 is above and to the left of blob2
        return abs_angle * -1.0;
    }else{
        return abs_angle;
    }
}