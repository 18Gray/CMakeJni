#include <jni.h>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "com_example_lutao_cmakejni_MainActivity.h"


JNIEXPORT jstring JNICALL Java_com_example_lutao_cmakejni_MainActivity_stringFromJNI
        (JNIEnv *env, jobject){
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

struct NativeWorkerArgs
{
    jint id;
    jint iterations;
};

// Method ID can be cached
static jmethodID gOnNativeMessage = NULL;

// Java VM interface pointer
static JavaVM* gVm = NULL;

// Global reference to object
static jobject gObj = NULL;

// Mutex instance
static pthread_mutex_t mutex;

jint JNI_OnLoad (JavaVM* vm, void* reserved)
{
    // Cache the JavaVM interface pointer
    gVm = vm;

    return JNI_VERSION_1_4;
}


JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_MainActivity_nativeInit
        (JNIEnv *env, jobject obj)
{
    // Initialize mutex
    if (0 != pthread_mutex_init(&mutex, NULL))
    {
        // Get the exception class
        jclass exceptionClazz = env->FindClass(
                "java/lang/RuntimeException");

        // Throw exception
        env->ThrowNew(exceptionClazz, "Unable to initialize mutex");
        goto exit;
    }

    // If object global reference is not set
    if (NULL == gObj)
    {
        // Create a new global reference for the object
        gObj = env->NewGlobalRef(obj);

        if (NULL == gObj)
        {
            goto exit;
        }
    }

    // If method ID is not cached
    if (NULL == gOnNativeMessage)
    {
        // Get the class from the object
        jclass clazz = env->GetObjectClass(obj);

        // Get the method id for the callback
        gOnNativeMessage = env->GetMethodID(clazz,
                                            "onNativeMessage",
                                            "(Ljava/lang/String;)V");

        // If method could not be found
        if (NULL == gOnNativeMessage)
        {
            // Get the exception class
            jclass exceptionClazz = env->FindClass(
                    "java/lang/RuntimeException");

            // Throw exception
            env->ThrowNew(exceptionClazz, "Unable to find method");
        }
    }
    exit:
    return;
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_MainActivity_nativeFree
        (JNIEnv *env, jobject obj)
{
    // If object global reference is set
    if (NULL != gObj)
    {
        // Delete the global reference
        env->DeleteGlobalRef(gObj);
        gObj = NULL;
    }

    // Destory mutex
    if (0 != pthread_mutex_destroy(&mutex))
    {
        // Get the exception class
        jclass exceptionClazz = env->FindClass(
                "java/lang/RuntimeException");

        // Throw exception
        env->ThrowNew(exceptionClazz, "Unable to destroy mutex");
    }
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_MainActivity_nativeWorker
        (JNIEnv* env,
         jobject obj,
         jint id,
         jint iterations){
    // Lock mutex
    if (0 != pthread_mutex_lock(&mutex))
    {
        // Get the exception class
        jclass exceptionClazz = env->FindClass(
                "java/lang/RuntimeException");

        // Throw exception
        env->ThrowNew(exceptionClazz, "Unable to lock mutex");
        goto exit;
    }

    // Loop for given number of iterations
    for (jint i = 0; i < iterations; i++)
    {
        // Prepare message
        char message[26];
        sprintf(message, "Worker %d: Iteration %d", id, i);

        // Message from the C string
        jstring messageString = env->NewStringUTF(message);

        // Call the on native message method
        env->CallVoidMethod(obj, gOnNativeMessage, messageString);

        // Check if an exception occurred
        if (NULL != env->ExceptionOccurred())
            break;

        // Sleep for a second
        sleep(1);
    }

    // Unlock mutex
    if (0 != pthread_mutex_unlock(&mutex))
    {
        // Get the exception class
        jclass exceptionClazz = env->FindClass(
                "java/lang/RuntimeException");

        // Throw exception
        env->ThrowNew(exceptionClazz, "Unable to unlock mutex");
    }

    exit:
    return;
}


static void* nativeWorkerThread (void* args)
{
    JNIEnv* env = NULL;

    // Attach current thread to Java virtual machine
    // and obrain JNIEnv interface pointer
    if (0 == gVm->AttachCurrentThread(&env, NULL))
    {
        // Get the native worker thread arguments
        NativeWorkerArgs* nativeWorkerArgs = (NativeWorkerArgs*) args;

        // Run the native worker in thread context
        Java_com_example_lutao_cmakejni_MainActivity_nativeWorker(env,
                                                          gObj,
                                                          nativeWorkerArgs->id,
                                                          nativeWorkerArgs->iterations);

        // Free the native worker thread arguments
        delete nativeWorkerArgs;

        // Detach current thread from Java virtual machine
        gVm->DetachCurrentThread();
    }

    return (void*) 1;
}


JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_MainActivity_posixThreads
        (JNIEnv* env,
         jobject obj,
         jint threads,
         jint iterations){
    // Create a POSIX thread for each worker
    for (jint i = 0; i < threads; i++)
    {
        // Thread handle
        pthread_t thread;

        // Native worker thread arguments
        NativeWorkerArgs* nativeWorkerArgs = new NativeWorkerArgs();
        nativeWorkerArgs->id = i;
        nativeWorkerArgs->iterations = iterations;

        // Create a new thread
        int result = pthread_create(
                &thread,
                NULL,
                nativeWorkerThread,
                (void*) nativeWorkerArgs);

        if (0 != result)
        {
            // Get the exception class
            jclass exceptionClazz = env->FindClass(
                    "java/lang/RuntimeException");

            // Throw exception
            env->ThrowNew(exceptionClazz, "Unable to create thread");
        }
    }
}
