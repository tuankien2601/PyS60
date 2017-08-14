/**
 * ====================================================================
 * glesmodule.h
 *
 * Copyright (c) 2006 Nokia Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __GLES_H
#define __GLES_H

#include "Python.h"

// For RDebug messages
#include <eikenv.h>

// For SPy(Add|Get)GlobalString
#include "symbian_python_ext_util.h"

#include <GLES/gl.h>
#include <GLES/egl.h>

// comment this out to disable RDebug messages
#define DEBUG_GLES

// Older SDK versions may include a 1.1 header file while the actual implementation is 1.0
#if SERIES60_VERSION<30
  #undef GL_OES_VERSION_1_1
#endif

#define RETURN_PYNONE do { Py_INCREF(Py_None); return Py_None; } while(0)
#define RETURN_PYTRUE do { Py_INCREF(Py_True); return Py_True; } while(0)
#define RETURN_PYFALSE do { Py_INCREF(Py_False); return Py_False; } while(0)

#define RETURN_IF_GLERROR do { if(gles_check_error() != 0) { return NULL; } } while(0)

#define GLES_ARRAY_TYPE ((PyTypeObject*)SPyGetGlobalString("GLESArrayType"))
#define IMAGE_TYPE ((PyTypeObject*)SPyGetGlobalString("ImageType"))

#define DEBUGMSG(msg) do { RDebug::Print(_L(msg)); } while(0)
#define DEBUGMSG1(msg,arg1) do { RDebug::Print(_L(msg),arg1); } while(0)
#define DEBUGMSG2(msg,arg1,arg2) do { RDebug::Print(_L(msg),arg1,arg2); } while(0)
#define DEBUGMSG3(msg,arg1,arg2,arg3) do { RDebug::Print(_L(msg),arg1,arg2,arg3); } while(0)
#define DEBUGMSG4(msg,arg1,arg2,arg3,arg4) do { RDebug::Print(_L(msg),arg1,arg2,arg3,arg4); } while(0)

// Structure for an array pointer
struct gles_array_t {
  void* ptr; // A pointer to the vertex/etc. data
  PyObject *obj; // Associated Python object (can be NULL).
};

// Structure for array storage in TLS
struct GLES_Arrays {
  gles_array_t color;     // Pointer for color array
  gles_array_t normal;    // Pointer for normal array
  gles_array_t texcoord;  // Pointer for texture coordinate array
  gles_array_t vertex;    // Pointer for vertex array
  gles_array_t matrix;    // Pointer for matrix index array
  gles_array_t pointsize; // Pointer for point size array
  gles_array_t weight;    // Pointer for weight array
};

// Definition for gles.array object
struct array_object {
  PyObject_VAR_HEAD
  // Array object specific fields below.
  void *arrdata; // A pointer to the raw data.
  signed int len; // How many elements in the array.
  GLenum arrtype; // Type of array.
  // Length of a single element (needed for glVertexPointer etc.)
  signed int dimension;
  // Lenght of an item in bytes
  signed int item_size;
  // Lenght of the data in bytes
  signed int real_len;
};

extern "C" PyObject *new_array_object(PyObject */*self*/, PyObject *args);
/*extern "C" static void array_dealloc(array_object *op);
extern "C" static PyObject *array_getattr(array_object *op, char *name);
extern "C" static int array_setattr(array_object *op, char *name, PyObject *v);*/

// Prototypes for functions in gles_util.cpp
void *gles_convert_fbsbitmap(CFbsBitmap *bitmap, GLenum format, GLenum type, unsigned int *datalen);
CFbsBitmap *Bitmap_AsFbsBitmap(PyObject *obj);
PyObject *PyCAPI_GetCAPI(PyObject *object, const char *apiname);

int gles_PySequence_Dimension(PyObject *seq);
PyObject *gles_PySequence_Collapse(GLenum type, PyObject *source, PyObject *target);
void *gles_assign_array(GLenum targetarr, void *ptr, array_object *object);

GLbyte   *gles_PySequence_AsGLbyteArray(PyObject *seq);
GLubyte  *gles_PySequence_AsGLubyteArray(PyObject *seq);
GLfloat  *gles_PySequence_AsGLfloatArray(PyObject *seq);
GLshort  *gles_PySequence_AsGLshortArray(PyObject *seq);
GLushort *gles_PySequence_AsGLushortArray(PyObject *seq);
GLint    *gles_PySequence_AsGLintArray(PyObject *seq);
GLuint   *gles_PySequence_AsGLuintArray(PyObject *seq);
GLfixed  *gles_PySequence_AsGLfixedArray(PyObject *seq);

void *gles_alloc(size_t size);
void gles_free(void *ptr);

void gles_free_array(GLenum arrtype);
void gles_init_arrays();
void gles_uninit_arrays();
unsigned int gles_check_error();
  
#endif // __GLES_H
