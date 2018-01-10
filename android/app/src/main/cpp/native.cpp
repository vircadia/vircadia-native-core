
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtAndroidExtras/QAndroidJniObject>

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

extern "C" {

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnCreate(JNIEnv* env, jobject obj, jobject instance, jobject asset_mgr) {
    qDebug() << "nativeOnCreate On thread " << QThread::currentThreadId();
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnPause(JNIEnv* env, jobject obj) {
    qDebug() << "nativeOnPause";
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnResume(JNIEnv* env, jobject obj) {
    qDebug() << "nativeOnResume";
}

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnExitVr(JNIEnv* env, jobject obj) {
    qDebug() << "nativeOnCreate On thread " << QThread::currentThreadId();
}

}
