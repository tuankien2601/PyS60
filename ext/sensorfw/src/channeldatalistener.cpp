/* Copyright (c) 2009 Nokia Corporation
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


#include <e32std.h>
#include <e32cmn.h>

#include <sensrvaccelerometersensor.h>
#include <sensrvchannel.h>
#include <sensrvchannelcondition.h>
#include <sensrvchannelconditionlistener.h>
#include <sensrvchannelconditionset.h>
#include <sensrvchannelfinder.h>
#include <sensrvchannelinfo.h>
#include <sensrvchannellistener.h>
#include <sensrvdatalistener.h>
#include <sensrvgeneralproperties.h>
#include <sensrvilluminationsensor.h>
#include <sensrvmagneticnorthsensor.h>
#include <sensrvmagnetometersensor.h>
#include <sensrvorientationsensor.h>
#include <sensrvproperty.h>
#include <sensrvpropertylistener.h>
#include <sensrvproximitysensor.h>
#include <sensrvtappingsensor.h>
#include <sensrvtypes.h>


#include "channeldatalistener.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CChannelDataListener::CChannelDataListener
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CChannelDataListener::CChannelDataListener(
        PyObject* aDataReceivedCallback, PyObject* aDataErrorCallback )
: iDataReceivedCallback( aDataReceivedCallback ), 
  iDataErrorCallback( aDataErrorCallback )
    {
    // Increment Python reference counting:
    Py_XINCREF( iDataReceivedCallback );
    Py_XINCREF( iDataErrorCallback );
    }

// -----------------------------------------------------------------------------
// CChannelDataListener::ConstructL
// Symbian second-phase constructor that may leave.
// -----------------------------------------------------------------------------
//
void CChannelDataListener::ConstructL()
    {
    // No implementation required.
    }

// -----------------------------------------------------------------------------
// CChannelDataListener::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CChannelDataListener* CChannelDataListener::NewL(
        PyObject* aDataReceivedCallback, PyObject* aDataErrorCallback )
    {
    CChannelDataListener* self = CChannelDataListener::NewLC( 
            aDataReceivedCallback, aDataErrorCallback );
    CleanupStack::Pop();
    return self;
    }

// -----------------------------------------------------------------------------
// CChannelDataListener::NewLC
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CChannelDataListener* CChannelDataListener::NewLC(
        PyObject* aDataReceivedCallback, PyObject* aDataErrorCallback )
    {
    CChannelDataListener* self = new( ELeave ) CChannelDataListener( 
            aDataReceivedCallback, aDataErrorCallback );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

// Destructor
CChannelDataListener::~CChannelDataListener()
    {
    // Decrement Python reference counting:
    Py_XDECREF( iDataReceivedCallback );
    Py_XDECREF( iDataErrorCallback );
    }

// -----------------------------------------------------------------------------
// CChannelDataListener::DataReceived
// -----------------------------------------------------------------------------
//
void CChannelDataListener::DataReceived( CSensrvChannel& aChannel, TInt aCount, 
        TInt /*aDataLost*/ )
    {
    // Return immediately, if no Python callback object.
    if ( iDataReceivedCallback == NULL ) return;
    
    // Save state of any current Python interpreter, and acquire the
    // interpreter lock.
    //TInt in_interpreter = (_PyThreadState_Current == PYTHON_TLS->thread_state);
    //if ( !in_interpreter ) PyEval_RestoreThread( PYTHON_TLS->thread_state );
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Handle multiple new samples in a loop.
    // NOTE: each sample is notified with a separate Python callback, even if
    // the input buffer size is set to larger than 1. This works ok, but the 
    // communication between C and Python is not fully optimized for speed.
    // NOTE: the number of lost samples is not send to Python side at all. This
    // could be done rather easily by utilizing the data error callback with
    // a new error code (different than the ones specified in Sensor API).
    for ( TInt i = 0; i < aCount; i++ )
        {
        PyObject* parameters = NULL;
        parameters = GetAndPackageData( aChannel );
        if ( parameters != NULL )
            {
            // Call Python callback function with parameters.
            PyObject* tmp = PyEval_CallObject( 
                    iDataReceivedCallback, parameters );

            // Decrement Python reference counting:
            Py_XDECREF( parameters );
            Py_XDECREF( tmp );
            
            // Check for errors:
            if ( PyErr_Occurred() )
                {
                PyErr_Print();
                }
            }
        }

    // Restore Python thread state.
    //if ( !in_interpreter ) PyEval_SaveThread();
    PyGILState_Release(gstate);
    }

// -----------------------------------------------------------------------------
// CChannelDataListener::DataError
// -----------------------------------------------------------------------------
//
void CChannelDataListener::DataError( CSensrvChannel& aChannel, 
        TSensrvErrorSeverity aError )
    {
    // Return immediately, if no Python callback object.
    if ( iDataErrorCallback == NULL ) return;

    // Save state of any current Python interpreter, and acquire the
    // interpreter lock.
    //TInt in_interpreter = (_PyThreadState_Current == PYTHON_TLS->thread_state);
    //if ( !in_interpreter ) PyEval_RestoreThread( PYTHON_TLS->thread_state );
    PyGILState_STATE gstate = PyGILState_Ensure();
    // Package channed ID and error code to Python parameters.
    PyObject* parameters = NULL;
    parameters = Py_BuildValue(
            "({s:i,s:i})",
            "channel_id", aChannel.GetChannelInfo().iChannelId,
            "error", aError );
    if ( parameters != NULL )
        {
        // Call Python callback function with parameters.
        PyObject* tmp = PyEval_CallObject( iDataErrorCallback, parameters );

        // Decrement Python reference counting:
        Py_XDECREF( parameters );
        Py_XDECREF( tmp );
        
        // Check for errors:
        if ( PyErr_Occurred() )
            {
            PyErr_Print();
            }
        }

    // Restore Python thread state.
    //if ( !in_interpreter ) PyEval_SaveThread();
    PyGILState_Release(gstate);
    }

// -----------------------------------------------------------------------------
// CChannelDataListener::GetAndPackageData
// -----------------------------------------------------------------------------
//
PyObject* CChannelDataListener::GetAndPackageData( CSensrvChannel& aChannel )
    {
    switch ( aChannel.GetChannelInfo().iChannelType )
        {
        case KSensrvChannelTypeIdAccelerometerXYZAxisData:
            {
            TSensrvAccelerometerAxisData data;
            TPckg<TSensrvAccelerometerAxisData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i,s:i,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "axis_x",               data.iAxisX,
                "axis_y",               data.iAxisY,
                "axis_z",               data.iAxisZ );
                  
            return parameters;
            break;
            }
        case KSensrvChannelTypeIdAccelerometerDoubleTappingData:
            {
            TSensrvTappingData data;
            TPckg<TSensrvTappingData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "direction",            data.iDirection ); //TODO: convert to textual?

            return parameters;
            break;
            }
        case KSensrvChannelTypeIdOrientationData:
            {
            TSensrvOrientationData data;
            TPckg<TSensrvOrientationData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "orientation",          data.iDeviceOrientation ); 

          return parameters;
            break;
            }
        case KSensrvChannelTypeIdRotationData:
            {
            TSensrvRotationData data;
            TPckg<TSensrvRotationData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i,s:i,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "axis_x",               data.iDeviceRotationAboutXAxis,
                "axis_y",               data.iDeviceRotationAboutYAxis,
                "axis_z",               data.iDeviceRotationAboutZAxis );

            return parameters;
            break;
            }
        case KSensrvChannelTypeIdMagnetometerXYZAxisData:
            {
            TSensrvMagnetometerAxisData data;
            TPckg<TSensrvMagnetometerAxisData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i,s:i,s:i,s:i,s:i,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "axis_x_raw",           data.iAxisXRaw,
                "axis_y_raw",           data.iAxisYRaw,
                "axis_z_raw",           data.iAxisZRaw,
                "axis_x_calib",         data.iAxisXCalibrated,
                "axis_y_calib",         data.iAxisYCalibrated,
                "axis_z_calib",         data.iAxisZCalibrated );

            return parameters;
            break;
            }
        case KSensrvChannelTypeIdMagneticNorthData:
            {
            TSensrvMagneticNorthData data;
            TPckg<TSensrvMagneticNorthData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "yaw",                  data.iAngleFromMagneticNorth );

            return parameters;
            break;
            }
            
        case KSensrvChannelTypeIdProximityMonitor:
            {
            TSensrvProximityData data;
            TPckg<TSensrvProximityData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "iProximityState",      data.iProximityState );
            return parameters;
            break;
            }
            
            /*
        case KSensrvChannelTypeIdProximityMonitor:
            {
            TInt datalen = aChannel.GetChannelInfo().iDataItemSize;
                HBufC8* buf = HBufC8::NewL( datalen);
                
                TPtr8 ptr( buf->Des());                     
            aChannel.GetData( ptr );
        
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:s#})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "raw_data",                 (const char*)buf->Ptr(), datalen);
            
            delete buf; 
                
                
            return parameters;
            break;
            }
            */
        case KSensrvChannelTypeIdAmbientLightData:
            {
            TSensrvAmbientLightData data;
            TPckg<TSensrvAmbientLightData> package( data );
            aChannel.GetData( package );
            PyObject* parameters = NULL;
            parameters = Py_BuildValue(
                "({s:N,s:N,s:N,s:i})",
                "channel_id",           PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelId ),
                "channel_type",         PyLong_FromUnsignedLong( aChannel.GetChannelInfo().iChannelType ),
                "timestamp",            epoch_to_datetime(data.iTimeStamp, false),
                "iAmbientLight",        data.iAmbientLight );
            return parameters;
            break;
            }
        case KSensrvChannelTypeIdUndefined:
        default:
            {
            return NULL;
            break;
            }
        }
    }

void CChannelDataListener::GetDataListenerInterfaceL( TUid aInterfaceUid, TAny*& aInterface )
{}

