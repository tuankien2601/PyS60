/*
* ====================================================================
*  globalui.cpp  
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
#include "Python.h"
#include <aknglobalnote.h>
#include <eiknotapi.h>
#include <aknglobalconfirmationquery.h>
#include <aknglobalmsgquery.h>
#include <akngloballistquery.h>
#include <e32std.h>
#include <e32base.h>
#include <badesca.h>
#include "globaluimodule.h"

#define TO_MILLISEC(s) (s)*1000*1000

/* TYPE METHODS */

extern "C" PyObject*
global_note(PyObject* /*self*/, PyObject* args)
{
    TInt noteId = -1;
    char* notestr = NULL;
    char* b_type="info";
    int note_l = 0;
    int l_type=4;
    TAknGlobalNoteType giveType;

    if(!PyArg_ParseTuple(args, "u#|s#:global_note", &notestr, &note_l, &b_type, &l_type)){
        return NULL;
    }

    TPtrC8 type_string((TUint8*)b_type, l_type);
    TPtrC note_str((TUint16*)notestr, note_l);
    TBool error_note_type = EFalse;

    CAknGlobalNote *dialog = NULL;
    TRAPD(err,
        {
            dialog = CAknGlobalNote::NewL();
        });

    if(err != KErrNone){
        RETURN_ERROR_OR_PYNONE(err);
    }
    CleanupStack::PushL(dialog);

    if (type_string.Compare(KGlobalInformationNoteType) == 0){ 
        giveType = EAknGlobalInformationNote;
    }
    else if(type_string.Compare(KGlobalWarningNoteType) == 0){
        giveType = EAknGlobalWarningNote;
    }
    else if(type_string.Compare(KGlobalConfirmationNoteType) == 0){
        giveType = EAknGlobalConfirmationNote;
    }
    else if(type_string.Compare(KGlobalErrorNoteType) == 0){
        giveType = EAknGlobalErrorNote;
    }
    else if(type_string.Compare(KGlobalChargingNoteType) == 0){
        giveType = EAknGlobalChargingNote;
    }
    else if(type_string.Compare(KGlobalWaitNoteType) == 0){
        giveType = EAknGlobalWaitNote;
    }
    else if(type_string.Compare(KGlobalPermanentNoteType) == 0){
        giveType = EAknGlobalPermanentNote;
    }
    else if(type_string.Compare(KGlobalNotChargingNoteType) == 0){
        giveType = EAknGlobalNotChargingNote;
    }
    else if(type_string.Compare(KGlobalBatteryFullNoteType) == 0){
        giveType = EAknGlobalBatteryFullNote;
    }
    else if(type_string.Compare(KGlobalBatteryLowNoteType) == 0){
        giveType = EAknGlobalBatteryLowNote;
    }
    else if(type_string.Compare(KGlobalRechargeBatteryNoteType) == 0){
        giveType = EAknGlobalRechargeBatteryNote;
    }
#if SERIES60_VERSION>20
    else if(type_string.Compare(KGlobalTextNoteType) == 0){
        giveType = EAknGlobalTextNote;
    }
#endif /* SERIES60_VERSION */
    else {
        error_note_type = ETrue;
    }

    if(error_note_type) {
        PyErr_SetString(PyExc_ValueError,"unknown note type");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    TRAPD(err,
        {
            noteId = dialog->ShowNoteL(giveType, note_str);
        });
    Py_END_ALLOW_THREADS  
    CleanupStack::PopAndDestroy(dialog);

    if(err != KErrNone){
        RETURN_ERROR_OR_PYNONE(err);
    }
    else {
        RETURN_ERROR_OR_PYNONE(KErrNone);
    }
}

extern "C" PyObject*
global_query(PyObject* /*self*/, PyObject* args)
{
    TInt queryAnswer = 1;
    char* querystr = NULL;
    int querystr_l = 0;
    TRequestStatus queryStatus = KRequestPending;
    TRequestStatus timerStatus = KRequestPending;
    int delayInSeconds = 0;
    TInt timeout = 0;

    if(!PyArg_ParseTuple(args, "u#|i:global_query", &querystr, &querystr_l, &delayInSeconds)){
        return NULL;
    }
    TPtrC query_str((TUint16*)querystr, querystr_l);
    timeout = TO_MILLISEC(delayInSeconds);
    TTimeIntervalMicroSeconds32 timeoutValue = TTimeIntervalMicroSeconds32(timeout);

    CAknGlobalConfirmationQuery *queryDialog = NULL;
    TRAPD(err,
        {
            queryDialog = CAknGlobalConfirmationQuery::NewL();
        });
    if(err != KErrNone){
        RETURN_ERROR_OR_PYNONE(err);
    }
    CleanupStack::PushL(queryDialog);

    Py_BEGIN_ALLOW_THREADS
    TRAPD(err,
        {
            queryDialog->ShowConfirmationQueryL(queryStatus,query_str);
        });
    Py_END_ALLOW_THREADS
    if(err != KErrNone){ 
        CleanupStack::PopAndDestroy(queryDialog);
        RETURN_ERROR_OR_PYNONE(err);
    }
    bool dialogTimedOut = false;
    if(delayInSeconds > 0){
        RTimer timer;
        CleanupClosePushL(timer);
        timer.CreateLocal();
        timer.After(timerStatus,timeoutValue);
        Py_BEGIN_ALLOW_THREADS
        User::WaitForRequest(timerStatus,queryStatus);
        Py_END_ALLOW_THREADS
		if (timerStatus != KRequestPending)
			dialogTimedOut = true;
        timer.Cancel();
        queryDialog->CancelConfirmationQuery();
        CleanupStack::PopAndDestroy();  // calls timer.Close()
    }
    Py_BEGIN_ALLOW_THREADS
    User::WaitForRequest(queryStatus);
    Py_END_ALLOW_THREADS

    CleanupStack::PopAndDestroy(queryDialog);

    if (dialogTimedOut) {
		Py_INCREF(Py_None);
		return Py_None;
    } else {
    	return Py_BuildValue("i", queryStatus.Int() == EAknSoftkeyYes ? 1 : 0);
    }
}

extern "C" PyObject*
global_popup_menu(PyObject* /*self*/, PyObject* args)
{
    TInt error = KErrNone;
    PyObject* list = NULL;
    TRequestStatus menuStatus = KRequestPending;
    TRequestStatus timerStatus = KRequestPending;
    char* headerstr = NULL;
    int headerstr_l = 0;
    int delayInSeconds = 0;
    TInt timeout = 0;
    
    if(!PyArg_ParseTuple(args, "O!|u#i:global_list_query", &PyList_Type, &list, &headerstr, &headerstr_l, &delayInSeconds)){
        return NULL;
    }
    int sz = PyList_Size(list);
    if (sz==0){
        PyErr_SetString(PyExc_ValueError, "List is empty");
        return NULL;
    }

    TPtrC header_str((TUint16*)headerstr, headerstr_l);
    timeout = TO_MILLISEC(delayInSeconds);
    TTimeIntervalMicroSeconds32 timeoutValue = TTimeIntervalMicroSeconds32(timeout);
    
    /* Creation of list array */
    CDesCArrayFlat* listArray = new (ELeave) CDesCArrayFlat(sz);
    if(listArray == NULL){
        PyErr_SetString(PyExc_MemoryError, "Could not allocate memory for array");
        return NULL;
    }
    CleanupStack::PushL(listArray);
    TBuf<KMaxFileName> temp;
    for (int i = 0; i < sz; i++) {
        PyObject* s = PyList_GetItem(list, i);
        if (!PyUnicode_Check(s)) {
            error = KErrArgument;
        }
        else {
            temp.Copy(PyUnicode_AsUnicode(s),
                        Min(PyUnicode_GetSize(s), KMaxFileName));
            TRAP(error, listArray->AppendL(temp));
        }
        if (error != KErrNone)
            break;
    }

    /* Creation of list query */
    CAknGlobalListQuery *listQueryDialog = NULL;
    TRAPD(err,
        {
            listQueryDialog = CAknGlobalListQuery::NewL();
        });
    if(err != KErrNone){
        RETURN_ERROR_OR_PYNONE(err);
    }
    CleanupStack::PushL(listQueryDialog);   

#if SERIES60_VERSION>20 
    if(headerstr){
        TRAPD(error_header,
            {
                listQueryDialog->SetHeadingL(header_str);
            });

        if(error_header != KErrNone){ 
            CleanupStack::PopAndDestroy(listQueryDialog);
            RETURN_ERROR_OR_PYNONE(error);
        }
    }
#endif /* SERIES60_VERSION */

    Py_BEGIN_ALLOW_THREADS

    TRAPD(err,
        {
            listQueryDialog->ShowListQueryL(listArray,menuStatus);
        });
    Py_END_ALLOW_THREADS
    if(err != KErrNone){ 
        CleanupStack::PopAndDestroy(listQueryDialog);
        RETURN_ERROR_OR_PYNONE(err);
    }
    bool dialogTimedOut = false;
    if(delayInSeconds > 0){
        RTimer timer;
        CleanupClosePushL(timer);
        timer.CreateLocal();  
        timer.After(timerStatus,timeoutValue);
        Py_BEGIN_ALLOW_THREADS
        User::WaitForRequest(timerStatus,menuStatus);
        Py_END_ALLOW_THREADS
        if (timerStatus != KRequestPending)
        	dialogTimedOut = true;				
        timer.Cancel();
        listQueryDialog->CancelListQuery();
        CleanupStack::PopAndDestroy();  // calls timer.Close()
    }
    Py_BEGIN_ALLOW_THREADS
    User::WaitForRequest(menuStatus);
    Py_END_ALLOW_THREADS

    CleanupStack::PopAndDestroy(listQueryDialog);
    CleanupStack::PopAndDestroy(listArray);

    if (dialogTimedOut) {
    	Py_INCREF(Py_None);
    	return Py_None;
    } else
    	return Py_BuildValue("i", menuStatus.Int());
}

extern "C" PyObject*
global_msg_query(PyObject* /*self*/, PyObject* args)
{
    TInt queryAnswer = 1;
    char* querystr = NULL;
    int querystr_l = 0;
    char* headerstr = NULL;
    int headerstr_l = 0;
    TRequestStatus msgQueryStatus = KRequestPending;
    TRequestStatus timerStatus = KRequestPending;
    int delayInSeconds = 0;
    TInt timeout = 0;

    if(!PyArg_ParseTuple(args, "u#u#|i:global_msg_query", &querystr, &querystr_l, &headerstr, &headerstr_l, &delayInSeconds)){
        return NULL;
    }
    TPtrC query_str((TUint16*)querystr, querystr_l);
    TPtrC header_str((TUint16*)headerstr, headerstr_l);
    timeout = TO_MILLISEC(delayInSeconds);
    TTimeIntervalMicroSeconds32 timeoutValue = TTimeIntervalMicroSeconds32(timeout);

    CAknGlobalMsgQuery *msgQueryDialog = NULL;
    TRAPD(err,
        {
            msgQueryDialog = CAknGlobalMsgQuery::NewL();
        });
    if(err != KErrNone){
        RETURN_ERROR_OR_PYNONE(err);
    }
    CleanupStack::PushL(msgQueryDialog);
    Py_BEGIN_ALLOW_THREADS
    TRAPD(err,
        {
            msgQueryDialog->ShowMsgQueryL(msgQueryStatus,query_str,R_AVKON_SOFTKEYS_OK_CANCEL,header_str, KNullDesC);
        });
    Py_END_ALLOW_THREADS
    if(err != KErrNone){
       CleanupStack::PopAndDestroy(msgQueryDialog);
       RETURN_ERROR_OR_PYNONE(err);
    } 
    bool dialogTimedOut = false;
    if (delayInSeconds > 0){
        RTimer timer;
        CleanupClosePushL(timer);
        timer.CreateLocal();
        timer.After(timerStatus,timeoutValue);
        Py_BEGIN_ALLOW_THREADS
        User::WaitForRequest(timerStatus,msgQueryStatus);
        Py_END_ALLOW_THREADS
        if (timerStatus != KRequestPending)
        	dialogTimedOut = true;				
        timer.Cancel();
        msgQueryDialog->CancelMsgQuery();
        CleanupStack::PopAndDestroy();  // calls timer.Close()
    } 
	Py_BEGIN_ALLOW_THREADS
	User::WaitForRequest(msgQueryStatus);
	Py_END_ALLOW_THREADS
    
    CleanupStack::PopAndDestroy(msgQueryDialog);
    if (dialogTimedOut) {
    	Py_INCREF(Py_None);
    	return Py_None;
    } else
    	return Py_BuildValue("i", 
					msgQueryStatus.Int() == EAknSoftkeyOk ? 1 : 0);		
}

/* INIT */
extern "C" {
    static const PyMethodDef globalui_methods[] = {
        {"global_note", (PyCFunction)global_note, METH_VARARGS, NULL},
        {"global_query", (PyCFunction)global_query, METH_VARARGS, NULL},
        {"global_msg_query", (PyCFunction)global_msg_query, METH_VARARGS, NULL},
        {"global_popup_menu", (PyCFunction)global_popup_menu, METH_VARARGS, NULL},
        {NULL,               NULL}/* sentinel*/
    };
    DL_EXPORT(void) initglobalui(void)
    {
        Py_InitModule("globalui", (PyMethodDef*)globalui_methods);
    }
}  /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
      return KErrNone;
}
#endif
