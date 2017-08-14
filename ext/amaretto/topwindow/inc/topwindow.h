/**
 * ====================================================================
 * topwindow.h
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

#include "Python.h"
#include "symbian_python_ext_util.h"

#include<coecntrl.h>


// white non-transparent background.
#define DEFAULT_BG_COLOR 0xffffffff


/*
 * Helper class for storing images. 
 */
#ifndef EKA2
class CImageData : public CBase
#else
NONSHARABLE_CLASS(CImageData) : public CBase
#endif
{
  private:
    TRect iRect;
    PyObject* iImageObj;
    CFbsBitmap* iBitmap;
  public:
    static CImageData* NewL(PyObject* aImageObj, TRect& aRect);
    CImageData(PyObject* aImageObj, TRect& aRect);
    virtual ~CImageData();
    // Draw the image if the drawArea and iRect intersect.
    void Draw(CWindowGc* aGc, TRect& aDrawArea);
    TRect Rect();
};

 
#ifndef EKA2
class CTopWindow : public CActive
#else
NONSHARABLE_CLASS(CTopWindow) : public CActive
#endif
{
  public:
    virtual ~CTopWindow();
    static CTopWindow* NewL();
    void ShowL();
    void HideL();
    void SetSize(TSize& aSize);
    TSize Size();
    TSize MaxSize();
    void SetPosition(TPoint& aPos);
    TUint PutImageL(PyObject* aBitmapObj, TRect& aRect);
    TInt RemoveImage(TUint aKey);
    void SetCornerType(TInt aCornerType);
    void SetShadow(TInt aShadow);
    void SetBgColor(TRgb& aRgb);
    void SetFocus(TBool aFocus);
    void SetFading(TInt aFading);
    void RunL();
    void DoCancel();
    void Start();
    void Flush(); // Sends all pending commands in the buffer to the window server.
                  // Normally this is not needed.
    void DoRedraw();            // Causes the window to get a redraw message.
    void DoRedraw(TRect& aRect); // Causes the window to get a redraw message 
                              // concerning the specified area. 
                              
                              
    RWindow* RWindowPtr();
    RWsSession* RWsSessionPtr();
    RWindowGroup* RWindowGroupPtr();
    CWsScreenDevice* ScreenDevicePtr();
  private:
    void Redraw(TWsRedrawEvent* aRedrawEvent=NULL);
    void ConstructL();
    CTopWindow();
    RWsSession* iSession;
    RWindowGroup* iWinGroup;
    RWindow* iWindow;
    CWsScreenDevice* iScrDevice;
    CWindowGc* iGc;
    TWsRedrawEvent iRedrawEvent;
    RPointerArray<CImageData> iImages;
};

