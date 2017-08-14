/** 
 * ====================================================================
 * takephoto.cpp
 *
 * Modified from "miso" (HIIT) and video example available Forum Nokia.
 *
 * Copyright 2005 Helsinki Institute for Information Technology (HIIT)
 * and the authors.  All rights reserved.
 * Authors: Tero Hasu <tero.hasu@hut.fi>
 *
 * Portions Copyright (c) 2005-2008 Nokia Corporation 
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

#include "cameramodule.h"

// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyCamCallBack::NewViewFinderFrameReady(CFbsBitmap* aFrame)
  {
  PyGILState_STATE gstate = PyGILState_Ensure();

  TInt error = KErrNone;

  PyObject* bmp = PyCObject_FromVoidPtr(aFrame, NULL);
  PyObject* arg = Py_BuildValue("(O)", bmp);
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  Py_DECREF(bmp);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
      Py_XDECREF(type);
      Py_XDECREF(value);
      Py_XDECREF(traceback);
    } else {
      PyErr_Print();
    }
  }
  else
    Py_DECREF(rval);  

  PyGILState_Release(gstate);
  return error;
  }

//////////////////////////////////////////////

CMisoPhotoTaker* CMisoPhotoTaker::NewL(TInt aPosition)
{
  CMisoPhotoTaker* taker = new (ELeave) CMisoPhotoTaker;
  CleanupStack::PushL(taker);
  taker->iPosition = aPosition;
  taker->ConstructL();
  CleanupStack::Pop();
  return taker;
}

CMisoPhotoTaker::CMisoPhotoTaker()
{
  iStatus = KErrNone;
  iTakingPhoto = EFalse;
  iViewFinderStatus = EFinderInactive;
  iViewFinderSize = TSize();
}

void CMisoPhotoTaker::ConstructL()
{
  if (!CCamera::CamerasAvailable())
    User::Leave(KErrHardwareNotAvailable);
  
  iLoop = new (ELeave) CActiveSchedulerWait();
  iCamera = CCamera::NewL(*this, iPosition);

  iCamera->CameraInfo(iInfo);
}

CMisoPhotoTaker::~CMisoPhotoTaker()
{
  if (iCamera) {
    // assuming this is safe even if not on
    iCamera->PowerOff();
    // assuming safe even if not reserved
    iCamera->Release();
    delete iCamera;
  }
  delete iLoop;
}

void CMisoPhotoTaker::TakePhotoL(TInt aMode, TInt aSize, 
                                        TInt aZoom, TInt aFlash, 
                                        TInt aExp, TInt aWhite)
{
  iTakingPhoto = ETrue;
  iMode = static_cast<CCamera::TFormat>(aMode);
  iSize = aSize;
  iZoom = aZoom;
  iFlash = static_cast<CCamera::TFlash>(aFlash);
  iExp = static_cast<CCamera::TExposure>(aExp);
  iWhite = static_cast<CCamera::TWhiteBalance>(aWhite);

  MakeTakePhotoRequest();
  // Start the wait for reserve complete, this ends after the completion
  // of ImageReady
  iLoop->Start(); 

  iTakingPhoto = EFalse;
  User::LeaveIfError(iStatus);
}

void CMisoPhotoTaker::StartViewFinder(TInt aXSize, TInt aYSize)
{
  iViewFinderSize.SetSize(aXSize, aYSize);
  // TODO Check if we are taking photo already will be needed when
  // there is Python "Camera" type
  iViewFinderStatus = EFinderInitializing;
  iCamera->Reserve();
  iLoop->Start();
}

void CMisoPhotoTaker::StopViewFinder()
{
  if(iViewFinderStatus == EFinderActive){
    iCamera->StopViewFinder();
    iCamera->PowerOff();
    iCamera->Release();
	}
  iViewFinderStatus = EFinderInactive;
}

void CMisoPhotoTaker::SetCallBack(TPyCamCallBack &aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }

void CMisoPhotoTaker::UnSetCallBack() 
  {
  iCallBackSet = EFalse;
  }

void CMisoPhotoTaker::Release() const
  {
  iCamera->Release();
  }

TBool CMisoPhotoTaker::TakingPhoto() const
{
  return iTakingPhoto;
}

TBool CMisoPhotoTaker::ViewFinderOn() const
{
  if(iViewFinderStatus==EFinderInactive)
	  return EFalse;
	else
	  return ETrue;
}

TInt CMisoPhotoTaker::GetImageModes() const
{
  return iInfo.iImageFormatsSupported;
}

TInt CMisoPhotoTaker::GetImageSizeMax() const
{
  return iInfo.iNumImageSizesSupported;
}

void CMisoPhotoTaker::GetImageSize(TSize& aSize, TInt aSizeIndex, CCamera::TFormat aFormat) const
{
  iCamera->EnumerateCaptureSizes(aSize, aSizeIndex, aFormat);
}

TInt CMisoPhotoTaker::GetMaxZoom() const
{
  return iInfo.iMaxDigitalZoom;
}

TInt CMisoPhotoTaker::GetFlashModes() const
{
  return iInfo.iFlashModesSupported;
}

TInt CMisoPhotoTaker::GetExpModes() const
{
  return iInfo.iExposureModesSupported;
}

TInt CMisoPhotoTaker::GetWhiteModes() const
{
  return iInfo.iWhiteBalanceModesSupported;
}

TInt CMisoPhotoTaker::GetHandle() const
{
  return iCamera->Handle();
}

CFbsBitmap* CMisoPhotoTaker::GetBitmap() const
{
  CFbsBitmap* ret = iBitMap;
  return ret;
}

HBufC8* CMisoPhotoTaker::GetJpg() const
{
  HBufC8* ret = iData;
  return ret;
}

void CMisoPhotoTaker::MakeTakePhotoRequest()
{
  iCamera->Reserve();
}

void CMisoPhotoTaker::CapturePhoto()
{
  // the settings, notice that these can be only set here,
  // i.e. in camera state MCameraObserver::PowerOnComplete()
  TRAPD(aError, SetSettingsL());
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else  {
    TRAP(aError, iCamera->PrepareImageCaptureL(
      iMode, iSize));    
    if (aError) {
      iStatus = aError;
      iLoop->AsyncStop();
    } else {
      // completes with ImageReady
      iCamera->CaptureImage();
    }
  }
}

/*
// Returns highest color mode supported by HW
CCamera::TFormat CMisoPhotoTaker::ImageFormatMax() const
{
  if ( iInfo.iImageFormatsSupported & CCamera::EFormatFbsBitmapColor16M )
    {
    return CCamera::EFormatFbsBitmapColor16M;
    }
  else if ( iInfo.iImageFormatsSupported & CCamera::EFormatFbsBitmapColor64K )
    {
    return CCamera::EFormatFbsBitmapColor64K;
    }
  else 
    {
    return CCamera::EFormatFbsBitmapColor4K;
    }
}
*/

void CMisoPhotoTaker::SetSettingsL()
{
  // The parameters need to be checked prior invoking the 
  // methods since otherwise a panic may occur (and not leave)
  if (iZoom < 0 || iZoom > iInfo.iMaxDigitalZoom)
    User::Leave(KErrNotSupported);
  else
    iCamera->SetDigitalZoomFactorL(iZoom);
  
  if (iFlash & iInfo.iFlashModesSupported || iFlash == CCamera::EFlashNone)
    iCamera->SetFlashL(iFlash);
  else
    User::Leave(KErrNotSupported);
  
  if (iExp & iInfo.iExposureModesSupported || iExp == CCamera::EExposureAuto)
    iCamera->SetExposureL(iExp);
  else
    User::Leave(KErrNotSupported);
  
  if (iWhite & iInfo.iWhiteBalanceModesSupported || iWhite == CCamera::EWBAuto)
    iCamera->SetWhiteBalanceL(iWhite);
  else
    User::Leave(KErrNotSupported);
}

void CMisoPhotoTaker::ReserveComplete(TInt aError) 
{
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else {
    iCamera->PowerOn();
  }
}

void CMisoPhotoTaker::PowerOnComplete(TInt aError) 
{
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else {
    if (iTakingPhoto) {
      CapturePhoto();
    } else {
      TInt err = KErrNone;
  		TRAP(err, iCamera->StartViewFinderBitmapsL(iViewFinderSize));
  		if (err!=KErrNone) {
        iStatus = aError;
        iLoop->AsyncStop();
      }
      iViewFinderStatus = EFinderActive;
  	  iLoop->AsyncStop();
    }
  }
}

void CMisoPhotoTaker::ViewFinderFrameReady(CFbsBitmap& aFrame)
{
  // copying of bitmap is needed since we pass the ownership on:
  CFbsBitmap* frameCopy = NULL;
  TInt err = KErrNone;

  // CFbsBitmap copying from http://forum.newlc.com/index.php/topic,10093.0.html
  TRAP(err, {
    frameCopy = new (ELeave) CFbsBitmap();
    CleanupStack::PushL(frameCopy);
    frameCopy->Create(aFrame.SizeInPixels(), aFrame.DisplayMode());
    
    CFbsBitmapDevice* fbsDev = CFbsBitmapDevice::NewL(frameCopy);
    CleanupStack::PushL(fbsDev);
    CFbsBitGc* fbsGc = CFbsBitGc::NewL();
    CleanupStack::PushL(fbsGc);
    fbsGc->Activate(fbsDev);
    fbsGc->BitBlt(TPoint(0,0), &aFrame);
    
    CleanupStack::PopAndDestroy(2); // fbsDev, fbsGc
    CleanupStack::Pop(1); // frameCopy
  });
  
  if (err!=KErrNone) {
    return; // TODO what is the best way to report this?
  }
  
  iErrorState = iCallMe.NewViewFinderFrameReady(frameCopy);
}

void CMisoPhotoTaker::ImageReady(CFbsBitmap* aBitmap, HBufC8* aData, 
				 TInt aError) 
{
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else {
  
    if (iMode == CCamera::EFormatExif) {
      delete iData;
      iData = aData;
    } else {
      delete iBitMap;
      iBitMap = aBitmap;
    }
    
    iLoop->AsyncStop();
  }
}

void CMisoPhotoTaker::FrameBufferReady(MFrameBuffer* /*aFrameBuffer*/, 
          TInt /*aError*/) {}

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////

// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyVidCallBack::VideoCameraEvent(TInt aError, TInt aStatus)
  {
  PyGILState_STATE gstate = PyGILState_Ensure();

  TInt error = KErrNone;

  PyObject* arg = Py_BuildValue("(ii)", aError, aStatus);
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
      Py_XDECREF(type);
      Py_XDECREF(value);
      Py_XDECREF(traceback);
    } else {
      PyErr_Print();
    }
  }
  else
    Py_DECREF(rval);  

  PyGILState_Release(gstate);
  return error;
  }

CVideoCamera* CVideoCamera::NewL()
{
  CVideoCamera* video = new (ELeave) CVideoCamera;
  CleanupStack::PushL(video);
  video->ConstructL();
  CleanupStack::Pop();
  return video;
}

CVideoCamera::CVideoCamera(){;}

void CVideoCamera::ConstructL()
{
  iVideo = CVideoRecorderUtility::NewL(*this);

  ////////////////// Following copied from the Forum Nokia VREX example:

	CMMFControllerPluginSelectionParameters* cSelect =
		CMMFControllerPluginSelectionParameters::NewLC();
	CMMFFormatSelectionParameters* fSelect =
		CMMFFormatSelectionParameters::NewLC();

	cSelect->SetRequiredPlayFormatSupportL(*fSelect);
	cSelect->SetRequiredRecordFormatSupportL(*fSelect);

	RArray<TUid> mediaIds;
	CleanupClosePushL(mediaIds);

	User::LeaveIfError(mediaIds.Append(KUidMediaTypeVideo));

	cSelect->SetMediaIdsL(mediaIds,
		CMMFPluginSelectionParameters::EAllowOtherMediaIds);
	cSelect->SetPreferredSupplierL(KNullDesC,
		CMMFPluginSelectionParameters::EPreferredSupplierPluginsFirstInList);

	RMMFControllerImplInfoArray controllers;
	CleanupResetAndDestroyPushL(controllers);
	cSelect->ListImplementationsL(controllers);

	TBool recordingSupported = EFalse;

	// Find the first controller with at least one record format available
	for(TInt i = 0; i < controllers.Count(); ++i)
		{
		RMMFFormatImplInfoArray recordFormats = controllers[i]->RecordFormats();

		if(recordFormats.Count() > 0)
			{
			iControllerUid = controllers[i]->Uid();
			iFormatUid = recordFormats[0]->Uid();
			recordingSupported = ETrue;
			break;
			}
		}

	CleanupStack::PopAndDestroy(&controllers);
	CleanupStack::PopAndDestroy(&mediaIds);
	CleanupStack::PopAndDestroy(fSelect);
	CleanupStack::PopAndDestroy(cSelect);

	// Leave if recording is not supported
	if(!recordingSupported)
		{
		User::Leave(KErrNotSupported);
		}
  ////////////////// End copy from the Forum Nokia VREX example
}

CVideoCamera::~CVideoCamera()
{
  if (iVideo) {
    delete iVideo;
  }
}

void CVideoCamera::StartRecordL(TInt aHandle, const TDesC& aFileName)
{
	iVideo->OpenFileL(aFileName, aHandle, iControllerUid, iFormatUid);
}

void CVideoCamera::StopRecord()
{
	/* error = */ iVideo->Stop();
	iVideo->Close();
}

void CVideoCamera::SetCallBack(TPyVidCallBack &aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }

void CVideoCamera::MvruoEvent(const TMMFEvent &aEvent)
{
  // XXX modify the callback as there are not enough parameters
  //     this will not show properly to the Python layer
  if(iCallBackSet)
    iCallMe.VideoCameraEvent(aEvent.iErrorCode, aEvent.iEventType.iUid);
}

void CVideoCamera::MvruoOpenComplete(TInt aError)
{
  iCallMe.VideoCameraEvent(aError, CVideoCamera::EOpenComplete);
  if(aError==KErrNone) {
    // XXX err to Python level?
    TRAPD(err, iVideo->SetMaxClipSizeL(KMMFNoMaxClipSize));
    if(err==KErrNone) {
       iVideo->Prepare();
    }
  }
}

void CVideoCamera::MvruoPrepareComplete(TInt aError) 
{
  iCallMe.VideoCameraEvent(aError, CVideoCamera::EPrepareComplete);
	if(aError==KErrNone)
		{
    iVideo->Record();
    }
}

void CVideoCamera::MvruoRecordComplete(TInt aError)
{
  iCallMe.VideoCameraEvent(aError, CVideoCamera::ERecordComplete);
}
