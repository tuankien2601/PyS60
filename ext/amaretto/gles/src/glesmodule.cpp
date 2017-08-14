/**
 * ====================================================================
 * glesmodule.cpp
 *
 * Copyright (c) 2006-2008 Nokia Corporation
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
#include "glesmodule.h"

extern "C" PyObject *new_array_object(PyObject */*self*/, PyObject *args)
{
  array_object *op = NULL;
  PyObject *sequence;
  void *arrdata = NULL;
  int type;
  int dimension;
  
#ifdef DEBUG_GLES
  DEBUGMSG("new_array_object()");
#endif
  if (!PyArg_ParseTuple(args, "iiO:array", &type, &dimension, &sequence)) {
    return NULL;
  }
  
  if (!PySequence_Check(sequence) ) {
    PyErr_SetString(PyExc_TypeError, "Expecting a sequence");
    return NULL;
  }
  
  op = PyObject_GC_New(array_object, GLES_ARRAY_TYPE);
  if (!op) {
    return PyErr_NoMemory();
  }
  
  sequence = gles_PySequence_Collapse(type, sequence, NULL);
  if(sequence==NULL) {
    // There should be an exception
    assert(PyErr_Occurred() != NULL);
    // Delete the object since we're not going to use it anymore anyways
    PyObject_Del(op);
    return NULL;
  }
  
  switch(type) {
    case GL_FLOAT:
      arrdata = gles_PySequence_AsGLfloatArray(sequence);
      op->item_size = sizeof(GLfloat);
      break;
    case GL_BYTE:
      arrdata = gles_PySequence_AsGLbyteArray(sequence);
      op->item_size = sizeof(GLbyte);
      break;
    case GL_UNSIGNED_BYTE:
      arrdata = gles_PySequence_AsGLubyteArray(sequence);
      op->item_size = sizeof(GLubyte);
      break;
    case GL_SHORT:
      arrdata = gles_PySequence_AsGLshortArray(sequence);
      op->item_size = sizeof(GLshort);
      break;
    case GL_UNSIGNED_SHORT:
      arrdata = gles_PySequence_AsGLushortArray(sequence);
      op->item_size = sizeof(GLushort);
      break;
    case GL_FIXED:
      arrdata = gles_PySequence_AsGLfixedArray(sequence);
      op->item_size = sizeof(GLfixed);
      break;
    default:
      PyErr_SetString(PyExc_ValueError, "Invalid array type specified");
      return NULL;
  }
  
  // We should have an exception from the previous functions if arrdata==NULL
  if(arrdata == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  op->arrdata = arrdata;
  op->len = PySequence_Length(sequence);
  // Calculate the length in memory
  op->real_len = op->item_size * op->len;
  op->arrtype = type;
  op->dimension = dimension;
  
  if (!op->arrdata) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  // Commented for now as it crashes (->gles_demo.py, simplecube.py) when 
  // gc.collect runs
  //PyObject_GC_Track(op);
  return (PyObject*)op;
}

extern "C" int array_length(PyObject *self) {
  array_object *op = (array_object*)self;
  
  return op->len;
}

extern "C" PyObject *array_getitem(PyObject *self, int idx) {
  array_object *op = (array_object*)self;
  PyObject *ret = NULL;
  
  if(idx > op->len-1 ) {
    PyErr_SetString(PyExc_IndexError, "list index out of range");
    return NULL;
  }
  
  switch(op->arrtype) {
    case GL_FLOAT:
      ret = PyFloat_FromDouble(*((GLfloat*)op->arrdata+idx));
      break;
    case GL_BYTE:
      ret = PyInt_FromLong(*((GLbyte*)op->arrdata+idx));
      break;
    case GL_UNSIGNED_BYTE:
      /// Python 2.2.2 does not have PyInt_FromUnsignedLong()
      ret = PyLong_FromUnsignedLong(*((GLubyte*)op->arrdata+idx));
      break;
    case GL_SHORT:
      ret = PyInt_FromLong(*((GLshort*)op->arrdata+idx));
      break;
    case GL_UNSIGNED_SHORT:
      ret = PyLong_FromUnsignedLong(*((GLushort*)op->arrdata+idx));
      break;
    case GL_FIXED:
      ret = PyInt_FromLong(*((GLfixed*)op->arrdata+idx));
      break;
    default:
      // This should never happen, but it's never too safe not to check
      PyErr_SetString(PyExc_RuntimeError, "Unknown type of array");
      return NULL;
  }
  return ret;
}

extern "C" int array_setitem(PyObject *self, int idx, PyObject *v) {
  array_object *op = (array_object*)self;
  
  if(idx > op->len-1 ) {
    PyErr_SetString(PyExc_IndexError, "list index out of range");
    return -1;
  }
  
  if(!PyNumber_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "Expecting a number");
    return -1;
  }
  
  switch(op->arrtype) {
    case GL_FLOAT:
      *((GLfloat*)op->arrdata+idx) = (GLfloat)PyFloat_AsDouble(v);
      break;
    case GL_BYTE:
      *((GLbyte*)op->arrdata+idx) = (GLbyte)PyInt_AsLong(v);
      break;
    case GL_UNSIGNED_BYTE:
      *((GLubyte*)op->arrdata+idx) = (GLubyte)PyInt_AsLong(v);
      break;
    case GL_SHORT:
      *((GLshort*)op->arrdata+idx) = (GLshort)PyInt_AsLong(v);
      break;
    case GL_UNSIGNED_SHORT:
      *((GLushort*)op->arrdata+idx) = (GLushort)PyInt_AsLong(v);
      break;
    case GL_FIXED:
      *((GLfixed*)op->arrdata+idx) = (GLfixed)PyInt_AsLong(v);
      break;
    default:
      // This should never happen, but it's never too safe not to check
      PyErr_SetString(PyExc_RuntimeError, "Unknown type of array");
      return 0;
  }
  return 0;
}


extern "C" {
  static void
  array_dealloc(array_object *op)
  {
#ifdef DEBUG_GLES
    DEBUGMSG("array_dealloc\n");
#endif
    if(op->arrdata) {
      gles_free(op->arrdata);
    }
    //PyObject_GC_UnTrack((PyObject*)op);
    PyObject_GC_Del(op);
  }
  
  const static PyMethodDef array_methods[] = {
    {NULL,              NULL}           // sentinel
  };
  
  const static PySequenceMethods array_as_sequence = {
    (inquiry)array_length,			/* sq_length */
    /*(binaryfunc)*/0,		      /* sq_concat */
    /*(intargfunc)*/0,		      /* sq_repeat */
    (intargfunc)array_getitem,  /* sq_item */
    /*(intintargfunc)*/0,		    /* sq_slice */
    (intobjargproc)array_setitem,  /* sq_ass_item */
    0,					                /* sq_ass_slice */
    /*(objobjproc)*/0,		      /* sq_contains */
  };
  
  static PyObject *
  array_getattr(array_object *op, char *name)
  {
    PyObject *ret;
    if (!strcmp(name,"len")) {
      ret = PyInt_FromLong(op->len);
      return ret;
    }
    return Py_FindMethod((PyMethodDef*)array_methods,
                         (PyObject*)op, name);
  }
  
  static int
  array_setattr(array_object */*op*/, char */*name*/, PyObject */*v*/ )
  {
    PyErr_SetString(PyExc_AttributeError, "no such attribute");
    return -1;
  }
  
  static const PyTypeObject c_array_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "GLESArray",                          /*tp_name*/
    sizeof(array_object),                       /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)array_dealloc,                  /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)array_getattr,                 /*tp_getattr*/
    (setattrfunc)array_setattr,                 /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    (PySequenceMethods*)&array_as_sequence,   /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };

}       // extern "C"

/***********************************************
 * The bindings code starts here               *
 ***********************************************/

extern "C" PyObject *gles_glActiveTexture(PyObject *self, PyObject *args) {
  GLenum texture;
  
  if( !PyArg_ParseTuple(args, "i:glActiveTexture", &texture) ) {
    return NULL;
  }
  
  glActiveTexture(texture);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glAlphaFunc(PyObject *self, PyObject *args) {
  GLenum func;
  GLclampf ref;
  
  if( !PyArg_ParseTuple(args, "if:glAlphaFunc", &func, &ref) ) {
    return NULL;
  }
  
  glAlphaFunc(func, ref);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glAlphaFuncx(PyObject *self, PyObject *args) {
  GLenum func;
  GLclampx ref;
  
  if( !PyArg_ParseTuple(args, "ii:glAlphaFuncx", &func, &ref) ) {
    return NULL;
  }
  
  glAlphaFuncx(func, ref);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glBindTexture(PyObject *self, PyObject *args) {
  GLenum target;
  GLuint texture;
  
  if( !PyArg_ParseTuple(args, "ii:glBindTexture", &target, &texture)) {
    return NULL;
  }
  
  glBindTexture(target, texture);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glBlendFunc(PyObject *self, PyObject *args) {
  GLenum sfactor;
  GLenum dfactor;
  
  if( !PyArg_ParseTuple(args, "ii:glBlendFunc", &sfactor, &dfactor) ) {
    return NULL;
  }
  
  glBlendFunc(sfactor, dfactor);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClear (PyObject *self, PyObject *args) {
  unsigned int mask;
  
  if ( !PyArg_ParseTuple(args, "i:glClear", &mask) ) {
    return NULL;
  }
  glClear(mask);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClearColor (PyObject *self, PyObject *args) {
  GLclampf red;
  GLclampf green;
  GLclampf blue;
  GLclampf alpha;
  
  if ( !PyArg_ParseTuple(args, "ffff:glClearColor", &red, &green, &blue, &alpha ) ) {
    return NULL;
  }
  glClearColor(red, green, blue, alpha);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClearColorx(PyObject *self, PyObject *args) {
  GLclampx red;
  GLclampx green;
  GLclampx blue;
  GLclampx alpha;
  
  if ( !PyArg_ParseTuple(args, "iiii:glClearColorx", &red, &green, &blue, &alpha ) ) {
    return NULL;
  }
  glClearColorx(red, green, blue, alpha);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClearDepthf(PyObject *self, PyObject *args) {
  GLclampf depth;
  
  if ( !PyArg_ParseTuple(args, "f:glClearDepthf", &depth) ) {
    return NULL;
  }
  glClearDepthf(depth);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClearDepthx(PyObject *self, PyObject *args) {
  GLclampx depth;
  
  if ( !PyArg_ParseTuple(args, "i:glClearDepthx", &depth) ) {
    return NULL;
  }
  glClearDepthx(depth);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClearStencil(PyObject *self, PyObject *args) {
  GLint s;
  
  if ( !PyArg_ParseTuple(args, "i:glClearStencil", &s) ) {
    return NULL;
  }
  glClearStencil(s);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClientActiveTexture(PyObject *self, PyObject *args) {
  GLenum texture;
  
  if ( !PyArg_ParseTuple(args, "i:glClientActiveTexture", &texture) ) {
    return NULL;
  }
  glClientActiveTexture(texture);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
  
}

extern "C" PyObject *gles_glColor4f(PyObject *self, PyObject *args) {
  GLfloat red;
  GLfloat green;
  GLfloat blue;
  GLfloat alpha;
  
  if ( !PyArg_ParseTuple(args, "ffff:glColor4f", &red, &green, &blue, &alpha) ) {
    return NULL;
  }
  glColor4f(red, green, blue, alpha);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glColor4x(PyObject *self, PyObject *args) {
  GLfixed red;
  GLfixed green;
  GLfixed blue;
  GLfixed alpha;
  
  if ( !PyArg_ParseTuple(args, "iiii:glColor4x", &red, &green, &blue, &alpha) ) {
    return NULL;
  }
  glColor4x(red, green, blue, alpha);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glColorMask(PyObject *self, PyObject *args) {
  GLboolean red;
  GLboolean green;
  GLboolean blue;
  GLboolean alpha;
  
  if ( !PyArg_ParseTuple(args, "iiii:glColorMask", &red, &green, &blue, &alpha) ) {
    return NULL;
  }
  glColorMask(red, green, blue, alpha);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
  
}

PyObject *gles_wrap_glColorPointer(GLint size, GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *cptr = NULL;
  array_object *arrobj=NULL;
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    cptr = arrobj->arrdata;
  }
#ifdef GL_OES_VERSION_1_1
  else if(pointerPy == Py_None) {
    cptr = NULL;
  }
#endif
  // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    if(pointerPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    // glColorPointer accepts only GL_UNSIGNED_BYTE, GL_FLOAT or GL_FIXED types
    switch(type) {
      case GL_UNSIGNED_BYTE:
        cptr = gles_PySequence_AsGLubyteArray(pointerPy);
        break;
      case GL_FLOAT:
        cptr = gles_PySequence_AsGLfloatArray(pointerPy);
        break;
      case GL_FIXED:
        cptr = gles_PySequence_AsGLfixedArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Unsupported array type");
        return NULL;
    }
    if(cptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
#ifndef GL_OES_VERSION_1_1
  if(cptr == NULL) {
    // Something went horribly wrong
    // However, we should have an exception already
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
#endif
  gles_assign_array(GL_COLOR_ARRAY, cptr, arrobj);
  glColorPointer(size, type, stride, cptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glColorPointer(PyObject *self, PyObject *args) {
  GLint size;
  GLenum type;
  GLsizei stride;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "iiiO:glColorPointer", &size, &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  return gles_wrap_glColorPointer(size, type, stride, pointerPy);
}

extern "C" PyObject *gles_glColorPointerub(PyObject *self, PyObject *args) {
  GLint size;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glColorPointerub", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_COLOR_ARRAY);
    RETURN_PYNONE;
  }
  
  if( PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glColorPointer(size, /*type*/ GL_UNSIGNED_BYTE, /*stride*/ 0, pointerPy);
}

extern "C" PyObject *gles_glColorPointerf(PyObject *self, PyObject *args) {
  GLint size;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glColorPointerf", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_COLOR_ARRAY);
    RETURN_PYNONE;
  }
  
  if( PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glColorPointer(size, /*type*/ GL_FLOAT, /*stride*/ 0, pointerPy);
}

extern "C" PyObject *gles_glColorPointerx(PyObject *self, PyObject *args) {
  GLint size;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glColorPointerx", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_COLOR_ARRAY);
    RETURN_PYNONE;
  }
  
  if( PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glColorPointer(size, /*type*/ GL_FIXED, /*stride*/ 0, pointerPy);
}

extern "C" PyObject *gles_glCompressedTexImage2D(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLenum internalformat;
  GLsizei width;
  GLsizei height;
  GLint border;
  GLsizei imageSize;
  GLvoid *data;
  
  PyObject *pixelsPy;
  
  if( !PyArg_ParseTuple(args, "iiiiiiiO:glCompressedTexImage2D", &target, &level, &internalformat, &width, &height, &border, &imageSize, &pixelsPy) ) {
    return NULL;
  }
  
  // Only accepting array or string objects 
  if( PyObject_TypeCheck(pixelsPy, GLES_ARRAY_TYPE) ) {
    array_object *arr = (array_object*)pixelsPy;
    data = arr->arrdata;
  } else if( PyString_Check(pixelsPy) ) {
    // "Convert" the string object into an array.
    // The returned pointer points to the raw C string
    // data held by the object, so no actual conversion needed.
    data = PyString_AsString(pixelsPy);
    if(data == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"Only strings and gles.array objects supported");
    return NULL;
  }
  
  glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
  
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glCompressedTexSubImage2D(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLint xoffset;
  GLint yoffset;
  GLsizei width;
  GLsizei height;
  GLenum format;
  GLsizei imageSize;
  GLvoid *data;
  
  PyObject *pixelsPy;
  
  if( !PyArg_ParseTuple(args, "iiiiiiiiO:glCompressedTexSubImage2D", &target, &level, &xoffset, &yoffset, &width, &height, &format, &imageSize, &pixelsPy) ) {
    return NULL;
  }
  
  if( PyObject_TypeCheck(pixelsPy, GLES_ARRAY_TYPE) ) {
    array_object *arr = (array_object*)pixelsPy;
    data = arr->arrdata;
  } else if( PyString_Check(pixelsPy) ) {
    // "Convert" the string object into an array.
    // The returned pointer points to the raw C string
    // data held by the object, so no actual conversion needed.
    data = PyString_AsString(pixelsPy);
    if(data == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  } // The last resort is to check if the given object is an Image object
  else {
    PyErr_SetString(PyExc_TypeError,"Only strings and gles.array objects supported");
    return NULL;
  }
  
  glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
  
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glCopyTexImage2D(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLenum internalformat;
  GLint x;
  GLint y;
  GLsizei width;
  GLsizei height;
  GLint border;
  
  if ( !PyArg_ParseTuple(args, "iiiiiiii:glCopyTexImage2D", &target, &level, &internalformat, &x, &y, &width, &height, &border) ) {
    return NULL;
  }
  glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glCopyTexSubImage2D(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLint xoffset;
  GLint yoffset;
  GLint x;
  GLint y;
  GLsizei width;
  GLsizei height;
  
  if ( !PyArg_ParseTuple(args, "iiiiiiii:glCopyTexSubImage2D", &target, &level, &xoffset, &yoffset, &x, &y, &width, &height) ) {
    return NULL;
  }
  glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glCullFace(PyObject *self, PyObject *args) {
  GLenum mode;
  
  if ( !PyArg_ParseTuple(args, "i:glCullFace", &mode) ) {
    return NULL;
  }
  glCullFace(mode);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDeleteTextures(PyObject *self, PyObject *args) {
  GLsizei n;
  GLuint *textures = NULL;
  PyObject *texturesPy = NULL;
  
  if( !PyArg_ParseTuple(args, "O:glDeleteTextures", &texturesPy) ) {
    return NULL;
  }
  
  if(! PySequence_Check(texturesPy) ) {
    PyErr_SetString(PyExc_TypeError, "Expecting a sequence");
    return NULL;
  }
  
  
  textures = gles_PySequence_AsGLuintArray(texturesPy);
  if( textures == NULL ) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  n = PySequence_Length(texturesPy);
  glDeleteTextures(n, textures);
  gles_free(textures);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDepthFunc(PyObject *self, PyObject *args) {
  GLenum func;
  
  if ( !PyArg_ParseTuple(args, "i:glDepthFunc", &func) ) {
    return NULL;
  }
  glDepthFunc(func);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDepthMask(PyObject *self, PyObject *args) {
  GLboolean flag;
  
  if ( !PyArg_ParseTuple(args, "i:glDepthMask", &flag) ) {
    return NULL;
  }
  glDepthMask(flag);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDepthRangef(PyObject *self, PyObject *args) {
  GLclampf zNear;
  GLclampf zFar;
  
  if ( !PyArg_ParseTuple(args, "ff:glDepthRangef", &zNear, &zFar) ) {
    return NULL;
  }
  glDepthRangef(zNear, zFar);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDepthRangex(PyObject *self, PyObject *args) {
  GLclampx zNear;
  GLclampx zFar;
  
  if ( !PyArg_ParseTuple(args, "ii:glDepthRangex", &zNear, &zFar) ) {
    return NULL;
  }
  glDepthRangex(zNear, zFar);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDisable(PyObject *self, PyObject *args) {
  GLenum cap;
  if ( !PyArg_ParseTuple(args, "i:glDisable", &cap) ) {
    return NULL;
  }
  glDisable(cap);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDisableClientState(PyObject *self, PyObject *args) {
  GLenum array;
  
  if ( !PyArg_ParseTuple(args, "i:glDisableClientState", &array) ) {
    return NULL;
  }
  glDisableClientState(array);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawArrays(PyObject *self, PyObject *args) {
  GLenum mode;
  GLint first;
  GLsizei count;
  
  if ( !PyArg_ParseTuple(args, "iii:glDrawArrays", &mode, &first, &count) ) {
    return NULL;
  }
  glDrawArrays(mode, first, count);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawElements(PyObject *self, PyObject *args) {
  GLenum mode;
  GLsizei count = 0;
  GLenum type;
  void *eptr = NULL;
  array_object *arrobj = NULL;
  PyObject *indicesPy;
  
  if ( !PyArg_ParseTuple(args, "iiiO:glDrawElements", &mode, &count, &type, &indicesPy) ) {
    return NULL;
  }
  
  // We can use either the array type or a sequence.
  // Check for gles.array type first.
  if(PyObject_TypeCheck(indicesPy, GLES_ARRAY_TYPE) ) {
    arrobj=(array_object*)indicesPy;
    eptr = (GLubyte*)arrobj->arrdata;
  } // We can also use None as an argument.
  else if(indicesPy == Py_None) {
    eptr = NULL;
  } // Try to feed the object as a sequence.
  else {
    indicesPy = gles_PySequence_Collapse(type, indicesPy, NULL);
    if(indicesPy == NULL) {
      // We should have an exception from PySequence_Collapse if something went wrong
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    
    // glDrawElements accepts only GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT types
    switch(type) {
      case GL_UNSIGNED_BYTE:
        eptr = gles_PySequence_AsGLubyteArray(indicesPy);
        break;
      case GL_UNSIGNED_SHORT:
        eptr = gles_PySequence_AsGLushortArray(indicesPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Invalid array type");
        return NULL;
    }
    if(eptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
  glDrawElements(mode, count, type, eptr);
  if(arrobj == NULL && eptr != NULL) {
    gles_free(eptr);
  }
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawElementsub(PyObject *self, PyObject *args) {
  GLenum mode;
  GLsizei count = 0;
  GLenum type = GL_UNSIGNED_BYTE;
  GLubyte *eptr;
  array_object *arrobj = NULL;
  PyObject *indicesPy;
  
  if ( !PyArg_ParseTuple(args, "iO:glDrawElementsub", &mode, &indicesPy) ) {
    return NULL;
  }
  
  // We can use either the array type or a sequence.
  // Check for gles.array type first.
  if(PyObject_TypeCheck(indicesPy, GLES_ARRAY_TYPE) ) {
    arrobj=(array_object*)indicesPy;
    if(arrobj->arrtype != type) {
      PyErr_SetString(PyExc_TypeError, "Invalid type; expected GL_UNSIGNED_BYTE");
      return NULL;
    }
    eptr = (GLubyte*)arrobj->arrdata;
    count = arrobj->len;
  } // We can also use None as an argument.
  else if(indicesPy == Py_None) {
    eptr = NULL;
  } // Try to feed the object as a sequence.
  else {
    indicesPy = gles_PySequence_Collapse(type, indicesPy, NULL);
    if(indicesPy == NULL) {
      // We should have an exception from PySequence_Collapse if something went wrong
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    
    eptr = gles_PySequence_AsGLubyteArray(indicesPy);
    if(eptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    count = PySequence_Length(indicesPy);
  }
  
  glDrawElements(mode, count, type, eptr);
  if(arrobj == NULL && eptr != NULL) {
    gles_free(eptr);
  }
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawElementsus (PyObject *self, PyObject *args) {
  GLenum mode;
  GLsizei count = 0;
  GLenum type = GL_UNSIGNED_SHORT;
  GLushort *eptr;
  array_object *arrobj = NULL;
  PyObject *indicesPy;
  
  if ( !PyArg_ParseTuple(args, "iO:glDrawElementsus", &mode, &indicesPy) ) {
    return NULL;
  }
  
  // We can use either the array type or a sequence.
  // Check for gles.array type first.
  if(PyObject_TypeCheck(indicesPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)indicesPy;
    if(arrobj->arrtype != type) {
      PyErr_SetString(PyExc_TypeError, "Invalid type; expected GL_UNSIGNED_SHORT");
      return NULL;
    }
    eptr = (GLushort*)arrobj->arrdata;
    count = arrobj->len;
  } // We can also use None as an argument.
  else if(indicesPy == Py_None) {
    eptr = NULL;
  } // Try to feed the object as a sequence.
  else {
    indicesPy = gles_PySequence_Collapse(type, indicesPy, NULL);
    if(indicesPy == NULL) {
      // We should have an exception from PySequence_Collapse if something went wrong
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    eptr = gles_PySequence_AsGLushortArray(indicesPy);
    if(eptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    count = PySequence_Length(indicesPy);
  }
  
  glDrawElements(mode, count, type, eptr);
  if(arrobj == NULL && eptr != NULL) {
    gles_free(eptr);
  }
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glEnable (PyObject *self, PyObject *args) {
  GLenum cap;
  if ( !PyArg_ParseTuple(args, "i:glEnable", &cap) ) {
    return NULL;
  }
  glEnable(cap);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glEnableClientState(PyObject *self, PyObject *args) {
  GLenum array;
  
  if ( !PyArg_ParseTuple(args, "i:glEnableClientState", &array) ) {
    return NULL;
  }
  glEnableClientState(array);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFinish(PyObject *self, PyObject */*args*/) {
  glFinish();
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFlush(PyObject *self, PyObject */*args*/) {
  glFlush();
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFogf(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfloat param;
  
  if ( !PyArg_ParseTuple(args, "if:glFogf", &pname, &param) ) {
    return NULL;
  }
  glFogf(pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFogfv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfloat *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iO:glFogfv", &pname, &paramsPy) ) {
    return NULL;
  }
  params = gles_PySequence_AsGLfloatArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glFogfv(pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFogx(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfixed param;
  
  if ( !PyArg_ParseTuple(args, "ii:glFogx", &pname, &param) ) {
    return NULL;
  }
  glFogx(pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFogxv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfixed *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iO:glFogxv", &pname, &paramsPy) ) {
    return NULL;
  }
  params = gles_PySequence_AsGLfixedArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glFogxv(pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFrontFace(PyObject *self, PyObject *args) {
  GLenum mode;
  
  if ( !PyArg_ParseTuple(args, "i:glFrontFace", &mode) ) {
    return NULL;
  }
  glFrontFace(mode);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFrustumf(PyObject *self, PyObject *args) {
  GLfloat left;
  GLfloat right;
  GLfloat bottom;
  GLfloat top;
  GLfloat zNear;
  GLfloat zFar;
  
  if ( !PyArg_ParseTuple(args, "ffffff:glFrustumf", &left, &right, &bottom, &top, &zNear, &zFar) ) {
    return NULL;
  }
  glFrustumf(left, right, bottom, top, zNear, zFar);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glFrustumx(PyObject *self, PyObject *args) {
  GLfixed left;
  GLfixed right;
  GLfixed bottom;
  GLfixed top;
  GLfixed zNear;
  GLfixed zFar;
  
  if ( !PyArg_ParseTuple(args, "iiiiii:glFrustumx", &left, &right, &bottom, &top, &zNear, &zFar) ) {
    return NULL;
  }
  glFrustumx(left, right, bottom, top, zNear, zFar);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glGenTextures(PyObject *self, PyObject *args) {
  GLuint *textures = NULL;
  GLuint n;
  unsigned int i;
  PyObject *ret;
  
  if( !PyArg_ParseTuple(args, "i:glGenTextures", &n) ) {
    return NULL;
  }
  
  if(n < 1) {
    PyErr_SetString(PyExc_ValueError, "Value must be positive");
    return NULL;
  }
  
  textures = (GLuint*)gles_alloc( sizeof(GLuint)*n );
  
  glGenTextures( n, textures);
  if(n > 1) {
    ret = PyTuple_New(n);
  
    for(i=0;i<n;i++) {
      PyTuple_SetItem(ret, i, Py_BuildValue("i", textures[i]));
    }
  } else {
    ret = Py_BuildValue("i", textures[0]);
  }
  gles_free(textures);
  
  RETURN_IF_GLERROR;
  Py_INCREF(ret);
  return ret;
}

extern "C" PyObject *gles_glGetIntegerv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLint *params = NULL;
  int values = 1;
  int i;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "i:glGetIntegerv", &pname) ) {
    return NULL;
  }
  
  switch(pname) {
    // 2 elements
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
      values = 2;
      break;
    // Special cases
    case GL_COMPRESSED_TEXTURE_FORMATS:
      glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &values);
      break;
    // 1 element
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_DEPTH_BITS:
    case GL_GREEN_BITS:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_MAX_ELEMENTS_INDICES:
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_LIGHTS:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MAX_TEXTURE_UNITS:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_RED_BITS:
    case GL_STENCIL_BITS:
    case GL_SUBPIXEL_BITS:
    default:
      values = 1;
      break;
  }
  
  params = (int*)gles_alloc(values*sizeof(int));
  if(params == NULL) {
    return PyErr_NoMemory();
  }
  glGetIntegerv(pname, params);
  
  paramsPy = PyTuple_New(values);
  if(paramsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    gles_free(params);
    return NULL;
  }
  
  for(i = 0; i < values; i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("i", params[i]));
  }
  
  gles_free(params);
  RETURN_IF_GLERROR;
  return paramsPy;
}

extern "C" PyObject *gles_glGetString(PyObject *self, PyObject *args) {
  GLenum name;
  GLubyte *s;
  PyObject *ret;
  
  if ( !PyArg_ParseTuple(args, "i:glGetString", &name) ) {
    return NULL;
  }
  
  s = (GLubyte*)glGetString(name);
  if(s == NULL) {
    RETURN_IF_GLERROR;
    RETURN_PYNONE;
  }
  
  RETURN_IF_GLERROR;
  ret = PyString_FromString((const char*)s);
  Py_INCREF(ret);
  return ret;
}

extern "C" PyObject *gles_glHint(PyObject *self, PyObject *args) {
  GLenum target;
  GLenum mode;
  
  if ( !PyArg_ParseTuple(args, "ii:glHint", &target, &mode) ) {
    return NULL;
  }
  glHint(target, mode);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightModelf(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfloat param;
  
  if( !PyArg_ParseTuple(args, "if:glLightModelf", &pname, &param) ) {
    return NULL;
  }
  
  glLightModelf(pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightModelfv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfloat *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iO:glLightModelfv", &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfloat*)gles_PySequence_AsGLfloatArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glLightModelfv(pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightModelx(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfixed param;
  
  if( !PyArg_ParseTuple(args, "ii:glLightModelx", &pname, &param) ) {
    return NULL;
  }
  
  glLightModelx(pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightModelxv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfixed *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iO:glLightModelxv", &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfixed*)gles_PySequence_AsGLfixedArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glLightModelxv(pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightf(PyObject *self, PyObject *args) {
  GLenum light;
  GLenum pname;
  GLfloat param;
  
  if ( !PyArg_ParseTuple(args, "iif:glLightf", &light, &pname, &param) ) {
    return NULL;
  }
  
  glLightf(light, pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightfv(PyObject *self, PyObject *args) {
  GLenum light;
  GLenum pname;
  GLfloat *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iiO:glLightfv", &light, &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfloat*)gles_PySequence_AsGLfloatArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glLightfv(light, pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightx(PyObject *self, PyObject *args) {
  GLenum light;
  GLenum pname;
  GLfixed param;
  
  if ( !PyArg_ParseTuple(args, "iii:glLightx", &light, &pname, &param) ) {
    return NULL;
  }
  
  glLightx(light, pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLightxv (PyObject *self, PyObject *args) {
  GLenum light;
  GLenum pname;
  GLfixed *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iiO:glLightxv", &light, &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfixed*)gles_PySequence_AsGLfixedArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glLightxv(light, pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLineWidth(PyObject *self, PyObject *args) {
  GLfloat width;
  
  if ( !PyArg_ParseTuple(args, "f:glLineWidth", &width) ) {
    return NULL;
  }
  glLineWidth(width);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLineWidthx(PyObject *self, PyObject *args) {
  GLfixed width;
  
  if ( !PyArg_ParseTuple(args, "i:glLineWidthx", &width) ) {
    return NULL;
  }
  glLineWidthx(width);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLoadIdentity (PyObject *self, PyObject */*args*/) { 
  glLoadIdentity();
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLoadMatrixf(PyObject *self, PyObject *args) {
  GLfloat *m;
  
  PyObject *mPy;
  if ( !PyArg_ParseTuple(args, "O:glLoadMatrixf", &mPy) ) {
    return NULL;
  }
  mPy = gles_PySequence_Collapse(GL_FLOAT, mPy, NULL);
  if(mPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  if(PySequence_Length(mPy) != 16) {
    PyErr_SetString(PyExc_ValueError, "Expecting a sequence of 16 floats");
    return NULL;
  }
  
  m = gles_PySequence_AsGLfloatArray(mPy);
  if(m == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glLoadMatrixf(m);
  gles_free(m);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLoadMatrixx(PyObject *self, PyObject *args) {
  GLfixed *m;
  
  PyObject *mPy;
  if ( !PyArg_ParseTuple(args, "O:glLoadMatrixx", &mPy) ) {
    return NULL;
  }
  mPy = gles_PySequence_Collapse(GL_FIXED, mPy, NULL);
  if(mPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  if(PySequence_Length(mPy) != 16) {
    PyErr_SetString(PyExc_ValueError, "Expecting a sequence of 16 ints");
    return NULL;
  }
  
  m = gles_PySequence_AsGLfixedArray(mPy);
  if(m == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glLoadMatrixx(m);
  gles_free(m);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glLogicOp(PyObject *self, PyObject *args) {
  GLenum opcode;
  
  if ( !PyArg_ParseTuple(args, "i:glLogicOp", &opcode) ) {
    return NULL;
  }
  glLogicOp(opcode);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMaterialf(PyObject *self, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfloat param;
  
  if ( !PyArg_ParseTuple(args, "iif:glMaterialf", &face, &pname, &param) ) {
    return NULL;
  }
  
  glMaterialf(face, pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMaterialfv (PyObject *self, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfloat *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iiO:glMaterialfv", &face, &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfloat*)gles_PySequence_AsGLfloatArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glMaterialfv(face, pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMaterialx (PyObject *self, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfixed param;
  
  if ( !PyArg_ParseTuple(args, "iii:glMaterialx", &face, &pname, &param) ) {
    return NULL;
  }
  
  glMaterialx(face, pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMaterialxv (PyObject *self, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfixed *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iiO:glMaterialxv", &face, &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfixed*)gles_PySequence_AsGLfixedArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glMaterialxv(face, pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMatrixMode (PyObject *self, PyObject *args) {
  GLenum mode;

  if ( !PyArg_ParseTuple(args, "i:glMatrixMode", &mode) ) {
    return NULL;
  }
  glMatrixMode(mode);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMultMatrixf(PyObject *self, PyObject *args) {
  GLfloat *m;
  
  PyObject *mPy;
  if ( !PyArg_ParseTuple(args, "O:glMultMatrixf", &mPy) ) {
    return NULL;
  }
  
  mPy = gles_PySequence_Collapse(GL_FLOAT, mPy, NULL);
  if(mPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  if(PySequence_Length(mPy) != 16) {
    PyErr_SetString(PyExc_ValueError, "Expecting a sequence of 16 floats");
    return NULL;
  }
  
  m = gles_PySequence_AsGLfloatArray(mPy);
  if(m == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glMultMatrixf(m);
  gles_free(m);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMultMatrixx(PyObject *self, PyObject *args) {
  GLfixed *m;
  
  PyObject *mPy;
  if ( !PyArg_ParseTuple(args, "O:glMultMatrixx", &mPy) ) {
    return NULL;
  }
  
  mPy = gles_PySequence_Collapse(GL_FIXED, mPy, NULL);
  if(mPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  if(PySequence_Length(mPy) != 16) {
    PyErr_SetString(PyExc_ValueError, "Expecting a sequence of 16 ints");
    return NULL;
  }
  
  m = gles_PySequence_AsGLfixedArray(mPy);
  if(m == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glMultMatrixx(m);
  gles_free(m);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMultiTexCoord4f(PyObject *self, PyObject *args) {
  GLenum target;
  GLfloat s;
  GLfloat t;
  GLfloat r;
  GLfloat q;
  
  if ( !PyArg_ParseTuple(args, "iffff:glMultiTexCoord4f", &target, &s, &t, &r, &q) ) {
    return NULL;
  }
  glMultiTexCoord4f(target, s, t, r, q);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMultiTexCoord4x(PyObject *self, PyObject *args) {
  GLenum target;
  GLfixed s;
  GLfixed t;
  GLfixed r;
  GLfixed q;
  
  if ( !PyArg_ParseTuple(args, "iiiii:glMultiTexCoord4x", &target, &s, &t, &r, &q) ) {
    return NULL;
  }
  glMultiTexCoord4x(target, s, t, r, q);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glNormal3f(PyObject *self, PyObject *args) {
  GLfloat nx;
  GLfloat ny;
  GLfloat nz;
  
  if ( !PyArg_ParseTuple(args, "fff:glNormal3f", &nx, &ny, &nz) ) {
    return NULL;
  }
  glNormal3f(nx, ny, nz);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glNormal3x(PyObject *self, PyObject *args) {
  GLfixed nx;
  GLfixed ny;
  GLfixed nz;
  
  if ( !PyArg_ParseTuple(args, "iii:glNormal3x", &nx, &ny, &nz) ) {
    return NULL;
  }
  glNormal3x(nx, ny, nz);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glNormalPointer(GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *nptr = NULL;
  array_object *arrobj=NULL;
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    nptr = arrobj->arrdata;
  } 
#ifdef GL_OES_VERSION_1_1
  else if(pointerPy == Py_None) {
    nptr = NULL;
  }
#endif
  // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    if( pointerPy == NULL ) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    // glNormalPointer accepts only GL_BYTE, GL_SHORT, GL_FIXED or GL_FLOAT types
    switch(type) {
      case GL_BYTE:
        nptr = gles_PySequence_AsGLbyteArray(pointerPy);
        break;
      case GL_SHORT:
        nptr = gles_PySequence_AsGLshortArray(pointerPy);
        break;
      case GL_FIXED:
        nptr = gles_PySequence_AsGLfixedArray(pointerPy);
        break;
      case GL_FLOAT:
        nptr = gles_PySequence_AsGLfloatArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Invalid array type");
        return NULL;
    }
    if(nptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
#ifndef GL_OES_VERSION_1_1
  if(nptr == NULL) {
    // Something went horribly wrong
    // However, we should have an exception already
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
#endif
  gles_assign_array(GL_NORMAL_ARRAY, nptr, arrobj);
  glNormalPointer(type, stride, nptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glNormalPointer(PyObject *self, PyObject *args) {
  GLenum type;
  GLsizei stride;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glNormalPointer", &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_NORMAL_ARRAY);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glNormalPointer(type, stride, pointerPy);
}

extern "C" PyObject *gles_glNormalPointerb(PyObject *self, PyObject *args) {
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glNormalPointerb", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_NORMAL_ARRAY);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glNormalPointer(/*type*/GL_BYTE, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glNormalPointers(PyObject *self, PyObject *args) {
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glNormalPointers", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_NORMAL_ARRAY);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glNormalPointer(/*type*/GL_SHORT, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glNormalPointerf(PyObject *self, PyObject *args) {
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glNormalPointerf", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_NORMAL_ARRAY);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glNormalPointer(/*type*/GL_FLOAT, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glNormalPointerx(PyObject *self, PyObject *args) {
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glNormalPointerx", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_NORMAL_ARRAY);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glNormalPointer(/*type*/GL_FIXED, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glOrthof(PyObject *self, PyObject *args) {
  GLfloat left;
  GLfloat right;
  GLfloat bottom;
  GLfloat top;
  GLfloat zNear;
  GLfloat zFar;
  
  if ( !PyArg_ParseTuple(args, "ffffff:glOrthof", &left, &right, &bottom, &top, &zNear, &zFar) ) {
    return NULL;
  }
  glOrthof(left, right, bottom, top, zNear, zFar);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glOrthox(PyObject *self, PyObject *args) {
  GLfixed left;
  GLfixed right;
  GLfixed bottom;
  GLfixed top;
  GLfixed zNear;
  GLfixed zFar;
  
  if ( !PyArg_ParseTuple(args, "iiiiii:glOrthox", &left, &right, &bottom, &top, &zNear, &zFar) ) {
    return NULL;
  }
  glOrthox(left, right, bottom, top, zNear, zFar);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPixelStorei(PyObject *self, PyObject *args) {
  GLenum pname;
  GLint param;
  
  if ( !PyArg_ParseTuple(args, "ii:glPixelStorei", &pname, &param) ) {
    return NULL;
  }
  glPixelStorei(pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPointSize(PyObject *self, PyObject *args) {
  GLfloat size;
  
  if ( !PyArg_ParseTuple(args, "f:glPointSize", &size) ) {
    return NULL;
  }
  glPointSize(size);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPointSizex(PyObject *self, PyObject *args) {
  GLfixed size;
  
  if ( !PyArg_ParseTuple(args, "i:glPointSizex", &size) ) {
    return NULL;
  }
  glPointSizex(size);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPolygonOffset(PyObject *self, PyObject *args) {
  GLfloat factor;
  GLfloat units;
  
  if ( !PyArg_ParseTuple(args, "ff:glPolygonOffset", &factor, &units) ) {
    return NULL;
  }
  glPolygonOffset(factor, units);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPolygonOffsetx(PyObject *self, PyObject *args) {
  GLfixed factor;
  GLfixed units;
  
  if ( !PyArg_ParseTuple(args, "ii:glPolygonOffsetx", &factor, &units) ) {
    return NULL;
  }
  glPolygonOffsetx(factor, units);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPopMatrix(PyObject *self, PyObject */*args*/) {
  glPopMatrix();
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPushMatrix(PyObject *self, PyObject */*args*/) {
  glPushMatrix();
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glReadPixels(PyObject *self, PyObject *args) {
  GLint x;
  GLint y;
  GLsizei width;
  GLsizei height;
  GLenum format;
  GLenum type;
  GLvoid *pixels;
  
  int len=0;
  PyObject *pixelsPy=NULL;
  if ( !PyArg_ParseTuple(args, "iiiiii:glReadPixels", &x, &y, &width, &height, &format, &type) ) {
    return NULL;
  }
  
  // Determine the size of the result
  switch(format) {
    case GL_RGBA:
      len = width*height*4;
      break;
    case GL_RGB:
      len = width*height*3;
      break;
    case GL_LUMINANCE:
      len = width*height;
      break;
    case GL_LUMINANCE_ALPHA:
      len = width*height*2;
      break;
    case GL_ALPHA:
      len = width*height;
      break;
    default:
      PyErr_SetString(PyExc_ValueError, "Unsupported format");
      return NULL;
  }
  
  pixels = gles_alloc( len*sizeof(GLubyte) );
  if( pixels == NULL ) {
    return PyErr_NoMemory();
  }
  
  glReadPixels(x, y, width, height, format, type, pixels);
  
  // Make a string object out of the data
  pixelsPy = PyString_FromStringAndSize( (char*)pixels, len);
  gles_free(pixels);
  
  RETURN_IF_GLERROR;
  Py_INCREF(pixelsPy);
  return pixelsPy;
}

extern "C" PyObject *gles_glRotatef(PyObject *self, PyObject *args) {
  GLfloat angle;
  GLfloat x;
  GLfloat y;
  GLfloat z;
  
  if ( !PyArg_ParseTuple(args, "ffff:glRotatef", &angle, &x, &y, &z) ) {
    return NULL;
  }
  glRotatef(angle, x, y, z);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glRotatex (PyObject *self, PyObject *args) {
  GLfixed angle;
  GLfixed x;
  GLfixed y;
  GLfixed z;
  
  if ( !PyArg_ParseTuple(args, "iiii:glRotatex", &angle, &x, &y, &z) ) {
    return NULL;
  }
  glRotatex(angle, x, y, z);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glSampleCoverage(PyObject *self, PyObject *args) {
  GLclampf value;
  GLboolean invert;
  
  if ( !PyArg_ParseTuple(args, "fi:glSampleCoverage", &value, &invert) ) {
    return NULL;
  }
  
  if(invert) {
    invert = GL_TRUE;
  } else {
    invert = GL_FALSE;
  }
  
  glSampleCoverage(value, invert);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glSampleCoveragex(PyObject *self, PyObject *args) {
  GLclampx value;
  GLboolean invert;
  
  if ( !PyArg_ParseTuple(args, "ii:glSampleCoverage", &value, &invert) ) {
    return NULL;
  }
  
  if(invert) {
    invert = GL_TRUE;
  } else {
    invert = GL_FALSE;
  }
  
  glSampleCoveragex(value, invert);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glScalef (PyObject *self, PyObject *args) {
  GLfloat x;
  GLfloat y;
  GLfloat z;
  
  if ( !PyArg_ParseTuple(args, "fff:glScalef", &x, &y, &z) ) {
    return NULL;
  }
  glScalef(x, y, z);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glScalex(PyObject *self, PyObject *args) {
  GLfixed x;
  GLfixed y;
  GLfixed z;

  if ( !PyArg_ParseTuple(args, "iii:glScalex", &x, &y, &z) ) {
    return NULL;
  }
  glScalex(x, y, z);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glScissor(PyObject *self, PyObject *args) {
  GLint x;
  GLint y;
  GLsizei width;
  GLsizei height;
  
  if ( !PyArg_ParseTuple(args, "iiii:glScissor", &x, &y, &width, &height) ) {
    return NULL;
  }
  glScissor(x, y, width, height);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glShadeModel(PyObject *self, PyObject *args) {
  GLenum mode;
  
  if ( !PyArg_ParseTuple(args, "i:glShadeModel", &mode) ) {
    return NULL;
  }
  glShadeModel(mode);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glStencilFunc(PyObject *self, PyObject *args) {
  GLenum func;
  GLint ref;
  GLuint mask;
  
  if ( !PyArg_ParseTuple(args, "iii:glStencilFunc", &func, &ref, &mask) ) {
    return NULL;
  }
  glStencilFunc(func, ref, mask);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glStencilMask(PyObject *self, PyObject *args) {
  GLuint mask;
  
  if ( !PyArg_ParseTuple(args, "i:glStencilMask", &mask) ) {
    return NULL;
  }
  glStencilMask(mask);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glStencilOp(PyObject *self, PyObject *args) {
  GLenum fail;
  GLenum zfail;
  GLenum zpass;
  
  if ( !PyArg_ParseTuple(args, "iii:glStencilOp", &fail, &zfail, &zpass) ) {
    return NULL;
  }
  glStencilOp(fail, zfail, zpass);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *tcptr = NULL;
  array_object *arrobj=NULL;
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    tcptr = arrobj->arrdata;
  } 
#ifdef GL_OES_VERSION_1_1
  else if(pointerPy == Py_None) {
    tcptr = NULL;
  }
#endif
  // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    if(pointerPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    // glTexCoordPointer accepts only GL_BYTE, GL_SHORT, GL_FIXED or GL_FLOAT types
    switch(type) {
      case GL_BYTE:
        tcptr = gles_PySequence_AsGLbyteArray(pointerPy);
        break;
      case GL_SHORT:
        tcptr = gles_PySequence_AsGLshortArray(pointerPy);
        break;
      case GL_FIXED:
        tcptr = gles_PySequence_AsGLfixedArray(pointerPy);
        break;
      case GL_FLOAT:
        tcptr = gles_PySequence_AsGLfloatArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Invalid array type");
        return NULL;
    }
    if(tcptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
#ifndef GL_OES_VERSION_1_1
  if(tcptr == NULL) {
    // Something went horribly wrong
    // However, we should have an exception already
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
#endif
  gles_assign_array(GL_TEXTURE_COORD_ARRAY, tcptr, arrobj);
  glTexCoordPointer(size, type, stride, tcptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexCoordPointer(PyObject *self, PyObject *args) {
  GLint size;
  GLenum type;
  GLsizei stride = 0;
  
  PyObject *pointerPy;
  if ( !PyArg_ParseTuple(args, "iiiO:glTexCoordPointer", &size, &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_TEXTURE_COORD_ARRAY);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glTexCoordPointer(size, type, stride, pointerPy);
}

extern "C" PyObject *gles_glTexCoordPointerb(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glTexCoordPointerb", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_TEXTURE_COORD_ARRAY);
    RETURN_PYNONE;
  }
  
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  return gles_wrap_glTexCoordPointer(size, /*type*/GL_BYTE, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glTexCoordPointers(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glTexCoordPointers", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_TEXTURE_COORD_ARRAY);
    RETURN_PYNONE;
  }
  
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  return gles_wrap_glTexCoordPointer(size, /*type*/GL_SHORT, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glTexCoordPointerf(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glTexCoordPointerf", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_TEXTURE_COORD_ARRAY);
    RETURN_PYNONE;
  }
  
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  return gles_wrap_glTexCoordPointer(size, /*type*/GL_FLOAT, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glTexCoordPointerx(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glTexCoordPointerx", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_TEXTURE_COORD_ARRAY);
    RETURN_PYNONE;
  }
  
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  return gles_wrap_glTexCoordPointer(size, /*type*/GL_FIXED, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glTexEnvf(PyObject *self, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfloat param;
  
  if( !PyArg_ParseTuple(args, "iif:glTexEnvf", &target, &pname, &param)) {
    return NULL;
  }
  glTexEnvf(target, pname, param);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexEnvfv (PyObject *self, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfloat *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iiO:glTexEnvfv", &face, &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfloat*)gles_PySequence_AsGLfloatArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glTexEnvfv(face, pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexEnvx(PyObject *self, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfixed param;
  
  if( !PyArg_ParseTuple(args, "iii:glTexEnvx", &target, &pname, &param)) {
    return NULL;
  }
  glTexEnvx(target, pname, param);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexEnvxv (PyObject *self, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfixed *params;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "iiO:glTexEnvfv", &face, &pname, &paramsPy) ) {
    return NULL;
  }
  
  params = (GLfixed*)gles_PySequence_AsGLfixedArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  glTexEnvxv(face, pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, PyObject *pixelsPy) {
  GLvoid *pixels;
  
  // Indicates if the pixel data should be freed after the call to glTexImage2D.
  int free_pixels = 0;
  
  if( PyObject_TypeCheck(pixelsPy, GLES_ARRAY_TYPE) ) {
    array_object *arr = (array_object*)pixelsPy;
    pixels = arr->arrdata;
  } else if( PyString_Check(pixelsPy) ) {
    // "Convert" the string object into an array.
    // The returned pointer points to the raw C string
    // data held by the object, so no actual conversion needed.
    pixels = PyString_AsString(pixelsPy);
    if(pixels == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  } // The last resort is to check if the given object is an Image object
  else {
    CFbsBitmap *bitmap=Bitmap_AsFbsBitmap(pixelsPy);
#ifdef DEBUG_GLES
    DEBUGMSG1("Image: %x", bitmap);
#endif
    if(bitmap == NULL) {
      PyErr_SetString(PyExc_TypeError, "Unsupported object type");
      return NULL;
    }
    free_pixels = 1;
    pixels = gles_convert_fbsbitmap(bitmap, format, type, NULL);
    if(pixels == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
  glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels );
  if(free_pixels == 1) {
    gles_free(pixels);
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexImage2D(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLint internalformat;
  GLsizei width;
  GLsizei height;
  GLint border;
  GLenum format;
  GLenum type;
  
  PyObject *pixelsPy = NULL;
  
  if( !PyArg_ParseTuple(args, "iiiiiiiiO:glTexImage2D", &target, &level, &internalformat, &width, &height, &border, &format, &type, &pixelsPy) ) {
    return NULL;
  }
  
  return gles_wrap_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixelsPy);
}

extern "C" PyObject *gles_glTexImage2DIO(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLsizei width;
  GLsizei height;
  GLint border;
  GLenum format;
  GLenum type;
  PyObject *image_object;
  TSize imgsize;
  
  if( !PyArg_ParseTuple(args, "iiiiiO:glTexImage2DIO", &target, &level, &format, &border, &type, &image_object) ) {
    return NULL;
  }
  
  CFbsBitmap *bitmap=Bitmap_AsFbsBitmap(image_object);
  if(bitmap == NULL) {
    PyErr_SetString(PyExc_TypeError, "Expecting a graphics.Image object");
    return NULL;
  }
  
  imgsize = bitmap->SizeInPixels();
  width = imgsize.iWidth;
  height = imgsize.iHeight;
  
  return gles_wrap_glTexImage2D(target, level, /*internalformat*/format, width, height, border, format, type, image_object);
}

extern "C" PyObject *gles_glTexParameterf(PyObject *self, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfloat param;
  
  if ( !PyArg_ParseTuple(args, "iif:glTexParameterf", &target, &pname, &param) ) {
    return NULL;
  }
  glTexParameterf(target, pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexParameterx(PyObject *self, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfixed param;
  
  if ( !PyArg_ParseTuple(args, "iii:glTexParameterx", &target, &pname, &param) ) {
    return NULL;
  }
  glTexParameterx(target, pname, param);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, PyObject *pixelsPy) {
  GLvoid *pixels;
  
  // Indicates if the pixel data should be freed after the call to glTexSubImage2D.
  int free_pixels = 0;
  
  if( PyObject_TypeCheck(pixelsPy, GLES_ARRAY_TYPE) ) {
    array_object *arr = (array_object*)pixelsPy;
    pixels = arr->arrdata;
  } else if( PyString_Check(pixelsPy) ) {
    // "Convert" the string object into an array.
    // The returned pointer points to the raw C string
    // data held by the object, so no actual conversion needed.
    pixels = PyString_AsString(pixelsPy);
    if(pixels == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  } // The last resort is to check if the given object is an Image object
  else {
    CFbsBitmap *bitmap=Bitmap_AsFbsBitmap(pixelsPy);
#ifdef DEBUG_GLES
    DEBUGMSG1("Image: %x", bitmap);
#endif
    if(bitmap == NULL) {
      PyErr_SetString(PyExc_TypeError, "Unsupported object type");
      return NULL;
    }
    free_pixels = 1;
    pixels = gles_convert_fbsbitmap(bitmap, format, type, NULL);
    if(pixels == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
  glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels );
  if(free_pixels == 1) {
    gles_free(pixels);
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTexSubImage2D(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLint xoffset;
  GLint yoffset;
  GLsizei width;
  GLsizei height;
  GLenum format;
  GLenum type;
  PyObject *pixelsPy;
  
  if( !PyArg_ParseTuple(args, "iiiiiiiiO:glTexSubImage2D", &target, &level, &xoffset, &yoffset, &width, &height, &format, &type, &pixelsPy) ) {
    return NULL;
  }
  
  return gles_wrap_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixelsPy);
}

extern "C" PyObject *gles_glTexSubImage2DIO(PyObject *self, PyObject *args) {
  GLenum target;
  GLint level;
  GLint xoffset;
  GLint yoffset;
  GLsizei width;
  GLsizei height;
  GLenum format;
  GLenum type;
  PyObject *image_object;
  TSize imgsize;
  
  if( !PyArg_ParseTuple(args, "iiiiiiO:glTexSubImage2D", &target, &level, &xoffset, &yoffset, &format, &type, &image_object) ) {
    return NULL;
  }
  
  CFbsBitmap *bitmap=Bitmap_AsFbsBitmap(image_object);
  if(bitmap == NULL) {
    PyErr_SetString(PyExc_TypeError, "Expecting a graphics.Image object");
    return NULL;
  }
  
  imgsize = bitmap->SizeInPixels();
  width = imgsize.iWidth;
  height = imgsize.iHeight;
  
  return gles_wrap_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, image_object);
}

extern "C" PyObject *gles_glTranslatex (PyObject *self, PyObject *args) {
  GLfixed x;
  GLfixed y;
  GLfixed z;
  
  if ( !PyArg_ParseTuple(args, "iii:glTranslatex", &x, &y, &z) ) {
    return NULL;
  }
  glTranslatex(x, y, z);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glTranslatef(PyObject *self, PyObject *args) {
  GLfloat x;
  GLfloat y;
  GLfloat z;
  
  if ( !PyArg_ParseTuple(args, "fff:glTranslatef", &x, &y, &z) ) {
    return NULL;
  }
  glTranslatef(x, y, z);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glViewport (PyObject *self, PyObject *args) {
  GLint x;
  GLint y;
  GLsizei width;
  GLsizei height;
  
  if ( !PyArg_ParseTuple(args, "iiii:glViewport", &x, &y, &width, &height) ) {
    return NULL;
  }
  glViewport(x, y, width, height);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glVertexPointer(GLint size, GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *vptr = NULL;
  array_object *arrobj=NULL;
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    vptr = arrobj->arrdata;
  }
#ifdef GL_OES_VERSION_1_1
  else if(pointerPy == Py_None) {
    vptr = NULL;
  }
#endif
  // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    // glVertexPointer accepts only GL_BYTE, GL_SHORT, GL_FIXED or GL_FLOAT types
    switch(type) {
      case GL_BYTE:
        vptr = gles_PySequence_AsGLbyteArray(pointerPy);
        break;
      case GL_SHORT:
        vptr = gles_PySequence_AsGLshortArray(pointerPy);
        break;
      case GL_FIXED:
        vptr = gles_PySequence_AsGLfixedArray(pointerPy);
        break;
      case GL_FLOAT:
        vptr = gles_PySequence_AsGLfloatArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Invalid array type");
        return NULL;
    }
    if(vptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
#ifndef GL_OES_VERSION_1_1
  if(vptr == NULL) {
    // Something went horribly wrong
    // However, we should have an exception already
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
#endif
  gles_assign_array(GL_VERTEX_ARRAY, vptr, arrobj);
  glVertexPointer(size, type, stride, vptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glVertexPointer(PyObject *self, PyObject *args) {
  GLint size;
  GLenum type;
  GLsizei stride;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "iiiO:glVertexPointer", &size, &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  return gles_wrap_glVertexPointer(size, type, stride, pointerPy);
}

extern "C" PyObject *gles_glVertexPointerb(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glVertexPointerb", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_VERTEX_ARRAY);
    RETURN_PYNONE;
  }
  
  // We can use either the array type or a sequence.
  // Check the array type first.
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } // Try to feed the object as a sequence.
  else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glVertexPointer(size, /*type*/GL_BYTE, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glVertexPointers(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glVertexPointers", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_VERTEX_ARRAY);
    RETURN_PYNONE;
  }
  
  // We can use either the array type or a sequence.
  // Check the array type first.
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } // Try to feed the object as a sequence.
  else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glVertexPointer(size, /*type*/GL_SHORT, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glVertexPointerx(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glVertexPointerx", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_VERTEX_ARRAY);
    RETURN_PYNONE;
  }
  
  // We can use either the array type or a sequence.
  // Check the array type first.
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } // Try to feed the object as a sequence.
  else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glVertexPointer(size, /*type*/GL_FIXED, /*stride*/0, pointerPy);
}

extern "C" PyObject *gles_glVertexPointerf(PyObject *self, PyObject *args) {
  GLint size;
  PyObject *pointerPy;
  
  if ( !PyArg_ParseTuple(args, "O:glVertexPointerf", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_VERTEX_ARRAY);
    RETURN_PYNONE;
  }
  
  // We can use either the array type or a sequence.
  // Check the array type first.
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } // Try to feed the object as a sequence.
  else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glVertexPointer(size, /*type*/GL_FLOAT, /*stride*/0, pointerPy);
}

// Functions which belong only to OpenGL ES 1.1 should go here
#ifdef GL_OES_VERSION_1_1
extern "C" PyObject *gles_glBindBuffer(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLuint buffer;
  
  if( !PyArg_ParseTuple(args,"ii:glBindBuffer", &target, &buffer)) {
    return NULL;
  }
  
  glBindBuffer(target, buffer);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glBufferData(GLenum type, GLenum target, GLsizeiptr size, PyObject *dataPy, GLenum usage) {
  GLvoid *data = NULL;
  array_object *arrobj = NULL;
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)dataPy;
    data = arrobj->arrdata;
    
    assert(data != NULL);
    
    if(size == -1) {
      size = arrobj->real_len;
    }
  } // Try to convert it as a sequence
  else {
    if(type == 0) {
      // Type not specified and cannot be determined
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine data type");
      return NULL;
    }
    GLsizeiptr size2=0;
    dataPy = gles_PySequence_Collapse(type, dataPy, NULL);
    switch(type) {
      case GL_BYTE:
        size2 = PySequence_Length(dataPy) * sizeof(GLbyte);
        data = gles_PySequence_AsGLbyteArray(dataPy);
        break;
      case GL_UNSIGNED_BYTE:
        size2 = PySequence_Length(dataPy) * sizeof(GLubyte);
        data = gles_PySequence_AsGLubyteArray(dataPy);
        break;
      case GL_SHORT:
        size2 = PySequence_Length(dataPy) * sizeof(GLshort);
        data = gles_PySequence_AsGLshortArray(dataPy);
        break;
      case GL_UNSIGNED_SHORT:
        size2 = PySequence_Length(dataPy) * sizeof(GLushort);
        data = gles_PySequence_AsGLushortArray(dataPy);
        break;
      case GL_FIXED:
        size2 = PySequence_Length(dataPy) * sizeof(GLfixed);
        data = gles_PySequence_AsGLfixedArray(dataPy);
        break;
      case GL_FLOAT:
        size2 = PySequence_Length(dataPy) * sizeof(GLfloat);
        data = gles_PySequence_AsGLfloatArray(dataPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Invalid array type");
        return NULL;
        break;
    }
    if(size == -1) {
      size = size2;
    }
  }
  
  glBufferData(target, size, data, usage);
  if(arrobj == NULL) {
    gles_free(data);
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glBufferData(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiOi:glBufferData", &target, &size, &dataPy, &usage)) {
    return NULL;
  }
  
  if( !PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    PyErr_SetString(PyExc_TypeError, "Expecting a gles.array type");
    return NULL;
  }
  
  return gles_wrap_glBufferData(NULL, target, size, dataPy, usage);
}

extern "C" PyObject *gles_glBufferDatab(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iOi:glBufferDatab", &target, &dataPy, &usage)) {
    return NULL;
  }
  
  // Only array objects or sequences here
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_BYTE, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLbyte);
  }
  
  return gles_wrap_glBufferData(GL_BYTE, target, size, dataPy, usage);
}

extern "C" PyObject *gles_glBufferDataub(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iOi:glBufferDataub", &target, &dataPy, &usage)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_UNSIGNED_BYTE, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLubyte);
  }
  
  return gles_wrap_glBufferData(GL_UNSIGNED_BYTE, target, size, dataPy, usage);
}

extern "C" PyObject *gles_glBufferDatas(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iOi:glBufferDatas", &target, &dataPy, &usage)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_SHORT, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLshort);
  }
  
  return gles_wrap_glBufferData(GL_SHORT, target, size, dataPy, usage);
}

extern "C" PyObject *gles_glBufferDataus(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iOi:glBufferDataus", &target, &dataPy, &usage)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_UNSIGNED_SHORT, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLushort);
  }
  
  return gles_wrap_glBufferData(GL_UNSIGNED_SHORT, target, size, dataPy, usage);
}



extern "C" PyObject *gles_glBufferDataf(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iOi:glBufferDataf", &target, &dataPy, &usage)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_FLOAT, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLfloat);
  }
  
  return gles_wrap_glBufferData(GL_FLOAT, target, size, dataPy, usage);
}

extern "C" PyObject *gles_glBufferDatax(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLsizeiptr size;
  //GLvoid *data;
  GLenum usage;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iOi:glBufferDatax", &target, &dataPy, &usage)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_FIXED, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLfixed);
  }
  

  
  return gles_wrap_glBufferData(GL_FIXED, target, size, dataPy, usage);
}

PyObject *gles_wrap_glBufferSubData(GLenum type, GLenum target, GLintptr offset, GLsizeiptr size, PyObject *dataPy) {
  GLvoid *data = NULL;
  array_object *arrobj = NULL;
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)dataPy;
    data = arrobj->arrdata;
    
    assert(data != NULL);
    
    if(size == -1) {
      size = arrobj->real_len;
    }
  } // Try to convert it as a sequence
  else {
    if(type == 0) {
      // Type not specified and cannot be determined
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine data type");
      return NULL;
    }
    GLsizeiptr size2=0;
    dataPy = gles_PySequence_Collapse(type, dataPy, NULL);
    switch(type) {
      case GL_BYTE:
        size2 = PySequence_Length(dataPy) * sizeof(GLbyte);
        data = gles_PySequence_AsGLbyteArray(dataPy);
        break;
      case GL_UNSIGNED_BYTE:
        size2 = PySequence_Length(dataPy) * sizeof(GLubyte);
        data = gles_PySequence_AsGLubyteArray(dataPy);
        break;
      case GL_SHORT:
        size2 = PySequence_Length(dataPy) * sizeof(GLshort);
        data = gles_PySequence_AsGLshortArray(dataPy);
        break;
      case GL_UNSIGNED_SHORT:
        size2 = PySequence_Length(dataPy) * sizeof(GLushort);
        data = gles_PySequence_AsGLushortArray(dataPy);
        break;
      case GL_FIXED:
        size2 = PySequence_Length(dataPy) * sizeof(GLfixed);
        data = gles_PySequence_AsGLfixedArray(dataPy);
        break;
      case GL_FLOAT:
        size2 = PySequence_Length(dataPy) * sizeof(GLfloat);
        data = gles_PySequence_AsGLfloatArray(dataPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Invalid array type");
        return NULL;
        break;
    }
    if(size == -1) {
      size = size2;
    }
  }
  
  glBufferSubData(target, offset, size, data);
  if(arrobj == NULL) {
    gles_free(data);
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glBufferSubData(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubData", &target, &offset, &size, &dataPy)) {
    return NULL;
  }
  
  if( !PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    PyErr_SetString(PyExc_TypeError, "Expecting a gles.array type");
    return NULL;
  }
  
  return gles_wrap_glBufferSubData(NULL, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glBufferSubDatab(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubDatab", &target, &offset, &dataPy)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_BYTE, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLbyte);
  }
  

  
  return gles_wrap_glBufferSubData(GL_BYTE, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glBufferSubDataub(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubDataub", &target, &offset, &dataPy)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_UNSIGNED_BYTE, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLubyte);
  }
  
  return gles_wrap_glBufferSubData(GL_UNSIGNED_BYTE, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glBufferSubDatas(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubDatas", &target, &offset, &dataPy)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_SHORT, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLshort);
  }
  
  
  
  return gles_wrap_glBufferSubData(GL_SHORT, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glBufferSubDataus(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubDataus", &target, &offset, &dataPy)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_UNSIGNED_SHORT, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLushort);
  }
  
  
  
  return gles_wrap_glBufferSubData(GL_UNSIGNED_SHORT, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glBufferSubDataf(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubDataf", &target, &offset, &dataPy)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_FLOAT, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLfloat);
  }
  
  
  
  return gles_wrap_glBufferSubData(GL_FLOAT, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glBufferSubDatax(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLintptr offset;
  GLsizeiptr size;
  PyObject *dataPy = NULL;
  
  if ( !PyArg_ParseTuple(args, "iiO:glBufferSubDatax", &target, &offset, &dataPy)) {
    return NULL;
  }
  
  if(PyObject_TypeCheck(dataPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)dataPy)->real_len;
  } // Try to feed the object as a sequence.
  else {
    // Expecting a 1D sequence
    dataPy = gles_PySequence_Collapse(GL_FIXED, dataPy, NULL);
    if(dataPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    size = PySequence_Length(dataPy)*sizeof(GLfixed);
  }
  
  return gles_wrap_glBufferSubData(GL_FIXED, target, offset, size, dataPy);
}

extern "C" PyObject *gles_glClipPlanef(PyObject */*self*/, PyObject *args) {
  GLenum plane;
  GLfloat *equation = NULL;
  PyObject *equationPy = NULL;
  
  if( !PyArg_ParseTuple(args, "iO:glClipPlanef", &plane, equationPy)) {
    return NULL;
  }
  
  if(! PySequence_Check(equationPy) || PySequence_Length(equationPy) != 4) {
    PyErr_SetString(PyExc_ValueError,"Expecting a sequence of 4 values");
    return NULL;
  }
  
  equation = gles_PySequence_AsGLfloatArray(equationPy);
  if(equation == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glClipPlanef(plane, equation);
  gles_free(equation);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glClipPlanex(PyObject */*self*/, PyObject *args) {
  GLenum plane;
  GLfixed *equation = NULL;
  PyObject *equationPy = NULL;
  
  if( !PyArg_ParseTuple(args, "iO:glClipPlanex", &plane, equationPy)) {
    return NULL;
  }
  
  if(! PySequence_Check(equationPy) || PySequence_Length(equationPy) != 4) {
    PyErr_SetString(PyExc_ValueError,"Expecting a sequence of 4 values");
    return NULL;
  }
  
  equation = gles_PySequence_AsGLfixedArray(equationPy);
  if(equation == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glClipPlanex(plane, equation);
  gles_free(equation);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glCurrentPaletteMatrixOES(PyObject */*self*/, PyObject *args) {
  GLuint index;
  
  if( !PyArg_ParseTuple(args, "i:glCurrentPaletteMatrixOES", &index)) {
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glCurrentPaletteMatrixOES");
  if(func) {
    func(index);
  } else {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDeleteBuffers(PyObject */*self*/, PyObject *args) {
  GLsizei n;
  GLuint *buffers = NULL;
  PyObject *buffersPy;
  
  if( !PyArg_ParseTuple(args, "O:glDeleteBuffers", &buffersPy)) {
    return NULL;
  }
  
  n = PySequence_Length(buffersPy);
  buffers = gles_PySequence_AsGLuintArray(buffersPy);
  if(buffers == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glDeleteBuffers(n, buffers);
  gles_free(buffers);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexsOES(PyObject */*self*/, PyObject *args) {
  GLshort x;
  GLshort y;
  GLshort z;
  GLshort width;
  GLshort height;
  
  if( !PyArg_ParseTuple(args, "iiiii:glDrawTexsOES", &x, &y, &z, &width, &height)) {
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexsOES");
  if(func) {
    func(x,y,z,width,height);
  } else {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexiOES(PyObject */*self*/, PyObject *args) {
  GLint x;
  GLint y;
  GLint z;
  GLint width;
  GLint height;
  
  if( !PyArg_ParseTuple(args, "iiiii:glDrawTexiOES", &x, &y, &z, &width, &height)) {
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexiOES");
  if(func) {
    func(x,y,z,width,height);
  } else {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexfOES(PyObject */*self*/, PyObject *args) {
  GLfloat x;
  GLfloat y;
  GLfloat z;
  GLfloat width;
  GLfloat height;
  
  if( !PyArg_ParseTuple(args, "fffff:glDrawTexfOES", &x, &y, &z, &width, &height)) {
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexfOES");
  if(func) {
    func(x,y,z,width,height);
  } else {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexxOES(PyObject */*self*/, PyObject *args) {
  GLfixed x;
  GLfixed y;
  GLfixed z;
  GLfixed width;
  GLfixed height;
  
  if( !PyArg_ParseTuple(args, "iiiii:glDrawTexxOES", &x, &y, &z, &width, &height)) {
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexxOES");
  if(func) {
    func(x,y,z,width,height);
  } else {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexsvOES(PyObject */*self*/, PyObject *args) {
  GLshort *coords;
  PyObject *coordsPy;
  
  if( !PyArg_ParseTuple(args, "O", &coordsPy)) {
    return NULL;
  }
  
  coordsPy = gles_PySequence_Collapse(GL_SHORT, coordsPy, NULL);
  if(coordsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  coords = gles_PySequence_AsGLshortArray(coordsPy);
  if(coords == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexsvOES");
  if(func) {
    func(coords);
  } else {
    gles_free(coords);
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  gles_free(coords);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexivOES(PyObject */*self*/, PyObject *args) {
  GLint *coords = NULL;
  PyObject *coordsPy = NULL;
  
  if( !PyArg_ParseTuple(args, "O", &coordsPy)) {
    return NULL;
  }
  
  coordsPy = gles_PySequence_Collapse(GL_SHORT, coordsPy, NULL);
  if(coordsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  coords = gles_PySequence_AsGLintArray(coordsPy);
  if(coords == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexivOES");
  if(func) {
    func(coords);
  } else {
    gles_free(coords);
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  gles_free(coords);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexfvOES(PyObject */*self*/, PyObject *args) {
  GLfloat *coords;
  PyObject *coordsPy;
  
  if( !PyArg_ParseTuple(args, "O", &coordsPy)) {
    return NULL;
  }
  
  coordsPy = gles_PySequence_Collapse(GL_FLOAT, coordsPy, NULL);
  if(coordsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  coords = gles_PySequence_AsGLfloatArray(coordsPy);
  if(coords == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexfvOES");
  if(func) {
    func(coords);
  } else {
    gles_free(coords);
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  gles_free(coords);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glDrawTexxvOES(PyObject */*self*/, PyObject *args) {
  GLfixed *coords;
  PyObject *coordsPy;
  
  if( !PyArg_ParseTuple(args, "O", &coordsPy)) {
    return NULL;
  }
  
  coordsPy = gles_PySequence_Collapse(GL_FIXED, coordsPy, NULL);
  if(coordsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  coords = gles_PySequence_AsGLfixedArray(coordsPy);
  if(coords == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glDrawTexxvOES");
  if(func) {
    func(coords);
  } else {
    gles_free(coords);
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  gles_free(coords);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glGenBuffers(PyObject *self, PyObject *args) {
  GLuint *buffers = NULL;
  GLuint n;
  unsigned int i;
  PyObject *ret;
  
  if( !PyArg_ParseTuple(args, "i:glGenBuffers", &n) ) {
    return NULL;
  }
  
  if(n < 1) {
    PyErr_SetString(PyExc_ValueError, "Value must be positive");
    return NULL;
  }
  
  buffers = (GLuint*)gles_alloc( sizeof(GLuint)*n );
  
  glGenBuffers( n, buffers);
  ret = PyTuple_New(n);
  
  if(n > 1) {
    ret = PyTuple_New(n);
  
    for(i=0;i<n;i++) {
      PyTuple_SetItem(ret, i, Py_BuildValue("i", buffers[i]));
    }
  } else {
    ret = Py_BuildValue("i", buffers[0]);
  }
  gles_free(buffers);
  
  RETURN_IF_GLERROR;
  Py_INCREF(ret);
  return ret;
}

extern "C" PyObject *gles_glGetBooleanv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLboolean *params = NULL;
  int values = 1;
  int i;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "i:glGetBooleanv", &pname) ) {
    return NULL;
  }
  
  switch(pname) {
    // 2 elements
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
      values = 2;
      break;
    // Special cases
    case GL_COMPRESSED_TEXTURE_FORMATS:
      glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &values);
      break;
    // 1 element
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_DEPTH_BITS:
    case GL_GREEN_BITS:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_MAX_ELEMENTS_INDICES:
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_LIGHTS:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MAX_TEXTURE_UNITS:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_RED_BITS:
    case GL_STENCIL_BITS:
    case GL_SUBPIXEL_BITS:
    default:
      values = 1;
      break;
  }
  
  params = (GLboolean*)gles_alloc(values*sizeof(GLboolean));
  if(params == NULL) {
    return PyErr_NoMemory();
  }
  glGetBooleanv(pname, params);
  
  paramsPy = PyTuple_New(values);
  if(paramsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    gles_free(params);
    return NULL;
  }
  
  for(i = 0; i < values; i++) {
    if(params[i]) {
      PyTuple_SetItem(paramsPy, i, Py_True);
    } else {
      PyTuple_SetItem(paramsPy, i, Py_False);
    }
  }
  
  gles_free(params);
  RETURN_IF_GLERROR;
  return paramsPy;
}

extern "C" PyObject *gles_glGetBufferParameteriv(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLint *params = NULL;
  PyObject *paramsPy = NULL;
  
  if( !PyArg_ParseTuple(args, "i:glGetBufferParameteriv", &target, &pname)) {
    return NULL;
  }
  
  params = (GLint*)gles_alloc( sizeof(GLint) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  glGetBufferParameteriv(target, pname, params);
  
  paramsPy = Py_BuildValue("i", *params);
  gles_free(params);
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetClipPlanef(PyObject */*self*/, PyObject *args) {
  GLenum plane;
  GLfloat *equation = NULL;
  PyObject *equationPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "i:glGetClipPlanef", &plane)) {
    return NULL;
  }
  
  equation = (GLfloat*)gles_alloc( 4 * sizeof(GLfloat) );
  if( equation == NULL) {
    return PyErr_NoMemory();
  }
  
  glClipPlanef(plane, equation);
  equationPy = PyTuple_New(4);
  for(i = 0;i < 4;i++) {
    PyTuple_SetItem(equationPy, i, Py_BuildValue("f", equation[i]));
  }
  gles_free(equation);
  
  RETURN_IF_GLERROR;
  Py_INCREF(equationPy);
  return equationPy;
}

extern "C" PyObject *gles_glGetClipPlanex(PyObject */*self*/, PyObject *args) {
  GLenum plane;
  GLfixed *equation = NULL;
  PyObject *equationPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "i:glGetClipPlanef", &plane)) {
    return NULL;
  }
  
  equation = (GLfixed*)gles_alloc( 4 * sizeof(GLfixed) );
  if( equation == NULL) {
    return PyErr_NoMemory();
  }
  
  glClipPlanex(plane, equation);
  equationPy = PyTuple_New(4);
  for(i = 0;i < 4;i++) {
    PyTuple_SetItem(equationPy, i, Py_BuildValue("i", equation[i]));
  }
  gles_free(equation);
  
  RETURN_IF_GLERROR;
  Py_INCREF(equationPy);
  return equationPy;
}

extern "C" PyObject *gles_glGetFixedv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfixed *params = NULL;
  int values = 1;
  int i;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "i:glGetFixedv", &pname) ) {
    return NULL;
  }
  
  switch(pname) {
    // 2 elements
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
      values = 2;
      break;
    // Special cases
    case GL_COMPRESSED_TEXTURE_FORMATS:
      glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &values);
      break;
    // 1 element
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_DEPTH_BITS:
    case GL_GREEN_BITS:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_MAX_ELEMENTS_INDICES:
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_LIGHTS:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MAX_TEXTURE_UNITS:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_RED_BITS:
    case GL_STENCIL_BITS:
    case GL_SUBPIXEL_BITS:
    default:
      values = 1;
      break;
  }
  
  params = (GLfixed*)gles_alloc(values*sizeof(GLfixed));
  if(params == NULL) {
    return PyErr_NoMemory();
  }
  glGetFixedv(pname, params);
  
  paramsPy = PyTuple_New(values);
  if(paramsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    gles_free(params);
    return NULL;
  }
  
  for(i = 0; i < values; i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("i", params[i]));
  }
  
  gles_free(params);
  RETURN_IF_GLERROR;
  return paramsPy;
}

extern "C" PyObject *gles_glGetFloatv(PyObject *self, PyObject *args) {
  GLenum pname;
  GLfloat *params = NULL;
  int values = 1;
  int i;
  
  PyObject *paramsPy;
  if ( !PyArg_ParseTuple(args, "i:glGetFloatv", &pname) ) {
    return NULL;
  }
  
  switch(pname) {
    // 2 elements
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
      values = 2;
      break;
    // Special cases
    case GL_COMPRESSED_TEXTURE_FORMATS:
      glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &values);
      break;
    // 1 element
    case GL_ALPHA_BITS:
    case GL_BLUE_BITS:
    case GL_DEPTH_BITS:
    case GL_GREEN_BITS:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_MAX_ELEMENTS_INDICES:
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_LIGHTS:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MAX_TEXTURE_UNITS:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_RED_BITS:
    case GL_STENCIL_BITS:
    case GL_SUBPIXEL_BITS:
    default:
      values = 1;
      break;
  }
  
  params = (GLfloat*)gles_alloc(values*sizeof(GLfloat));
  if(params == NULL) {
    return PyErr_NoMemory();
  }
  glGetFloatv(pname, params);
  
  paramsPy = PyTuple_New(values);
  if(paramsPy == NULL) {
    assert(PyErr_Occurred() != NULL);
    gles_free(params);
    return NULL;
  }
  
  for(i = 0; i < values; i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("f", params[i]));
  }
  
  gles_free(params);
  RETURN_IF_GLERROR;
  return paramsPy;
}

extern "C" PyObject *gles_glGetLightfv(PyObject */*self*/, PyObject *args) {
  GLenum light;
  GLenum pname;
  GLfloat *params = NULL;
  GLsizei size;
  PyObject *paramsPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "ii:glGetLightfv", &light, &pname)) {
    return NULL;
  }
  
  switch(pname) {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
      size = 4;
      break;
    case GL_SPOT_DIRECTION:
      size = 3;
      break;
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
      size = 1;
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine result size");
      return NULL;
  }
  
  params = (GLfloat*)gles_alloc( size * sizeof(GLfloat) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetLightfv(light, pname, params);
  paramsPy = PyTuple_New(4);
  for(i = 0;i < size;i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("f", params[i]));
  }
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetLightxv(PyObject */*self*/, PyObject *args) {
  GLenum light;
  GLenum pname;
  GLfixed *params = NULL;
  GLsizei size;
  PyObject *paramsPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "ii:glGetLightxv", &light, &pname)) {
    return NULL;
  }
  
  switch(pname) {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
      size = 4;
      break;
    case GL_SPOT_DIRECTION:
      size = 3;
      break;
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
      size = 1;
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine result size");
      return NULL;
  }
  
  params = (GLfixed*)gles_alloc( size * sizeof(GLfixed) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetLightxv(light, pname, params);
  paramsPy = PyTuple_New(4);
  for(i = 0;i < size;i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("i", params[i]));
  }
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetMaterialfv(PyObject */*self*/, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfloat *params = NULL;
  GLsizei size;
  PyObject *paramsPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "ii:glGetMaterialfv", &face, &pname)) {
    return NULL;
  }
  
  switch(pname) {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
      size = 4;
      break;
    case GL_SHININESS:
      size = 1;
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine result size");
      return NULL;
  }
  
  params = (GLfloat*)gles_alloc( size * sizeof(GLfloat) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetMaterialfv(face, pname, params);
  paramsPy = PyTuple_New(4);
  for(i = 0;i < size;i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("f", params[i]));
  }
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetMaterialxv(PyObject */*self*/, PyObject *args) {
  GLenum face;
  GLenum pname;
  GLfixed *params = NULL;
  GLsizei size;
  PyObject *paramsPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "ii:glGetMaterialxv", &face, &pname)) {
    return NULL;
  }
  
  switch(pname) {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
      size = 4;
      break;
    case GL_SHININESS:
      size = 1;
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine result size");
      return NULL;
  }
  
  params = (GLfixed*)gles_alloc( size * sizeof(GLfixed) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetMaterialxv(face, pname, params);
  paramsPy = PyTuple_New(4);
  for(i = 0;i < size;i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("i", params[i]));
  }
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

// glGetPointerv not implemented

extern "C" PyObject *gles_glGetTexEnvf(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfloat *params = NULL;
  GLsizei size;
  PyObject *paramsPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "ii:glGetTexEnvf", &target, &pname)) {
    return NULL;
  }
  
  switch(pname) {
    case GL_TEXTURE_ENV_COLOR:
      size = 4;
      break;
    case GL_TEXTURE_ENV_MODE:
      size = 1;
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine result size");
      return NULL;
  }
  
  params = (GLfloat*)gles_alloc( size * sizeof(GLfloat) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetTexEnvfv(target, pname, params);
  paramsPy = PyTuple_New(4);
  for(i = 0;i < size;i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("f", params[i]));
  }
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetTexEnvx(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfixed *params = NULL;
  GLsizei size;
  PyObject *paramsPy = NULL;
  int i;
  
  if( !PyArg_ParseTuple(args, "ii:glGetTexEnvx", &target, &pname)) {
    return NULL;
  }
  
  switch(pname) {
    case GL_TEXTURE_ENV_COLOR:
      size = 4;
      break;
    case GL_TEXTURE_ENV_MODE:
      size = 1;
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "Cannot determine result size");
      return NULL;
  }
  
  params = (GLfixed*)gles_alloc( size * sizeof(GLfixed) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetTexEnvxv(target, pname, params);
  paramsPy = PyTuple_New(4);
  for(i = 0;i < size;i++) {
    PyTuple_SetItem(paramsPy, i, Py_BuildValue("i", params[i]));
  }
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetTexParameterf(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfloat *params = NULL;
  PyObject *paramsPy = NULL;
  
  if( !PyArg_ParseTuple(args, "ii:glGetTexParameterf", &target, &pname)) {
    return NULL;
  }
  
  params = (GLfloat*)gles_alloc( sizeof(GLfloat) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetTexParameterfv(target, pname, params);
  paramsPy = Py_BuildValue("f", params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glGetTexParameterx(PyObject */*self*/, PyObject *args) {
  GLenum target;
  GLenum pname;
  GLfixed *params = NULL;
  PyObject *paramsPy = NULL;
  
  if( !PyArg_ParseTuple(args, "ii:glGetTexParameterx", &target, &pname)) {
    return NULL;
  }
  
  params = (GLfixed*)gles_alloc( sizeof(GLfixed) );
  if( params == NULL) {
    return PyErr_NoMemory();
  }
  
  glGetTexParameterxv(target, pname, params);
  paramsPy = Py_BuildValue("i", params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  Py_INCREF(paramsPy);
  return paramsPy;
}

extern "C" PyObject *gles_glIsBuffer(PyObject */*self*/, PyObject *args) {
  GLuint buffer;
  GLboolean result;
  
  if( !PyArg_ParseTuple(args, "i:glIsBuffer", &buffer)) {
    return NULL;
  }
  
  result = glIsBuffer(buffer);
  
  RETURN_IF_GLERROR;
  
  if(result == GL_TRUE) {
    RETURN_PYTRUE;
  }
  RETURN_PYFALSE;
}

extern "C" PyObject *gles_glIsEnabled (PyObject *self, PyObject *args) {
  int cap;
  GLboolean result;
  
  if ( !PyArg_ParseTuple(args, "i:glIsEnabled", &cap) ) {
    return NULL;
  }
  
  result = glIsEnabled( cap );
  
  RETURN_IF_GLERROR;
  
  if( result == GL_TRUE ) {
    RETURN_PYTRUE;
  }
  RETURN_PYFALSE;
}

extern "C" PyObject *gles_glIsTexture(PyObject */*self*/, PyObject *args) {
  GLuint texture;
  GLboolean result;
  
  if( !PyArg_ParseTuple(args, "i:glIsTexture", &texture)) {
    return NULL;
  }
  
  result = glIsTexture(texture);
  
  RETURN_IF_GLERROR;
  
  if(result == GL_TRUE) {
    RETURN_PYTRUE;
  }
  RETURN_PYFALSE;
}

extern "C" PyObject *gles_glLoadPaletteFromModelViewMatrixOES(PyObject */*self*/, PyObject */*args*/) {
  void (*func)(...) = NULL;
  func = eglGetProcAddress("glLoadPaletteFromModelViewMatrixOES");
  if(func) {
    func();
  } else {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  //glLoadPaletteFromModelViewMatrixOES();
  RETURN_PYNONE;
}

PyObject *gles_wrap_glMatrixIndexPointerOES(GLint size, GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *mptr = NULL;
  array_object *arrobj=NULL;
  void (*func)(...) = NULL;
  
  func = eglGetProcAddress("glMatrixIndexPointerOES");
  if(!func) {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    mptr = arrobj->arrdata;
  } else if(pointerPy == Py_None) {
    mptr = NULL;
  } // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    if(pointerPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    switch(type) {
      case GL_UNSIGNED_BYTE:
        mptr = gles_PySequence_AsGLubyteArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Unsupported array type");
        return NULL;
        break;
    }
    if(mptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
  gles_assign_array(GL_MATRIX_INDEX_ARRAY_OES, mptr, arrobj);
  func(size, type, stride, mptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glMatrixIndexPointerOES(PyObject *self, PyObject *args) {
  GLint size;
  GLenum type;
  GLsizei stride;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "iiiO:glMatrixIndexPointerOES", &size, &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  return gles_wrap_glMatrixIndexPointerOES(size, type, stride, pointerPy);
}

extern "C" PyObject *gles_glMatrixIndexPointerOESub(PyObject *self, PyObject *args) {
  GLint size;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glMatrixIndexPointerOESub", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_MATRIX_INDEX_ARRAY_OES);
    RETURN_PYNONE;
  }
  
  if( PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glMatrixIndexPointerOES(size, /*type*/ GL_UNSIGNED_BYTE, /*stride*/ 0, pointerPy);
}

extern "C" PyObject * gles_glPointParameterf(PyObject */*self*/, PyObject *args) {
  GLenum pname;
  GLfloat param;
  
  if( !PyArg_ParseTuple(args, "if:glPointParameterf", &pname, &param)) {
    return NULL;
  }
  
  glPointParameterf(pname, param);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject * gles_glPointParameterx(PyObject */*self*/, PyObject *args) {
  GLenum pname;
  GLfixed param;
  
  if( !PyArg_ParseTuple(args, "ii:glPointParameterx", &pname, &param)) {
    return NULL;
  }
  
  glPointParameterx(pname, param);
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPointParameterfv(PyObject */*self*/, PyObject *args) {
  GLenum pname;
  GLfloat *params = NULL;
  PyObject *paramsPy;
  
  if( !PyArg_ParseTuple(args, "iO:glPointParameterfv", &pname, &paramsPy)) {
    return NULL;
  }
  
  params = gles_PySequence_AsGLfloatArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glPointParameterfv(pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPointParameterxv(PyObject */*self*/, PyObject *args) {
  GLenum pname;
  GLfixed *params = NULL;
  PyObject *paramsPy;
  
  if( !PyArg_ParseTuple(args, "iO:glPointParameterfv", &pname, &paramsPy)) {
    return NULL;
  }
  
  params = gles_PySequence_AsGLfixedArray(paramsPy);
  if(params == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  glPointParameterxv(pname, params);
  gles_free(params);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

PyObject *gles_wrap_glPointSizePointerOES(GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *psptr = NULL;
  array_object *arrobj=NULL;
  void (*func)(...) = NULL;
  
  func = eglGetProcAddress("glPointSizePointerOES");
  if(!func) {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    psptr = arrobj->arrdata;
  } else if(pointerPy == Py_None) {
    psptr = NULL;
  } // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    if(pointerPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    switch(type) {
      case GL_FLOAT:
        psptr = gles_PySequence_AsGLfloatArray(pointerPy);
        break;
      case GL_FIXED:
        psptr = gles_PySequence_AsGLfixedArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Unsupported array type");
        return NULL;
        break;
    }
    if(psptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
  gles_assign_array(GL_POINT_SIZE_ARRAY_OES, psptr, arrobj);
  func(type, stride, psptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glPointSizePointerOES(PyObject *self, PyObject *args) {
  GLenum type;
  GLsizei stride;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "iiiO:glPointSizePointerOES", &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  return gles_wrap_glPointSizePointerOES(type, stride, pointerPy);
}

extern "C" PyObject *gles_glPointSizePointerOESf(PyObject *self, PyObject *args) {
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glPointSizePointerOESf", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_POINT_SIZE_ARRAY_OES);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glPointSizePointerOES(/*type*/ GL_FLOAT, /*stride*/ 0, pointerPy);
}

extern "C" PyObject *gles_glPointSizePointerOESx(PyObject *self, PyObject *args) {
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glPointSizePointerOESx", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_POINT_SIZE_ARRAY_OES);
    RETURN_PYNONE;
  }
  
  return gles_wrap_glPointSizePointerOES(/*type*/ GL_FIXED, /*stride*/ 0, pointerPy);
}

PyObject *gles_wrap_glWeightPointerOES(GLint size, GLenum type, GLsizei stride, PyObject *pointerPy) {
  void *wptr = NULL;
  array_object *arrobj=NULL;
  void (*func)(...) = NULL;
  
  func = eglGetProcAddress("glWeightPointerOES");
  if(!func) {
    PyErr_SetString(PyExc_NotImplementedError, "Extension not found");
    return NULL;
  }
  
  // We can use either the array type or a sequence
  // Check the array type first
  if(PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    arrobj = (array_object*)pointerPy;
    wptr = arrobj->arrdata;
  } else if(pointerPy == Py_None) {
    wptr = NULL;
  } // Try to convert it as a sequence
  else {
    pointerPy = gles_PySequence_Collapse(type, pointerPy, NULL);
    if(pointerPy == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
    switch(type) {
      case GL_FLOAT:
        wptr = gles_PySequence_AsGLfloatArray(pointerPy);
        break;
      case GL_FIXED:
        wptr = gles_PySequence_AsGLfixedArray(pointerPy);
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "Unsupported array type");
        return NULL;
        break;
    }
    if(wptr == NULL) {
      assert(PyErr_Occurred() != NULL);
      return NULL;
    }
  }
  
  gles_assign_array(GL_WEIGHT_ARRAY_OES, wptr, arrobj);
  func(size, type, stride, wptr);
  
  RETURN_IF_GLERROR;
  RETURN_PYNONE;
}

extern "C" PyObject *gles_glWeightPointerOES(PyObject *self, PyObject *args) {
  GLint size;
  GLenum type;
  GLsizei stride;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "iiiO:glWeightPointerOES", &size, &type, &stride, &pointerPy) ) {
    return NULL;
  }
  
  return gles_wrap_glWeightPointerOES(size, type, stride, pointerPy);
}

extern "C" PyObject *gles_glWeightPointerOESf(PyObject *self, PyObject *args) {
  GLint size;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glWeightPointerOESf", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_WEIGHT_ARRAY_OES);
    RETURN_PYNONE;
  }
  
  if( PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glWeightPointerOES(size, /*type*/ GL_FLOAT, /*stride*/ 0, pointerPy);
}

extern "C" PyObject *gles_glWeightPointerOESx(PyObject *self, PyObject *args) {
  GLint size;
  
  PyObject * pointerPy;
  if ( !PyArg_ParseTuple(args, "O:glWeightPointerOESx", &pointerPy) ) {
    return NULL;
  }
  
  if(pointerPy == Py_None) {
    gles_free_array(GL_WEIGHT_ARRAY_OES);
    RETURN_PYNONE;
  }
  
  if( PyObject_TypeCheck(pointerPy, GLES_ARRAY_TYPE) ) {
    size = ((array_object*)pointerPy)->dimension;
  } else {
    size = gles_PySequence_Dimension(pointerPy);
  }
  
  return gles_wrap_glWeightPointerOES(size, /*type*/ GL_FIXED, /*stride*/ 0, pointerPy);
}

#endif

/*****************************************************
 * Utility functions for your Python coding pleasure *
 *****************************************************/

/**
 * Converts a Image object to a string object
 *
 * @return Python string object
 */
extern "C" PyObject *gles_Image2str(PyObject */*self*/, PyObject *args) {
  PyObject *bitmapPy = NULL;
  GLenum format;
  GLenum type;
  PyObject *ret = NULL;
  unsigned int imagesize=0;
  char *pixels = NULL;
  
  if( !PyArg_ParseTuple(args, "iiO:Image2str", &format, &type, &bitmapPy) ) {
    return NULL;
  }
  
  CFbsBitmap *bitmap=Bitmap_AsFbsBitmap(bitmapPy);
  if(bitmap == NULL) {
    PyErr_SetString(PyExc_TypeError, "Expecting a graphics.Image object");
    return NULL;
  }
  
  pixels = (char*)gles_convert_fbsbitmap(bitmap, format, type, &imagesize);
  if(pixels == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  
  ret = PyString_FromStringAndSize(pixels, (int)imagesize);
  gles_free(pixels);
  Py_INCREF(ret);
  return ret;
}

/**
 * Checks if an extension function is available
 *
 * @return Py_True if the given extension function is available, Py_False otherwise
 */
extern "C" PyObject *gles_CheckExtension(PyObject */*self*/, PyObject *args) {
  char *funcname = NULL;
  
  if( !PyArg_ParseTuple(args, "s", &funcname) ) {
    return NULL;
  }
  
  if(eglGetProcAddress(funcname) != NULL) {
    RETURN_PYTRUE;
  }
  RETURN_PYFALSE;
}

//////////////INIT////////////////////////////

extern "C" {
  static const PyMethodDef gles_methods[] = {
    {"Image2str",                 (PyCFunction)gles_Image2str,                  METH_VARARGS, NULL},
    {"CheckExtension",            (PyCFunction)gles_CheckExtension,             METH_VARARGS, NULL},
#ifdef GL_OES_VERSION_1_1
    {"glBindBuffer",              (PyCFunction)gles_glBindBuffer,               METH_VARARGS, NULL},
    {"glBufferData",              (PyCFunction)gles_glBufferData,               METH_VARARGS, NULL},
    {"glBufferDatab",             (PyCFunction)gles_glBufferDatab,              METH_VARARGS, NULL},
    {"glBufferDataub",            (PyCFunction)gles_glBufferDataub,             METH_VARARGS, NULL},
    {"glBufferDatas",             (PyCFunction)gles_glBufferDatas,              METH_VARARGS, NULL},
    {"glBufferDataus",            (PyCFunction)gles_glBufferDataus,             METH_VARARGS, NULL},
    {"glBufferDataf",             (PyCFunction)gles_glBufferDataf,              METH_VARARGS, NULL},
    {"glBufferDatax",             (PyCFunction)gles_glBufferDatax,              METH_VARARGS, NULL},
    {"glBufferSubData",           (PyCFunction)gles_glBufferSubData,            METH_VARARGS, NULL},
    {"glBufferSubDataub",         (PyCFunction)gles_glBufferSubDataub,          METH_VARARGS, NULL},
    {"glBufferSubDatas",          (PyCFunction)gles_glBufferSubDatas,           METH_VARARGS, NULL},
    {"glBufferSubDataus",         (PyCFunction)gles_glBufferSubDataus,          METH_VARARGS, NULL},
    {"glBufferSubDataf",          (PyCFunction)gles_glBufferSubDataf,           METH_VARARGS, NULL},
    {"glBufferSubDatax",          (PyCFunction)gles_glBufferSubDatax,           METH_VARARGS, NULL},
    
    {"glClipPlanef",              (PyCFunction)gles_glClipPlanef,               METH_VARARGS, NULL},
    {"glClipPlanex",              (PyCFunction)gles_glClipPlanex,               METH_VARARGS, NULL},
    {"glCurrentPaletteMatrixOES", (PyCFunction)gles_glCurrentPaletteMatrixOES,  METH_VARARGS, NULL},
    
    {"glDeleteBuffers",           (PyCFunction)gles_glDeleteBuffers,            METH_VARARGS, NULL},
    {"glDrawTexsOES",             (PyCFunction)gles_glDrawTexsOES,              METH_VARARGS, NULL},
    {"glDrawTexiOES",             (PyCFunction)gles_glDrawTexiOES,              METH_VARARGS, NULL},
    {"glDrawTexfOES",             (PyCFunction)gles_glDrawTexfOES,              METH_VARARGS, NULL},
    {"glDrawTexxOES",             (PyCFunction)gles_glDrawTexxOES,              METH_VARARGS, NULL},
    {"glDrawTexsvOES",            (PyCFunction)gles_glDrawTexsvOES,             METH_VARARGS, NULL},
    {"glDrawTexivOES",            (PyCFunction)gles_glDrawTexivOES,             METH_VARARGS, NULL},
    {"glDrawTexfvOES",            (PyCFunction)gles_glDrawTexfvOES,             METH_VARARGS, NULL},
    {"glDrawTexxvOES",            (PyCFunction)gles_glDrawTexxvOES,             METH_VARARGS, NULL},
    
    {"glGenBuffers",              (PyCFunction)gles_glGenBuffers,               METH_VARARGS, NULL},
    {"glGetBooleanv",             (PyCFunction)gles_glGetBooleanv,              METH_VARARGS, NULL},
    {"glGetBufferParameteriv",    (PyCFunction)gles_glGetBufferParameteriv,     METH_VARARGS, NULL},
    {"glGetClipPlanef",           (PyCFunction)gles_glGetClipPlanef,            METH_VARARGS, NULL},
    {"glGetClipPlanex",           (PyCFunction)gles_glGetClipPlanex,            METH_VARARGS, NULL},
    {"glGetFixedv",               (PyCFunction)gles_glGetFixedv,                METH_VARARGS, NULL},
    {"glGetFloatv",               (PyCFunction)gles_glGetFloatv,                METH_VARARGS, NULL},
    {"glGetLightfv",              (PyCFunction)gles_glGetLightfv,               METH_VARARGS, NULL},
    {"glGetLightxv",              (PyCFunction)gles_glGetLightxv,               METH_VARARGS, NULL},
    {"glGetMaterialfv",           (PyCFunction)gles_glGetMaterialfv,            METH_VARARGS, NULL},
    {"glGetMaterialxv",           (PyCFunction)gles_glGetMaterialxv,            METH_VARARGS, NULL},
    // glGetPointerv not implemented
    {"glGetTexEnvf",              (PyCFunction)gles_glGetTexEnvf,               METH_VARARGS, NULL},
    {"glGetTexEnvx",              (PyCFunction)gles_glGetTexEnvx,               METH_VARARGS, NULL},
    
    {"glIsBuffer",                (PyCFunction)gles_glIsBuffer,                 METH_VARARGS, NULL},
    {"glIsEnabled",               (PyCFunction)gles_glIsEnabled,                METH_VARARGS, NULL},
    {"glIsTexture",               (PyCFunction)gles_glIsTexture,                METH_VARARGS, NULL},
    
    {"glLoadPaletteFromModelViewMatrixOES",(PyCFunction)gles_glLoadPaletteFromModelViewMatrixOES,METH_NOARGS, NULL},
    {"glMatrixIndexPointerOES",   (PyCFunction)gles_glMatrixIndexPointerOES,    METH_VARARGS, NULL},
    {"glMatrixIndexPointerOESub", (PyCFunction)gles_glMatrixIndexPointerOESub,  METH_VARARGS, NULL},
    {"glPointParameterf",         (PyCFunction)gles_glPointParameterf,          METH_VARARGS, NULL},
    {"glPointParameterx",         (PyCFunction)gles_glPointParameterx,          METH_VARARGS, NULL},
    {"glPointParameterfv",        (PyCFunction)gles_glPointParameterfv,         METH_VARARGS, NULL},
    {"glPointParameterxv",        (PyCFunction)gles_glPointParameterxv,         METH_VARARGS, NULL},
    
    {"glPointSizePointerOES",     (PyCFunction)gles_glPointSizePointerOES,      METH_VARARGS, NULL},
    {"glPointSizePointerOESf",    (PyCFunction)gles_glPointSizePointerOESf,     METH_VARARGS, NULL},
    {"glPointSizePointerOESx",    (PyCFunction)gles_glPointSizePointerOESx,     METH_VARARGS, NULL},
    
    {"glWeightPointerOES",        (PyCFunction)gles_glWeightPointerOES,         METH_VARARGS, NULL},
    {"glWeightPointerOESf",       (PyCFunction)gles_glWeightPointerOESf,        METH_VARARGS, NULL},
    {"glWeightPointerOESx",       (PyCFunction)gles_glWeightPointerOESx,        METH_VARARGS, NULL},
#endif
    {"array",                     (PyCFunction)new_array_object,                METH_VARARGS, NULL},
    
    {"glActiveTexture",           (PyCFunction)gles_glActiveTexture,            METH_VARARGS, NULL},
    {"glAlphaFunc",               (PyCFunction)gles_glAlphaFunc,                METH_VARARGS, NULL},
    {"glAlphaFuncx",              (PyCFunction)gles_glAlphaFuncx,               METH_VARARGS, NULL},
    
    {"glBindTexture",             (PyCFunction)gles_glBindTexture,              METH_VARARGS, NULL},
    {"glBlendFunc",               (PyCFunction)gles_glBlendFunc,                METH_VARARGS, NULL},
    
    {"glClear",                   (PyCFunction)gles_glClear,                    METH_VARARGS, NULL},
    {"glClearColor",              (PyCFunction)gles_glClearColor,               METH_VARARGS, NULL},
    {"glClearColorx",             (PyCFunction)gles_glClearColorx,              METH_VARARGS, NULL},
    {"glClearDepthf",             (PyCFunction)gles_glClearDepthf,              METH_VARARGS, NULL},
    {"glClearDepthx",             (PyCFunction)gles_glClearDepthx,              METH_VARARGS, NULL},
    {"glClearStencil",            (PyCFunction)gles_glClearStencil,             METH_VARARGS, NULL},
    {"glClientActiveTexture",     (PyCFunction)gles_glClientActiveTexture,      METH_VARARGS, NULL},
    {"glColor4f",                 (PyCFunction)gles_glColor4f,                  METH_VARARGS, NULL},
    {"glColor4x",                 (PyCFunction)gles_glColor4x,                  METH_VARARGS, NULL},
    {"glColorPointer",            (PyCFunction)gles_glColorPointer,             METH_VARARGS, NULL},
    {"glColorPointerub",          (PyCFunction)gles_glColorPointerub,           METH_VARARGS, NULL},
    {"glColorPointerf",           (PyCFunction)gles_glColorPointerf,            METH_VARARGS, NULL},
    {"glColorPointerx",           (PyCFunction)gles_glColorPointerx,            METH_VARARGS, NULL},
    
    {"glCompressedTexImage2D",    (PyCFunction)gles_glCompressedTexImage2D,     METH_VARARGS, NULL},
    {"glCompressedTexSubImage2D", (PyCFunction)gles_glCompressedTexSubImage2D,  METH_VARARGS, NULL},
    {"glCopyTexImage2D",          (PyCFunction)gles_glCopyTexImage2D,           METH_VARARGS, NULL},
    {"glCopyTexSubImage2D",       (PyCFunction)gles_glCopyTexSubImage2D,        METH_VARARGS, NULL},
    {"glCullFace",                (PyCFunction)gles_glCullFace,                 METH_VARARGS, NULL},
    
    {"glDeleteTextures",          (PyCFunction)gles_glDeleteTextures,           METH_VARARGS, NULL},
    {"glDepthFunc",               (PyCFunction)gles_glDepthFunc,                METH_VARARGS, NULL},
    {"glDepthMask",               (PyCFunction)gles_glDepthMask,                METH_VARARGS, NULL},
    {"glDepthRangef",             (PyCFunction)gles_glDepthRangef,              METH_VARARGS, NULL},
    {"glDepthRangex",             (PyCFunction)gles_glDepthRangex,              METH_VARARGS, NULL},
    {"glDisable",                 (PyCFunction)gles_glDisable,                  METH_VARARGS, NULL},
    {"glDisableClientState",      (PyCFunction)gles_glDisableClientState,       METH_VARARGS, NULL},
    {"glDrawArrays",              (PyCFunction)gles_glDrawArrays,               METH_VARARGS, NULL},
    {"glDrawElements",            (PyCFunction)gles_glDrawElements,             METH_VARARGS, NULL},
    {"glDrawElementsub",          (PyCFunction)gles_glDrawElementsub,           METH_VARARGS, NULL},
    {"glDrawElementsus",          (PyCFunction)gles_glDrawElementsus,           METH_VARARGS, NULL},
    
    {"glEnable",                  (PyCFunction)gles_glEnable,                   METH_VARARGS, NULL},
    {"glEnableClientState",       (PyCFunction)gles_glEnableClientState,        METH_VARARGS, NULL},
    
    {"glFinish",                  (PyCFunction)gles_glFinish,                   METH_VARARGS, NULL},
    {"glFlush",                   (PyCFunction)gles_glFlush,                    METH_VARARGS, NULL},
    
    {"glFogf",                    (PyCFunction)gles_glFogf,                     METH_VARARGS, NULL},
    {"glFogfv",                   (PyCFunction)gles_glFogfv,                    METH_VARARGS, NULL},
    {"glFogx",                    (PyCFunction)gles_glFogx,                     METH_VARARGS, NULL},
    {"glFogxv",                   (PyCFunction)gles_glFogxv,                    METH_VARARGS, NULL},
    {"glFrontFace",               (PyCFunction)gles_glFrontFace,                METH_VARARGS, NULL},
    {"glFrustumf",                (PyCFunction)gles_glFrustumf,                 METH_VARARGS, NULL},
    {"glFrustumx",                (PyCFunction)gles_glFrustumx,                 METH_VARARGS, NULL},
    
    {"glGenTextures",             (PyCFunction)gles_glGenTextures,              METH_VARARGS, NULL},
    {"glGetIntegerv",             (PyCFunction)gles_glGetIntegerv,              METH_VARARGS, NULL},
    {"glGetString",               (PyCFunction)gles_glGetString,                METH_VARARGS, NULL},
    
    {"glHint",                    (PyCFunction)gles_glHint,                     METH_VARARGS, NULL},
    
    {"glLightModelf",             (PyCFunction)gles_glLightModelf,              METH_VARARGS, NULL},
    {"glLightModelfv",            (PyCFunction)gles_glLightModelfv,             METH_VARARGS, NULL},
    {"glLightModelx",             (PyCFunction)gles_glLightModelx,              METH_VARARGS, NULL},
    {"glLightModelxv",            (PyCFunction)gles_glLightModelxv,             METH_VARARGS, NULL},
    {"glLightf",                  (PyCFunction)gles_glLightf,                   METH_VARARGS, NULL},
    {"glLightfv",                 (PyCFunction)gles_glLightfv,                  METH_VARARGS, NULL},
    {"glLightx",                  (PyCFunction)gles_glLightx,                   METH_VARARGS, NULL},
    {"glLightxv",                 (PyCFunction)gles_glLightxv,                  METH_VARARGS, NULL},
    {"glLineWidth",               (PyCFunction)gles_glLineWidth,                METH_VARARGS, NULL},
    {"glLineWidthx",              (PyCFunction)gles_glLineWidthx,               METH_VARARGS, NULL},
    {"glLoadIdentity",            (PyCFunction)gles_glLoadIdentity,             METH_NOARGS, NULL},
    {"glLoadMatrixf",             (PyCFunction)gles_glLoadMatrixf,              METH_VARARGS, NULL},
    {"glLoadMatrixx",             (PyCFunction)gles_glLoadMatrixx,              METH_VARARGS, NULL},
    {"glLogicOp",                 (PyCFunction)gles_glLogicOp,                  METH_VARARGS, NULL},
    
    {"glMaterialf",               (PyCFunction)gles_glMaterialf,                METH_VARARGS, NULL},
    {"glMaterialfv",              (PyCFunction)gles_glMaterialfv,               METH_VARARGS, NULL},
    {"glMaterialx",               (PyCFunction)gles_glMaterialx,                METH_VARARGS, NULL},
    {"glMaterialxv",              (PyCFunction)gles_glMaterialxv,               METH_VARARGS, NULL},
    {"glMatrixMode",              (PyCFunction)gles_glMatrixMode,               METH_VARARGS, NULL},
    {"glMultMatrixf",             (PyCFunction)gles_glMultMatrixf,              METH_VARARGS, NULL},
    {"glMultMatrixx",             (PyCFunction)gles_glMultMatrixx,              METH_VARARGS, NULL},
    {"glMultiTexCoord4f",         (PyCFunction)gles_glMultiTexCoord4f,          METH_VARARGS, NULL},
    {"glMultiTexCoord4x",         (PyCFunction)gles_glMultiTexCoord4x,          METH_VARARGS, NULL},
    
    {"glNormal3f",                (PyCFunction)gles_glNormal3f,                 METH_VARARGS, NULL},
    {"glNormal3x",                (PyCFunction)gles_glNormal3x,                 METH_VARARGS, NULL},
    {"glNormalPointer",           (PyCFunction)gles_glNormalPointer,            METH_VARARGS, NULL},
    {"glNormalPointerb",          (PyCFunction)gles_glNormalPointerb,           METH_VARARGS, NULL},
    {"glNormalPointers",          (PyCFunction)gles_glNormalPointers,           METH_VARARGS, NULL},
    {"glNormalPointerf",          (PyCFunction)gles_glNormalPointerf,           METH_VARARGS, NULL},
    {"glNormalPointerx",          (PyCFunction)gles_glNormalPointerx,           METH_VARARGS, NULL},
    
    {"glOrthof",                  (PyCFunction)gles_glOrthof,                   METH_VARARGS, NULL},
    {"glOrthox",                  (PyCFunction)gles_glOrthox,                   METH_VARARGS, NULL},
    
    {"glPixelStorei",             (PyCFunction)gles_glPixelStorei,              METH_VARARGS, NULL},
    {"glPointSize",               (PyCFunction)gles_glPointSize,                METH_VARARGS, NULL},
    {"glPointSizex",              (PyCFunction)gles_glPointSizex,               METH_VARARGS, NULL},
    {"glPolygonOffset",           (PyCFunction)gles_glPolygonOffset,            METH_VARARGS, NULL},
    {"glPolygonOffsetx",          (PyCFunction)gles_glPolygonOffsetx,           METH_VARARGS, NULL},
    {"glPopMatrix",               (PyCFunction)gles_glPopMatrix,                METH_NOARGS, NULL},
    {"glPushMatrix",              (PyCFunction)gles_glPushMatrix,               METH_NOARGS, NULL},
    
    {"glReadPixels",              (PyCFunction)gles_glReadPixels,               METH_VARARGS, NULL},
    {"glRotatex",                 (PyCFunction)gles_glRotatex,                  METH_VARARGS, NULL},
    {"glRotatef",                 (PyCFunction)gles_glRotatef,                  METH_VARARGS, NULL},
    
    {"glScalef",                  (PyCFunction)gles_glScalef,                   METH_VARARGS, NULL},
    {"glScalex",                  (PyCFunction)gles_glScalex,                   METH_VARARGS, NULL},
    {"glScissor",                 (PyCFunction)gles_glScissor,                  METH_VARARGS, NULL},
    {"glShadeModel",              (PyCFunction)gles_glShadeModel,               METH_VARARGS, NULL},
    {"glStencilFunc",             (PyCFunction)gles_glStencilFunc,              METH_VARARGS, NULL},
    {"glStencilMask",             (PyCFunction)gles_glStencilMask,              METH_VARARGS, NULL},
    {"glStencilOp",               (PyCFunction)gles_glStencilOp,                METH_VARARGS, NULL},
    
    {"glTexCoordPointer",         (PyCFunction)gles_glTexCoordPointer,          METH_VARARGS, NULL},
    {"glTexCoordPointerb",        (PyCFunction)gles_glTexCoordPointerb,         METH_VARARGS, NULL},
    {"glTexCoordPointers",        (PyCFunction)gles_glTexCoordPointers,         METH_VARARGS, NULL},
    {"glTexCoordPointerf",        (PyCFunction)gles_glTexCoordPointerf,         METH_VARARGS, NULL},
    {"glTexCoordPointerx",        (PyCFunction)gles_glTexCoordPointerx,         METH_VARARGS, NULL},
    
    {"glTexEnvf",                 (PyCFunction)gles_glTexEnvf,                  METH_VARARGS, NULL},
    {"glTexEnvfv",                (PyCFunction)gles_glTexEnvfv,                 METH_VARARGS, NULL},
    {"glTexEnvx",                 (PyCFunction)gles_glTexEnvx,                  METH_VARARGS, NULL},
    {"glTexEnvxv",                (PyCFunction)gles_glTexEnvxv,                 METH_VARARGS, NULL},
    {"glTexImage2D",              (PyCFunction)gles_glTexImage2D,               METH_VARARGS, NULL},
    // A reduced argument version of glTexImage2D, accepting only Image objects.
    {"glTexImage2DIO",            (PyCFunction)gles_glTexImage2DIO,             METH_VARARGS, NULL},
    {"glTexParameterf",           (PyCFunction)gles_glTexParameterf,            METH_VARARGS, NULL},
    {"glTexParameterx",           (PyCFunction)gles_glTexParameterx,            METH_VARARGS, NULL},
    {"glTexSubImage2D",           (PyCFunction)gles_glTexSubImage2D,            METH_VARARGS, NULL},
    {"glTexSubImage2DIO",         (PyCFunction)gles_glTexSubImage2DIO,          METH_VARARGS, NULL},
    {"glTranslatex",              (PyCFunction)gles_glTranslatex,               METH_VARARGS, NULL},
    {"glTranslatef",              (PyCFunction)gles_glTranslatef,               METH_VARARGS, NULL},
    
    {"glVertexPointer",           (PyCFunction)gles_glVertexPointer,            METH_VARARGS, NULL},
    {"glVertexPointerb",          (PyCFunction)gles_glVertexPointerb,           METH_VARARGS, NULL},
    {"glVertexPointers",          (PyCFunction)gles_glVertexPointers,           METH_VARARGS, NULL},
    {"glVertexPointerf",          (PyCFunction)gles_glVertexPointerf,           METH_VARARGS, NULL},
    {"glVertexPointerx",          (PyCFunction)gles_glVertexPointerx,           METH_VARARGS, NULL},
    {"glViewport",                (PyCFunction)gles_glViewport,                 METH_VARARGS, NULL},
    
    {NULL,              NULL}           /* sentinel */
  };
  
  DL_EXPORT(void) zfinalizegles(void)
  {
    gles_uninit_arrays();
#ifdef DEBUG_GLES
    DEBUGMSG("GLES finalized\n");
#endif
  }
  
  DL_EXPORT(void) initgles(void)
  {
    PyObject *m, *d; // Pointer for module object and it's dictionary

    PyObject *GLError;
    PyTypeObject *array_type;
#ifdef DEBUG_GLES
    DEBUGMSG1("GLES: Dll::Tls() returned %x\n", Dll::Tls());
#endif
    
    // Initialize internal array memory
    gles_init_arrays();
    // The previous function may fail at runtime
    if(PyErr_Occurred()) {
      return;
    }
    
    // Initialize the module
    m = Py_InitModule("gles", (PyMethodDef*)gles_methods);
    // Add the GLError
    GLError = PyErr_NewException( "_gles.GLError", NULL, NULL);
    PyModule_AddObject(m, "GLError", GLError);
    
    // Create a new type for gles.array
    array_type = PyObject_New(PyTypeObject, &PyType_Type);
    *array_type = c_array_type;
    array_type->ob_type = &PyType_Type;
    SPyAddGlobalString("GLESArrayType", (PyObject*)array_type);
    
    d = PyModule_GetDict(m);
    
    // defines for OpenGL ES 1.0
    PyDict_SetItemString(d, "GL_OES_VERSION_1_0", PyInt_FromLong(GL_OES_VERSION_1_0));
    PyDict_SetItemString(d, "GL_OES_read_format", PyInt_FromLong(GL_OES_read_format));
    PyDict_SetItemString(d, "GL_OES_compressed_paletted_texture", PyInt_FromLong(GL_OES_compressed_paletted_texture));
    
    PyDict_SetItemString(d, "GL_DEPTH_BUFFER_BIT", PyInt_FromLong(GL_DEPTH_BUFFER_BIT));
    PyDict_SetItemString(d, "GL_STENCIL_BUFFER_BIT", PyInt_FromLong(GL_STENCIL_BUFFER_BIT));
    PyDict_SetItemString(d, "GL_COLOR_BUFFER_BIT", PyInt_FromLong(GL_COLOR_BUFFER_BIT));
    
    PyDict_SetItemString(d, "GL_FALSE", PyInt_FromLong(GL_FALSE));
    PyDict_SetItemString(d, "GL_TRUE", PyInt_FromLong(GL_TRUE));
    
    PyDict_SetItemString(d, "GL_POINTS", PyInt_FromLong(GL_POINTS));
    PyDict_SetItemString(d, "GL_LINES", PyInt_FromLong(GL_LINES));
    PyDict_SetItemString(d, "GL_LINE_LOOP", PyInt_FromLong(GL_LINE_LOOP));
    PyDict_SetItemString(d, "GL_LINE_STRIP", PyInt_FromLong(GL_LINE_STRIP));
    PyDict_SetItemString(d, "GL_TRIANGLES", PyInt_FromLong(GL_TRIANGLES));
    PyDict_SetItemString(d, "GL_TRIANGLE_STRIP", PyInt_FromLong(GL_TRIANGLE_STRIP));
    PyDict_SetItemString(d, "GL_TRIANGLE_FAN", PyInt_FromLong(GL_TRIANGLE_FAN));
    
    PyDict_SetItemString(d, "GL_NEVER", PyInt_FromLong(GL_NEVER));
    PyDict_SetItemString(d, "GL_LESS", PyInt_FromLong(GL_LESS));
    PyDict_SetItemString(d, "GL_EQUAL", PyInt_FromLong(GL_EQUAL));
    PyDict_SetItemString(d, "GL_LEQUAL", PyInt_FromLong(GL_LEQUAL));
    PyDict_SetItemString(d, "GL_GREATER", PyInt_FromLong(GL_GREATER));
    PyDict_SetItemString(d, "GL_NOTEQUAL", PyInt_FromLong(GL_NOTEQUAL));
    PyDict_SetItemString(d, "GL_GEQUAL", PyInt_FromLong(GL_GEQUAL));
    PyDict_SetItemString(d, "GL_ALWAYS", PyInt_FromLong(GL_ALWAYS));
    
    PyDict_SetItemString(d, "GL_ZERO", PyInt_FromLong(GL_ZERO));
    PyDict_SetItemString(d, "GL_ONE", PyInt_FromLong(GL_ONE));
    PyDict_SetItemString(d, "GL_SRC_COLOR", PyInt_FromLong(GL_SRC_COLOR));
    PyDict_SetItemString(d, "GL_ONE_MINUS_SRC_COLOR", PyInt_FromLong(GL_ONE_MINUS_SRC_COLOR));
    PyDict_SetItemString(d, "GL_SRC_ALPHA", PyInt_FromLong(GL_SRC_ALPHA));
    PyDict_SetItemString(d, "GL_ONE_MINUS_SRC_ALPHA", PyInt_FromLong(GL_ONE_MINUS_SRC_ALPHA));
    PyDict_SetItemString(d, "GL_DST_ALPHA", PyInt_FromLong(GL_DST_ALPHA));
    PyDict_SetItemString(d, "GL_ONE_MINUS_DST_ALPHA", PyInt_FromLong(GL_ONE_MINUS_DST_ALPHA));
    
    PyDict_SetItemString(d, "GL_DST_COLOR", PyInt_FromLong(GL_DST_COLOR));
    PyDict_SetItemString(d, "GL_ONE_MINUS_DST_COLOR", PyInt_FromLong(GL_ONE_MINUS_DST_COLOR));
    PyDict_SetItemString(d, "GL_SRC_ALPHA_SATURATE", PyInt_FromLong(GL_SRC_ALPHA_SATURATE));
    
    PyDict_SetItemString(d, "GL_FRONT", PyInt_FromLong(GL_FRONT));
    PyDict_SetItemString(d, "GL_BACK", PyInt_FromLong(GL_BACK));
    PyDict_SetItemString(d, "GL_FRONT_AND_BACK", PyInt_FromLong(GL_FRONT_AND_BACK));
    
    PyDict_SetItemString(d, "GL_FOG", PyInt_FromLong(GL_FOG));
    PyDict_SetItemString(d, "GL_LIGHTING", PyInt_FromLong(GL_LIGHTING));
    PyDict_SetItemString(d, "GL_TEXTURE_2D", PyInt_FromLong(GL_TEXTURE_2D));
    PyDict_SetItemString(d, "GL_CULL_FACE", PyInt_FromLong(GL_CULL_FACE));
    PyDict_SetItemString(d, "GL_ALPHA_TEST", PyInt_FromLong(GL_ALPHA_TEST));
    PyDict_SetItemString(d, "GL_BLEND", PyInt_FromLong(GL_BLEND));
    PyDict_SetItemString(d, "GL_COLOR_LOGIC_OP", PyInt_FromLong(GL_COLOR_LOGIC_OP));
    PyDict_SetItemString(d, "GL_DITHER", PyInt_FromLong(GL_DITHER));
    PyDict_SetItemString(d, "GL_STENCIL_TEST", PyInt_FromLong(GL_STENCIL_TEST));
    PyDict_SetItemString(d, "GL_DEPTH_TEST", PyInt_FromLong(GL_DEPTH_TEST));
    PyDict_SetItemString(d, "GL_POINT_SMOOTH", PyInt_FromLong(GL_POINT_SMOOTH));
    PyDict_SetItemString(d, "GL_LINE_SMOOTH", PyInt_FromLong(GL_LINE_SMOOTH));
    PyDict_SetItemString(d, "GL_SCISSOR_TEST", PyInt_FromLong(GL_SCISSOR_TEST));
    PyDict_SetItemString(d, "GL_COLOR_MATERIAL", PyInt_FromLong(GL_COLOR_MATERIAL));
    PyDict_SetItemString(d, "GL_NORMALIZE", PyInt_FromLong(GL_NORMALIZE));
    PyDict_SetItemString(d, "GL_RESCALE_NORMAL", PyInt_FromLong(GL_RESCALE_NORMAL));
    PyDict_SetItemString(d, "GL_POLYGON_OFFSET_FILL", PyInt_FromLong(GL_POLYGON_OFFSET_FILL));
    PyDict_SetItemString(d, "GL_VERTEX_ARRAY", PyInt_FromLong(GL_VERTEX_ARRAY));
    PyDict_SetItemString(d, "GL_NORMAL_ARRAY", PyInt_FromLong(GL_NORMAL_ARRAY));
    PyDict_SetItemString(d, "GL_COLOR_ARRAY", PyInt_FromLong(GL_COLOR_ARRAY));
    PyDict_SetItemString(d, "GL_TEXTURE_COORD_ARRAY", PyInt_FromLong(GL_TEXTURE_COORD_ARRAY));
    PyDict_SetItemString(d, "GL_MULTISAMPLE", PyInt_FromLong(GL_MULTISAMPLE));
    PyDict_SetItemString(d, "GL_SAMPLE_ALPHA_TO_COVERAGE", PyInt_FromLong(GL_SAMPLE_ALPHA_TO_COVERAGE));
    PyDict_SetItemString(d, "GL_SAMPLE_ALPHA_TO_ONE", PyInt_FromLong(GL_SAMPLE_ALPHA_TO_ONE));
    PyDict_SetItemString(d, "GL_SAMPLE_COVERAGE", PyInt_FromLong(GL_SAMPLE_COVERAGE));
    
    PyDict_SetItemString(d, "GL_NO_ERROR", PyInt_FromLong(GL_NO_ERROR));
    PyDict_SetItemString(d, "GL_INVALID_ENUM", PyInt_FromLong(GL_INVALID_ENUM));
    PyDict_SetItemString(d, "GL_INVALID_VALUE", PyInt_FromLong(GL_INVALID_VALUE));
    PyDict_SetItemString(d, "GL_INVALID_OPERATION", PyInt_FromLong(GL_INVALID_OPERATION));
    PyDict_SetItemString(d, "GL_STACK_OVERFLOW", PyInt_FromLong(GL_STACK_OVERFLOW));
    PyDict_SetItemString(d, "GL_STACK_UNDERFLOW", PyInt_FromLong(GL_STACK_UNDERFLOW));
    PyDict_SetItemString(d, "GL_OUT_OF_MEMORY", PyInt_FromLong(GL_OUT_OF_MEMORY));
    
    PyDict_SetItemString(d, "GL_EXP", PyInt_FromLong(GL_EXP));
    PyDict_SetItemString(d, "GL_EXP2", PyInt_FromLong(GL_EXP2));
    
    PyDict_SetItemString(d, "GL_FOG_DENSITY", PyInt_FromLong(GL_FOG_DENSITY));
    PyDict_SetItemString(d, "GL_FOG_START", PyInt_FromLong(GL_FOG_START));
    PyDict_SetItemString(d, "GL_FOG_END", PyInt_FromLong(GL_FOG_END));
    PyDict_SetItemString(d, "GL_FOG_MODE", PyInt_FromLong(GL_FOG_MODE));
    PyDict_SetItemString(d, "GL_FOG_COLOR", PyInt_FromLong(GL_FOG_COLOR));
    
    PyDict_SetItemString(d, "GL_CW", PyInt_FromLong(GL_CW));
    PyDict_SetItemString(d, "GL_CCW", PyInt_FromLong(GL_CCW));
    
    PyDict_SetItemString(d, "GL_SMOOTH_POINT_SIZE_RANGE", PyInt_FromLong(GL_SMOOTH_POINT_SIZE_RANGE));
    PyDict_SetItemString(d, "GL_SMOOTH_LINE_WIDTH_RANGE", PyInt_FromLong(GL_SMOOTH_LINE_WIDTH_RANGE));
    PyDict_SetItemString(d, "GL_ALIASED_POINT_SIZE_RANGE", PyInt_FromLong(GL_ALIASED_POINT_SIZE_RANGE));
    PyDict_SetItemString(d, "GL_ALIASED_LINE_WIDTH_RANGE", PyInt_FromLong(GL_ALIASED_LINE_WIDTH_RANGE));
    PyDict_SetItemString(d, "GL_IMPLEMENTATION_COLOR_READ_TYPE_OES", PyInt_FromLong(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES));
    PyDict_SetItemString(d, "GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES", PyInt_FromLong(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES));
    PyDict_SetItemString(d, "GL_MAX_LIGHTS", PyInt_FromLong(GL_MAX_LIGHTS));
    PyDict_SetItemString(d, "GL_MAX_TEXTURE_SIZE", PyInt_FromLong(GL_MAX_TEXTURE_SIZE));
    PyDict_SetItemString(d, "GL_MAX_MODELVIEW_STACK_DEPTH", PyInt_FromLong(GL_MAX_MODELVIEW_STACK_DEPTH));
    PyDict_SetItemString(d, "GL_MAX_PROJECTION_STACK_DEPTH", PyInt_FromLong(GL_MAX_PROJECTION_STACK_DEPTH));
    PyDict_SetItemString(d, "GL_MAX_TEXTURE_STACK_DEPTH", PyInt_FromLong(GL_MAX_TEXTURE_STACK_DEPTH));
    PyDict_SetItemString(d, "GL_MAX_VIEWPORT_DIMS", PyInt_FromLong(GL_MAX_VIEWPORT_DIMS));
    PyDict_SetItemString(d, "GL_MAX_ELEMENTS_VERTICES", PyInt_FromLong(GL_MAX_ELEMENTS_VERTICES));
    PyDict_SetItemString(d, "GL_MAX_ELEMENTS_INDICES", PyInt_FromLong(GL_MAX_ELEMENTS_INDICES));
    PyDict_SetItemString(d, "GL_MAX_TEXTURE_UNITS", PyInt_FromLong(GL_MAX_TEXTURE_UNITS));
    PyDict_SetItemString(d, "GL_NUM_COMPRESSED_TEXTURE_FORMATS", PyInt_FromLong(GL_NUM_COMPRESSED_TEXTURE_FORMATS));
    PyDict_SetItemString(d, "GL_COMPRESSED_TEXTURE_FORMATS", PyInt_FromLong(GL_COMPRESSED_TEXTURE_FORMATS));
    PyDict_SetItemString(d, "GL_SUBPIXEL_BITS", PyInt_FromLong(GL_SUBPIXEL_BITS));
    PyDict_SetItemString(d, "GL_RED_BITS", PyInt_FromLong(GL_RED_BITS));
    PyDict_SetItemString(d, "GL_GREEN_BITS", PyInt_FromLong(GL_GREEN_BITS));
    PyDict_SetItemString(d, "GL_BLUE_BITS", PyInt_FromLong(GL_BLUE_BITS));
    PyDict_SetItemString(d, "GL_ALPHA_BITS", PyInt_FromLong(GL_ALPHA_BITS));
    PyDict_SetItemString(d, "GL_DEPTH_BITS", PyInt_FromLong(GL_DEPTH_BITS));
    PyDict_SetItemString(d, "GL_STENCIL_BITS", PyInt_FromLong(GL_STENCIL_BITS));
    
    PyDict_SetItemString(d, "GL_DONT_CARE", PyInt_FromLong(GL_DONT_CARE));
    PyDict_SetItemString(d, "GL_FASTEST", PyInt_FromLong(GL_FASTEST));
    PyDict_SetItemString(d, "GL_NICEST", PyInt_FromLong(GL_NICEST));
    
    PyDict_SetItemString(d, "GL_PERSPECTIVE_CORRECTION_HINT", PyInt_FromLong(GL_PERSPECTIVE_CORRECTION_HINT));
    PyDict_SetItemString(d, "GL_POINT_SMOOTH_HINT", PyInt_FromLong(GL_POINT_SMOOTH_HINT));
    PyDict_SetItemString(d, "GL_LINE_SMOOTH_HINT", PyInt_FromLong(GL_LINE_SMOOTH_HINT));
    PyDict_SetItemString(d, "GL_POLYGON_SMOOTH_HINT", PyInt_FromLong(GL_POLYGON_SMOOTH_HINT));
    PyDict_SetItemString(d, "GL_FOG_HINT", PyInt_FromLong(GL_FOG_HINT));
    
    PyDict_SetItemString(d, "GL_LIGHT_MODEL_AMBIENT", PyInt_FromLong(GL_LIGHT_MODEL_AMBIENT));
    PyDict_SetItemString(d, "GL_LIGHT_MODEL_TWO_SIDE", PyInt_FromLong(GL_LIGHT_MODEL_TWO_SIDE));
    
    PyDict_SetItemString(d, "GL_AMBIENT", PyInt_FromLong(GL_AMBIENT));
    PyDict_SetItemString(d, "GL_DIFFUSE", PyInt_FromLong(GL_DIFFUSE));
    PyDict_SetItemString(d, "GL_SPECULAR", PyInt_FromLong(GL_SPECULAR));
    PyDict_SetItemString(d, "GL_POSITION", PyInt_FromLong(GL_POSITION));
    PyDict_SetItemString(d, "GL_SPOT_DIRECTION", PyInt_FromLong(GL_SPOT_DIRECTION));
    PyDict_SetItemString(d, "GL_SPOT_EXPONENT", PyInt_FromLong(GL_SPOT_EXPONENT));
    PyDict_SetItemString(d, "GL_SPOT_CUTOFF", PyInt_FromLong(GL_SPOT_CUTOFF));
    PyDict_SetItemString(d, "GL_CONSTANT_ATTENUATION", PyInt_FromLong(GL_CONSTANT_ATTENUATION));
    PyDict_SetItemString(d, "GL_LINEAR_ATTENUATION", PyInt_FromLong(GL_LINEAR_ATTENUATION));
    PyDict_SetItemString(d, "GL_QUADRATIC_ATTENUATION", PyInt_FromLong(GL_QUADRATIC_ATTENUATION));
    
    PyDict_SetItemString(d, "GL_BYTE", PyInt_FromLong(GL_BYTE));
    PyDict_SetItemString(d, "GL_UNSIGNED_BYTE", PyInt_FromLong(GL_UNSIGNED_BYTE));
    PyDict_SetItemString(d, "GL_SHORT", PyInt_FromLong(GL_SHORT));
    PyDict_SetItemString(d, "GL_UNSIGNED_SHORT", PyInt_FromLong(GL_UNSIGNED_SHORT));
    PyDict_SetItemString(d, "GL_FLOAT", PyInt_FromLong(GL_FLOAT));
    PyDict_SetItemString(d, "GL_FIXED", PyInt_FromLong(GL_FIXED));
    
    PyDict_SetItemString(d, "GL_CLEAR", PyInt_FromLong(GL_CLEAR));
    PyDict_SetItemString(d, "GL_AND", PyInt_FromLong(GL_AND));
    PyDict_SetItemString(d, "GL_AND_REVERSE", PyInt_FromLong(GL_AND_REVERSE));
    PyDict_SetItemString(d, "GL_COPY", PyInt_FromLong(GL_COPY));
    PyDict_SetItemString(d, "GL_AND_INVERTED", PyInt_FromLong(GL_AND_INVERTED));
    PyDict_SetItemString(d, "GL_NOOP", PyInt_FromLong(GL_NOOP));
    PyDict_SetItemString(d, "GL_XOR", PyInt_FromLong(GL_XOR));
    PyDict_SetItemString(d, "GL_OR", PyInt_FromLong(GL_OR));
    PyDict_SetItemString(d, "GL_NOR", PyInt_FromLong(GL_NOR));
    PyDict_SetItemString(d, "GL_EQUIV", PyInt_FromLong(GL_EQUIV));
    PyDict_SetItemString(d, "GL_INVERT", PyInt_FromLong(GL_INVERT));
    PyDict_SetItemString(d, "GL_OR_REVERSE", PyInt_FromLong(GL_OR_REVERSE));
    PyDict_SetItemString(d, "GL_COPY_INVERTED", PyInt_FromLong(GL_COPY_INVERTED));
    PyDict_SetItemString(d, "GL_OR_INVERTED", PyInt_FromLong(GL_OR_INVERTED));
    PyDict_SetItemString(d, "GL_NAND", PyInt_FromLong(GL_NAND));
    PyDict_SetItemString(d, "GL_SET", PyInt_FromLong(GL_SET));
    
    PyDict_SetItemString(d, "GL_EMISSION", PyInt_FromLong(GL_EMISSION));
    PyDict_SetItemString(d, "GL_SHININESS", PyInt_FromLong(GL_SHININESS));
    PyDict_SetItemString(d, "GL_AMBIENT_AND_DIFFUSE", PyInt_FromLong(GL_AMBIENT_AND_DIFFUSE));
    
    PyDict_SetItemString(d, "GL_MODELVIEW", PyInt_FromLong(GL_MODELVIEW));
    PyDict_SetItemString(d, "GL_PROJECTION", PyInt_FromLong(GL_PROJECTION));
    PyDict_SetItemString(d, "GL_TEXTURE", PyInt_FromLong(GL_TEXTURE));
    
    PyDict_SetItemString(d, "GL_ALPHA", PyInt_FromLong(GL_ALPHA));
    PyDict_SetItemString(d, "GL_RGB", PyInt_FromLong(GL_RGB));
    PyDict_SetItemString(d, "GL_RGBA", PyInt_FromLong(GL_RGBA));
    PyDict_SetItemString(d, "GL_LUMINANCE", PyInt_FromLong(GL_LUMINANCE));
    PyDict_SetItemString(d, "GL_LUMINANCE_ALPHA", PyInt_FromLong(GL_LUMINANCE_ALPHA));
    
    PyDict_SetItemString(d, "GL_UNPACK_ALIGNMENT", PyInt_FromLong(GL_UNPACK_ALIGNMENT));
    PyDict_SetItemString(d, "GL_PACK_ALIGNMENT", PyInt_FromLong(GL_PACK_ALIGNMENT));
    
    PyDict_SetItemString(d, "GL_UNSIGNED_SHORT_4_4_4_4", PyInt_FromLong(GL_UNSIGNED_SHORT_4_4_4_4));
    PyDict_SetItemString(d, "GL_UNSIGNED_SHORT_5_5_5_1", PyInt_FromLong(GL_UNSIGNED_SHORT_5_5_5_1));
    PyDict_SetItemString(d, "GL_UNSIGNED_SHORT_5_6_5", PyInt_FromLong(GL_UNSIGNED_SHORT_5_6_5));
    
    PyDict_SetItemString(d, "GL_FLAT", PyInt_FromLong(GL_FLAT));
    PyDict_SetItemString(d, "GL_SMOOTH", PyInt_FromLong(GL_SMOOTH));
    
    PyDict_SetItemString(d, "GL_KEEP", PyInt_FromLong(GL_KEEP));
    PyDict_SetItemString(d, "GL_REPLACE", PyInt_FromLong(GL_REPLACE));
    PyDict_SetItemString(d, "GL_INCR", PyInt_FromLong(GL_INCR));
    PyDict_SetItemString(d, "GL_DECR", PyInt_FromLong(GL_DECR));
    
    PyDict_SetItemString(d, "GL_VENDOR", PyInt_FromLong(GL_VENDOR));
    PyDict_SetItemString(d, "GL_RENDERER", PyInt_FromLong(GL_RENDERER));
    PyDict_SetItemString(d, "GL_VERSION", PyInt_FromLong(GL_VERSION));
    PyDict_SetItemString(d, "GL_EXTENSIONS", PyInt_FromLong(GL_EXTENSIONS));
    
    PyDict_SetItemString(d, "GL_MODULATE", PyInt_FromLong(GL_MODULATE));
    PyDict_SetItemString(d, "GL_DECAL", PyInt_FromLong(GL_DECAL));
    PyDict_SetItemString(d, "GL_ADD", PyInt_FromLong(GL_ADD));
    
    PyDict_SetItemString(d, "GL_TEXTURE_ENV_MODE", PyInt_FromLong(GL_TEXTURE_ENV_MODE));
    PyDict_SetItemString(d, "GL_TEXTURE_ENV_COLOR", PyInt_FromLong(GL_TEXTURE_ENV_COLOR));
    
    PyDict_SetItemString(d, "GL_TEXTURE_ENV", PyInt_FromLong(GL_TEXTURE_ENV));
    
    PyDict_SetItemString(d, "GL_NEAREST", PyInt_FromLong(GL_NEAREST));
    PyDict_SetItemString(d, "GL_LINEAR", PyInt_FromLong(GL_LINEAR));
    
    PyDict_SetItemString(d, "GL_NEAREST_MIPMAP_NEAREST", PyInt_FromLong(GL_NEAREST_MIPMAP_NEAREST));
    PyDict_SetItemString(d, "GL_LINEAR_MIPMAP_NEAREST", PyInt_FromLong(GL_LINEAR_MIPMAP_NEAREST));
    PyDict_SetItemString(d, "GL_NEAREST_MIPMAP_LINEAR", PyInt_FromLong(GL_NEAREST_MIPMAP_LINEAR));
    PyDict_SetItemString(d, "GL_LINEAR_MIPMAP_LINEAR", PyInt_FromLong(GL_LINEAR_MIPMAP_LINEAR));
    
    PyDict_SetItemString(d, "GL_TEXTURE_MAG_FILTER", PyInt_FromLong(GL_TEXTURE_MAG_FILTER));
    PyDict_SetItemString(d, "GL_TEXTURE_MIN_FILTER", PyInt_FromLong(GL_TEXTURE_MIN_FILTER));
    PyDict_SetItemString(d, "GL_TEXTURE_WRAP_S", PyInt_FromLong(GL_TEXTURE_WRAP_S));
    PyDict_SetItemString(d, "GL_TEXTURE_WRAP_T", PyInt_FromLong(GL_TEXTURE_WRAP_T));
    
    PyDict_SetItemString(d, "GL_TEXTURE0", PyInt_FromLong(GL_TEXTURE0));
    PyDict_SetItemString(d, "GL_TEXTURE1", PyInt_FromLong(GL_TEXTURE1));
    PyDict_SetItemString(d, "GL_TEXTURE2", PyInt_FromLong(GL_TEXTURE2));
    PyDict_SetItemString(d, "GL_TEXTURE3", PyInt_FromLong(GL_TEXTURE3));
    PyDict_SetItemString(d, "GL_TEXTURE4", PyInt_FromLong(GL_TEXTURE4));
    PyDict_SetItemString(d, "GL_TEXTURE5", PyInt_FromLong(GL_TEXTURE5));
    PyDict_SetItemString(d, "GL_TEXTURE6", PyInt_FromLong(GL_TEXTURE6));
    PyDict_SetItemString(d, "GL_TEXTURE7", PyInt_FromLong(GL_TEXTURE7));
    PyDict_SetItemString(d, "GL_TEXTURE8", PyInt_FromLong(GL_TEXTURE8));
    PyDict_SetItemString(d, "GL_TEXTURE9", PyInt_FromLong(GL_TEXTURE9));
    PyDict_SetItemString(d, "GL_TEXTURE10", PyInt_FromLong(GL_TEXTURE10));
    PyDict_SetItemString(d, "GL_TEXTURE11", PyInt_FromLong(GL_TEXTURE11));
    PyDict_SetItemString(d, "GL_TEXTURE12", PyInt_FromLong(GL_TEXTURE12));
    PyDict_SetItemString(d, "GL_TEXTURE13", PyInt_FromLong(GL_TEXTURE13));
    PyDict_SetItemString(d, "GL_TEXTURE14", PyInt_FromLong(GL_TEXTURE14));
    PyDict_SetItemString(d, "GL_TEXTURE15", PyInt_FromLong(GL_TEXTURE15));
    PyDict_SetItemString(d, "GL_TEXTURE16", PyInt_FromLong(GL_TEXTURE16));
    PyDict_SetItemString(d, "GL_TEXTURE17", PyInt_FromLong(GL_TEXTURE17));
    PyDict_SetItemString(d, "GL_TEXTURE18", PyInt_FromLong(GL_TEXTURE18));
    PyDict_SetItemString(d, "GL_TEXTURE19", PyInt_FromLong(GL_TEXTURE19));
    PyDict_SetItemString(d, "GL_TEXTURE20", PyInt_FromLong(GL_TEXTURE20));
    PyDict_SetItemString(d, "GL_TEXTURE21", PyInt_FromLong(GL_TEXTURE21));
    PyDict_SetItemString(d, "GL_TEXTURE22", PyInt_FromLong(GL_TEXTURE22));
    PyDict_SetItemString(d, "GL_TEXTURE23", PyInt_FromLong(GL_TEXTURE23));
    PyDict_SetItemString(d, "GL_TEXTURE24", PyInt_FromLong(GL_TEXTURE24));
    PyDict_SetItemString(d, "GL_TEXTURE25", PyInt_FromLong(GL_TEXTURE25));
    PyDict_SetItemString(d, "GL_TEXTURE26", PyInt_FromLong(GL_TEXTURE26));
    PyDict_SetItemString(d, "GL_TEXTURE27", PyInt_FromLong(GL_TEXTURE27));
    PyDict_SetItemString(d, "GL_TEXTURE28", PyInt_FromLong(GL_TEXTURE28));
    PyDict_SetItemString(d, "GL_TEXTURE29", PyInt_FromLong(GL_TEXTURE29));
    PyDict_SetItemString(d, "GL_TEXTURE30", PyInt_FromLong(GL_TEXTURE30));
    PyDict_SetItemString(d, "GL_TEXTURE31", PyInt_FromLong(GL_TEXTURE31));
    
    PyDict_SetItemString(d, "GL_REPEAT", PyInt_FromLong(GL_REPEAT));
    PyDict_SetItemString(d, "GL_CLAMP_TO_EDGE", PyInt_FromLong(GL_CLAMP_TO_EDGE));
    
    PyDict_SetItemString(d, "GL_PALETTE4_RGB8_OES", PyInt_FromLong(GL_PALETTE4_RGB8_OES));
    PyDict_SetItemString(d, "GL_PALETTE4_RGBA8_OES", PyInt_FromLong(GL_PALETTE4_RGBA8_OES));
    PyDict_SetItemString(d, "GL_PALETTE4_R5_G6_B5_OES", PyInt_FromLong(GL_PALETTE4_R5_G6_B5_OES));
    PyDict_SetItemString(d, "GL_PALETTE4_RGBA4_OES", PyInt_FromLong(GL_PALETTE4_RGBA4_OES));
    PyDict_SetItemString(d, "GL_PALETTE4_RGB5_A1_OES", PyInt_FromLong(GL_PALETTE4_RGB5_A1_OES));
    PyDict_SetItemString(d, "GL_PALETTE8_RGB8_OES", PyInt_FromLong(GL_PALETTE8_RGB8_OES));
    PyDict_SetItemString(d, "GL_PALETTE8_RGBA8_OES", PyInt_FromLong(GL_PALETTE8_RGBA8_OES));
    PyDict_SetItemString(d, "GL_PALETTE8_R5_G6_B5_OES", PyInt_FromLong(GL_PALETTE8_R5_G6_B5_OES));
    PyDict_SetItemString(d, "GL_PALETTE8_RGBA4_OES", PyInt_FromLong(GL_PALETTE8_RGBA4_OES));
    PyDict_SetItemString(d, "GL_PALETTE8_RGB5_A1_OES", PyInt_FromLong(GL_PALETTE8_RGB5_A1_OES));
    
    PyDict_SetItemString(d, "GL_LIGHT0", PyInt_FromLong(GL_LIGHT0));
    PyDict_SetItemString(d, "GL_LIGHT1", PyInt_FromLong(GL_LIGHT1));
    PyDict_SetItemString(d, "GL_LIGHT2", PyInt_FromLong(GL_LIGHT2));
    PyDict_SetItemString(d, "GL_LIGHT3", PyInt_FromLong(GL_LIGHT3));
    PyDict_SetItemString(d, "GL_LIGHT4", PyInt_FromLong(GL_LIGHT4));
    PyDict_SetItemString(d, "GL_LIGHT5", PyInt_FromLong(GL_LIGHT5));
    PyDict_SetItemString(d, "GL_LIGHT6", PyInt_FromLong(GL_LIGHT6));
    PyDict_SetItemString(d, "GL_LIGHT7", PyInt_FromLong(GL_LIGHT7));

#ifdef GL_OES_VERSION_1_1
    // Defines for OpenGL ES 1.1
    PyDict_SetItemString(d, "GL_OES_VERSION_1_1", PyInt_FromLong(GL_OES_VERSION_1_1));
    
    PyDict_SetItemString(d, "GL_OES_byte_coordinates", PyInt_FromLong(GL_OES_byte_coordinates));
    PyDict_SetItemString(d, "GL_OES_draw_texture", PyInt_FromLong(GL_OES_draw_texture));
    PyDict_SetItemString(d, "GL_OES_fixed_point", PyInt_FromLong(GL_OES_fixed_point));
    PyDict_SetItemString(d, "GL_OES_matrix_get", PyInt_FromLong(GL_OES_matrix_get));
    PyDict_SetItemString(d, "GL_OES_matrix_palette", PyInt_FromLong(GL_OES_matrix_palette));
    PyDict_SetItemString(d, "GL_OES_point_size_array", PyInt_FromLong(GL_OES_point_size_array));
    PyDict_SetItemString(d, "GL_OES_point_sprite", PyInt_FromLong(GL_OES_point_sprite));
    PyDict_SetItemString(d, "GL_OES_single_precision", PyInt_FromLong(GL_OES_single_precision));
    
    PyDict_SetItemString(d, "GL_CLIP_PLANE0", PyInt_FromLong(GL_CLIP_PLANE0));
    PyDict_SetItemString(d, "GL_CLIP_PLANE1", PyInt_FromLong(GL_CLIP_PLANE1));
    PyDict_SetItemString(d, "GL_CLIP_PLANE2", PyInt_FromLong(GL_CLIP_PLANE2));
    PyDict_SetItemString(d, "GL_CLIP_PLANE3", PyInt_FromLong(GL_CLIP_PLANE3));
    PyDict_SetItemString(d, "GL_CLIP_PLANE4", PyInt_FromLong(GL_CLIP_PLANE4));
    PyDict_SetItemString(d, "GL_CLIP_PLANE5", PyInt_FromLong(GL_CLIP_PLANE5));
    
    PyDict_SetItemString(d, "GL_CURRENT_COLOR", PyInt_FromLong(GL_CURRENT_COLOR));
    PyDict_SetItemString(d, "GL_CURRENT_NORMAL", PyInt_FromLong(GL_CURRENT_NORMAL));
    PyDict_SetItemString(d, "GL_CURRENT_TEXTURE_COORDS", PyInt_FromLong(GL_CURRENT_TEXTURE_COORDS));
    PyDict_SetItemString(d, "GL_POINT_SIZE", PyInt_FromLong(GL_POINT_SIZE));
    PyDict_SetItemString(d, "GL_POINT_SIZE_MIN", PyInt_FromLong(GL_POINT_SIZE_MIN));
    PyDict_SetItemString(d, "GL_POINT_SIZE_MAX", PyInt_FromLong(GL_POINT_SIZE_MAX));
    PyDict_SetItemString(d, "GL_POINT_FADE_THRESHOLD_SIZE", PyInt_FromLong(GL_POINT_FADE_THRESHOLD_SIZE));
    PyDict_SetItemString(d, "GL_POINT_DISTANCE_ATTENUATION", PyInt_FromLong(GL_POINT_DISTANCE_ATTENUATION));
    
    PyDict_SetItemString(d, "GL_LINE_WIDTH", PyInt_FromLong(GL_LINE_WIDTH));
    
    PyDict_SetItemString(d, "GL_CULL_FACE_MODE", PyInt_FromLong(GL_CULL_FACE_MODE));
    PyDict_SetItemString(d, "GL_FRONT_FACE", PyInt_FromLong(GL_FRONT_FACE));
    PyDict_SetItemString(d, "GL_SHADE_MODEL", PyInt_FromLong(GL_SHADE_MODEL));
    PyDict_SetItemString(d, "GL_DEPTH_RANGE", PyInt_FromLong(GL_DEPTH_RANGE));
    PyDict_SetItemString(d, "GL_DEPTH_WRITEMASK", PyInt_FromLong(GL_DEPTH_WRITEMASK));
    PyDict_SetItemString(d, "GL_DEPTH_CLEAR_VALUE", PyInt_FromLong(GL_DEPTH_CLEAR_VALUE));
    PyDict_SetItemString(d, "GL_DEPTH_FUNC", PyInt_FromLong(GL_DEPTH_FUNC));
    PyDict_SetItemString(d, "GL_STENCIL_CLEAR_VALUE", PyInt_FromLong(GL_STENCIL_CLEAR_VALUE));
    PyDict_SetItemString(d, "GL_STENCIL_FUNC", PyInt_FromLong(GL_STENCIL_FUNC));
    PyDict_SetItemString(d, "GL_STENCIL_VALUE_MASK", PyInt_FromLong(GL_STENCIL_VALUE_MASK));
    PyDict_SetItemString(d, "GL_STENCIL_FAIL", PyInt_FromLong(GL_STENCIL_FAIL));
    PyDict_SetItemString(d, "GL_STENCIL_PASS_DEPTH_FAIL", PyInt_FromLong(GL_STENCIL_PASS_DEPTH_FAIL));
    PyDict_SetItemString(d, "GL_STENCIL_PASS_DEPTH_PASS", PyInt_FromLong(GL_STENCIL_PASS_DEPTH_PASS));
    PyDict_SetItemString(d, "GL_STENCIL_REF", PyInt_FromLong(GL_STENCIL_REF));
    PyDict_SetItemString(d, "GL_STENCIL_WRITEMASK", PyInt_FromLong(GL_STENCIL_WRITEMASK));
    PyDict_SetItemString(d, "GL_MATRIX_MODE", PyInt_FromLong(GL_MATRIX_MODE));
    PyDict_SetItemString(d, "GL_VIEWPORT", PyInt_FromLong(GL_VIEWPORT));
    PyDict_SetItemString(d, "GL_MODELVIEW_STACK_DEPTH", PyInt_FromLong(GL_MODELVIEW_STACK_DEPTH));
    PyDict_SetItemString(d, "GL_PROJECTION_STACK_DEPTH", PyInt_FromLong(GL_PROJECTION_STACK_DEPTH));
    PyDict_SetItemString(d, "GL_TEXTURE_STACK_DEPTH", PyInt_FromLong(GL_TEXTURE_STACK_DEPTH));
    PyDict_SetItemString(d, "GL_MODELVIEW_MATRIX", PyInt_FromLong(GL_MODELVIEW_MATRIX));
    PyDict_SetItemString(d, "GL_PROJECTION_MATRIX", PyInt_FromLong(GL_PROJECTION_MATRIX));
    PyDict_SetItemString(d, "GL_TEXTURE_MATRIX", PyInt_FromLong(GL_TEXTURE_MATRIX));
    PyDict_SetItemString(d, "GL_ALPHA_TEST_FUNC", PyInt_FromLong(GL_ALPHA_TEST_FUNC));
    PyDict_SetItemString(d, "GL_ALPHA_TEST_REF", PyInt_FromLong(GL_ALPHA_TEST_REF));
    PyDict_SetItemString(d, "GL_BLEND_DST", PyInt_FromLong(GL_BLEND_DST));
    PyDict_SetItemString(d, "GL_BLEND_SRC", PyInt_FromLong(GL_BLEND_SRC));
    PyDict_SetItemString(d, "GL_LOGIC_OP_MODE", PyInt_FromLong(GL_LOGIC_OP_MODE));
    PyDict_SetItemString(d, "GL_SCISSOR_BOX", PyInt_FromLong(GL_SCISSOR_BOX));
    PyDict_SetItemString(d, "GL_SCISSOR_TEST", PyInt_FromLong(GL_SCISSOR_TEST));
    PyDict_SetItemString(d, "GL_COLOR_CLEAR_VALUE", PyInt_FromLong(GL_COLOR_CLEAR_VALUE));
    PyDict_SetItemString(d, "GL_COLOR_WRITEMASK", PyInt_FromLong(GL_COLOR_WRITEMASK));
    PyDict_SetItemString(d, "GL_UNPACK_ALIGNMENT", PyInt_FromLong(GL_UNPACK_ALIGNMENT));
    PyDict_SetItemString(d, "GL_PACK_ALIGNMENT", PyInt_FromLong(GL_PACK_ALIGNMENT));
    
    PyDict_SetItemString(d, "GL_MAX_CLIP_PLANES", PyInt_FromLong(GL_MAX_CLIP_PLANES));
    
    PyDict_SetItemString(d, "GL_POLYGON_OFFSET_UNITS", PyInt_FromLong(GL_POLYGON_OFFSET_UNITS));
    PyDict_SetItemString(d, "GL_POLYGON_OFFSET_FILL", PyInt_FromLong(GL_POLYGON_OFFSET_FILL));
    PyDict_SetItemString(d, "GL_POLYGON_OFFSET_FACTOR", PyInt_FromLong(GL_POLYGON_OFFSET_FACTOR));
    PyDict_SetItemString(d, "GL_TEXTURE_BINDING_2D", PyInt_FromLong(GL_TEXTURE_BINDING_2D));
    PyDict_SetItemString(d, "GL_VERTEX_ARRAY_SIZE", PyInt_FromLong(GL_VERTEX_ARRAY_SIZE));
    PyDict_SetItemString(d, "GL_VERTEX_ARRAY_TYPE", PyInt_FromLong(GL_VERTEX_ARRAY_TYPE));
    PyDict_SetItemString(d, "GL_VERTEX_ARRAY_STRIDE", PyInt_FromLong(GL_VERTEX_ARRAY_STRIDE));
    PyDict_SetItemString(d, "GL_NORMAL_ARRAY_TYPE", PyInt_FromLong(GL_NORMAL_ARRAY_TYPE));
    PyDict_SetItemString(d, "GL_NORMAL_ARRAY_STRIDE", PyInt_FromLong(GL_NORMAL_ARRAY_STRIDE));
    PyDict_SetItemString(d, "GL_COLOR_ARRAY_SIZE", PyInt_FromLong(GL_COLOR_ARRAY_SIZE));
    PyDict_SetItemString(d, "GL_COLOR_ARRAY_TYPE", PyInt_FromLong(GL_COLOR_ARRAY_TYPE));
    PyDict_SetItemString(d, "GL_COLOR_ARRAY_STRIDE", PyInt_FromLong(GL_COLOR_ARRAY_STRIDE));
    PyDict_SetItemString(d, "GL_TEXTURE_COORD_ARRAY_SIZE", PyInt_FromLong(GL_TEXTURE_COORD_ARRAY_SIZE));
    PyDict_SetItemString(d, "GL_TEXTURE_COORD_ARRAY_TYPE", PyInt_FromLong(GL_TEXTURE_COORD_ARRAY_TYPE));
    PyDict_SetItemString(d, "GL_TEXTURE_COORD_ARRAY_STRIDE", PyInt_FromLong(GL_TEXTURE_COORD_ARRAY_STRIDE));
    PyDict_SetItemString(d, "GL_VERTEX_ARRAY_POINTER", PyInt_FromLong(GL_VERTEX_ARRAY_POINTER));
    PyDict_SetItemString(d, "GL_NORMAL_ARRAY_POINTER", PyInt_FromLong(GL_NORMAL_ARRAY_POINTER));
    PyDict_SetItemString(d, "GL_COLOR_ARRAY_POINTER", PyInt_FromLong(GL_COLOR_ARRAY_POINTER));
    PyDict_SetItemString(d, "GL_TEXTURE_COORD_ARRAY_POINTER", PyInt_FromLong(GL_TEXTURE_COORD_ARRAY_POINTER));
    PyDict_SetItemString(d, "GL_SAMPLE_BUFFERS", PyInt_FromLong(GL_SAMPLE_BUFFERS));
    PyDict_SetItemString(d, "GL_SAMPLES", PyInt_FromLong(GL_SAMPLES));
    PyDict_SetItemString(d, "GL_SAMPLE_COVERAGE_VALUE", PyInt_FromLong(GL_SAMPLE_COVERAGE_VALUE));
    PyDict_SetItemString(d, "GL_SAMPLE_COVERAGE_INVERT", PyInt_FromLong(GL_SAMPLE_COVERAGE_INVERT));
    PyDict_SetItemString(d, "GL_IMPLEMENTATION_COLOR_READ_TYPE_OES", PyInt_FromLong(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES));
    PyDict_SetItemString(d, "GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES", PyInt_FromLong(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES));
    PyDict_SetItemString(d, "GL_NUM_COMPRESSED_TEXTURE_FORMATS", PyInt_FromLong(GL_NUM_COMPRESSED_TEXTURE_FORMATS));
    PyDict_SetItemString(d, "GL_COMPRESSED_TEXTURE_FORMATS", PyInt_FromLong(GL_COMPRESSED_TEXTURE_FORMATS));
    
    PyDict_SetItemString(d, "GL_GENERATE_MIPMAP_HINT", PyInt_FromLong(GL_GENERATE_MIPMAP_HINT));
    
    PyDict_SetItemString(d, "GL_GENERATE_MIPMAP", PyInt_FromLong(GL_GENERATE_MIPMAP));
    
    PyDict_SetItemString(d, "GL_ACTIVE_TEXTURE", PyInt_FromLong(GL_ACTIVE_TEXTURE));
    PyDict_SetItemString(d, "GL_CLIENT_ACTIVE_TEXTURE", PyInt_FromLong(GL_CLIENT_ACTIVE_TEXTURE));
    
    PyDict_SetItemString(d, "GL_ARRAY_BUFFER", PyInt_FromLong(GL_ARRAY_BUFFER));
    PyDict_SetItemString(d, "GL_ELEMENT_ARRAY_BUFFER", PyInt_FromLong(GL_ELEMENT_ARRAY_BUFFER));
    PyDict_SetItemString(d, "GL_ARRAY_BUFFER_BINDING", PyInt_FromLong(GL_ARRAY_BUFFER_BINDING));
    PyDict_SetItemString(d, "GL_ELEMENT_ARRAY_BUFFER_BINDING", PyInt_FromLong(GL_ELEMENT_ARRAY_BUFFER_BINDING));
    PyDict_SetItemString(d, "GL_VERTEX_ARRAY_BUFFER_BINDING", PyInt_FromLong(GL_VERTEX_ARRAY_BUFFER_BINDING));
    PyDict_SetItemString(d, "GL_NORMAL_ARRAY_BUFFER_BINDING", PyInt_FromLong(GL_NORMAL_ARRAY_BUFFER_BINDING));
    PyDict_SetItemString(d, "GL_COLOR_ARRAY_BUFFER_BINDING", PyInt_FromLong(GL_COLOR_ARRAY_BUFFER_BINDING));
    PyDict_SetItemString(d, "GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING", PyInt_FromLong(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING));
    PyDict_SetItemString(d, "GL_STATIC_DRAW", PyInt_FromLong(GL_STATIC_DRAW));
    PyDict_SetItemString(d, "GL_DYNAMIC_DRAW", PyInt_FromLong(GL_DYNAMIC_DRAW));
    PyDict_SetItemString(d, "GL_WRITE_ONLY", PyInt_FromLong(GL_WRITE_ONLY));
    PyDict_SetItemString(d, "GL_BUFFER_SIZE", PyInt_FromLong(GL_BUFFER_SIZE));
    PyDict_SetItemString(d, "GL_BUFFER_USAGE", PyInt_FromLong(GL_BUFFER_USAGE));
    PyDict_SetItemString(d, "GL_BUFFER_ACCESS", PyInt_FromLong(GL_BUFFER_ACCESS));
    PyDict_SetItemString(d, "GL_SUBTRACT", PyInt_FromLong(GL_SUBTRACT));
    PyDict_SetItemString(d, "GL_COMBINE", PyInt_FromLong(GL_COMBINE));
    PyDict_SetItemString(d, "GL_COMBINE_RGB", PyInt_FromLong(GL_COMBINE_RGB));
    PyDict_SetItemString(d, "GL_COMBINE_ALPHA", PyInt_FromLong(GL_COMBINE_ALPHA));
    PyDict_SetItemString(d, "GL_RGB_SCALE", PyInt_FromLong(GL_RGB_SCALE));
    PyDict_SetItemString(d, "GL_ADD_SIGNED", PyInt_FromLong(GL_ADD_SIGNED));
    PyDict_SetItemString(d, "GL_INTERPOLATE", PyInt_FromLong(GL_INTERPOLATE));
    PyDict_SetItemString(d, "GL_CONSTANT", PyInt_FromLong(GL_CONSTANT));
    PyDict_SetItemString(d, "GL_PRIMARY_COLOR", PyInt_FromLong(GL_PRIMARY_COLOR));
    PyDict_SetItemString(d, "GL_PREVIOUS", PyInt_FromLong(GL_PREVIOUS));
    PyDict_SetItemString(d, "GL_OPERAND0_RGB", PyInt_FromLong(GL_OPERAND0_RGB));
    PyDict_SetItemString(d, "GL_OPERAND1_RGB", PyInt_FromLong(GL_OPERAND1_RGB));
    PyDict_SetItemString(d, "GL_OPERAND2_RGB", PyInt_FromLong(GL_OPERAND2_RGB));
    PyDict_SetItemString(d, "GL_OPERAND0_ALPHA", PyInt_FromLong(GL_OPERAND0_ALPHA));
    PyDict_SetItemString(d, "GL_OPERAND1_ALPHA", PyInt_FromLong(GL_OPERAND1_ALPHA));
    PyDict_SetItemString(d, "GL_OPERAND2_ALPHA", PyInt_FromLong(GL_OPERAND2_ALPHA));
    PyDict_SetItemString(d, "GL_ALPHA_SCALE", PyInt_FromLong(GL_ALPHA_SCALE));
    PyDict_SetItemString(d, "GL_SRC0_RGB", PyInt_FromLong(GL_SRC0_RGB));
    PyDict_SetItemString(d, "GL_SRC1_RGB", PyInt_FromLong(GL_SRC1_RGB));
    PyDict_SetItemString(d, "GL_SRC2_RGB", PyInt_FromLong(GL_SRC2_RGB));
    PyDict_SetItemString(d, "GL_SRC0_ALPHA", PyInt_FromLong(GL_SRC0_ALPHA));
    PyDict_SetItemString(d, "GL_SRC1_ALPHA", PyInt_FromLong(GL_SRC1_ALPHA));
    PyDict_SetItemString(d, "GL_SRC2_ALPHA", PyInt_FromLong(GL_SRC2_ALPHA));
    PyDict_SetItemString(d, "GL_DOT3_RGB", PyInt_FromLong(GL_DOT3_RGB));
    PyDict_SetItemString(d, "GL_DOT3_RGBA", PyInt_FromLong(GL_DOT3_RGBA));
    PyDict_SetItemString(d, "GL_TEXTURE_CROP_RECT_OES", PyInt_FromLong(GL_TEXTURE_CROP_RECT_OES));
    PyDict_SetItemString(d, "GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES", PyInt_FromLong(GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES));
    PyDict_SetItemString(d, "GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES", PyInt_FromLong(GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES));
    PyDict_SetItemString(d, "GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES", PyInt_FromLong(GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES));
    PyDict_SetItemString(d, "GL_MAX_VERTEX_UNITS_OES", PyInt_FromLong(GL_MAX_VERTEX_UNITS_OES));
    PyDict_SetItemString(d, "GL_MAX_PALETTE_MATRICES_OES", PyInt_FromLong(GL_MAX_PALETTE_MATRICES_OES));
    PyDict_SetItemString(d, "GL_MATRIX_PALETTE_OES", PyInt_FromLong(GL_MATRIX_PALETTE_OES));
    PyDict_SetItemString(d, "GL_MATRIX_INDEX_ARRAY_OES", PyInt_FromLong(GL_MATRIX_INDEX_ARRAY_OES));
    PyDict_SetItemString(d, "GL_WEIGHT_ARRAY_OES", PyInt_FromLong(GL_WEIGHT_ARRAY_OES));
    PyDict_SetItemString(d, "GL_MATRIX_INDEX_ARRAY_SIZE_OES", PyInt_FromLong(GL_MATRIX_INDEX_ARRAY_SIZE_OES));
    PyDict_SetItemString(d, "GL_MATRIX_INDEX_ARRAY_TYPE_OES", PyInt_FromLong(GL_MATRIX_INDEX_ARRAY_TYPE_OES));
    PyDict_SetItemString(d, "GL_MATRIX_INDEX_ARRAY_STRIDE_OES", PyInt_FromLong(GL_MATRIX_INDEX_ARRAY_STRIDE_OES));
    PyDict_SetItemString(d, "GL_MATRIX_INDEX_ARRAY_POINTER_OES", PyInt_FromLong(GL_MATRIX_INDEX_ARRAY_POINTER_OES));
    PyDict_SetItemString(d, "GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES", PyInt_FromLong(GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES));
    PyDict_SetItemString(d, "GL_WEIGHT_ARRAY_SIZE_OES", PyInt_FromLong(GL_WEIGHT_ARRAY_SIZE_OES));
    PyDict_SetItemString(d, "GL_WEIGHT_ARRAY_TYPE_OES", PyInt_FromLong(GL_WEIGHT_ARRAY_TYPE_OES));
    PyDict_SetItemString(d, "GL_WEIGHT_ARRAY_STRIDE_OES", PyInt_FromLong(GL_WEIGHT_ARRAY_STRIDE_OES));
    PyDict_SetItemString(d, "GL_WEIGHT_ARRAY_POINTER_OES", PyInt_FromLong(GL_WEIGHT_ARRAY_POINTER_OES));
    PyDict_SetItemString(d, "GL_WEIGHT_ARRAY_BUFFER_BINDING_OES", PyInt_FromLong(GL_WEIGHT_ARRAY_BUFFER_BINDING_OES));
    PyDict_SetItemString(d, "GL_POINT_SIZE_ARRAY_OES", PyInt_FromLong(GL_POINT_SIZE_ARRAY_OES));
    PyDict_SetItemString(d, "GL_POINT_SIZE_ARRAY_TYPE_OES", PyInt_FromLong(GL_POINT_SIZE_ARRAY_TYPE_OES));
    PyDict_SetItemString(d, "GL_POINT_SIZE_ARRAY_STRIDE_OES", PyInt_FromLong(GL_POINT_SIZE_ARRAY_STRIDE_OES));
    PyDict_SetItemString(d, "GL_POINT_SIZE_ARRAY_POINTER_OES", PyInt_FromLong(GL_POINT_SIZE_ARRAY_POINTER_OES));
    PyDict_SetItemString(d, "GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES", PyInt_FromLong(GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES));
    PyDict_SetItemString(d, "GL_POINT_SPRITE_OES", PyInt_FromLong(GL_POINT_SPRITE_OES));
    PyDict_SetItemString(d, "GL_COORD_REPLACE_OES", PyInt_FromLong(GL_COORD_REPLACE_OES));
#endif /* GL_OES_VERSION_1_1 */

  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif /*EKA2*/
