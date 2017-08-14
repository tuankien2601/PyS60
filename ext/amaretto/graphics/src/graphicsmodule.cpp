/**
 * ====================================================================
 * graphicsmodule.cpp
 *
 * Copyright (c) 2005-2008 Nokia Corporation
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

#include "Python.h"
#include "symbian_python_ext_util.h"
#include <pythread.h>

#if SERIES60_VERSION > 12
#define ICL_SUPPORT
#endif

#include <e32std.h>
#include <w32std.h>
#include <eikenv.h>
#include <f32file.h>
#include <s32mem.h>
#include <apmstd.h>
#include <apparc.h>
#include <aknenv.h>
#include <aknappui.h>
#include <aknapp.h>
#include <eikon.hrh>          // Form
#include <avkon.hrh>
#include <avkon.rsg>
#include <eikedwin.h>         // Form
#include <eikcapc.h>          // Form
#include <txtfrmat.h>         // Text
#include <txtfmlyr.h>         // Text
#include <eikgted.h>          // Text
#include <eikrted.h>	      // RText
#include <txtrich.h>
#include <gdi.h>	      // RText
#include <aknlists.h>
#include <aknselectionlist.h>
#include <aknquerydialog.h>
#include <akntitle.h>
#include <aknnotewrappers.h>
#include <aknPopup.h>
#include <AknForm.h>
#include <documenthandler.h>


#ifdef ICL_SUPPORT
#include <ImageConversion.h>
#include <BitmapTransforms.h>
#endif

#include "colorspec.cpp"
#include "fontspec.cpp"

//#include <ImageCodecData.h>

#ifdef EKA2
#include <akniconutils.h> 
#endif


#define PY_RETURN_NONE do {			\
    Py_INCREF(Py_None);				\
    return Py_None;				\
  } while (0)					\


// ******** Image object

/*
 * A helper function for the implementation of callbacks
 * from C/C++ code to Python callables
 */

static TInt app_callback_handler(void* func, void* arg);


static TInt app_callback_handler(void* func, void* arg)
{
  TInt error = KErrNone;
  
  PyObject* rval = PyEval_CallObject((PyObject*)func, (PyObject*)arg);
  
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
    }
  }
  else
    Py_DECREF(rval);

  return error;
}

// Helper function for extracting C API's from objects.
static PyObject *
PyCAPI_GetCAPI(PyObject *object, 
	       const char *apiname)
{
  PyObject *capi_func=NULL;
  PyObject *capi_cobject=NULL;
  if (!(capi_func=PyObject_GetAttrString(object,(char *)apiname)) ||
      !PyCallable_Check(capi_func) ||
      !(capi_cobject=PyObject_CallObject(capi_func,NULL)) ||
      !(PyCObject_Check(capi_cobject))) {
    Py_XDECREF(capi_func);
    Py_XDECREF(capi_cobject);
    return NULL;
  }
  Py_DECREF(capi_func);
  return capi_cobject;
}

/*
 * Compute how many coordinate pairs are in the given coordinate sequence. 
 * Store result in *length.
 * 
 * Return: 1 if successful, 0 if error.
 */
static int 
PyCoordSeq_Length(PyObject *coordseq_obj, int *length)
{
#define ERROR(msg) do { PyErr_SetString(PyExc_ValueError,"invalid coordinates: " msg); goto error; } while(0)
  PyObject *item=NULL, *subitem=NULL;
  int seq_len=0;
  if (!PySequence_Check(coordseq_obj)) 
    ERROR("not a sequence");
  seq_len=PySequence_Length(coordseq_obj);
  item=PySequence_GetItem(coordseq_obj,0);
  if (!item) 
    goto error; // propagate error;
  if (PyInt_Check(item)||PyFloat_Check(item)) { // Coordinate sequence style (x,y,x,y,x,y...)
    if (seq_len % 2) 
      ERROR("sequence length is odd"); 
    Py_DECREF(item);
    *length=seq_len/2;
    return 1;
  } else if (PySequence_Check(item)) { // Coordinate sequence style ((x,y),(x,y),(x,y)...)
    subitem=PySequence_GetItem(item,0);
    if (!subitem) 
      goto error; // propagate error;
    if (!PyInt_Check(subitem) && !PyFloat_Check(subitem)) 
      ERROR("non-number in sequence");
    Py_DECREF(subitem);
    Py_DECREF(item);
    *length=seq_len;
    return 1;
  } else
    ERROR("sequence of numbers or 2-tuples expected");
 error:
  Py_XDECREF(subitem);
  Py_XDECREF(item);
  return NULL;
}

/*
 * Get a coordinate pair out of the given coordinate sequence.
 * Store result in *x, *y.
 * Return: 1 if successful, 0 if error.
 */
static int 
PyCoordSeq_GetItem(PyObject *coordseq_obj, int index, int *x, int *y)
{
  int seq_len=PySequence_Length(coordseq_obj);
  PyObject *item=NULL, *x_obj=NULL, *y_obj=NULL;
  if (!PySequence_Check(coordseq_obj) || seq_len == 0) 
    ERROR("not a sequence or empty sequence");
  item=PySequence_GetItem(coordseq_obj,0);
  if (!item) 
    goto error; // propagate error
  if (PyInt_Check(item)||PyFloat_Check(item)) { // Coordinate sequence style (x,y,x,y,x,y...)
    if (!(x_obj=PySequence_GetItem(coordseq_obj,index*2)) ||
	!(y_obj=PySequence_GetItem(coordseq_obj,index*2+1)))
      goto error; // propagate error
  } else if (PySequence_Check(item)) { // Coordinate sequence style ((x,y),(x,y),(x,y)...)
    Py_DECREF(item);    
    if (!(item=PySequence_GetItem(coordseq_obj,index)) ||
	!(x_obj=PySequence_GetItem(item,0)) ||
	!(y_obj=PySequence_GetItem(item,1)))
      goto error;
  } else
    ERROR("sequence of numbers or 2-tuples expected");
  Py_XDECREF(item); item=NULL;

  if (PyInt_Check(x_obj))
    *x=PyInt_AsLong(x_obj);
  else if (PyFloat_Check(x_obj))
    *x=(int)PyFloat_AsDouble(x_obj);
  else 
    ERROR("X coordinate non-numeric");
  if (PyInt_Check(y_obj))
    *y=PyInt_AsLong(y_obj);
  else if (PyFloat_Check(y_obj))
    *y=(int)PyFloat_AsDouble(y_obj);
  else 
    ERROR("Y coordinate non-numeric");
  Py_XDECREF(x_obj);
  Py_XDECREF(y_obj);
  return 1;

error:
  Py_XDECREF(x_obj);
  Py_XDECREF(y_obj);
  Py_XDECREF(item);
  return NULL;
#undef ERROR
}


#define IS_OBJECT(obj,name) ((obj)->ob_type==(name##_type))

static CFbsBitmap *
Bitmap_AsFbsBitmap(PyObject *obj)
{
  PyObject *capi_object=PyCAPI_GetCAPI(obj,"_bitmapapi");
  if (!capi_object)
    return NULL;
  CFbsBitmap *bitmap=(CFbsBitmap *)PyCObject_AsVoidPtr(capi_object);
  Py_DECREF(capi_object);
  return bitmap;
}

static int 
Bitmap_Check(PyObject *obj)
{
  return Bitmap_AsFbsBitmap(obj)?1:0;
}

#define PANIC(label) User::Panic(_L(label),__LINE__)
#define DEBUG_ASSERT(expr) __ASSERT_DEBUG((expr), User::Panic(_L(#expr), __LINE__))

static void
CloseFbsSession(void)
{
  RFbsSession::Disconnect();
}

static void
EnsureFbsSessionIsOpen(void)
{
  if (!RFbsSession::GetSession()) {
    RFbsSession::Connect();
    PyThread_AtExit(CloseFbsSession);
  }
}

#define Image_type ((PyTypeObject*)SPyGetGlobalString("ImageType"))
struct ImageObject : public CActive {
  enum ImageState {
#ifdef ICL_SUPPORT
    EPendingLoad, 
    EPendingSave, 
    EPendingScale, 
    EPendingRotate, 
#endif 
    ENormalIdle, 
    EInvalid
  } iState;
  enum ImageFormat {EJpeg,EPng};

  ImageObject():
    CActive(EPriorityStandard),
    iState(EInvalid)
  {
    CActiveScheduler::Add(this);
  }
  
  void ConstructL(CFbsBitmap *aBitmap) {
    EnsureFbsSessionIsOpen();        
    iBitmap=aBitmap;
    iState=ENormalIdle;
  }

  void ConstructL(int xsize, int ysize, int mode)
  {
    EnsureFbsSessionIsOpen();    
    iBitmap=new CFbsBitmap();
    TSize sz(xsize,ysize);
    iBitmap->Create(sz,(enum TDisplayMode) mode);
    //   iBitmapDevice=CFbsBitmapDevice::NewL(iBitmap);        
    iState=ENormalIdle;
  }
#ifdef ICL_SUPPORT
  void ConstructL(TDesC& filename)
  {   
    EnsureFbsSessionIsOpen();
    iDecoder=CImageDecoder::FileNewL(FsSession(),filename);    
    const TFrameInfo *frmInfo=&iDecoder->FrameInfo();        
    iBitmap=new (ELeave) CFbsBitmap();    
    User::LeaveIfError(iBitmap->Create(frmInfo->iFrameCoordsInPixels.Size(),EColor64K));
    //iBitmapDevice=CFbsBitmapDevice::NewL(iBitmap);    
    iDecoder->Convert(&iStatus, *iBitmap);    
    iState=EPendingLoad;
    SetActive();    
  }

  int StartLoadL(TDesC& aFilename) {
    EnsureFbsSessionIsOpen();
    DEBUG_ASSERT(iState == ENormalIdle);
    iDecoder=CImageDecoder::FileNewL(FsSession(),aFilename);
    const TFrameInfo *frmInfo=&iDecoder->FrameInfo();        
    if (frmInfo->iFrameCoordsInPixels != iBitmap->SizeInPixels()) {
      delete iDecoder;
      return 0;
    }
    iDecoder->Convert(&iStatus, *iBitmap);    
    iState=EPendingLoad;
    SetActive();    
    return 1;
  }

  static void InspectFileL(TDesC& aFilename, int &aWidth, int &aHeight, int &aMode) {
    EnsureFbsSessionIsOpen();
    //RFs *rfs=&graphics_GetRFs();
    RFs rfs;
    rfs.Connect();
    CImageDecoder *decoder=
      CImageDecoder::FileNewL(rfs,aFilename);    
    const TFrameInfo *frmInfo=&decoder->FrameInfo();        
    aWidth=frmInfo->iFrameCoordsInPixels.Size().iWidth;
    aHeight=frmInfo->iFrameCoordsInPixels.Size().iHeight;
    aMode=frmInfo->iFrameDisplayMode;
    delete decoder;
    rfs.Close();
  }  
#endif // ICL_SUPPORT
  ~ImageObject() {
    Cancel();
    //delete iBitmapDevice;
    delete iBitmap;
    Py_XDECREF(iCompletionCallback); 
    iCompletionCallback=NULL;
#ifdef ICL_SUPPORT
    delete iFrameImageData;
    delete iEncoder;
    delete iDecoder;
    delete iScaler;
    delete iRotator;
    if (iFsSession) {
      iFsSession->Close();
      delete iFsSession; iFsSession=NULL;
    }    
#endif // ICL_SUPPORT
  }  
  
  TSize Size() {
    return iBitmap->SizeInPixels();
  }  
  
  TSize TwipSize() {
    return iBitmap->SizeInTwips();
  }
  
  void SetTwipSize(TInt aWidth, TInt aHeight) {
    TSize sizeInTwip(aWidth, aHeight);
    iBitmap->SetSizeInTwips(sizeInTwip);
  }

  TDisplayMode DisplayMode() {
    return iBitmap->DisplayMode();
  }  

#ifdef ICL_SUPPORT
  void StartSaveL(const TDesC& aFilename, ImageFormat aFormat, 
		  int aQuality, 
		  TPngEncodeData::TPngCompressLevel aCompressionLevel, int aBpp, int aColor) {
    DEBUG_ASSERT(iState == ENormalIdle);
    EnsureFbsSessionIsOpen();
    switch (aFormat) {
    case EJpeg: {
      // For some reason iFsSession causes a KERN-EXEC 0 panic here. I have no idea why.
      iEncoder = CImageEncoder::FileNewL(FsSession(), aFilename, 
					 CImageEncoder::EOptionNone, KImageTypeJPGUid);
      TJpegImageData *imageData=new (ELeave) TJpegImageData;
      CleanupStack::PushL(imageData);
      imageData->iSampleScheme=TJpegImageData::EColor444;
      imageData->iQualityFactor=aQuality;
      iFrameImageData=CFrameImageData::NewL();
      iFrameImageData->AppendImageData(imageData); // assumes ownership of imageData
      CleanupStack::Pop(imageData);
      break;
    }
    case EPng: {
      // For some reason iFsSession causes a KERN-EXEC 0 panic here. I have no idea why.
      iEncoder = CImageEncoder::FileNewL(FsSession(), aFilename, 
					 CImageEncoder::EOptionNone, KImageTypePNGUid);
      TPngEncodeData *frameData=new (ELeave) TPngEncodeData;
      CleanupStack::PushL(frameData);
      frameData->iLevel=aCompressionLevel;
      frameData->iBitsPerPixel=aBpp;
      frameData->iColor=aColor;
      frameData->iPaletted=EFalse;
      iFrameImageData=CFrameImageData::NewL();
      iFrameImageData->AppendFrameData(frameData); // assumes ownership of frameData
      CleanupStack::Pop(frameData);
      break;
    }
    default:
      PANIC("ImageObject");
    }
    iEncoder->Convert(&iStatus, *iBitmap, iFrameImageData);
    iState=EPendingSave;
    SetActive();
  }
  
  void StartScaleL(CFbsBitmap *aTargetBitmap,int aMaintainAspectRatio=EFalse) {
    DEBUG_ASSERT(iState == ENormalIdle);
    EnsureFbsSessionIsOpen();
    iScaler = CBitmapScaler::NewL();
    iScaler->Scale(&iStatus, *iBitmap, *aTargetBitmap, aMaintainAspectRatio); 
    iState=EPendingScale;
    SetActive();
  }  

  void StartRotateL(CFbsBitmap *aTargetBitmap, CBitmapRotator::TRotationAngle aDirection) {
    DEBUG_ASSERT(iState == ENormalIdle);
    EnsureFbsSessionIsOpen();
    iRotator = CBitmapRotator::NewL();
    iRotator->Rotate(&iStatus, *iBitmap, *aTargetBitmap, aDirection);
    iState=EPendingRotate;
    SetActive();
  }  
#endif // ICL_SUPPORT
  
  void RunL() {
    // Note: Even if save, rotation or scale fails, the original
    // bitmap should still be intact, so we are safe to switch back to
    // ENormalIdle state.
    switch (iState) {
#ifdef ICL_SUPPORT
    case EPendingLoad:
      delete iDecoder; iDecoder=NULL;      
      iState=ENormalIdle;
      break;
    case EPendingSave:
      delete iEncoder; iEncoder=NULL;
      delete iFrameImageData; iFrameImageData=NULL;
      iState=ENormalIdle; 
      break;
    case EPendingScale:
      delete iScaler; iScaler=NULL;
      iState=ENormalIdle; 
      
      break;
    case EPendingRotate:
      delete iRotator; iRotator=NULL;
      iState=ENormalIdle;       
      break;
#endif // ICL_SUPPORT
    default:
      PANIC("ImageObject");
    }
    PyGILState_STATE gstate = PyGILState_Ensure();
    InvokeCompletionCallback();
    PyGILState_Release(gstate);
  }
  
  /* Stop the current operation, if any. If a completion callback has
     been set, DECREF it. */
  void Stop() {
    Cancel(); // DoCancel sets the proper state.
    // Note that this can indirectly cause this object to be destroyed!
    Py_XDECREF(iCompletionCallback);
    iCompletionCallback=NULL;
  }
  void DoCancel() {
    switch(iState) {
    case ENormalIdle:
    case EInvalid:
      break;
#ifdef ICL_SUPPORT
    case EPendingLoad: 
      iDecoder->Cancel(); 
      delete iDecoder; iDecoder=NULL;
      iState=ENormalIdle;
      break;
    case EPendingSave: 
      iEncoder->Cancel(); 
      delete iEncoder; iEncoder=NULL;
      delete iFrameImageData; iFrameImageData=NULL;
      iState=ENormalIdle;      
      break;
    case EPendingScale: 
      iScaler->Cancel(); 
      delete iScaler;
      iScaler=NULL;
      iState=ENormalIdle;      
      break;
    case EPendingRotate: 
      iRotator->Cancel(); 
      delete iRotator;
      iRotator=NULL;
      iState=ENormalIdle;      
      break;
#endif // ICL_SUPPORT
    default: PANIC("ImageObject");
    }
  }
  /*  CBitmapDevice *GetDeviceL() {    
      CBitmapDevice *device=CFbsBitmapDevice::NewL(iBitmap);
      return device;
      } */   
  CFbsBitmap *GetBitmap() {
    return iBitmap;
  }    
  void GetPixel(TRgb &aColor, const TPoint &aPixel) const {
    EnsureFbsSessionIsOpen();
    iBitmap->GetPixel(aColor, aPixel);
  }

  void SetCompletionCallback(PyObject *callback) {
    Py_XDECREF(iCompletionCallback);
    iCompletionCallback=callback;
    Py_XINCREF(callback);
  }  
  
private:
  void InvokeCompletionCallback() {
    if (iCompletionCallback) {
      PyObject *arg=Py_BuildValue("(i)",iStatus.Int());
      app_callback_handler(iCompletionCallback,arg);
      if (PyErr_Occurred())
	PyErr_Print();
      Py_DECREF(arg);
      Py_DECREF(iCompletionCallback);
      iCompletionCallback=NULL;
    }
  }
  RFs &FsSession();
  PyObject *iCompletionCallback;
  CFbsBitmap *iBitmap;
#ifdef ICL_SUPPORT
  CImageDecoder *iDecoder;
  CImageEncoder *iEncoder;
  CBitmapScaler *iScaler;
  CBitmapRotator *iRotator;
  CFrameImageData *iFrameImageData;
#endif // ICL_SUPPORT
  RFs *iFsSession;
};

RFs &
ImageObject::FsSession()
{
  if (!iFsSession) {
    iFsSession=new RFs;
    iFsSession->Connect();
  }
  return *iFsSession;
}


struct Image_object {
  PyObject_VAR_HEAD
  ImageObject* data;
};

/* Create an ImageObject from a CObject that contains a CFbsBitmap *.
   The ImageObject assumes ownership of the CFbsBitmap. */
extern "C" PyObject *
graphics_ImageFromCFbsBitmap(PyObject* /*self*/, PyObject* args)
{
  PyObject *bitmap_object=NULL;
  if (!PyArg_ParseTuple(args, "O",&bitmap_object))
    return NULL;  
  if (!PyCObject_Check(bitmap_object)) {
    PyErr_SetString(PyExc_TypeError, 
		    "Expected CObject containing a pointer to CFbsBitmap");
    return NULL;
  }
  CFbsBitmap *bitmap=(CFbsBitmap *)PyCObject_AsVoidPtr(bitmap_object);
  Image_object *obj;
  if (!(obj = PyObject_New(Image_object, Image_type))) 
    return PyErr_NoMemory();    
  TRAPD(error, {
      obj->data = new (ELeave) ImageObject();
      CleanupStack::PushL(obj->data);
      obj->data->ConstructL(bitmap);
      CleanupStack::Pop();
    });
  if(error != KErrNone){
    PyObject_Del(obj);   
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return (PyObject*)obj;
}


#ifdef EKA2
/* Create an ImageObject from an Icon file. */
extern "C" PyObject *
graphics_ImageFromIcon(PyObject* /*self*/, PyObject* args)
{
  TInt width = 0;
  TInt height = 0;
  TInt image_id = 0;
  PyObject* filename;
  CFbsBitmap* bitmap = NULL;
  
  
  if (!PyArg_ParseTuple(args, "Uiii", &filename, &image_id, &width, &height))
    return NULL;  

  TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), 
                    PyUnicode_GetSize(filename));
             
                    
  Image_object *obj;
  if (!(obj = PyObject_New(Image_object, Image_type))){
    return PyErr_NoMemory();    
  }
  
  TRAPD(error, {
      bitmap = AknIconUtils::CreateIconL(filenamePtr, image_id);
      CleanupStack::PushL(bitmap);
      obj->data = new (ELeave) ImageObject();
      CleanupStack::PushL(obj->data);
      User::LeaveIfError(AknIconUtils::SetSize(bitmap, TSize(width, height)));
      obj->data->ConstructL(bitmap);
      CleanupStack::Pop();
      CleanupStack::Pop();
    });

  if(error != KErrNone){ 
    PyObject_Del(obj);   
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return (PyObject*)obj;  
}
#endif


extern "C" PyObject *
graphics_ImageNew(PyObject* /*self*/, PyObject* args)
{
  int xSize,ySize;
  enum TDisplayMode mode=EColor64K;
  if (!PyArg_ParseTuple(args, "(ii)i",&xSize,&ySize,&mode)) 
    return NULL;  
  switch (mode) {
  case EGray2:
  case EGray256:
  case EColor4K:
  case EColor64K:
  case EColor16M:
    break;
  default:
    PyErr_SetString(PyExc_ValueError, "Invalid mode specifier");
    return NULL;
  }
  Image_object *obj;
  if (!(obj = PyObject_New(Image_object, Image_type))) 
    return PyErr_NoMemory();    
  TRAPD(error, {
      obj->data = new (ELeave) ImageObject();
      CleanupStack::PushL(obj->data);
      obj->data->ConstructL(xSize,ySize,mode);
      CleanupStack::Pop();
    });
  if(error != KErrNone){
    PyObject_Del(obj);   
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return (PyObject*)obj;
}

/*
static void
pyprintf(const char *format, ...)
{
  va_list args;
  va_start(args,format);
  char buf[128];
  // Unfortunately Symbian doesn't seem to support the vsnprintf
  // function, which would allow us to specify the length of the
  // buffer we are writing to. As a result using this function is
  // unsafe and will lead to a buffer overflow if given an argument
  // that is too big. Do not use this function in production code.
  vsprintf(buf, format, args);    
  char buf2[128];
  sprintf(buf2, "print '%s'", buf);
  PyRun_SimpleString(buf2);
}
*/

extern "C" PyObject*
Image_getpixel(Image_object *self, PyObject *args)
{
  PyObject *coordseq_obj=NULL;
  PyObject *retval=NULL;
  int n_coords=0,i;
  
  if (!PyArg_ParseTuple(args, "O", &coordseq_obj) ||
      !PyCoordSeq_Length(coordseq_obj, &n_coords))
    return 0;
  
  if (!(retval=PyList_New(n_coords)))
    goto error;
  
  for (i=0; i<n_coords; i++) {
    TPoint point;
    if (!PyCoordSeq_GetItem(coordseq_obj, i, &point.iX, &point.iY)) 
      goto error;
    TRgb color;
    self->data->GetPixel(color,point);
    
    PyObject *coordpair=Py_BuildValue("(iii)",
				     color.Red(), 
				     color.Green(), 
				     color.Blue());
    if (!coordpair)
      goto error;
    if (-1==PyList_SetItem(retval, i, coordpair))
      goto error;
  }
  return retval;
 error:
  Py_XDECREF(retval);
  return 0;
}


#ifdef ICL_SUPPORT
extern "C" PyObject *
graphics_ImageOpen(PyObject* /*self*/, PyObject* args)
{
  Image_object *obj;
  PyObject *filename=NULL;
  PyObject *callback=NULL;
  
  if (!PyArg_ParseTuple(args, "U|O", &filename, &callback)) 
    return NULL;  
  TPtrC filenamePtr((TUint16*)PyUnicode_AsUnicode(filename), 
		    PyUnicode_GetSize(filename));
  if (!(obj = PyObject_New(Image_object, Image_type))) 
    return PyErr_NoMemory();    
  TRAPD(error, {
      obj->data = new (ELeave) ImageObject();
      CleanupStack::PushL(obj->data);
      if (callback)
	obj->data->SetCompletionCallback(callback);
      obj->data->ConstructL(filenamePtr);
      CleanupStack::Pop();
    });
  if(error != KErrNone){
    PyObject_Del(obj);
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return (PyObject *)obj;
}

extern "C" PyObject *
graphics_ImageInspect(PyObject* /*self*/, PyObject* args)
{
  PyObject *filename=NULL;
  if (!PyArg_ParseTuple(args, "U", &filename)) 
    return NULL;  
  TPtrC filenamePtr((TUint16*)PyUnicode_AsUnicode(filename), 
		    PyUnicode_GetSize(filename));
  int width,height,mode;
  TRAPD(error, {
      ImageObject::InspectFileL(filenamePtr,width, height, mode);
    });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("(ii)i",width,height,mode);
}


extern "C" PyObject*
Image_load(Image_object *self, PyObject *args)
{
  PyObject *filename;
  ImageObject *image=self->data;
  PyObject *callback=NULL;
  int sizeMatches=1;
  if (!PyArg_ParseTuple(args, "UO", &filename, &callback)) 
    return NULL;
  
  TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), 
		    PyUnicode_GetSize(filename));  
  TRAPD(error, {
      if (callback)
	image->SetCompletionCallback(callback);      
      sizeMatches=image->StartLoadL(filenamePtr);      
    });
  if (!error && !sizeMatches) {
    PyErr_SetString(PyExc_RuntimeError, "file size doesn't match image size");
    return NULL;
  }
  RETURN_ERROR_OR_PYNONE(error);
}


#define ERROR(msg) do { PyErr_SetString(PyExc_ValueError, msg); goto error; } while(0)

extern "C" PyObject*
Image_save(Image_object *self, PyObject *args)
{
  PyObject *filename;
  ImageObject *image=self->data;
  PyObject *callback=NULL;
  char *formatstring, *compressionstring;
  int quality, bpp, color=0;;
    if (!PyArg_ParseTuple(args, "UOsisi", &filename, &callback,
			&formatstring, &quality, &compressionstring, &bpp)) 
    return NULL;  

  TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), 
		    PyUnicode_GetSize(filename));  
  ImageObject::ImageFormat format;
  TPngEncodeData::TPngCompressLevel compression=TPngEncodeData::EDefaultCompression;

  if (!strcmp(formatstring,"JPEG")) {
    if (quality < 0 || quality > 100) 
      ERROR("invalid quality");    
    format=ImageObject::EJpeg;
  } else if (!strcmp(formatstring,"PNG")) {
    format=ImageObject::EPng;
    if (!strcmp(compressionstring,"default")) {
      compression=TPngEncodeData::EDefaultCompression;
    } else if (!strcmp(compressionstring,"no")) {
      compression=TPngEncodeData::ENoCompression;
    } else if (!strcmp(compressionstring,"fast")) {
      compression=TPngEncodeData::EBestSpeed;
    } else if (!strcmp(compressionstring,"best")) {
      compression=TPngEncodeData::EBestCompression;
    } else {
      ERROR("invalid compression level");
    }
    switch (bpp) {
    case 1:
    case 8:
      color=0;
      break;
    case 24:
      color=1;
      break;
    default:
      ERROR("invalid number of bits per pixel");
    }
  } else {
    ERROR("invalid format");
  }
  
  TRAPD(error, {
      if (callback)
	image->SetCompletionCallback(callback);
      image->StartSaveL(filenamePtr,format,quality,compression,bpp,color);
    });
  RETURN_ERROR_OR_PYNONE(error);
 error:
  return NULL;
}
#undef ERROR

extern "C" PyObject*
Image_resize(Image_object *self, PyObject *args)
{
  ImageObject *image=self->data;
  PyObject *callback=NULL;
  int maintainAspect=0;
  PyObject *target_bitmap_object=NULL;
  if (!PyArg_ParseTuple(args, "OiO", &target_bitmap_object, &maintainAspect, &callback))
    return NULL;
  
  CFbsBitmap *target_bitmap=Bitmap_AsFbsBitmap(target_bitmap_object);
  if (!target_bitmap) 
    return NULL;
  TRAPD(error, {
      if (callback)
	image->SetCompletionCallback(callback);
      image->StartScaleL(target_bitmap,maintainAspect?ETrue:EFalse);
    });
  RETURN_ERROR_OR_PYNONE(error);
}

#define FLIP_LEFT_RIGHT 1
#define FLIP_TOP_BOTTOM 2
#define ROTATE_90 3
#define ROTATE_180 4
#define ROTATE_270 5

extern "C" PyObject*
Image_transpose(Image_object *self, PyObject *args)
{
  ImageObject *image=self->data;
  PyObject *callback=NULL;
  int direction;
  PyObject *target_bitmap_object=NULL;
  if (!PyArg_ParseTuple(args, "OiO", &target_bitmap_object, &direction, &callback))
    return NULL;
  
  CBitmapRotator::TRotationAngle angle;
  switch (direction) {
  case FLIP_LEFT_RIGHT: angle=CBitmapRotator::EMirrorVerticalAxis; break;
  case FLIP_TOP_BOTTOM: angle=CBitmapRotator::EMirrorHorizontalAxis; break;
  case ROTATE_90:       angle=CBitmapRotator::ERotation270DegreesClockwise; break;
  case ROTATE_180:      angle=CBitmapRotator::ERotation180DegreesClockwise; break;
  case ROTATE_270:      angle=CBitmapRotator::ERotation90DegreesClockwise; break;
  default:
    PyErr_SetString(PyExc_ValueError, "invalid transpose direction");
    return NULL;
  }

  CFbsBitmap *target_bitmap=Bitmap_AsFbsBitmap(target_bitmap_object);
  if (!target_bitmap) 
    return NULL;
  TRAPD(error, {
      if (callback)
	image->SetCompletionCallback(callback);
      image->StartRotateL(target_bitmap, angle);
    });
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Image_stop(Image_object *self, PyObject * /* args */)
{
  ImageObject *image=self->data;
  image->Stop();
  PY_RETURN_NONE;
}
#endif /* ICL_SUPPORT */

static void _Image__gc_dealloc(void *gc, void *a2)
{
  CFbsBitGc *context=STATIC_CAST(CFbsBitGc *,gc);
  CGraphicsDevice *device=context->Device();
  delete context;
  delete device;
  Py_DECREF((PyObject *)a2);
}


extern "C" PyObject*
Image__drawapi(Image_object *self, PyObject * /*args*/)
{
  PyObject *ret=NULL;
  CFbsBitmapDevice *device=NULL;
  CFbsBitGc *gc=NULL;
  TRAPD(error, {
      device=CFbsBitmapDevice::NewL(self->data->GetBitmap());
      CleanupStack::PushL(device);
      User::LeaveIfError(device->CreateContext(gc));
      CleanupStack::Pop(device);
    });
  if (error == KErrNone) {
    ret=PyCObject_FromVoidPtrAndDesc(gc, self,_Image__gc_dealloc);
    if (ret) {
      Py_INCREF(self);      
      return ret;
    } else 
      return NULL;
  } else {
    SPyErr_SetFromSymbianOSErr(error);
    return NULL;
  }
}

static void _Image__bitmapapi_dealloc(void *gc, void *a2)
{
  Py_DECREF((PyObject *)a2);
}

extern "C" PyObject*
Image__bitmapapi(Image_object *self, PyObject * /*args*/)
{
  Py_INCREF(self);
  return PyCObject_FromVoidPtrAndDesc(self->data->GetBitmap(), self,_Image__bitmapapi_dealloc);
}

/*
static void _graphics_rawscreen_dealloc(void *gc_)
{
  CBitmapContext *gc=STATIC_CAST(CBitmapContext *,gc_);
  CFbsScreenDevice *device=STATIC_CAST(CFbsScreenDevice *,gc->Device());  
  delete gc;
  delete device;
}

extern "C" PyObject*
graphics_rawscreen(PyObject *args)
{
  CFbsScreenDevice *dev;
  dev=CFbsScreenDevice::NewL(TPtr(NULL,0),EColor64K);
  //RHardwareBitmap hwbm=dev->HardwareBitmap();
  //CBitmapDevice *dev=CCoeEnv::Static()->ScreenDevice();
  CGraphicsContext *gc;
  dev->CreateContext(gc);  
  gc->CancelClippingRect();
  return PyCObject_FromVoidPtr(STATIC_CAST(CBitmapContext *,gc), _graphics_rawscreen_dealloc);
}
*/

/* LRU cache that maps Python level font specification objects to CFont objects. */
#ifdef EKA2
NONSHARABLE_CLASS(CPyFontCache) : public CBase {
#else
    class CPyFontCache : public CBase {
#endif  
    
public:
    CPyFontCache(CBitmapDevice *aDevice):iDevice(aDevice) {}
    ~CPyFontCache();
    int CFont_from_python_fontspec(PyObject *aArg, CFont *&aFont);
private:
    /* This seems like a reasonable balance. 
     * It's probably rare for applications to constantly need more than 8 fonts. */
    #define FONTCACHE_SIZE 8

    struct TPyFontCacheEntry {
        PyObject *pyfontspec;
        CFont *cfont;        
        int last_access;
    } iEntries[FONTCACHE_SIZE];
    CBitmapDevice *iDevice;
    int iTime;
};

CPyFontCache::~CPyFontCache(void) 
{
    for (int i=0; i<FONTCACHE_SIZE; i++) {
               Py_XDECREF(iEntries[i].pyfontspec);        
        if (iEntries[i].cfont) 
            iDevice->ReleaseFont(iEntries[i].cfont);            
    }
}

int 
CPyFontCache::CFont_from_python_fontspec(PyObject *aArg, CFont *&aFont)
{
    int i;
    for (i=0; i<FONTCACHE_SIZE; i++) {
        if (iEntries[i].pyfontspec == aArg) {
            iEntries[i].last_access = iTime;
            aFont=iEntries[i].cfont;
            return 0;
        }
    }
    // No match found, find the oldest entry.
    int oldestentry_time=0;
    int oldestentry_index=0;
    for (i=0; i<FONTCACHE_SIZE; i++)
        /* Not doing oldestentry_time > iEntries[i].last_access since 
         * that wouldn't handle iTime wraparound correctly. 
         * At a hypothetical rate of 1000 font lookups a second 
         * we'd run into trouble already in 22 days :) */
        if ((oldestentry_time - iEntries[i].last_access) >= 0) {
            oldestentry_time=iEntries[i].last_access;
            oldestentry_index=i;
        }
    
    // Lookup the font.
    CFont *font;
    int error = ::CFont_from_python_fontspec(aArg, font, *iDevice);
    if (error != 0)
        return error;
    
    // If lookup was successful, release the old cache entry.
    TPyFontCacheEntry &oldest=iEntries[oldestentry_index];
    Py_XDECREF(oldest.pyfontspec);
    if (oldest.cfont)
        iDevice->ReleaseFont(oldest.cfont);
    
    // Place font in cache.
    iTime++;
    oldest.pyfontspec = aArg;
    Py_INCREF(aArg);
    oldest.cfont = font;
    oldest.last_access = iTime;
    
    // All done.
    aFont = font;
    return 0;
}

// ******** Draw object

#define Draw_type ((PyTypeObject*)SPyGetGlobalString("DrawType"))
struct Draw_object {
  PyObject_VAR_HEAD  
  PyObject *drawapi_cobject;
  CBitmapContext *gc;
  PyObject *font_spec_object;
  CFont *cfont;
  CPyFontCache *fontcache;
};


extern "C" PyObject *
graphics_Draw(PyObject* /*self*/, PyObject* args)
{
  PyObject *drawable=NULL;
  if (!PyArg_ParseTuple(args, "O", &drawable))
    return NULL;
  PyObject *drawapi_cobject=PyCAPI_GetCAPI(drawable,"_drawapi");
  if (!drawapi_cobject) {
    PyErr_SetString(PyExc_TypeError, 
		    "Expected a drawable object as argument");
    return NULL;
  }
  Draw_object *obj = PyObject_New(Draw_object, Draw_type);
  if (obj == NULL)
    return PyErr_NoMemory();    
  obj->gc=STATIC_CAST(CBitmapContext *,PyCObject_AsVoidPtr(drawapi_cobject));
  obj->drawapi_cobject = drawapi_cobject;
  obj->font_spec_object=NULL; 
  obj->cfont=NULL;
  obj->fontcache=new CPyFontCache(static_cast<CBitmapDevice *>(obj->gc->Device()));
  if (!obj->fontcache) 
      return PyErr_NoMemory();
  return (PyObject *)obj;
}




extern "C" PyObject*
Draw_blit(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *image_object=NULL; 
  PyObject *source_object=NULL;
  PyObject *target_object=NULL;
  int scaling=0;
  PyObject *mask_object=NULL;
  static const char *const kwlist[] = 
    {"image", "source", "target", "scale", "mask", NULL};  
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOiO", (char**)kwlist,
                                   &image_object, 
                                   &source_object, 
                                   &target_object,
				   &scaling,
				   &mask_object)){ 
    return NULL;
  }  
  CFbsBitmap *source_bitmap=Bitmap_AsFbsBitmap(image_object);
  if (!source_bitmap) {
    PyErr_SetString(PyExc_TypeError, "Image object expected as 1st argument"); 
    return NULL;
  }
  CFbsBitmap *mask_bitmap=NULL;
  // If a mask is given, it must be a bitmap.
  if (mask_object) {
    mask_bitmap=Bitmap_AsFbsBitmap(mask_object);
    if (!mask_bitmap) {
      PyErr_SetString(PyExc_TypeError, "Mask must be an Image object");
      return NULL;
    }
#if S60_VERSION < 26
    if (mask_bitmap->DisplayMode() != EGray2) {
        PyErr_SetString(PyExc_ValueError, "Mask must be a binary (1-bit) Image. Partial transparency is not supported in S60 before 2nd edition FP2.");
        return NULL;            
    }    
#else    
    if (mask_bitmap->DisplayMode() != EGray2 && mask_bitmap->DisplayMode() != EGray256) {
        PyErr_SetString(PyExc_ValueError, "Mask must be a binary (1-bit) or grayscale (8-bit) Image");
        return NULL;
    }
#endif
  }
  
  TSize target_size=gc->Device()->SizeInPixels();
  TSize source_size=source_bitmap->SizeInPixels();
  int fx1=0,fy1=0,fx2=source_size.iWidth,fy2=source_size.iHeight;
  int tx1=0,ty1=0,tx2=target_size.iWidth,ty2=target_size.iHeight;
  int n=0;
  if (source_object) {
    if (!PyCoordSeq_Length(source_object,&n) ||
	!PyCoordSeq_GetItem(source_object,0,&fx1,&fy1) ||
	((n==2) && !PyCoordSeq_GetItem(source_object,1,&fx2,&fy2))) {
      PyErr_SetString(PyExc_TypeError, "invalid 'source' specification");
      return NULL;
    }
  }
  if (target_object) {
    if (!PyCoordSeq_Length(target_object,&n) ||
	!PyCoordSeq_GetItem(target_object,0,&tx1,&ty1) ||
	((n==2) && !PyCoordSeq_GetItem(target_object,1,&tx2,&ty2))) {
      PyErr_SetString(PyExc_TypeError, "invalid 'target' specification");
      return NULL;
    }
  }
  TRect fromRect(fx1,fy1,fx2,fy2);
  TRect toRect(tx1,ty1,tx2,ty2);
 
  if (scaling) {
    if (mask_bitmap) {
      PyErr_SetString(PyExc_ValueError, 
		      "sorry, scaling and masking is not supported at the same time.");
      return NULL;
    }
    gc->DrawBitmap(toRect,source_bitmap,fromRect);            
  } else {
    if (mask_bitmap) 
      gc->BitBltMasked(TPoint(tx1,ty1),source_bitmap,fromRect,mask_bitmap,EFalse);
    else
      gc->BitBlt(TPoint(tx1,ty1),source_bitmap,fromRect);        
  }    
  PY_RETURN_NONE;
}


/* Note: All drawing functions are allowed to change pen and brush
   color and pen size without restoring them. Other settings must be
   restored to defaults on exit. */

static int
Graphics_ParseAndSetParams(Draw_object *self, PyObject *args, PyObject *keywds, 
			   PyObject **coordseq_obj, int *n_coords, 
			   float *start=NULL, float *end=NULL)
{
  CBitmapContext *gc=self->gc;
  PyObject 
    *outline_obj=NULL, 
    *fill_obj=NULL,
    *pattern_obj=NULL;
  int pen_width=1;
  TRgb outline_color(0,0,0);
  TRgb fill_color(0,0,0);
  
  if (start && end) {
    static const char *const kwlist[] = {"coords", "start", "end", "outline", "fill", 
					 "width", "pattern", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Off|OOiO", (char**)kwlist,
				     coordseq_obj,
				     start,
				     end,
				     &outline_obj,
				     &fill_obj,
				     &pen_width,
				     &pattern_obj) ||
	!PyCoordSeq_Length(*coordseq_obj, n_coords) ||
	(outline_obj && outline_obj != Py_None && !ColorSpec_AsRgb(outline_obj,&outline_color)) ||
	(fill_obj && fill_obj != Py_None && !ColorSpec_AsRgb(fill_obj,&fill_color)) ||
	(pattern_obj && !Bitmap_Check(pattern_obj)))
      return 0;    
  } else {
    static const char *const kwlist[] = {"coords", "outline", "fill", "width", "pattern",
					 NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOiO", (char**)kwlist,
				     coordseq_obj,
				     &outline_obj,
				     &fill_obj,
				     &pen_width,
				     &pattern_obj) ||
	!PyCoordSeq_Length(*coordseq_obj, n_coords) ||
	(outline_obj && outline_obj != Py_None && !ColorSpec_AsRgb(outline_obj,&outline_color)) ||
	(fill_obj && fill_obj != Py_None && !ColorSpec_AsRgb(fill_obj,&fill_color)) ||
	(pattern_obj && !Bitmap_Check(pattern_obj)))
      return 0;    
  }

  if (outline_obj == Py_None)
    outline_obj=NULL;
  if (fill_obj == Py_None)
    fill_obj=NULL;

  gc->SetPenColor(outline_color);
  gc->SetPenSize(TSize(pen_width,pen_width));  
  gc->SetPenStyle(outline_obj ? CGraphicsContext::ESolidPen   : CGraphicsContext::ENullPen);
  gc->SetBrushColor(fill_color);
  if (pattern_obj) {
    gc->UseBrushPattern(Bitmap_AsFbsBitmap(pattern_obj));
    gc->SetBrushStyle(CGraphicsContext::EPatternedBrush);
  } else {
    gc->SetBrushStyle(fill_obj ? CGraphicsContext::ESolidBrush : CGraphicsContext::ENullBrush);  
  }
  return 1;
}

static void
Graphics_ResetParams(Draw_object *self)
{
  CBitmapContext *gc=self->gc;
  gc->SetPenStyle(CGraphicsContext::ESolidPen);
  gc->SetBrushStyle(CGraphicsContext::ENullBrush);
  // Should we gc->DiscardBrushPattern() here?
}

extern "C" PyObject*
Draw_rectangle(Draw_object *self, PyObject *args, PyObject *keywds)
{
  /*
  {
    CBitmapContext *gc=self->gc;
    PyObject 
      *outline_obj=NULL, 
      *fill_obj=NULL,
      *pattern_obj=NULL;
    int i;
    int pen_width=1;
    TRgb outline_color(0,0,0);
    TRgb fill_color(0,0,0);
    
    static const char *const kwlist[] = {"coords", "outline", "fill", "width", "pattern",
					 NULL};
    PyObject *coordseq_obj=NULL;
    int n_coords=0;
    PyArg_ParseTupleAndKeywords(args, keywds, "O|OOiO", (char**)kwlist,
				&coordseq_obj,
				&outline_obj,
				&fill_obj,
				&pen_width,
				&pattern_obj);
  }
  */
  /*
  CBitmapContext *gc=self->gc;
  PyObject 
    *outline_obj=NULL, 
    *fill_obj=NULL,
    *pattern_obj=NULL;
  int i;
  int pen_width=1;
  TRgb outline_color(0,0,0);
  TRgb fill_color(0,0,0);
  
  static const char *const kwlist[] = {"coords", "outline", "fill", "width", "pattern",
				       NULL};
  PyObject *coordseq_obj=NULL;
  int n_coords=0;
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOiO", (char**)kwlist,
				   &coordseq_obj,
				   &outline_obj,
				   &fill_obj,
				   &pen_width,
				   &pattern_obj) ||
      !PyCoordSeq_Length(coordseq_obj, &n_coords) ||
      (outline_obj && !ColorSpec_AsRgb(outline_obj,&outline_color)) ||
      (fill_obj && !ColorSpec_AsRgb(fill_obj,&fill_color)) ||
      (pattern_obj && !Bitmap_Check(pattern_obj))) {
    return NULL;
  }

  gc->SetPenColor(outline_color);
  gc->SetPenSize(TSize(pen_width,pen_width));  
  gc->SetPenStyle(outline_obj ? CGraphicsContext::ESolidPen   : CGraphicsContext::ENullPen);
  gc->SetBrushColor(fill_color);
  gc->SetBrushStyle(fill_obj ? CGraphicsContext::ESolidBrush : CGraphicsContext::ENullBrush);  
*/  
  //PY_RETURN_NONE;

  //PY_RETURN_NONE;
  
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords,i;
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords))
    return NULL;  
  
  /*PyObject *coordseq_obj=NULL;
  int n_coords=2,i;
  if (!PyArg_ParseTuple(args,"O", &coordseq_obj)) {
    return NULL;
  }
  CBitmapContext *gc=self->gc;
  gc->SetPenColor(TRgb(0,0,0));
  gc->SetPenSize(TSize(1,1));
  gc->SetPenStyle(CGraphicsContext::ENullPen);
  gc->SetBrushColor(TRgb(255,0,0));
  gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
  */
  if (n_coords % 2 != 0) {
    PyErr_SetString(PyExc_ValueError, "even number of coordinates expected");
    goto error;
  }
  for (i=0; i<n_coords/2; i++) {
    int x1,y1,x2,y2;
    if (!PyCoordSeq_GetItem(coordseq_obj,i*2,&x1,&y1)||
	!PyCoordSeq_GetItem(coordseq_obj,i*2+1,&x2,&y2))
      goto error;
    //sum += x1+y1+x2+y2;
    gc->DrawRect(TRect(x1,y1,x2,y2));
  }    
  //  if (sum == 12332)  // to defeat possible optimizer
  //  Py_INCREF(Py_None);
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  Graphics_ResetParams(self);
  return NULL;
}

extern "C" PyObject*
Draw_ellipse(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords,i;
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords))
    return NULL;
  
  if (n_coords % 2 != 0) {
    PyErr_SetString(PyExc_ValueError, "even number of coordinates expected");
    goto error;
  }
  for (i=0; i<n_coords/2; i++) {
    int x1,y1,x2,y2;
    if (!PyCoordSeq_GetItem(coordseq_obj,i*2,&x1,&y1)||
	!PyCoordSeq_GetItem(coordseq_obj,i*2+1,&x2,&y2))
      goto error;
    gc->DrawEllipse(TRect(x1,y1,x2,y2));
  }    
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  Graphics_ResetParams(self);
  return NULL;
}

static TPoint
TPoint_FromAngleAndBoundingBox(TRect bbox, float angle)
{
  TInt cx=bbox.iTl.iX+((bbox.iBr.iX-bbox.iTl.iX)/2);
  TInt cy=bbox.iTl.iY+((bbox.iBr.iY-bbox.iTl.iY)/2);
  return TPoint((TInt)(cx+cos(angle)*1024), (TInt)(cy-sin(angle)*1024));
}

extern "C" PyObject*
Draw_arc(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords,i;
  float start_angle,end_angle;
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords,&start_angle,&end_angle))
    return NULL;
  
  if (n_coords % 2 != 0) {
    PyErr_SetString(PyExc_ValueError, "even number of coordinates expected");
    goto error;
  }
  for (i=0; i<n_coords/2; i++) {
    int x1,y1,x2,y2;
    if (!PyCoordSeq_GetItem(coordseq_obj,i*2,&x1,&y1)||
	!PyCoordSeq_GetItem(coordseq_obj,i*2+1,&x2,&y2))
      goto error;
    TRect bbox(x1,y1,x2,y2);    
    TPoint startPoint=TPoint_FromAngleAndBoundingBox(bbox,start_angle); 
    TPoint endPoint=TPoint_FromAngleAndBoundingBox(bbox,end_angle); 
    gc->DrawArc(bbox,startPoint,endPoint);
  }    
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  Graphics_ResetParams(self);
  return NULL;
}


extern "C" PyObject*
Draw_pieslice(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords,i;
  float start_angle,end_angle;
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords,&start_angle,&end_angle))
    return NULL;
  
  if (n_coords % 2 != 0) {
    PyErr_SetString(PyExc_ValueError, "even number of coordinates expected");
    goto error;
  }
  for (i=0; i<n_coords/2; i++) {
    int x1,y1,x2,y2;
    if (!PyCoordSeq_GetItem(coordseq_obj,i*2,&x1,&y1)||
	!PyCoordSeq_GetItem(coordseq_obj,i*2+1,&x2,&y2))
      goto error;
    TRect bbox(x1,y1,x2,y2);    
    TPoint startPoint=TPoint_FromAngleAndBoundingBox(bbox,start_angle); 
    TPoint endPoint=TPoint_FromAngleAndBoundingBox(bbox,end_angle); 
    gc->DrawPie(bbox,startPoint,endPoint);
  }    
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  Graphics_ResetParams(self);
  return NULL;
}


extern "C" PyObject*
Draw_point(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords;    
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords))
    return NULL;

  TPoint point;
  if (!PyCoordSeq_GetItem(coordseq_obj, 0, &point.iX, &point.iY)) 
    goto error;
  gc->Plot(point);
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  Graphics_ResetParams(self);
  return NULL;
}

extern "C" PyObject*
Draw_line(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords,i;    
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords))
    return NULL;

  TPoint point;
  if (!PyCoordSeq_GetItem(coordseq_obj, 0, &point.iX, &point.iY)) 
    goto error;
  gc->MoveTo(point);
  for (i=0; i<n_coords; i++) {
    if (!PyCoordSeq_GetItem(coordseq_obj, i, &point.iX, &point.iY))
      goto error;
    gc->DrawLineTo(point);
  }
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  Graphics_ResetParams(self);
  return NULL;
}

extern "C" PyObject*
Draw_polygon(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *coordseq_obj=NULL;
  int n_coords,i;    
  if (!Graphics_ParseAndSetParams(self,args,keywds,&coordseq_obj,&n_coords))
    return NULL;

  TPoint *points=new TPoint[n_coords];
  if (!points)
    goto error;
  for (i=0; i<n_coords; i++) 
    if (!PyCoordSeq_GetItem(coordseq_obj,i, &points[i].iX, &points[i].iY))
      goto error;  
  gc->DrawPolygon(points,n_coords);

  delete[] points;
  Graphics_ResetParams(self);
  PY_RETURN_NONE;
 error:
  delete[] points;
  Graphics_ResetParams(self);
  return NULL;
}


extern "C" PyObject*
Draw_clear(Draw_object *self, PyObject *args, PyObject *keywds)
{
  CBitmapContext *gc=self->gc;
  PyObject *fill_obj=NULL;
  TRgb fill_color(255,255,255);  

  static const char *const kwlist[] = {"fill", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|O", (char**)kwlist,
				   &fill_obj) ||
      (fill_obj && !ColorSpec_AsRgb(fill_obj,&fill_color))) {
    return NULL;
  }
  gc->SetBrushColor(fill_color);
  gc->Clear();
  PY_RETURN_NONE;
}

#define FAILSYMBIAN(phase) do { ret=SPyErr_SetFromSymbianOSErr(error); goto fail##phase; } while(0)
#define FAILSYMBIAN_IFERROR(phase) do { if (error != KErrNone) FAILSYMBIAN(phase); } while (0)

extern "C" PyObject*
Draw_text(Draw_object *self, PyObject *args, PyObject *keywds)
{
  int length,i;
  char* buf;
  CBitmapContext *gc=self->gc;

  PyObject *fill_obj=NULL;
  TRgb fill_color(0,0,0);
  PyObject *coordseq_object=NULL;
  PyObject *new_font_spec_object=Py_None;
  int n_coords;
  
  static const char *const kwlist[] = {"coords", "text", "fill", "font", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "Ou#|OO", (char**)kwlist,
				   &coordseq_object,
				   &buf,&length,
				   &fill_obj,
				   &new_font_spec_object) ||
      !PyCoordSeq_Length(coordseq_object, &n_coords) ||
      (fill_obj && !ColorSpec_AsRgb(fill_obj,&fill_color)))
    return NULL;
  
  if (self->font_spec_object != new_font_spec_object) {
      /* Using a different font than before. 
       * For efficiency we care only about object identity, not value. */
      CFont *font=NULL;
      //    if (-1 == CFont_from_python_fontspec(new_font_spec_object, font, *device))
      if (-1 == self->fontcache->CFont_from_python_fontspec(new_font_spec_object, font))
          return NULL;          
      gc->DiscardFont(); /* This is safe even if font is not set. */
      /*if (self->cfont)
          device->ReleaseFont(self->cfont);*/
      self->cfont=font;
      Py_XDECREF(self->font_spec_object);
      self->font_spec_object=new_font_spec_object;
      Py_INCREF(self->font_spec_object);
      gc->UseFont(font);               
  }
  gc->SetPenColor(fill_color);
  for (i=0; i<n_coords; i++) {
    TPoint loc;
    if (!PyCoordSeq_GetItem(coordseq_object,i,&loc.iX,&loc.iY)) {
      return NULL;
    }
    gc->DrawText(TPtrC((TUint16 *)buf,length), loc);
  }
  PY_RETURN_NONE;
}

extern "C" PyObject*
Draw_measure_text(Draw_object *self, PyObject *args, PyObject *keywds)
{
  char* aText_buf;
  int aText_length;
  int aMaxWidth=-1;
  int aMaxAdvance=-1;
  PyObject *aPyFontSpec_object=Py_None;
  

  static const char *const kwlist[] = {"text", "font", "maxwidth", "maxadvance", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "u#|Oii", (char**)kwlist,
                   &aText_buf,&aText_length,
                   &aPyFontSpec_object,
                   &aMaxWidth,
                   &aMaxAdvance))
    return NULL;
  
  CFont *font=NULL;
//  if (-1 == CFont_from_python_fontspec(aPyFontSpec_object, font, *device))
  if (-1 == self->fontcache->CFont_from_python_fontspec(aPyFontSpec_object, font))
      return NULL;                  

  TPtrC textDes((const unsigned short *)aText_buf, aText_length);
  CFont::TMeasureTextInput mti;
  CFont::TMeasureTextOutput mto;
  if (aMaxWidth != -1) 
    mti.iMaxBounds = aMaxWidth;
  if (aMaxAdvance != -1)
    mti.iMaxAdvance = aMaxAdvance;
  int advance=font->MeasureText(textDes, &mti, &mto);
  
  return Py_BuildValue("((iiii)ii)", 
               mto.iBounds.iTl.iX, mto.iBounds.iTl.iY,
               mto.iBounds.iBr.iX, mto.iBounds.iBr.iY, 
               advance,
               mto.iChars);
}


extern "C" PyObject *
graphics_screenshot(PyObject* /*self*/)
{
  TInt error;

  CCoeEnv *env = CCoeEnv::Static();
  if (!env) {
    PyErr_SetString(PyExc_RuntimeError,
                    "Screenshot unavailable - no CCoeEnv");
    return NULL;
  }

  Image_object *obj;
  if (!(obj = PyObject_New(Image_object, Image_type))) 
    return PyErr_NoMemory();    
  TRAP(error, {
      obj->data = new (ELeave) ImageObject();
      CleanupStack::PushL(obj->data);
      obj->data->ConstructL(new (ELeave) CFbsBitmap());
      CleanupStack::Pop();
    });
  if(error != KErrNone){
    PyObject_Del(obj);   
    return SPyErr_SetFromSymbianOSErr(error);
  }


  
  error = (obj->data->GetBitmap())->Create(CCoeEnv::Static()->ScreenDevice()->SizeInPixels(), 
    CCoeEnv::Static()->ScreenDevice()->DisplayMode());
  if (error != KErrNone) {
    PyObject_Del(obj);   
    return SPyErr_SetFromSymbianOSErr(error);
  }

  error = CCoeEnv::Static()->ScreenDevice()->CopyScreenToBitmap(obj->data->GetBitmap());
  if (error != KErrNone) {
    PyObject_Del(obj);   
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  return (PyObject*)obj;
}

  static void Draw_dealloc(Draw_object *obj)
  {
    obj->gc->DiscardFont();
    /*if (obj->cfont) {
      static_cast<CBitmapDevice *>(obj->gc->Device())->ReleaseFont(obj->cfont);  
      obj->cfont=NULL;
    }*/
    delete obj->fontcache;
    Py_DECREF(obj->drawapi_cobject);
    Py_XDECREF(obj->font_spec_object);
    
    PyObject_Del(obj);
  }

  const static PyMethodDef Draw_methods[] = {
    {"line", (PyCFunction)Draw_line, METH_VARARGS|METH_KEYWORDS},
    {"blit", (PyCFunction)Draw_blit, METH_VARARGS|METH_KEYWORDS},
    {"rectangle", (PyCFunction)Draw_rectangle, METH_VARARGS|METH_KEYWORDS},
    {"ellipse", (PyCFunction)Draw_ellipse, METH_VARARGS|METH_KEYWORDS},
    {"arc", (PyCFunction)Draw_arc, METH_VARARGS|METH_KEYWORDS},
    {"pieslice", (PyCFunction)Draw_pieslice, METH_VARARGS|METH_KEYWORDS},
    {"point", (PyCFunction)Draw_point, METH_VARARGS|METH_KEYWORDS},
    {"polygon", (PyCFunction)Draw_polygon, METH_VARARGS|METH_KEYWORDS},
    {"clear", (PyCFunction)Draw_clear, METH_VARARGS|METH_KEYWORDS},
    {"text", (PyCFunction)Draw_text, METH_VARARGS|METH_KEYWORDS},
    {"measure_text", (PyCFunction)Draw_measure_text, METH_VARARGS|METH_KEYWORDS},
    {NULL, NULL}           // sentinel
  };

  static PyObject *
  Draw_getattr(Draw_object *op, char *name)
  {
    
    
    return Py_FindMethod((PyMethodDef*)Draw_methods,
                         (PyObject *)op, name);
  }

  static const PyTypeObject c_Draw_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "graphics.Draw",                             /*tp_name*/
    sizeof(Draw_object),                     /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Draw_dealloc,                /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Draw_getattr,               /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };



  static void Image_dealloc(Image_object *obj)
  {
    delete obj->data;
    PyObject_Del(obj);
  }

  const static PyMethodDef Image_methods[] = {
    {"_drawapi", (PyCFunction)Image__drawapi, METH_VARARGS},
    {"_bitmapapi", (PyCFunction)Image__bitmapapi, METH_VARARGS},
    {"getpixel", (PyCFunction)Image_getpixel, METH_VARARGS},
#ifdef ICL_SUPPORT
    {"load", (PyCFunction)Image_load, METH_VARARGS},
    {"save", (PyCFunction)Image_save, METH_VARARGS},
    {"resize", (PyCFunction)Image_resize, METH_VARARGS},
    {"transpose", (PyCFunction)Image_transpose, METH_VARARGS},
    {"stop", (PyCFunction)Image_stop, METH_NOARGS},
#endif /* ICL_SUPPORT */
    {NULL, NULL}           // sentinel
  };

  static PyObject *
  Image_getattr(Image_object *op, char *name)
  {
    if (!strcmp(name,"size")) {
      TSize size=op->data->Size();      
      return Py_BuildValue("(ii)",size.iWidth,size.iHeight);
    }    
    if (!strcmp(name,"twipsize")) {
      TSize twipSize=op->data->TwipSize();
      return Py_BuildValue("(ii)",twipSize.iWidth,twipSize.iHeight);
    }
    if (!strcmp(name,"mode")) {
      return Py_BuildValue("i",op->data->DisplayMode());
    }    
    return Py_FindMethod((PyMethodDef*)Image_methods,
                         (PyObject *)op, name);
  }
  static int Image_setattr(Image_object *op, char *name, PyObject *v)
  {
    float widthInTwips, heightInTwips;
    if (!strcmp(name,"twipsize")) {
      if (!PyArg_ParseTuple(v, "ff", &widthInTwips, &heightInTwips))
        return -1;
      else
        op->data->SetTwipSize((TInt)widthInTwips, (TInt)heightInTwips);
    }
    else {
      PyErr_SetString(PyExc_AttributeError, "no such attribute");
      return -1;
    }
    return 0;
  }

  static const PyTypeObject c_Image_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "graphics.Image",                             /*tp_name*/
    sizeof(Image_object),                     /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Image_dealloc,                /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Image_getattr,               /*tp_getattr*/
    (setattrfunc)Image_setattr,               /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };



  static const PyMethodDef graphics_methods[] = {
    {"ImageNew", (PyCFunction)graphics_ImageNew, METH_VARARGS, NULL},
    {"ImageFromCFbsBitmap", (PyCFunction)graphics_ImageFromCFbsBitmap, METH_VARARGS, NULL},
#ifdef EKA2
    {"ImageFromIcon", (PyCFunction)graphics_ImageFromIcon, METH_VARARGS, NULL},
#endif
#ifdef ICL_SUPPORT
    {"ImageOpen", (PyCFunction)graphics_ImageOpen, METH_VARARGS, NULL},
    {"ImageInspect", (PyCFunction)graphics_ImageInspect, METH_VARARGS, NULL},
#endif // ICL_SUPPORT
    {"Draw", (PyCFunction)graphics_Draw, METH_VARARGS, NULL},
    //    {"rawscreen", (PyCFunction)graphics_rawscreen, METH_VARARGS, NULL},
    {"screenshot", (PyCFunction)graphics_screenshot, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

#define DEFTYPE(name,type_template)  do {				\
    PyTypeObject* tmp = PyObject_New(PyTypeObject, &PyType_Type);	\
    *tmp = (type_template);						\
    tmp->ob_type = &PyType_Type;					\
    SPyAddGlobalString((name), (PyObject*)tmp);				\
  } while (0)
  

  extern "C" {
  DL_EXPORT(void) initgraphics(void)
  {
    PyObject *m;
 
    DEFTYPE("DrawType",c_Draw_type);
    DEFTYPE("ImageType",c_Image_type);
    //RFbsSession::Connect();
    m = Py_InitModule("_graphics", (PyMethodDef*)graphics_methods);
    PyObject *d = PyModule_GetDict(m);    
#define DEFLONG(pyname,value) PyDict_SetItemString(d,pyname, PyInt_FromLong(value))
#define DEFLONGX(name) DEFLONG(#name,name)  
    DEFLONGX(EGray2);
    DEFLONGX(EGray256);
    DEFLONGX(EColor4K);
    DEFLONGX(EColor64K);
    DEFLONGX(EColor16M);
    PyDict_SetItemString(d, "_draw_methods", 
			 Py_BuildValue("[sssssssssss]",
				       "line",
				       "blit",
				       "rectangle",
				       "ellipse",
				       "arc",
				       "pieslice",
				       "point",
				       "polygon",
				       "clear",
				       "text",
                       "measure_text"));
#ifdef ICL_SUPPORT
    DEFLONGX(FLIP_LEFT_RIGHT);
    DEFLONGX(FLIP_TOP_BOTTOM);
    DEFLONGX(ROTATE_90);
    DEFLONGX(ROTATE_180);
    DEFLONGX(ROTATE_270);    
    DEFLONG("ICL_SUPPORT",1);
#else 
    DEFLONG("ICL_SUPPORT",0);
#endif //ICL_SUPPORT
    

  }
  DL_EXPORT(void) zfinalizegraphics(void)
  {
    //RFbsSession::Disconnect();
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif /*EKA2*/
