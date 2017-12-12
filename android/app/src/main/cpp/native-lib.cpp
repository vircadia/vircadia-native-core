#include <jni.h>

#include <android/log.h>
#include <QtCore/QDebug>

#include "renderer.h"

int QtMsgTypeToAndroidPriority(QtMsgType type) {
    int priority = ANDROID_LOG_UNKNOWN;
    switch (type) {
        case QtDebugMsg: priority = ANDROID_LOG_DEBUG; break;
        case QtWarningMsg: priority = ANDROID_LOG_WARN; break;
        case QtCriticalMsg: priority = ANDROID_LOG_ERROR; break;
        case QtFatalMsg: priority = ANDROID_LOG_FATAL; break;
        case QtInfoMsg: priority = ANDROID_LOG_INFO; break;
        default: break;
    }
    return priority;
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    __android_log_write(QtMsgTypeToAndroidPriority(type), "Interface", message.toStdString().c_str());
}

static jlong toJni(NativeRenderer *renderer) {
    return reinterpret_cast<intptr_t>(renderer);
}

static NativeRenderer *fromJni(jlong renderer) {
    return reinterpret_cast<NativeRenderer*>(renderer);
}

#define JNI_METHOD(r, name) JNIEXPORT r JNICALL Java_org_saintandreas_testapp_MainActivity_##name

extern "C" {

JNI_METHOD(jlong, nativeCreateRenderer)
(JNIEnv *env, jclass clazz, jobject class_loader, jobject android_context, jlong native_gvr_api) {
    qInstallMessageHandler(messageHandler);
    return toJni(new NativeRenderer());
}

JNI_METHOD(void, nativeDestroyRenderer)
(JNIEnv *env, jclass clazz, jlong renderer) {
    delete fromJni(renderer);
}

JNI_METHOD(void, nativeInitializeGl)
(JNIEnv *env, jobject obj, jlong renderer) {
    fromJni(renderer)->InitializeGl();
}

JNI_METHOD(void, nativeDrawFrame)
(JNIEnv *env, jobject obj, jlong renderer) {
    fromJni(renderer)->DrawFrame();
}

JNI_METHOD(void, nativeOnTriggerEvent)
(JNIEnv *env, jobject obj, jlong renderer) {
    fromJni(renderer)->OnTriggerEvent();
}

JNI_METHOD(void, nativeOnPause)
(JNIEnv *env, jobject obj, jlong renderer) {
    fromJni(renderer)->OnPause();
}

JNI_METHOD(void, nativeOnResume)
(JNIEnv *env, jobject obj, jlong renderer) {
    fromJni(renderer)->OnResume();
}

}  // extern "C"
