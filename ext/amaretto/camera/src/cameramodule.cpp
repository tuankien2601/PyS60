/**
 * ====================================================================
 *  cameramodule.cpp
 *
 *
 *  Python API to camera, slightly modified from "miso" (HIIT), "CameraApp" 
 *  and video example application (available from Forum Nokia). 
 *
 *  Implements currently following Python types and functions:
 *
 *  Camera
 *   string take_photo([mode=value, size=valuer zoom=value, flash=value, 
 *                      exp=value, white=value, position=value])
 *      takes a photo, returns the image AND ownership in CFbsBitmap. 
 *      This operation fails with "SymbianError: KErrInUse" if some other 
 *      application is using the camera.
 *
 *      size
 *        image size (resolution)
 *      mode
 *        number of colors, defaults to EFormatFbsBitmapColor64K 
 *      zoom
 *        digital zoom factor, defaults to 0
 *      flash
 *        defaults to EFlashNone
 *      exp
 *        the exposure adjustment of the device, defaults to EExposureAuto
 *      white
 *        white balance, default EWBAuto
 *      position
 *        camera position, defaults to 0
 *
 *   image_modes()
 *   max_image_size()
 *   max_zoom()
 *   flash_modes()
 *   exp_modes()
 *   white_modes()
 *   cameras_available()
 *   start_finder(callable)
 *   stop_finder()
 *   release()
 *   handle()
 *
 *  Video
 *   start_record()
 *   stop_record()
 *    
 *  TODO 
 * 
 *  o The current implementation of finder/take photo does not take
 *    into consideration that camera might be reserved and powered
 *    already. The code needs refactoring when the Python interface
 *    is changed to utilize "Camera" type.
 *  o Add the support for other video sizes
 *
 * Copyright 2005 Helsinki Institute for Information Technology (HIIT)
 * and the authors.  All rights reserved.
 * Authors: Tero Hasu <tero.hasu@hut.fi>
 *
 * Portions Copyright (c) 2005-2007 Nokia Corporation 
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ====================================================================
 */

#include <e32std.h>

#include "cameramodule.h"

//////////////TYPE METHODS VIDEO CAMERA

/*
 * Deallocate video camera.
 */
extern "C" {
  static void vid_dealloc(VID_object *vido)
  {
    if (vido->video) {
      delete vido->video;
      vido->video = NULL;
    }
    if (vido->callBackSet) {
      Py_XDECREF(vido->myCallBack.iCb);
    }
    PyObject_Del(vido);
  }
}

/*
 * Allocate video camera.
 */
extern "C" PyObject *
new_vid_object(PyObject* /*self*/, PyObject *args)
{
  VID_object *vido = PyObject_New(VID_object, VID_type);
  if (vido == NULL)
    return PyErr_NoMemory();

  TRAPD(error, vido->video = CVideoCamera::NewL());
  if (error != KErrNone){
    PyObject_Del(vido);
    return SPyErr_SetFromSymbianOSErr(error);    
  }
  
  vido->callBackSet = EFalse;
  
  return (PyObject*) vido;
}

/*
 * Start video recording
 */
extern "C" PyObject *
vid_start_record(VID_object* self, PyObject *args)
{
  TInt handle;
  PyObject* filename = NULL;
  PyObject* c;
  TInt error = KErrNone;
  
  if (!PyArg_ParseTuple(args, "iUO", &handle, &filename, &c)) 
    return NULL;

  if ((filename && PyUnicode_GetSize(filename) > KMaxFileName)) {
    PyErr_SetString(PyExc_NameError, "filename too long");
    return NULL;
  }

  TPtrC videofile((TUint16*) PyUnicode_AsUnicode(filename), 
                    PyUnicode_GetSize(filename));

  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  if (self->callBackSet) {
    Py_XDECREF(self->myCallBack.iCb);
  }

  Py_XINCREF(c);
  
  self->myCallBack.iCb = c;
  self->callBackSet = ETrue;

  self->video->SetCallBack(self->myCallBack);
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(error, self->video->StartRecordL(handle, videofile));
  Py_END_ALLOW_THREADS
  
  if (error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);    
  }
    
  Py_INCREF(Py_None);
  return Py_None;  
}

/*
 * Stop video recording
 */
extern "C" PyObject *
vid_stop_record(VID_object* self)
{
  self->video->StopRecord();
  
  Py_INCREF(Py_None);
  return Py_None;  
}

//////////////TYPE METHODS CAMERA

/*
 * Deallocate cam.
 */
extern "C" {
  static void cam_dealloc(CAM_object *camo)
  {
    if (camo->camera) {
      delete camo->camera;
      camo->camera = NULL;
    }
    if (camo->callBackSet) {
      Py_XDECREF(camo->myCallBack.iCb);
    }
    PyObject_Del(camo);
  }
}

/*
 * Allocate cam.
 */
extern "C" PyObject *
new_cam_object(PyObject* /*self*/, PyObject *args)
{
  TInt position;
  if (!PyArg_ParseTuple(args, "i", &position))
    return NULL;
  CAM_object *camo = PyObject_New(CAM_object, CAM_type);
  if (camo == NULL)
    return PyErr_NoMemory();

  TRAPD(error, camo->camera = CMisoPhotoTaker::NewL(position));
  if (error != KErrNone){
    PyObject_Del(camo);
    return SPyErr_SetFromSymbianOSErr(error);    
  }
  
  camo->callBackSet = EFalse;
  camo->cameraUsed = ETrue;
  
  return (PyObject*) camo;
}

/*
 * Take photo.
 */
extern "C" PyObject *
cam_take_photo(CAM_object* self, PyObject *args, PyObject *keywds)
{
  // defaults:
  TInt mode = CCamera::EFormatFbsBitmapColor64K;
  TInt size = 0; 
  TInt zoom = 0;
  TInt flash = CCamera::EFlashNone;
  TInt exp = CCamera::EExposureAuto;
  TInt white = CCamera::EWBAuto;
  TInt position = 0; 
  
  TInt error = KErrNone;  

  PyObject *ret = NULL;
  
  static const char *const kwlist[] = 
    {"mode", "size", "zoom", "flash", "exp", "white", "position", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|iiiiiii", (char**)kwlist,
                                   &mode, 
                                   &size,
                                   &zoom,
                                   &flash,
                                   &exp, 
                                   &white,
                                   &position)){ 
    return NULL;
  }

  // TODO was this only for the "position"?
  // TODO work on this when there is a real "Camera" type in wrapper also
  
  // re-initialization of camera if needed
  // this is in-efficient but works
  if(self->cameraUsed) 
  {
   	delete self->camera;
   	self->camera = NULL;
    TRAPD(error, self->camera = CMisoPhotoTaker::NewL(position));
    if (error != KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);    
    }   
  }
  self->cameraUsed = ETrue;

  Py_BEGIN_ALLOW_THREADS
  TRAP(error, self->camera->TakePhotoL(mode, size, zoom, flash, exp, white));
  Py_END_ALLOW_THREADS
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if (mode == CCamera::EFormatExif || mode == CCamera::EFormatJpeg) {
    HBufC8* data = NULL;
    data = self->camera->GetJpg();
    
    ret = Py_BuildValue("s#", data->Ptr(), data->Length());
    
    delete data; // the data was copied in above - need to delete
      
  } else {
    CFbsBitmap* bmp = NULL;
    bmp = self->camera->GetBitmap();

    ret = PyCObject_FromVoidPtr(bmp, NULL);
    
    // no deletion here as we pass the ownership
  }
  
  if (!ret)
    // should have set an exception
    return NULL;
  return ret;
}

/*
 * Start view finder.
 */
extern "C" PyObject *
cam_start_finder(CAM_object* self, PyObject *args)
{
  PyObject* c;
  TInt xSize = 0;
  TInt ySize = 0;
  
  if (!PyArg_ParseTuple(args, "O(ii)", &c, &xSize, &ySize))
    return NULL;
  
  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Py_XINCREF(c);
  
  self->myCallBack.iCb = c;
  self->callBackSet = ETrue;

  self->camera->SetCallBack(self->myCallBack);
  Py_BEGIN_ALLOW_THREADS
  self->camera->StartViewFinder(xSize, ySize);
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Stop view finder.
 */
extern "C" PyObject *
cam_stop_finder(CAM_object* self, PyObject /**args*/)
{
  if (self->callBackSet) {
    Py_BEGIN_ALLOW_THREADS
    self->camera->StopViewFinder();
    Py_END_ALLOW_THREADS
    self->camera->UnSetCallBack();
  
    Py_XDECREF(self->myCallBack.iCb);
    self->callBackSet = EFalse;  
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Returns the image modes.
 */
extern "C" PyObject *
cam_image_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetImageModes());
}

/*
 * Returns the maximum image size (camera internal).
 */
extern "C" PyObject *
cam_max_image_size(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetImageSizeMax());
}

/*
 * Returns the image size possible with specified color mode and index.
 */
extern "C" PyObject *
cam_image_size(CAM_object* self, PyObject *args)
{
  TInt mode;
  TInt index;

  if (!PyArg_ParseTuple(args, "ii", &mode, &index))
    return NULL;

  TSize size(0, 0);
  self->camera->GetImageSize(size, index, static_cast<CCamera::TFormat>(mode));
  return Py_BuildValue("(ii)", size.iWidth, size.iHeight);
}

/*
 * Returns the maximum digital zoom supported in the device.
 */
extern "C" PyObject *
cam_max_zoom(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetMaxZoom());
}

/*
 * Returns the flash modes (in bitfield) supported in the device.
 */
extern "C" PyObject *
cam_flash_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetFlashModes());
}

/*
 * Returns the exposure modes (in bitfield) supported in the device.
 */
extern "C" PyObject *
cam_exposure_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetExpModes());
}

/*
 * Returns the white balance modes (in bitfield) supported in the device.
 */
extern "C" PyObject *
cam_white_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetWhiteModes());
}

/*
 * Returns the available cameras.
 */
extern "C" PyObject *
cam_cameras_available(CAM_object* self)
{
  return Py_BuildValue("i", CCamera::CamerasAvailable());
}

/*
 * Returns the camera handle.
 */
extern "C" PyObject *
cam_handle(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetHandle());
}

/*
 * Return whether there is a photo request ongoing.
 */
extern "C" PyObject *
cam_taking_photo(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->TakingPhoto());
}

/*
 * Return whether the view finder is on.
 */
extern "C" PyObject *
cam_finder_on(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->ViewFinderOn());
}

/*
 * Release the camera for other process to use.
 */
extern "C" PyObject *
cam_release(CAM_object* self)
{
  self->camera->Release();
  
  Py_INCREF(Py_None);
  return Py_None;  
}

//////////////TYPE SET CAMERA

extern "C" {

  static const PyMethodDef cam_methods[] = {
    {"take_photo", (PyCFunction)cam_take_photo, METH_VARARGS | METH_KEYWORDS, NULL},
    {"start_finder", (PyCFunction)cam_start_finder, METH_VARARGS, NULL},
    {"stop_finder", (PyCFunction)cam_stop_finder, METH_NOARGS, NULL},
    {"image_modes", (PyCFunction)cam_image_modes, METH_NOARGS, NULL},
    {"max_image_size", (PyCFunction)cam_max_image_size, METH_NOARGS, NULL},
    {"image_size", (PyCFunction)cam_image_size, METH_VARARGS, NULL},
    {"max_zoom", (PyCFunction)cam_max_zoom, METH_NOARGS, NULL},
    {"flash_modes", (PyCFunction)cam_flash_modes, METH_NOARGS, NULL},
    {"exposure_modes", (PyCFunction)cam_exposure_modes, METH_NOARGS, NULL},
    {"white_balance_modes", (PyCFunction)cam_white_modes, METH_NOARGS, NULL},
    {"cameras_available", (PyCFunction)cam_cameras_available, METH_NOARGS, NULL},
    {"handle", (PyCFunction)cam_handle, METH_NOARGS, NULL},
    {"taking_photo", (PyCFunction)cam_taking_photo, METH_NOARGS, NULL},
    {"finder_on", (PyCFunction)cam_finder_on, METH_NOARGS, NULL},
    {"release", (PyCFunction)cam_release, METH_NOARGS, NULL},
    {NULL, NULL, 0 , NULL}             /* sentinel */
  };
  
  static PyObject *
  cam_getattr(CAM_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)cam_methods, (PyObject *)op, name);
  }
  
  static const PyTypeObject c_cam_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_camera.Camera",                         /*tp_name*/
    sizeof(CAM_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)cam_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)cam_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };

//////////////TYPE SET VIDEO CAMERA  
  
  static const PyMethodDef vid_methods[] = {
    {"start_record", (PyCFunction)vid_start_record, METH_VARARGS, NULL},
    {"stop_record", (PyCFunction)vid_stop_record, METH_NOARGS, NULL},
    {NULL, NULL, 0 , NULL}             /* sentinel */
  };
  
  static PyObject *
  vid_getattr(VID_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)vid_methods, (PyObject *)op, name);
  }
  
  static const PyTypeObject c_vid_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_camera.Video",                          /*tp_name*/
    sizeof(VID_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)vid_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)vid_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };  
  
} //extern C

//////////////INIT

extern "C" {

  static const PyMethodDef camera_methods[] = {
    {"Camera", (PyCFunction)new_cam_object, METH_VARARGS, NULL},
    {"Video", (PyCFunction)new_vid_object, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initcamera(void)
  {
    PyTypeObject* cam_type = PyObject_New(PyTypeObject, &PyType_Type);
    *cam_type = c_cam_type;
    cam_type->ob_type = &PyType_Type;

    PyTypeObject* vid_type = PyObject_New(PyTypeObject, &PyType_Type);
    *vid_type = c_vid_type;
    vid_type->ob_type = &PyType_Type;

    SPyAddGlobalString("CAMType", (PyObject*)cam_type);
    SPyAddGlobalString("VIDType", (PyObject*)vid_type);

    PyObject *m, *d;

    m = Py_InitModule("_camera", (PyMethodDef*)camera_methods);
    d = PyModule_GetDict(m);
    
    PyDict_SetItemString(d,"EColor4K", PyInt_FromLong(CCamera::EFormatFbsBitmapColor4K));
    PyDict_SetItemString(d,"EColor64K", PyInt_FromLong(CCamera::EFormatFbsBitmapColor64K));
    PyDict_SetItemString(d,"EColor16M", PyInt_FromLong(CCamera::EFormatFbsBitmapColor16M));
    PyDict_SetItemString(d,"EFormatExif", PyInt_FromLong(CCamera::EFormatExif));
    PyDict_SetItemString(d,"EFormatJpeg", PyInt_FromLong(CCamera::EFormatJpeg));    
    
    PyDict_SetItemString(d,"EFlashNone", PyInt_FromLong(CCamera::EFlashNone));
    PyDict_SetItemString(d,"EFlashAuto", PyInt_FromLong(CCamera::EFlashAuto));
    PyDict_SetItemString(d,"EFlashForced", PyInt_FromLong(CCamera::EFlashForced));
    PyDict_SetItemString(d,"EFlashFillIn", PyInt_FromLong(CCamera::EFlashFillIn));
    PyDict_SetItemString(d,"EFlashRedEyeReduce", PyInt_FromLong(CCamera::EFlashRedEyeReduce));
    
    PyDict_SetItemString(d,"EExposureAuto", PyInt_FromLong(CCamera::EExposureAuto));
    PyDict_SetItemString(d,"EExposureNight", PyInt_FromLong(CCamera::EExposureNight));
    PyDict_SetItemString(d,"EExposureBacklight", PyInt_FromLong(CCamera::EExposureBacklight));
    PyDict_SetItemString(d,"EExposureCenter", PyInt_FromLong(CCamera::EExposureCenter));
    
    PyDict_SetItemString(d,"EWBAuto", PyInt_FromLong(CCamera::EWBAuto));
    PyDict_SetItemString(d,"EWBDaylight", PyInt_FromLong(CCamera::EWBDaylight));
    PyDict_SetItemString(d,"EWBCloudy", PyInt_FromLong(CCamera::EWBCloudy));
    PyDict_SetItemString(d,"EWBTungsten", PyInt_FromLong(CCamera::EWBTungsten));
    PyDict_SetItemString(d,"EWBFluorescent", PyInt_FromLong(CCamera::EWBFluorescent));
    PyDict_SetItemString(d,"EWBFlash", PyInt_FromLong(CCamera::EWBFlash));
    
    // video
    PyDict_SetItemString(d,"EOpenComplete", PyInt_FromLong(CVideoCamera::EOpenComplete));
    PyDict_SetItemString(d,"EPrepareComplete", PyInt_FromLong(CVideoCamera::EPrepareComplete));
    PyDict_SetItemString(d,"ERecordComplete", PyInt_FromLong(CVideoCamera::ERecordComplete)); 
  }
  
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif /*EKA2*/
