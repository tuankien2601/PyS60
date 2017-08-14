/**
 * ====================================================================
 * topwindow.cpp
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
 * ====================================================================
 */


#include "topwindow.h"


CImageData* CImageData::NewL(PyObject* aImageObj, TRect& aRect){
  CImageData* self = new (ELeave) CImageData(aImageObj, aRect);
  return self;
};


CImageData::CImageData(PyObject* aImageObj, TRect& aRect){
  Py_INCREF(aImageObj);
  iImageObj = aImageObj;
  iBitmap = (CFbsBitmap*) PyCObject_AsVoidPtr(aImageObj);
  iRect = aRect;
};


CImageData::~CImageData(){
  Py_DECREF(iImageObj);
};


void CImageData::Draw(CWindowGc* aGc, TRect& aDrawArea){
  if(iRect.Intersects(aDrawArea)){
    aGc->DrawBitmap(iRect, iBitmap);
  }
};


TRect CImageData::Rect(){
  return iRect;
};



CTopWindow::~CTopWindow()
{
  TInt count = 0;
  TInt i = 0;
  Cancel();
  delete iScrDevice;
  delete iGc;
  if(iWindow){
    iWindow->Close();
    delete iWindow;
  }
  if(iWinGroup){
    iWinGroup->Close();
    delete iWinGroup;
  }
  if(iSession){
    iSession->Close();
    delete iSession;
  }
  
  // Delete the CImageData objects.
  count = iImages.Count();

  for(i=0; i < count; i++){
    delete (iImages[0]);
    iImages.Remove(0);
  }
  iImages.Reset();  
};


CTopWindow::CTopWindow():CActive(CActive::EPriorityStandard),iSession(NULL),
iWinGroup(NULL),iWindow(NULL),iScrDevice(NULL),iGc(NULL)
{
};


void CTopWindow::ConstructL()
{
  TRect rect;
  
  CActiveScheduler::Add(this);
  
  iSession = new (ELeave) RWsSession();
  User::LeaveIfError(iSession->Connect());
  
  iWinGroup = new (ELeave) RWindowGroup(*iSession);
  iWinGroup->Construct((TUint32)iWinGroup);
  
  iWinGroup->EnableReceiptOfFocus(EFalse); // Don't capture any key events.
  iWinGroup->SetOrdinalPosition(0,ECoeWinPriorityAlwaysAtFront);
  
  iScrDevice = new (ELeave) CWsScreenDevice(*iSession);
  iScrDevice->Construct();
  User::LeaveIfError(iScrDevice->CreateContext(iGc));

  iWindow = new (ELeave) RWindow(*iSession);
  iWindow->Construct(*iWinGroup,(TUint32)this);
  iWindow->SetBackgroundColor(TRgb(DEFAULT_BG_COLOR));
  iWindow->Activate();
  iWindow->SetVisible(EFalse);
  SetFading(0);
 
  Start();
};


CTopWindow* CTopWindow::NewL()
{
  CTopWindow* self = new (ELeave) CTopWindow();
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop(self);
  return self;
};


void CTopWindow::ShowL()
{
  iWindow->SetVisible(ETrue);
  iSession->Flush();
};


void CTopWindow::HideL()
{
  iWindow->SetVisible(EFalse);
  iSession->Flush();
};


void CTopWindow::SetFading(TInt aFading)
{
  if(aFading==0){
    iWinGroup->SetNonFading(ETrue);
  }else{
    iWinGroup->SetNonFading(EFalse);
  }
  iSession->Flush();
};


void CTopWindow::SetShadow(TInt aShadow)
{
  if(aShadow<=0){
    iWindow->SetShadowDisabled(ETrue);
    iWindow->SetShadowHeight(0);
  }else{
    iWindow->SetShadowDisabled(EFalse);
    iWindow->SetShadowHeight(aShadow);
  }
  iSession->Flush();
};


void CTopWindow::SetCornerType(TInt aCornerType)
{
  TCornerType corner = static_cast<TCornerType>(aCornerType);
  iWindow->SetCornerType(corner);
};


void CTopWindow::SetSize(TSize& aSize)
{
  iWindow->SetSize(aSize);
  iSession->Flush();
};


TSize CTopWindow::Size()
{
  return iWindow->Size();
};


TSize CTopWindow::MaxSize()
{
  return iGc->Device()->SizeInPixels();
};


void CTopWindow::SetPosition(TPoint& aPos)
{
  iWindow->SetPosition(aPos);
  iSession->Flush();
};


TInt CTopWindow::RemoveImage(TUint aKey)
{
  TInt index = iImages.Find((CImageData*)aKey);
  if(index != KErrNotFound){
    TRect rect = (iImages[index])->Rect();
    delete (iImages[index]);
    iImages.Remove(index);
    iWindow->Invalidate(rect);
  }
  iSession->Flush();
  return index;
};


TUint CTopWindow::PutImageL(PyObject* aBitmapObj, TRect& aRect)
{
  CImageData* imageData = CImageData::NewL(aBitmapObj, aRect);
  iImages.Append(imageData);
  iWindow->Invalidate(aRect);
  iSession->Flush();
  
  // return "key" to find the item in the iImages array.
  return (TUint)imageData;
};


void CTopWindow::SetBgColor(TRgb& aRgb)
{
  iWindow->SetBackgroundColor(aRgb);
  iWindow->Invalidate();
  iSession->Flush();
};


void CTopWindow::Redraw(TWsRedrawEvent* aRedrawEvent)
{
  TRect rect;
  TInt i = 0;
  if(aRedrawEvent){
    rect = aRedrawEvent->Rect();
  }else{
    rect = iWindow->Size();
  }
  
  iGc->Activate(*iWindow);
  iWindow->BeginRedraw();
  iGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
  iGc->Clear();
  
  for(i=0;i<iImages.Count();i++){
    CImageData* imageData = iImages[i];
    imageData->Draw(iGc, rect);
  }
  iWindow->EndRedraw();
  iGc->Deactivate();
  iSession->Flush();
};


void CTopWindow::Start()
{
  iStatus=KRequestPending;
  iSession->RedrawReady(&iStatus);
  SetActive();
};


void CTopWindow::RunL()
{
  iSession->GetRedraw(iRedrawEvent);
  this->Redraw(&iRedrawEvent);
  iStatus=KRequestPending;
  iSession->Flush();
  iSession->RedrawReady(&iStatus);
  SetActive();
};


void CTopWindow::DoCancel()
{
  iSession->RedrawReadyCancel();
};


void CTopWindow::Flush()
{
  iSession->Flush();
};


void CTopWindow::DoRedraw(TRect& aRect)
{
  iWindow->Invalidate(aRect);
};
  

void CTopWindow::DoRedraw()
{
  iWindow->Invalidate();
};


RWindow* CTopWindow::RWindowPtr()
{
  return iWindow;
};


RWsSession* CTopWindow::RWsSessionPtr()
{
  return iSession;
};


RWindowGroup* CTopWindow::RWindowGroupPtr()
{
  return iWinGroup;
};


CWsScreenDevice* CTopWindow::ScreenDevicePtr()
{
  return iScrDevice;
};

