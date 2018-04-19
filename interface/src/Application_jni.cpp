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
