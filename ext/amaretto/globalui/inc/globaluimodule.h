/*
* ====================================================================
*  globaluimodule.h
*
*  Python API to Series 60 application UI framework facilities
*
* Copyright (c) 2008 Nokia Corporation
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
#ifndef __GLOBALUIMODULE_H
#define __GLOBALUIMODULE_H

#include "Python.h"
#include "symbian_python_ext_util.h"

#include <e32base.h>

#if SERIES60_VERSION>20
_LIT8(KGlobalTextNoteType, "text");
#endif /* SERIES60_VERSION */
_LIT8(KGlobalInformationNoteType, "info");
_LIT8(KGlobalWarningNoteType, "warn");
_LIT8(KGlobalConfirmationNoteType, "confirm");
_LIT8(KGlobalErrorNoteType, "error");
_LIT8(KGlobalChargingNoteType, "charging");
_LIT8(KGlobalWaitNoteType, "wait");
_LIT8(KGlobalPermanentNoteType, "perm");
_LIT8(KGlobalNotChargingNoteType, "not_charging");
_LIT8(KGlobalBatteryFullNoteType, "battery_full");
_LIT8(KGlobalBatteryLowNoteType, "battery_low");
_LIT8(KGlobalRechargeBatteryNoteType, "recharge_battery");
#endif /* __GLOBALUIMODULE_H */