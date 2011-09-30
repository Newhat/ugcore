/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class edu_gcsc_vrl_ug_UG */

#ifndef _Included_edu_gcsc_vrl_ug_UG
#define _Included_edu_gcsc_vrl_ug_UG
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    convertRegistryInfo
 * Signature: ()Ledu/gcsc/vrl/ug/NativeAPIInfo;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG_convertRegistryInfo
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    invokeMethod
 * Signature: (Ljava/lang/String;JZLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG_invokeMethod
  (JNIEnv *, jobject, jstring, jlong, jboolean, jstring, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    newInstance
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_edu_gcsc_vrl_ug_UG_newInstance__J
  (JNIEnv *, jobject, jlong);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    newInstance
 * Signature: (J[Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_edu_gcsc_vrl_ug_UG_newInstance__J_3Ljava_lang_Object_2
  (JNIEnv *, jobject, jlong, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    getExportedClassPtrByName
 * Signature: (Ljava/lang/String;Z)J
 */
JNIEXPORT jlong JNICALL Java_edu_gcsc_vrl_ug_UG_getExportedClassPtrByName
  (JNIEnv *, jobject, jstring, jboolean);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    getDefaultClassNameFromGroup
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getDefaultClassNameFromGroup
  (JNIEnv *, jobject, jstring);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    invokeFunction
 * Signature: (Ljava/lang/String;Z[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG_invokeFunction
  (JNIEnv *, jobject, jstring, jboolean, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    ugInit
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_edu_gcsc_vrl_ug_UG_ugInit
  (JNIEnv *, jclass, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    getSvnRevision
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getSvnRevision
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    getDescription
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getDescription
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    getAuthors
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getAuthors
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    getCompileDate
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getCompileDate
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
/* Header for class edu_gcsc_vrl_ug_UG_MessageThread */

#ifndef _Included_edu_gcsc_vrl_ug_UG_MessageThread
#define _Included_edu_gcsc_vrl_ug_UG_MessageThread
#ifdef __cplusplus
extern "C" {
#endif
#undef edu_gcsc_vrl_ug_UG_MessageThread_MIN_PRIORITY
#define edu_gcsc_vrl_ug_UG_MessageThread_MIN_PRIORITY 1L
#undef edu_gcsc_vrl_ug_UG_MessageThread_NORM_PRIORITY
#define edu_gcsc_vrl_ug_UG_MessageThread_NORM_PRIORITY 5L
#undef edu_gcsc_vrl_ug_UG_MessageThread_MAX_PRIORITY
#define edu_gcsc_vrl_ug_UG_MessageThread_MAX_PRIORITY 10L
#ifdef __cplusplus
}
#endif
#endif
/* Header for class edu_gcsc_vrl_ug_MemoryManager */

#ifndef _Included_edu_gcsc_vrl_ug_MemoryManager
#define _Included_edu_gcsc_vrl_ug_MemoryManager
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     edu_gcsc_vrl_ug_MemoryManager
 * Method:    delete
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_MemoryManager_delete
  (JNIEnv *, jclass, jlong, jlong);

/*
 * Class:     edu_gcsc_vrl_ug_MemoryManager
 * Method:    invalidate
 * Signature: (Ledu/gcsc/vrl/ug/SmartPointer;)V
 */
JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_MemoryManager_invalidate
  (JNIEnv *, jclass, jobject);

#ifdef __cplusplus
}
#endif
#endif
