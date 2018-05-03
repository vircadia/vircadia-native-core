#if defined(Q_OS_ANDROID)

#include <QtAndroidExtras/QAndroidJniObject>
#include "AndroidHelper.h"

extern "C" {

JNIEXPORT void
Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeEnterBackground(JNIEnv *env, jobject obj) {
    if (qApp) {
        qApp->enterBackground();
    }
}

JNIEXPORT void
Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeEnterForeground(JNIEnv *env, jobject obj) {
    if (qApp) {
        qApp->enterForeground();
    }
}


}
#endif