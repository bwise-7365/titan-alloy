// Copyright Ben Paul Wise. All Rights Reserved.
//
// JNI shim for the flow planner. This is the ONLY file that includes <jni.h>;
// it marshals Java double[] arguments into the flat buffers expected by the
// pure C-ABI core (flowplan_solve) and hands back the result as a double[].
//
// The exact mangled symbol name below corresponds to an unpackaged Java class
// named FlowPlannerNative with `static native double[] solve(double[], double[],
// double[])`. Regenerate with `javac -h <dir> FlowPlannerNative.java` if the
// class is moved into a package or its signature changes.
//
#include <jni.h>

#include <vector>

#include "flowplan_ffi.h"

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_FlowPlannerNative_solve(JNIEnv *env, jclass /*cls*/,
                             jdoubleArray jsrc,
                             jdoubleArray jdst,
                             jdoubleArray jcost) {
    const jsize nSrc = env->GetArrayLength(jsrc);
    const jsize nDst = env->GetArrayLength(jdst);
    const jsize nCost = env->GetArrayLength(jcost);

    if (nSrc <= 0 || nDst <= 0 || nCost != nSrc * nDst) {
        jclass iae = env->FindClass("java/lang/IllegalArgumentException");
        if (iae != nullptr) {
            env->ThrowNew(iae,
                "cost.length must equal src.length * dst.length (both > 0)");
        }
        return nullptr;
    }

    jdouble *s = env->GetDoubleArrayElements(jsrc,  nullptr);
    jdouble *d = env->GetDoubleArrayElements(jdst,  nullptr);
    jdouble *c = env->GetDoubleArrayElements(jcost, nullptr);

    std::vector<double> out(static_cast<size_t>(nSrc) * nDst);

    // This is where it creates a FlowPlanner and solves the problem
    const int32_t rc = flowplan_solve(s, nSrc, d, nDst, c, out.data());

    // Inputs are not modified, so JNI_ABORT avoids a needless copy-back.
    env->ReleaseDoubleArrayElements(jsrc,  s, JNI_ABORT);
    env->ReleaseDoubleArrayElements(jdst,  d, JNI_ABORT);
    env->ReleaseDoubleArrayElements(jcost, c, JNI_ABORT);

    if (rc != 0) {
        jclass iae = env->FindClass("java/lang/IllegalArgumentException");
        if (iae != nullptr) {
            env->ThrowNew(iae, "flowplan_solve failed (invalid arguments)");
        }
        return nullptr;
    }

    jdoubleArray jflow = env->NewDoubleArray(static_cast<jsize>(out.size()));
    if (jflow == nullptr) {
        return nullptr; // OutOfMemoryError already pending
    }
    env->SetDoubleArrayRegion(jflow, 0,
                              static_cast<jsize>(out.size()), out.data());
    return jflow;
}
// Copyright Ben Paul Wise. All Rights Reserved.
