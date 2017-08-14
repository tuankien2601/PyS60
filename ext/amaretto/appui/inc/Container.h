/*
 * Copyright (c) 2005-2009 Nokia Corporation
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

#ifndef __CONTAINER_H
#define __CONTAINER_H

#include <coecntrl.h>
#include <eikdef.h>
#include <eiklbx.h>
#include <eiklbo.h>
#include <aknnavi.h>
#include <aknnavide.h>
#include <akntabgrp.h>
#include "Python_appui.h"
#include "symbian_python_ext_util.h"

extern TBool touch_enabled_flag;
extern TBool directional_pad_flag;

#define DARKGREY TRgb(140, 140, 140)
#define LIGHTGREY TRgb(196, 196, 196)

#define TICKDELAY 500000    // 500 milliseconds
#define TICKINTERVAL 175000 // 175 milliseconds

// enum values for the UI widget controls
#define LISTBOX 0
#define TEXT 1
#define CANVAS 2
#define GLCANVAS 3

NONSHARABLE_CLASS(CRepeatEventGenerator) : public CBase
{
public:
    static CRepeatEventGenerator* NewL(const CCoeControl* aParent, TKeyEvent aKeyEvent);
    ~CRepeatEventGenerator();
protected:
    CRepeatEventGenerator(const CCoeControl* aParent, TKeyEvent aKeyEvent);
private:
    void ConstructL();
    static TInt Tick(TAny* aObject);
    void DoTick();
private:
    CPeriodic* iPeriodic;
    const CCoeControl* iParent;
    TKeyEvent iKeyEvent;
};

NONSHARABLE_CLASS(CDirectionalPad) : public CCoeControl
{
public:
  CDirectionalPad();
  void DrawKeys(CWindowGc *gc, TRect keyRect, CArrayFix<TPoint>* polygonPoints);
  virtual ~CDirectionalPad();
  virtual void ConstructL(const CCoeControl* aParent, CAmarettoAppUi* aAppui);
  void CalculateAndDrawGraphic(CWindowGc *gc);

private:
  const CCoeControl* iParent;
  CAmarettoAppUi* iAppui;
  TRect iPrevDpadRect;
  TRect iLeftButton, iRightButton, iCenterButton, iUpButton, iDownButton, iTopLeftButton, iTopRightButton;
  CArrayFix<TPoint>* iLeftTriangle, *iRightTriangle, *iUpTriangle, *iDownTriangle, *iCenterRect;
  virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);
  CRepeatEventGenerator* iRepeatEventHandle;

};

NONSHARABLE_CLASS(CAmarettoContainer) : public CCoeControl, MCoeControlObserver
{
public:
  static CAmarettoContainer *NewL(const TRect& aRect, CAmarettoAppUi* aAppui);
  virtual ~CAmarettoContainer();

  TInt SetComponentL(CCoeControl* aComponent,
             CAmarettoCallback* aEventHandler=NULL, TInt aControlName=-1);
  void Refresh() {SizeChanged(); DrawNow();}
  void EnableTabsL(const CDesCArray* aTabTexts, CAmarettoCallback* aFunc);
  void SetActiveTab(TInt aIndex);
  void HandlePointerEventL(const TPointerEvent& aPointerEvent);
  void DisablePointerForwarding(TBool aValue);
  TInt CreateDirectionalPad(CAmarettoAppUi* aAppui);
  void RefreshDirectionalPad();
  void RemoveDirectionalPad();
  CDirectionalPad *iDirectionalPad;
  CWindowGc *iDirectionalPadGc;
  CAmarettoAppUi* iAppui;
protected:
  void ConstructL(const TRect&, CAmarettoAppUi *aAppui);

private:
  void SizeChanged();
  TInt CountComponentControls() const {return (iTop ? 1 : 0);}
  CCoeControl* ComponentControl(TInt aIx) const {return ((aIx == 0) ? iTop : NULL);}
  void Draw(const TRect& aRect) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
  void HandleControlEventL(CCoeControl*, TCoeEvent) {;}
  void SetScrollbarVisibility(TBool aValue);

private:
  CAknNavigationControlContainer* GetNaviPane() const;
  CAknNavigationDecorator* iDecoratedTabGroup;
  CAknTabGroup* iTabGroup;
  CCoeControl* iTop;
  CAmarettoCallback* iTabCallback;
  CAmarettoCallback* iEventCallback;
  TBool iPointerForwardDisabled;
  TInt iTopControl;
};

#endif /* __CONTAINER_H */
