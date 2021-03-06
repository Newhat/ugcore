/*
 * Copyright (c) 2010-2014:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Author: Michael Hoffer
 * 
 * This file is part of UG4.
 * 
 * UG4 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (2) The following notice must be displayed at a prominent place in the
 * terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (3) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
 *   parallel geometric multigrid solver on hierarchically distributed grids.
 *   Computing and visualization in science 16, 4 (2013), 151-164"
 * "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
 *   flexible software system for simulating pde based models on high performance
 *   computers. Computing and visualization in science 16, 4 (2013), 165-179"
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#ifndef TYPE_CONVERTER_H
#define	TYPE_CONVERTER_H

#include <jni.h>
#include <string>
#include <vector>
#include "registry/class.h"
#include "registry/registry.h"
#include "messaging.h"


// we are not sure whether NULL is equivalent to null object in Java.
// thus, we use this define that allows us to change that easily in the future
#define JNULL NULL;

namespace ug {
namespace vrl {

//	added by Christian Poliwoda
//	christian.poliwoda@gcsc.uni-frankfurt.de
//	y 13 m 10 d 31
/**
 * Stores the information about a java parameter/object:
 * - the type
 * - and if it is an array
 */
struct TypeAndArray {
	ug::Variant::Type type;
	bool isArray;
};

/**
 * Converts a native string to a Java string.
 * @param env JVM environment to operate on
 * @param s string to convert
 * @return a java string
 */
jstring stringC2J(JNIEnv *env, const char* s);

/**
 * <p>
 * Converts a Java string to a native string.
 * </p>
 * <p>
 * <b>Note:</b> this function must not be used to convert large amounts
 * of strings because of inefficient memory handling.
 * </p>
 * @param env JVM environment to operate on
 * @param s string to convert
 * @return a native string
 */
std::string stringJ2C(JNIEnv *env, jstring const& s);

/**
 * Converts a native string array to a Java object array.
 * @param env JVM environment to operate on
 * @param strings array to convert
 * @return a java object array
 */
jobjectArray stringArrayC2J(JNIEnv *env, const std::string* strings);

/**
 * Converts a native string array to a java object array.
 * @param env JVM environment to operate on
 * @param strings array to convert
 * @return a java object array
 */
jobjectArray stringArrayC2J(
		JNIEnv *env,
		std::vector<std::string> const& strings);

/**
 * <p>
 * Converts a Java string array to a native string array.
 * </p>
 * <p>
 * <b>Note:</b> this function must not be used to convert large amounts
 * of strings because of inefficient memory handling.
 * </p>
 * @param env JVM environment to operate on
 * @param array array to convert
 * @return a native string array
 */
std::vector<std::string> stringArrayJ2C(
		JNIEnv *env, jobjectArray const& array);

/**
 * Converts a Java boolean to a native boolean.
 * @param b Java boolean to convert
 * @return native boolean
 */
inline bool boolJ2C(jboolean b) {
	return b == JNI_TRUE;
}

/**
 * Converts a native boolean to a Java boolean.
 * @param b native boolean to convert
 * @return Java boolean
 */
inline jboolean boolC2J(bool b) {
	return b ? JNI_TRUE : JNI_FALSE;
}

/**
 * Converts a jboolean to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return Java object (jobject)
 */
jobject boolean2JObject(JNIEnv *env, jboolean value);

/**
 * Converts a Java object to a native boolean value.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native boolean value
 */
jboolean jObject2Boolean(JNIEnv *env, jobject obj);

/**
 * Converts an integer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return Java object (jobject)
 */
jobject int2JObject(JNIEnv *env, jint value);

/**
 * Converts a Java object (jobject) to a native int value.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native int value
 */
jint jObject2Int(JNIEnv *env, jobject obj);

/**
 * Converts a double to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return a Java object (jobject)
 */
jobject double2JObject(JNIEnv *env, jdouble value);

/**
 * Converts a Java object (jobject) to a native double value.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native double value
 */
jdouble jObject2Double(JNIEnv *env, jobject obj);

/**
 * Converts a string to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return a Java object (jobject)
 */
jobject string2JObject(JNIEnv *env, const char* value);

/**
 * Converts a Java object (jobject) to a native string.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native string
 */
std::string jObject2String(JNIEnv *env, jobject obj);

/**
 * Converts a native pointer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value pointer to convert
 * @return a Java object (jobject)
 */
jobject pointer2JObject(JNIEnv *env, void* value);

/**
 * Converts a native const pointer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value const pointer to convert
 * @return a Java object (jobject)
 */
jobject constPointer2JObject(JNIEnv *env, const void* value);

/**
 * Converts a native smart-pointer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value smart-pointer to convert
 * @return a Java object (jobject)
 */
jobject smartPointer2JObject(JNIEnv *env, SmartPtr<void> value);

/**
 * Converts a native const smart-pointer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value const smart-pointer to convert
 * @return a Java object (jobject)
 */
jobject constSmartPointer2JObject(JNIEnv *env, ConstSmartPtr<void> value);

/**
 * Indicates whether the specified smart pointer is const, i.e., if the
 * specified Java object represents a const smart pointer.
 * @param env JVM environment to operate on
 * @param ptr smart pointer to check
 * @return <code>true</code> if the specified smart pointer is const;
 *         <code>false</code> otherwise
 */
bool isJSmartPointerConst(JNIEnv *env, jobject ptr);

/**
 * Invalidates the native equivalent of the specified Java smart pointer.
 * TODO what about error handling???
 * @param env JVM environment to operate on
 * @param obj smart pointer to invalidate
 */
void invalidateJSmartPointer(JNIEnv *env, jobject obj);

/**
 * Invalidates the native equivalent of the specified Java smart pointer.
 * TODO what about error handling???
 * @param env JVM environment to operate on
 * @param obj smart pointer to invalidate
 */
void invalidateJConstSmartPointer(JNIEnv *env, jobject obj);

/**
 *	Finds out whether a java pointer wrapper is const.
 * @param env	JVM environment to operate on
 * @param obj	pointer type to check for constness
 * @return	true iff pointer wrapper's readOnly member is true
 */
bool isConstJPtr(JNIEnv* env, jobject obj);

/**
 * Converts a Java object (jobject) to a native pointer.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native pointer
 */
void* jObject2Pointer(JNIEnv *env, jobject obj);

/**
 * Pendant of javas getName().
 * @param env JVM environment to operate on
 * @param obj object which class name we want to know
 * @return the class name of the jobject
 */
std::string jPointerGetName(JNIEnv *env, jobject obj);


//	added by Christian Poliwoda
//	christian.poliwoda@gcsc.uni-frankfurt.de
//	y 13 m 05 d 28
bool isJObjectAnArray(JNIEnv *env, jobject value);

void jObject2BoolVector(JNIEnv *env, jobject object, std::vector<bool>& bv);
void jObject2IntVector(JNIEnv *env, jobject object, std::vector<int>& iv);
void jObject2SizetVector(JNIEnv *env, jobject object, std::vector<size_t>& stv);
void jObject2NumberVector(JNIEnv *env, jobject object, std::vector<number>& nv);
void jObject2stdStringVector(JNIEnv *env, jobject object, std::vector<std::string>& sv);
void jObject2PtrVector
(
	JNIEnv *env,
	jobject object,
	ug::Variant::Type jo_type,
	ug::bridge::Registry* reg,
	std::vector<std::pair<void*, const ug::bridge::ClassNameNode*> >& pv
);
void jObject2SmartPtrVector
(
	JNIEnv *env,
	jobject object,
	ug::bridge::Registry* reg,
	std::vector<std::pair<SmartPtr<void>, const ug::bridge::ClassNameNode*> >& pv
);
void jObject2ConstSmartPtrVector
(
	JNIEnv *env,
	jobject object,
	ug::Variant::Type jo_type,
	ug::bridge::Registry* reg,
	std::vector<std::pair<ConstSmartPtr<void>, const ug::bridge::ClassNameNode*> >& pv
);

/**
 * Converts a bool vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param bv vector to convert
 * @return Java object (jobject)
 */
jobject boolVector2JObject(JNIEnv* env, const std::vector<bool>& bv);

/**
 * Converts an int vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param iv vector to convert
 * @return Java object (jobject)
 */
jobject intVector2JObject(JNIEnv* env, const std::vector<int>& iv);

/**
 * Converts a size_t vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param iv vector to convert
 * @return Java object (jobject)
 */
jobject sizetVector2JObject(JNIEnv* env, const std::vector<size_t>& iv);

/**
 * Converts a number vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param nv vector to convert
 * @return Java object (jobject)
 */
jobject numberVector2JObject(JNIEnv* env, const std::vector<number>& nv);

/**
 * Converts a c-string vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param sv vector to convert
 * @return Java object (jobject)
 */
jobject cStringVector2JObject(JNIEnv* env, const std::vector<const char*>& sv);

/**
 * Converts a std::string vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param sv vector to convert
 * @return Java object (jobject)
 */
jobject stdStringVector2JObject(JNIEnv* env, const std::vector<std::string>& sv);

/**
 * Converts a void-pointer vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param pv vector to convert
 * @return Java object (jobject)
 */
jobject ptrVector2JObject(JNIEnv* env, const std::vector<void*>& pv);

/**
 * Converts a const-void-pointer vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param pv vector to convert
 * @return Java object (jobject)
 */
jobject constPtrVector2JObject(JNIEnv* env, const std::vector<const void*>& pv);

/**
 * Converts a SmartPtr vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param pv vector to convert
 * @return Java object (jobject)
 */
jobject smartPtrVector2JObject(JNIEnv* env, const std::vector<SmartPtr<void> >& pv);

/**
 * Converts a ConstSmartPtr vector to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param pv vector to convert
 * @return Java object (jobject)
 */
jobject constSmartPtrVector2JObject(JNIEnv* env, const std::vector<ConstSmartPtr<void> >& pv);

/**
 * Converts an array of Java objects to a parameter stack.
 * @param env JVM environment to operate on
 * @param reg ug registry
 * @param paramsOut converted parameter stack (return value)
 * @param paramsTemplate template parameter stack used to get correct
 *                       parameter type
 * @param array object array to convert
 */
void jobjectArray2ParamStack(JNIEnv *env,
		ug::bridge::Registry* reg,
		ug::bridge::ParameterStack& paramsOut,
		const ug::bridge::ParameterInfo& paramsTemplate,
		jobjectArray const& array);

/**
 * Creates an empty Java array using the specified class as element
 * type.
 * @param env JVM environment to operate on
 * @param className name of the element class
 * @return emtpy Java array
 */
jobjectArray createEmptyJavaArray(
		JNIEnv *env, std::string className);

/**
 * Creates an empty Java array using the specified class as element
 * type.
 * @param env JVM environment to operate on
 * @param elementClass element class
 * @return emtpy Java array
 */
jobjectArray createEmptyJavaArray(
		JNIEnv *env, jclass elementClass);

/**
 * Converts a parameter stack entry to a Java object.
 * @param env JVM environment to operate on
 * @param params parameter stack to convert
 * @param index index of the element to convert
 * @return a Java object (jobject)
 */
jobject param2JObject(JNIEnv *env,
		ug::bridge::ParameterStack& params, size_t index);

/**
 * Returns the class object of the specified Java object.
 * @param env JVM environment to operate on
 * @param obj Java object
 * @return class object of the specified Java object
 */
jobject getClass(JNIEnv *env, jobject obj);

/**
 * Returns the class name of the specified Java object.
 * @param env JVM environment to operate on
 * @param obj Java object
 * @return class name of the specified Java object
 */
std::string getClassName(JNIEnv *env, jobject obj);

/**
 * Returns the class name of the specified param object (class UGObject).
 * @param env JVM environment to operate on
 * @param obj param object
 * @return class name of the specified param object
 */
std::string getParamClassName(JNIEnv *env, jobject obj);

/**
 * Returns the parameter type (ug::bridge::ParameterTypes) of the
 * specified Java object.
 * @param env JVM environment to operate on
 * @param obj Java object
 * @return parameter type (ug::bridge::ParameterTypes) of the
 *         specified Java object
 */
TypeAndArray paramClass2ParamType(JNIEnv *env, jobject obj);

/**
 * Compares the parameter types of a Java object array and a
 * paramter stack. It ignores differences regarding constness.
 * This is checked by Java via interface types.
 * @param env JVM environment to operate on
 * @param params array of Java objects
 * @param reg ug registry
 * @param paramStack parameter stack
 * @return <code>true</code> if parameter types are equal;
 *         <code>false</code> otherwise
 */
bool compareParamTypes(JNIEnv *env, jobjectArray params,
        ug::bridge::Registry *reg,
		const ug::bridge::ParameterInfo& paramStack,
                bool allowSmartToRawPtrConversion = false);

/**
 * Returns parent classes (super classes) of an exported class.
 * @param reg registry to search
 * @param clazz exported class
 * @return a vector containing all parent classes of the given class
 */
const std::vector<const ug::bridge::IExportedClass*> getParentClasses(
		ug::bridge::Registry* reg,
		const ug::bridge::IExportedClass* clazz);


/**
 * Converts registry information to Java objects.
 * @param env JVM environment to operate on
 * @param reg registry to convert
 * @return native api info
 */
jobject registry2NativeAPI(JNIEnv *env, ug::bridge::Registry* reg);

/**
 * Returns the base classes of the given class name node.
 * @return the base classes of the given class name node
 */
std::vector<const char*> getBaseClassNames(const ug::bridge::ClassNameNode* node);

/**
 * Returns the the name of the specified parameter type.
 * @return the the name of the specified parameter type
 */
std::string getParamTypeAsString(const uint type);

/**
 * Returns the the names of the specified parameter types.
 * @return the the names of the specified parameter types
 */
std::string getParamTypesAsString(JNIEnv *env, jobjectArray const& array);

/**
 * Converts the specified ug error to its equivalent Java representation
 * and throws it as Java exception.
 * @param env JVM environment to operate on
 * @param error the error to convert/throw
 */
void throwUgErrorAsJavaException(JNIEnv *env, ug::UGError error);

/**
 * Converts the specified message to its equivalent Java representation
 * and throws it as Java exception.
 * @param env JVM environment to operate on
 * @param error the error to convert/throw
 */
void throwUgErrorAsJavaException(JNIEnv *env, std::string error);

}// end vrl::
}// end ug::

#endif	/* TYPE_CONVERTER_H */

