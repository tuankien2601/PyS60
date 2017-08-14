/** 
 * ====================================================================
 * cameramodule.cpp
 *
 * Modified from "miso" (HIIT) and video example available Forum Nokia. 
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
 *
 * ====================================================================
 */
 
#include <ecam.h>
#include <fbs.h>
#include <e32std.h>
#include <w32std.h>
#include <bitdev.h>
#include <VideoRecorder.h>

#include "Python.h"
#include "symbian_python_ext_util.h"

//////////////CLASS DEFINITIONS CAMERA

class TPyCamCallBack
  {
  public:
    TInt NewViewFinderFrameReady(CFbsBitmap* aFrame);
  public:
    PyObject* iCb; // Not owned.
  };

#ifndef EKA2
class CMisoPhotoTaker : public CBase, public MCameraObserver
#else
NONSHARABLE_CLASS (CMisoPhotoTaker) : public CBase, 
                                      public MCameraObserver
#endif
{
public:
  static CMisoPhotoTaker* NewL(TInt aPosition);
  void ConstructL();
  ~CMisoPhotoTaker();
private:
  CMisoPhotoTaker();
public:
  void TakePhotoL(TInt aMode, TInt aSize, 
                         TInt aZoom, TInt aFlash, 
                         TInt aExp, TInt aWhite); // synchronous
  void StartViewFinder(TInt aXSize, TInt aYSize);
  void StopViewFinder();
  void SetCallBack(TPyCamCallBack &aCb);
  void UnSetCallBack();  
  
  void Release() const;
  
  // accessors:
  TBool TakingPhoto() const;
  TBool ViewFinderOn() const;
  TInt GetImageModes() const;
  TInt GetImageSizeMax() const;
  void GetImageSize(TSize& aSize, 
                    TInt aSizeIndex, 
                    CCamera::TFormat aFormat) const;
  TInt GetMaxZoom() const;
  TInt GetFlashModes() const;
  TInt GetExpModes() const;
  TInt GetWhiteModes() const;
  TInt GetHandle() const;
  
  CFbsBitmap* GetBitmap() const;
  HBufC8* GetJpg() const;
  
  enum TFinderState
    {
    EFinderInactive = 0,
    EFinderInitializing,
    EFinderFailed,
    EFinderActive
    };
private:
  CActiveSchedulerWait* iLoop;
private:
  void MakeTakePhotoRequest();
  void CapturePhoto();
  //CCamera::TFormat ImageFormatMax() const;
  void SetSettingsL();
private:
  CCamera* iCamera;
  
  // The first one is for the bitmap pictures and the second
  // one for the JPEG and other formats
  CFbsBitmap* iBitMap;
  HBufC8* iData;
  
  TInt iStatus;
  TBool iTakingPhoto;
  TFinderState iViewFinderStatus;
  TCameraInfo iInfo;
  CCamera::TFormat iMode;
  TInt iSize;
  TInt iZoom;
  CCamera::TFlash iFlash;
  CCamera::TExposure iExp;
  CCamera::TWhiteBalance iWhite;
  TInt iPosition;
  TSize iViewFinderSize;
     
private:
  TPyCamCallBack iCallMe;
  TInt iErrorState;
  TBool iCallBackSet;
private: // MCameraObserver
  void ReserveComplete(TInt aError);
  void PowerOnComplete(TInt aError);
  void ViewFinderFrameReady(CFbsBitmap& aFrame);
  void ImageReady(CFbsBitmap* aBitmap, HBufC8* aData, TInt aError);
  void FrameBufferReady(MFrameBuffer* aFrameBuffer, TInt aError);
};

//////////////CLASS DEFINITIONS VIDEO CAMERA

class TPyVidCallBack
  {
  public:
    TInt VideoCameraEvent(TInt aError, TInt aStatus);
  public:
    PyObject* iCb; // Not owned.
  };

#ifndef EKA2
class CVideoCamera : public CBase, public MVideoRecorderUtilityObserver
#else
NONSHARABLE_CLASS (CVideoCamera) : public CBase, 
                                   public MVideoRecorderUtilityObserver
#endif
{
public:
  static CVideoCamera* NewL();
  void ConstructL();
  ~CVideoCamera();
private:
  CVideoCamera();
public:
  void StartRecordL(TInt aHandle, const TDesC& aFileName);
  void StopRecord();
  void SetCallBack(TPyVidCallBack &aCb);
  void UnSetCallBack();
  enum TObserverEvents
    {
    EOpenComplete = 0xFA0,
    EPrepareComplete,
    ERecordComplete
    };
private:
  CVideoRecorderUtility* iVideo;
  TUid iControllerUid;
  TUid iFormatUid;
  TPyVidCallBack iCallMe;
  TInt iErrorState;
  TBool iCallBackSet;
private: // from MVideoRecorderUtilityObserver
  void MvruoEvent(const TMMFEvent &aEvent);
  void MvruoOpenComplete(TInt aError);
  void MvruoPrepareComplete(TInt aError);
  void MvruoRecordComplete(TInt aError);
};

//////////////TYPE DEFINITIONS

#define CAM_type ((PyTypeObject*)SPyGetGlobalString("CAMType"))

struct CAM_object {
  PyObject_VAR_HEAD
  CMisoPhotoTaker* camera;
  TBool cameraUsed;
  TPyCamCallBack myCallBack;
  TBool callBackSet;
};

#define VID_type ((PyTypeObject*)SPyGetGlobalString("VIDType"))

struct VID_object {
  PyObject_VAR_HEAD
  CVideoCamera* video;
  TPyVidCallBack myCallBack;
  TBool callBackSet;
};
