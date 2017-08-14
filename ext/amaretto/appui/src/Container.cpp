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
 *  Window-owning container control class CAmarettoContainer
 */

#include "Container.h"

CAmarettoContainer* CAmarettoContainer::NewL(const TRect& aRect, CAmarettoAppUi *aAppui)
{
  CAmarettoContainer *self = new (ELeave) CAmarettoContainer;
  CleanupStack::PushL(self);
  self->ConstructL(aRect, aAppui);
  CleanupStack::Pop();
  return self;
}

void CAmarettoContainer::ConstructL(const TRect& aRect, CAmarettoAppUi *aAppui)
{
  CreateWindowL();
  iTop = NULL;
  iTopControl = -1;
  iAppui = aAppui;
  SetRect(aRect);
  iPointerForwardDisabled = EFalse;
  ActivateL();
}

CAmarettoContainer::~CAmarettoContainer()
{
  if (iDecoratedTabGroup) {
    GetNaviPane()->Pop(iDecoratedTabGroup);
    delete iDecoratedTabGroup;
  }
  RemoveDirectionalPad();
}

CAknNavigationControlContainer* CAmarettoContainer::GetNaviPane() const
{
  CEikStatusPane* sp =
    (STATIC_CAST(CAknAppUi*, iEikonEnv->EikAppUi()))->StatusPane();
  return (STATIC_CAST(CAknNavigationControlContainer*,
                      sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi))));
}

void CAmarettoContainer::EnableTabsL(const CDesCArray* aTabTexts,
                                     CAmarettoCallback* aFunc)
{
  if (iDecoratedTabGroup) {
    GetNaviPane()->Pop(iDecoratedTabGroup);
    delete iDecoratedTabGroup;
  }
  if (aTabTexts) {
    iDecoratedTabGroup = GetNaviPane()->CreateTabGroupL();
    iTabGroup = STATIC_CAST(CAknTabGroup*,
                            iDecoratedTabGroup->DecoratedControl());

    for (int i = 0; i < aTabTexts->Count(); i++)
      iTabGroup->AddTabL(i, (*aTabTexts)[i]);

    GetNaviPane()->PushL(*iDecoratedTabGroup);
    iTabCallback = aFunc;
    iTabGroup->SetActiveTabByIndex(0);

    if (iTop)
      iTop->SetFocus(EFalse);
    iTabGroup->SetFocus(ETrue);

    //  Refresh();
  }
  else {
    iDecoratedTabGroup = NULL;
    iTabGroup = NULL;
    iTabCallback = NULL;
    if (iTop)
      iTop->SetFocus(ETrue);
  }
}

void CAmarettoContainer::SetActiveTab(TInt aIndex)
{
  if (iTabGroup && (aIndex >= 0) && (aIndex < iTabGroup->TabCount()))
    iTabGroup->SetActiveTabByIndex(aIndex);
}

void CAmarettoContainer::SetScrollbarVisibility(TBool aValue)
{
  if (iTopControl == LISTBOX) {
    CEikScrollBar *scrollBar = ((CEikListBox*)iTop)->ScrollBarFrame()->GetScrollBarHandle(CEikScrollBar::EVertical);
    if(scrollBar)
      scrollBar->MakeVisible(aValue);
  }
}

TInt CAmarettoContainer::SetComponentL(CCoeControl* aComponent,
                                       CAmarettoCallback* aEventHandler, TInt aControlName)
{
  if (iTop) {
      iTop->MakeVisible(EFalse);
      iTop->SetFocus(EFalse);
      if (((iTopControl == CANVAS) || (iTopControl == GLCANVAS)) && iDirectionalPad)
          iDirectionalPad->MakeVisible(EFalse);
  }

  // Hide the scrollbar if Listbox was the previous topmost widget
  SetScrollbarVisibility(EFalse);

  iTop = aComponent;

  if (aControlName != -1)
      iTopControl = aControlName;

  // Unhide the scrollbar if Listbox is currently being assigned to body
  SetScrollbarVisibility(ETrue);

  if (iTop) {
      if (((iTopControl == CANVAS) || (iTopControl == GLCANVAS)) && iDirectionalPad)
          iDirectionalPad->MakeVisible(ETrue);
    iTop->MakeVisible(ETrue);
  }
  iEventCallback = aEventHandler;

  if (aComponent && !iTabGroup)
    aComponent->SetFocus(ETrue);

  Refresh();

  return KErrNone;
}

void CAmarettoContainer::SizeChanged()
{
    if (iTop) {
        TInt containerTlX  = Rect().iTl.iX;
        TInt containerTlY = Rect().iTl.iY;
        TInt containerBrX = Rect().iBr.iX;
        TInt containerBrY = Rect().iBr.iY;
        TInt containerWidth = containerBrX - containerTlX;
        TInt containerHeight = containerBrY - containerTlY;
        TRect canvasRect, directionalPadRect;
        // If the topmost control is not canvas or if directional_pad_flag is False,
        // set the widget's rect as Container's rect.
        if (((iTopControl != CANVAS) && (iTopControl != GLCANVAS)) || !directional_pad_flag) {
            iTop->SetRect(Rect());
            if (iDirectionalPad)
                iDirectionalPad->MakeVisible(EFalse);
        }
        else {
            if (containerWidth < containerHeight) {
                TInt splitPoint = 3 * containerBrY / 4;
                // D-pad rect size relative to Container
                directionalPadRect = TRect(0, splitPoint, containerBrX, containerBrY);
                // User Canvas rect size relative to Container
                canvasRect = TRect(containerTlX, containerTlY, containerBrX, splitPoint);
            }
            else {
                TInt splitPoint = 3 * containerBrX / 4;
                // D-pad rect size relative to Container
                directionalPadRect = TRect(splitPoint, 0, containerBrX, containerBrY);
                // User Canvas rect size relative to Container
                canvasRect = TRect(containerTlX, containerTlY, splitPoint, containerBrY);
            }
            iTop->SetRect(canvasRect);
            if (!iDirectionalPad) {
                TInt error = CreateDirectionalPad(iAppui);
                if (error != KErrNone) {
                    SPyErr_SetFromSymbianOSErr(error);
                }
            }
            iDirectionalPad->SetRect(directionalPadRect);
            iDirectionalPad->CalculateAndDrawGraphic(iDirectionalPadGc);
        }
    }
}

void CAmarettoContainer::Draw(const TRect& aRect) const
{
  CWindowGc& gc = SystemGc();
  gc.SetPenStyle(CGraphicsContext::ENullPen);
  gc.SetBrushColor(KRgbWhite);
  gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
  gc.DrawRect(aRect);
  if (iDirectionalPad && iDirectionalPad->IsVisible())
      iDirectionalPad->CalculateAndDrawGraphic(iDirectionalPadGc);
}

void CAmarettoContainer::RefreshDirectionalPad()
{
    if (iDirectionalPad && iDirectionalPad->IsVisible()) {
        iDirectionalPad->CalculateAndDrawGraphic(iDirectionalPadGc);
    }
}

TKeyResponse
CAmarettoContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
  if (iTabGroup && ((aKeyEvent.iCode == EKeyLeftArrow) ||
                    (aKeyEvent.iCode == EKeyRightArrow))) {

    TInt active = iTabGroup->ActiveTabIndex();
    TInt count = iTabGroup->TabCount();

    if ((aKeyEvent.iCode == EKeyLeftArrow) && (active > 0))
      active--;
    else if ((aKeyEvent.iCode == EKeyRightArrow) && ((active + 1) < count))
      active++;
    else
      return EKeyWasConsumed;

    iTabGroup->SetActiveTabByIndex(active);
    iTabCallback->Call((void*)active);

    Refresh();

    return EKeyWasConsumed;
  }

  if (iTop) {
    if ((aType == EEventKey) && iEventCallback) {
      SAmarettoEventInfo event_info;
      event_info.iType = SAmarettoEventInfo::EKey;
      event_info.iKeyEvent = aKeyEvent;
      iEventCallback->Call((void*)&event_info);
    }
    return iTop->OfferKeyEventL(aKeyEvent, aType);
  }
  else
    return EKeyWasNotConsumed;
}

void CAmarettoContainer::DisablePointerForwarding(TBool aValue)
{
    iPointerForwardDisabled = aValue;
}

TInt CAmarettoContainer::CreateDirectionalPad(CAmarettoAppUi* aAppui)
{
    if (!iDirectionalPad) {
        iDirectionalPad = new CDirectionalPad();
        if (iDirectionalPad == NULL)
            return KErrNoMemory;
        TRAPD(error, {
                iDirectionalPad->ConstructL(this, aAppui);
                iDirectionalPadGc = iDirectionalPad->ControlEnv()->CreateGcL();
        });
        if (error != KErrNone)
            return error;
        iDirectionalPadGc->Activate(*iDirectionalPad->DrawableWindow());
    }
    return KErrNone;
}

void CAmarettoContainer::RemoveDirectionalPad()
{
    delete iDirectionalPad;
    iDirectionalPad = NULL;
    if (iDirectionalPadGc)
        iDirectionalPadGc->Deactivate();
    delete iDirectionalPadGc;
    iDirectionalPadGc = NULL;
}

void CAmarettoContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
    // Check if touch is enabled or not
    if(!touch_enabled_flag) {
        return;
    }
    if (iTop && iEventCallback) {
          SAmarettoEventInfo event_info;
          event_info.iType = SAmarettoEventInfo::EPointer;
          memset(&event_info.iKeyEvent, 0, sizeof(event_info.iKeyEvent));
          event_info.iPointerEvent = aPointerEvent;
          iEventCallback->Call((void*)&event_info);
        }
    // Call base class method, that forwards pointer event to the right child
    // component. We disable this for Canvas object.
    if(!iPointerForwardDisabled)
        CCoeControl::HandlePointerEventL(aPointerEvent);
}


CDirectionalPad::CDirectionalPad()
{
    iLeftTriangle = new CArrayFixFlat<TPoint>(12);
    iRightTriangle = new CArrayFixFlat<TPoint>(12);
    iUpTriangle = new CArrayFixFlat<TPoint>(12);
    iDownTriangle = new CArrayFixFlat<TPoint>(12);
    iCenterRect = new CArrayFixFlat<TPoint>(12);
    if (!iLeftTriangle || !iRightTriangle || !iUpTriangle || !iDownTriangle || !iCenterRect) {
        SPyErr_SetFromSymbianOSErr(KErrNoMemory);
    }
}

CDirectionalPad::~CDirectionalPad()
{
    delete iLeftTriangle;
    iLeftTriangle = NULL;
    delete iRightTriangle;
    iRightTriangle = NULL;
    delete iUpTriangle;
    iUpTriangle = NULL;
    delete iDownTriangle;
    iDownTriangle = NULL;
    delete iCenterRect;
    iCenterRect = NULL;
}

void CDirectionalPad::ConstructL(const CCoeControl* aParent, CAmarettoAppUi* aAppui)
{
    CreateWindowL(aParent);
    iParent = aParent;
    iAppui = aAppui;
    ActivateL();
}

void CDirectionalPad::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
  TKeyEvent keyEvent;
  keyEvent.iModifiers = EModifierKeypad;
  // TODO: Implement repeat support using RTimer?
  keyEvent.iRepeats   = 0;

  if (iLeftButton.Contains(aPointerEvent.iPosition)) {
    keyEvent.iCode      = EKeyLeftArrow;
    keyEvent.iScanCode  = EStdKeyLeftArrow;
  } else if (iRightButton.Contains(aPointerEvent.iPosition)) {
    keyEvent.iCode      = EKeyRightArrow;
    keyEvent.iScanCode  = EStdKeyRightArrow;
  } else if (iUpButton.Contains(aPointerEvent.iPosition)) {
      keyEvent.iCode      = EKeyUpArrow;
      keyEvent.iScanCode  = EStdKeyUpArrow;
  } else if (iDownButton.Contains(aPointerEvent.iPosition)) {
      keyEvent.iCode      = EKeyDownArrow;
      keyEvent.iScanCode  = EStdKeyDownArrow;
  } else if (iCenterButton.Contains(aPointerEvent.iPosition)) {
      keyEvent.iCode      = EKeyDevice3;
      keyEvent.iScanCode  = EStdKeyDevice3;
  } else if (iAppui->iScreenMode == 2 && iTopRightButton.Contains(aPointerEvent.iPosition)) {
      iAppui->ProcessCommandL(EAknSoftkeyExit);
  } else if (iAppui->iScreenMode == 2 && iTopLeftButton.Contains(aPointerEvent.iPosition)) {
      iAppui->ProcessCommandL(EAknSoftkeyOptions);
}

  if (aPointerEvent.iType == TPointerEvent::EButton1Up) {
      CONST_CAST(CCoeControl*, iParent)->OfferKeyEventL(keyEvent, EEventKeyUp);
      delete iRepeatEventHandle;
      iRepeatEventHandle = NULL;
  }

  if (aPointerEvent.iType == TPointerEvent::EButton1Down) {
      CONST_CAST(CCoeControl*, iParent)->OfferKeyEventL(keyEvent, EEventKeyDown);
      CONST_CAST(CCoeControl*, iParent)->OfferKeyEventL(keyEvent, EEventKey);
      iRepeatEventHandle = NULL;
      iRepeatEventHandle = CRepeatEventGenerator::NewL(iParent, keyEvent);
      if (iRepeatEventHandle == NULL) {
          SPyErr_SetFromSymbianOSErr(KErrNoMemory);
      }
  }
}

void CDirectionalPad::DrawKeys(CWindowGc *gc, TRect keyRect, CArrayFix<TPoint>* polygonPoints) {
    gc->SetBrushStyle((CGraphicsContext::TBrushStyle) 1 );
    gc->SetBrushColor(DARKGREY);
    gc->SetPenColor(DARKGREY);
    gc->DrawRoundRect(keyRect, TSize(3,3));
    //inner fill
    gc->SetBrushStyle((CGraphicsContext::TBrushStyle) 1 );
    gc->SetBrushColor(LIGHTGREY);
    gc->SetPenColor(LIGHTGREY);
    keyRect.Shrink(3,3);
    gc->DrawRoundRect(keyRect, TSize(3,3));
    gc->SetBrushColor(KRgbBlack);
    gc->SetPenColor(KRgbBlack);
    if (polygonPoints!=NULL)
        gc->DrawPolygon(polygonPoints, (CGraphicsContext::TFillRule) 0);
}

void CDirectionalPad::CalculateAndDrawGraphic(CWindowGc *gc)
{
    TRect containerRect = iAppui->iContainer->Rect();
    TInt containerTlX  = containerRect.iTl.iX;
    TInt containerTlY = containerRect.iTl.iY;
    TInt containerBrX = containerRect.iBr.iX;
    TInt containerBrY = containerRect.iBr.iY;
    TInt containerWidth = containerBrX - containerTlX;
    TInt containerHeight = containerBrY - containerTlY;
    TRect dpadRect;
    TInt buttonHeight, buttonWidth, dpadTlX, dpadTlY, dpadBrX, dpadBrY;

    // Portrait mode
    if (containerWidth < containerHeight) {
        TInt splitPoint = 3 * containerBrY / 4;
        // Rect co-ordinates w.r.t. itself
        dpadRect = TRect(0, 0, containerBrX, containerBrY - splitPoint);
        // Rect co-ordinates w.r.t Container
        SetRect(TRect(0, splitPoint, containerBrX, containerBrY));
    }
    else { // Landscape mode
        TInt splitPoint = 3 * containerBrX / 4;
        // Rect co-ordinates w.r.t. itself
        dpadRect = TRect(0, 0, containerBrX - splitPoint, containerBrY);
        // Rect co-ordinates w.r.t Container
        SetRect(TRect(splitPoint, 0, containerBrX, containerBrY));
    }

    // Calculate the button and graphic sizes only if the dpad size has changed
    if (iPrevDpadRect != dpadRect) {
        dpadTlX = dpadRect.iTl.iX;
        dpadTlY = dpadRect.iTl.iY;
        dpadBrX = dpadRect.iBr.iX;
        dpadBrY = dpadRect.iBr.iY;

        if (containerWidth < containerHeight) {
            iLeftButton.SetRect(dpadBrX/5, dpadBrY/3, 2*dpadBrX/5, 2*dpadBrY/3);
            iRightButton.SetRect(3*dpadBrX/5, dpadBrY/3, 4*dpadBrX/5, 2*dpadBrY/3);
            iCenterButton.SetRect(2*dpadBrX/5, dpadBrY/3, 3*dpadBrX/5, 2*dpadBrY/3);
            iUpButton.SetRect(2*dpadBrX/5, dpadTlY, 3*dpadBrX/5, dpadBrY/3);
            iDownButton.SetRect(2*dpadBrX/5, 2*dpadBrY/3, 3*dpadBrX/5, dpadBrY);
            iTopLeftButton.SetRect(dpadTlX, dpadTlY, 3*dpadBrX/10, dpadBrY/3);  // Options
            iTopRightButton.SetRect(7*dpadBrX/10, dpadTlX, dpadBrX, dpadBrY/3);  // Exit
            // Button matrix of 3x5
            buttonHeight = dpadBrY/3;
            buttonWidth = dpadBrX/5;
        } else {
            iLeftButton.SetRect(dpadTlX, 2*dpadBrY/5, dpadBrX/3, 3*dpadBrY/5);
            iRightButton.SetRect(2*dpadBrX/3, 2*dpadBrY/5, dpadBrX, 3*dpadBrY/5);
            iCenterButton.SetRect(dpadBrX/3, 2*dpadBrY/5, 2*dpadBrX/3, 3*dpadBrY/5);
            iUpButton.SetRect(dpadBrX/3, dpadBrY/5, 2*dpadBrX/3, 2*dpadBrY/5);
            iDownButton.SetRect(dpadBrX/3, 3*dpadBrY/5, 2*dpadBrX/3, 4*dpadBrY/5);
            iTopLeftButton.SetRect(dpadTlX, 4*dpadBrY/5, dpadBrX, dpadBrY);  // Exit
            iTopRightButton.SetRect(dpadTlX, dpadTlY, dpadBrX, dpadBrY/5);  // Options
            // Button matrix of 5x3
            buttonHeight = dpadBrY/5;
            buttonWidth = dpadBrX/3;
        }

        // These offsets are used to define how far the buttons are from the dpad
        // window boundary in X and Y directions
        TInt left_x_offset, left_y_offset, right_x_offset, right_y_offset, up_x_offset,
             up_y_offset, down_x_offset, down_y_offset, center_x_offset, center_y_offset;

        // Portrait mode
        if (containerWidth < containerHeight) {
            left_x_offset = buttonWidth;
            left_y_offset = buttonHeight;
            right_x_offset = 3 * buttonWidth;
            right_y_offset = buttonHeight;
            up_x_offset = 2 * buttonWidth;
            up_y_offset = 0;
            down_x_offset = 2 * buttonWidth;
            down_y_offset = 2 * buttonHeight;
            center_x_offset = 2 * buttonWidth;
            center_y_offset = buttonHeight;
        }
        else { // Landscape mode
            left_x_offset = 0;
            left_y_offset = 2 * buttonHeight;
            right_x_offset = 2 * buttonWidth;
            right_y_offset = 2 * buttonHeight;
            up_x_offset = buttonWidth;
            up_y_offset = buttonHeight;
            down_x_offset = buttonWidth;
            down_y_offset = 3 * buttonHeight;
            center_x_offset = buttonWidth;
            center_y_offset = 2 * buttonHeight;
        }
        if (iLeftTriangle->Count() == 3) {
            iLeftTriangle->Delete(0,3);
            iLeftTriangle->Compress();
        }
        if (iRightTriangle->Count() == 3) {
            iRightTriangle->Delete(0,3);
            iRightTriangle->Compress();
        }
        if (iUpTriangle->Count() == 3) {
            iUpTriangle->Delete(0,3);
            iUpTriangle->Compress();
        }
        if (iDownTriangle->Count() == 3) {
            iDownTriangle->Delete(0,3);
            iDownTriangle->Compress();
        }
        if (iCenterRect->Count() == 4) {
            iCenterRect->Delete(0,4);
            iCenterRect->Compress();
        }

        TRAPD(error, {
            // The button is divided into a 3x3 matrix and the triangle sits in the middle
            CleanupStack::PushL(iLeftTriangle);
            TPoint point_a = TPoint(2 * buttonWidth / 3 + left_x_offset, (buttonHeight / 3) + left_y_offset);
            TPoint point_b = TPoint(2 * buttonWidth / 3 + left_x_offset, (2 * buttonHeight / 3) + left_y_offset);
            TPoint point_c = TPoint(buttonWidth / 3 + left_x_offset, (((buttonHeight / 3) + (buttonHeight / (2*3)))) + left_y_offset);

            iLeftTriangle->AppendL(point_a);
            iLeftTriangle->AppendL(point_b);
            iLeftTriangle->AppendL(point_c);

            CleanupStack::Pop(iLeftTriangle);

            CleanupStack::PushL(iRightTriangle);
            point_a = TPoint((buttonWidth / 3) + right_x_offset, (buttonHeight / 3) + right_y_offset);
            point_b = TPoint((buttonWidth / 3) + right_x_offset, (2 * buttonHeight / 3) + right_y_offset);
            point_c = TPoint((2 * buttonWidth / 3) + right_x_offset, (((buttonHeight / 3) + (buttonHeight / (2*3)))) + right_y_offset);

            iRightTriangle->AppendL(point_a);
            iRightTriangle->AppendL(point_b);
            iRightTriangle->AppendL(point_c);

            CleanupStack::Pop(iRightTriangle);

            CleanupStack::PushL(iUpTriangle);
            point_a = TPoint((buttonWidth / 3) + up_x_offset, (2 * buttonHeight / 3) + up_y_offset);
            point_b = TPoint((2 * buttonWidth / 3) + up_x_offset, (2 * buttonHeight / 3) + up_y_offset);
            point_c = TPoint(((buttonWidth / 3) + (buttonWidth / 6)) + up_x_offset, ((buttonHeight / 3)) + up_y_offset);

            iUpTriangle->AppendL(point_a);
            iUpTriangle->AppendL(point_b);
            iUpTriangle->AppendL(point_c);

            CleanupStack::Pop(iUpTriangle);

            CleanupStack::PushL(iDownTriangle);
            point_a = TPoint((buttonWidth / 3) + down_x_offset, (buttonHeight / 3) + down_y_offset);
            point_b = TPoint((2 * buttonWidth / 3) + down_x_offset, (buttonHeight / 3) + down_y_offset);
            point_c = TPoint(((buttonWidth / 3) + (buttonWidth / 6)) + down_x_offset, (2 * buttonHeight / 3) + down_y_offset);

            iDownTriangle->AppendL(point_a);
            iDownTriangle->AppendL(point_b);
            iDownTriangle->AppendL(point_c);

            CleanupStack::Pop(iDownTriangle);

            // The center rect graphic encompasses the (2,2) cell in the 3x3 button matrix
            CleanupStack::PushL(iCenterRect);
            point_a = TPoint((buttonWidth / 3) + center_x_offset, (buttonHeight / 3) + center_y_offset);
            point_b = TPoint((2 * buttonWidth / 3) + center_x_offset, (buttonHeight / 3) + center_y_offset);
            point_c = TPoint((2 * buttonWidth / 3) + center_x_offset, (2 * buttonHeight / 3) + center_y_offset);
            TPoint point_d = TPoint((buttonWidth / 3) + center_x_offset, (2 * buttonHeight / 3) + center_y_offset);

            iCenterRect->AppendL(point_a);
            iCenterRect->AppendL(point_b);
            iCenterRect->AppendL(point_c);
            iCenterRect->AppendL(point_d);

            CleanupStack::Pop(iCenterRect);
        });
        if (error != KErrNone)
            SPyErr_SetFromSymbianOSErr(error);
    }

    if (gc) {
        gc->SetBrushColor(DARKGREY);
        gc->Clear();
        DrawKeys(gc, iLeftButton, iLeftTriangle);
        DrawKeys(gc, iRightButton, iRightTriangle);
        DrawKeys(gc, iCenterButton, iCenterRect);
        DrawKeys(gc, iUpButton, iUpTriangle);
        DrawKeys(gc, iDownButton, iDownTriangle);

        // Draw the Options and Exit buttons only in 'full' screen mode
        if (iAppui->iScreenMode == 2) {
            DrawKeys(gc, iTopLeftButton, NULL);
            DrawKeys(gc, iTopRightButton, NULL);
            const CFont* f = CEikonEnv::Static()->NormalFont();
            gc->UseFont(f);
            gc->SetPenStyle( CGraphicsContext::ESolidPen );
            gc->SetPenColor(KRgbBlack);
            gc->SetBrushStyle(CGraphicsContext::ENullBrush);
            gc->SetPenSize(TSize(1, 1));
            gc->DrawText(_L("Options"), iTopLeftButton, (iTopLeftButton.Height()/2 + f->HeightInPixels()/3), CGraphicsContext::ECenter, 0);
            gc->DrawText(_L("Exit"), iTopRightButton, (iTopRightButton.Height()/2 + f->HeightInPixels()/3), CGraphicsContext::ECenter, 0);
            gc->DiscardFont();
        }
    }
    iPrevDpadRect = dpadRect;
}


CRepeatEventGenerator::CRepeatEventGenerator(const CCoeControl* aParent, TKeyEvent aKeyEvent)
    : iParent(aParent), iKeyEvent(aKeyEvent)
{}

void CRepeatEventGenerator::ConstructL()
{
    iPeriodic = CPeriodic::NewL(CActive::EPriorityUserInput);
    iPeriodic->Start(TICKDELAY, TICKINTERVAL, TCallBack(Tick, this));
}

CRepeatEventGenerator* CRepeatEventGenerator::NewL(const CCoeControl* aParent, TKeyEvent aKeyEvent)
{
	CRepeatEventGenerator* self=new (ELeave) CRepeatEventGenerator(aParent, aKeyEvent);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

CRepeatEventGenerator::~CRepeatEventGenerator()
{
    iPeriodic->Cancel();
    delete iPeriodic;
}

TInt CRepeatEventGenerator::Tick(TAny* aObject)
{
    ((CRepeatEventGenerator*)aObject)->DoTick();
    return 1;
}

void CRepeatEventGenerator::DoTick()
{
    CONST_CAST(CCoeControl*, iParent)->OfferKeyEventL(iKeyEvent, EEventKeyDown);
    CONST_CAST(CCoeControl*, iParent)->OfferKeyEventL(iKeyEvent, EEventKey);
}
