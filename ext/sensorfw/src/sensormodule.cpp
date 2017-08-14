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

#include <python.h>
#include <symbian_python_ext_util.h>
#include <e32cmn.h>
#include <sensrvchannel.h>
#include <sensrvchannelfinder.h>
#include "channeldatalistener.h"

#define CHANNEL_TYPE ( ( PyTypeObject* )SPyGetGlobalString( "channel_type" ) )

struct channel_object
	{
	PyObject_VAR_HEAD
	CSensrvChannel* channel;
	CChannelDataListener* data_listener;
	};


// -----------------------------------------------------------------------------
// GetChannelInfoListL
// -----------------------------------------------------------------------------
//
extern "C" PyObject* GetChannelInfoListL()
	{
	TInt error = KErrNone;

	TSensrvChannelInfo sySearchInfo;
	//TODO Consider enabling filtering when retrieving list of channels
	//     (now returns all available channels)

	// Create a Symbian list for found channels.
	RSensrvChannelInfoList syChannelInfoList;
	CleanupClosePushL( syChannelInfoList );

	// Find sensor channels via Symbian API, and add them to the list.
	CSensrvChannelFinder* syChannelFinder = 
		CSensrvChannelFinder::NewLC();
	syChannelFinder->FindChannelsL( syChannelInfoList, sySearchInfo );
	CleanupStack::PopAndDestroy( syChannelFinder );
	
	// Create a Python list (dictionary object) for found channels.
	PyObject* pyChannelInfoList = PyDict_New();
	if ( pyChannelInfoList == NULL )
		{
		CleanupStack::PopAndDestroy( &syChannelInfoList );
		return PyErr_NoMemory();
		}

	// Copy found channels from Symbian list to Python list.
	for ( TInt i = 0; i < syChannelInfoList.Count(); i++ )
		{
		// Create channel info object.
		TSensrvChannelInfo syChannelInfo = syChannelInfoList[ i ];
		PyObject* pyChannelInfo = Py_BuildValue(
			"{s:N,s:i,s:i,s:N,s:s#,s:s#,s:i,s:N}",
			"channel_id",   				PyLong_FromUnsignedLong( syChannelInfo.iChannelId ),
			"context_type", 				syChannelInfo.iContextType,
			"quantity", 						syChannelInfo.iQuantity,
			"channel_type", 				PyLong_FromUnsignedLong(syChannelInfo.iChannelType),
			"location", 						syChannelInfo.iLocation.Ptr(),
										  				syChannelInfo.iLocation.Length(),
			"vendor_id", 						syChannelInfo.iVendorId.Ptr(),
										  				syChannelInfo.iVendorId.Length(),
			"data_item_size", 			syChannelInfo.iDataItemSize,
			"channel_data_type_id", PyLong_FromUnsignedLong( 
															syChannelInfo.iChannelDataTypeId )
			);
		if ( pyChannelInfo == NULL )
			{
			Py_DECREF( pyChannelInfoList );
			CleanupStack::PopAndDestroy( &syChannelInfoList );
			return NULL;
			}

		// Create dictionary key.
		PyObject* pyKey = Py_BuildValue( 
				"N", PyLong_FromUnsignedLong( syChannelInfo.iChannelId ) );
		if ( pyKey == NULL )
			{
			Py_DECREF( pyChannelInfo );
			Py_DECREF( pyChannelInfoList );
			CleanupStack::PopAndDestroy( &syChannelInfoList );
			return NULL;
			}
		
		// Add to dictionary.
		error = PyDict_SetItem( pyChannelInfoList, pyKey, pyChannelInfo );
		
		// Cleanup.
		Py_DECREF( pyKey );
		Py_DECREF( pyChannelInfo );
		
		// Check errors.
		if ( error )
			{
			Py_DECREF( pyChannelInfoList );
			CleanupStack::PopAndDestroy( &syChannelInfoList );
			return NULL;
			}
		
		} // for
	
	// Cleanup.
	CleanupStack::PopAndDestroy( &syChannelInfoList );

	return pyChannelInfoList;
	}

// -----------------------------------------------------------------------------
// sensor_channels
// -----------------------------------------------------------------------------
//
extern "C" PyObject* sensor_channels( PyObject* /*self*/, PyObject* /*args*/ )
	{
	TInt error = KErrNone;
	PyObject* channels = NULL;
	TRAP( error, channels = GetChannelInfoListL() );
	if ( error != KErrNone )
		{
		return SPyErr_SetFromSymbianOSErr( error );
		}
	return channels;
	}

// -----------------------------------------------------------------------------
// GetChannelInfoL
// -----------------------------------------------------------------------------
//
TSensrvChannelInfo GetChannelInfoL( TUint32 aChannelId )
	{
	// Create empty search filter (filtering with channel ID does not work).
	TSensrvChannelInfo sySearchInfo;
	
	// Create a Symbian list for found channels.
	RSensrvChannelInfoList syChannelInfoList;
	CleanupClosePushL( syChannelInfoList );

	// Find sensor channels via Symbian API, and add them to the list.
	CSensrvChannelFinder* syChannelFinder = 
		CSensrvChannelFinder::NewLC();
	syChannelFinder->FindChannelsL( syChannelInfoList, sySearchInfo );
	CleanupStack::PopAndDestroy( syChannelFinder );

	// Select the one that matches the channel ID we are looking for.
	TSensrvChannelInfo syChannelInfo;
	syChannelInfo.iChannelId = 0;
	for ( TInt i = 0; i < syChannelInfoList.Count(); i++ )
		{
		TSensrvChannelInfo tempInfo = syChannelInfoList[ i ];
		if ( ( TUint32 )tempInfo.iChannelId == aChannelId )
			{ // This is the one we are looking for.
			syChannelInfo = tempInfo;
			break;
			}
		}
	
	// Cleanup.
	CleanupStack::PopAndDestroy( &syChannelInfoList );

	return syChannelInfo;
	}

// -----------------------------------------------------------------------------
// CreateChannelObjectL
// -----------------------------------------------------------------------------
//
extern "C" PyObject* CreateChannelObjectL( TUint32 aChannelId )
	{
	channel_object* pyChannel = NULL;

	// Get channel info object for given channel id.
	TSensrvChannelInfo syChannelInfo;
	syChannelInfo = GetChannelInfoL( aChannelId );
	if ( syChannelInfo.iChannelId == 0 )
		{
		return SPyErr_SetFromSymbianOSErr( KErrNotFound );
		}
	
	// Get Sensor API channel for given channel info object.
	CSensrvChannel* syChannel = CSensrvChannel::NewLC( syChannelInfo );
	
	// Automatically open channel, as it is useless while not open.
	// TODO Consider letting the user to explicitly open/close channels 
	syChannel->OpenChannelL();

	// Construct Python channel object.
	pyChannel = PyObject_New( channel_object, CHANNEL_TYPE );
	if ( pyChannel == NULL )
		{
		syChannel->CloseChannel();
		CleanupStack::PopAndDestroy(); // syChannel
		return PyErr_NoMemory();
		}
	pyChannel->channel = syChannel;
	CleanupStack::Pop(); // syChannel
	pyChannel->data_listener = NULL;

	return ( PyObject* )pyChannel;
	}

// -----------------------------------------------------------------------------
// sensor_channel
// -----------------------------------------------------------------------------
//
extern "C" PyObject* sensor_channel( PyObject* /*self*/, PyObject* args )
	{
	TInt error = KErrNone;
	TUint32 channel_id = 0;
	TInt64 tmp;
	PyObject* channel_object = NULL;

	if ( !PyArg_ParseTuple( args, "L", &tmp ) )
		{
		return NULL;
		}
	channel_id = ( TUint32 )tmp;

	TRAP( error, channel_object = CreateChannelObjectL( channel_id ) );
	if ( error != KErrNone )
		{
		return SPyErr_SetFromSymbianOSErr( error );
		}
	
	return channel_object;
	}

// -----------------------------------------------------------------------------
// channel_start_data_listening
// -----------------------------------------------------------------------------
//
extern "C" PyObject* channel_start_data_listening( channel_object* self, 
		PyObject *args )
	{
	TInt error = KErrNone;
  	PyObject* data_callback = NULL;
  	PyObject* error_callback = NULL;
    CChannelDataListener* data_listener = NULL;

  	// Check that we are not already listening.
	if ( self->data_listener != NULL )
		{
		return SPyErr_SetFromSymbianOSErr( KErrAlreadyExists );
		}

    // Get callback functions.
  	if ( !PyArg_ParseTuple( args, "OO", &data_callback, &error_callback ) )
  		{
		return NULL;
		}

  	// Create data listener object with correct callbacks.
  	data_listener = CChannelDataListener::NewLC( data_callback, error_callback);
  	if ( data_listener == NULL )
  		{
  		return NULL;
  		}
  	
  	// For some reason, this (better) method did not work:
  	//TRAP( error, data_listener = CChannelDataListener::NewLC( 
    //		data_callback, error_callback ) );
  	//if ( error != KErrNone )
  	// 	{
  	// 	return SPyErr_SetFromSymbianOSErr( error );
  	// 	}

    // Start listening, and add data listener to channel object.
	TRAP( error, self->channel->StartDataListeningL( data_listener, 1, 50, 0 ) );
    if ( error != KErrNone ) 
    	{
    	CleanupStack::PopAndDestroy(); // data_listener
    	return SPyErr_SetFromSymbianOSErr( error );
    	}
  	self->data_listener = data_listener;
    CleanupStack::Pop(); // data_listener

	return Py_BuildValue( "i", 1 );
	}

// -----------------------------------------------------------------------------
// channel_stop_data_listening
// -----------------------------------------------------------------------------
//
extern "C" PyObject* channel_stop_data_listening( channel_object* self, 
		PyObject /**args*/ )
	{
	// Check that we really are listening.
	if ( self->data_listener == NULL )
		{
		return Py_BuildValue( "i", 0 );
		}
	
	// Stop data listening.
	self->channel->StopDataListening();

	// Delete listener object.
	delete self->data_listener;
	self->data_listener = NULL;
	
	return Py_BuildValue( "i", 1 );
	}

// -----------------------------------------------------------------------------
// GetAllPropertiesL
// -----------------------------------------------------------------------------
//
extern "C" PyObject* GetAllPropertiesL( CSensrvChannel* aChannel )
	{
	TInt error = KErrNone;

	// Create a Symbian list for found properties.
	RSensrvPropertyList syPropertyList;
	CleanupClosePushL( syPropertyList );
	
	// Get channel's properties (all of them).
	aChannel->GetAllPropertiesL( syPropertyList );

	// Create a Python list (dictionary object) for found properties.
	PyObject* pyPropertyList = PyDict_New();
	if ( pyPropertyList == NULL )
		{
		CleanupStack::PopAndDestroy( &syPropertyList );
		return PyErr_NoMemory();
		}
	
	// Copy properties from Symbian list to Python list.
	for ( TInt i = 0; i < syPropertyList.Count(); i++ )
		{
		// Create property object.
		TSensrvProperty syProperty = ( TSensrvProperty )syPropertyList[ i ];
		PyObject* pyProperty = NULL;
		
		if ( syProperty.PropertyType() == ESensrvUninitializedProperty )
			{
			// Don't handle uninitialized properties.
			}
		else if ( syProperty.PropertyType() == ESensrvIntProperty )
			{
			TInt value, minValue, maxValue;
			syProperty.GetValue( value );
			syProperty.GetMinValue( minValue );
			syProperty.GetMaxValue( maxValue );

			pyProperty = Py_BuildValue(	
				"{s:N,s:i,s:i,s:i,s:i,s:i,s:i,s:b}",
				"property_id", 					PyLong_FromUnsignedLong(
																syProperty.GetPropertyId() ),
				"property_type",				syProperty.PropertyType(),
				"property_item_index",	syProperty.PropertyItemIndex(),
				"property_array_index",	syProperty.GetArrayIndex(),
				"value",								value,
				"min_value",						minValue,
				"max_value",						maxValue,
				"read_only",						syProperty.ReadOnly()
				// TODO Consider including security info for properties
				);
			}
		else if ( syProperty.PropertyType() == ESensrvRealProperty )
			{
			TReal value, minValue, maxValue;
			syProperty.GetValue( value );
			syProperty.GetMinValue( minValue );
			syProperty.GetMaxValue( maxValue );

			pyProperty = Py_BuildValue(	
				"{s:N,s:i,s:i,s:i,s:d,s:d,s:d,s:b}",
				"property_id", 					PyLong_FromUnsignedLong(
																syProperty.GetPropertyId() ),
				"property_type",				syProperty.PropertyType(),
				"property_item_index",	syProperty.PropertyItemIndex(),
				"property_array_index",	syProperty.GetArrayIndex(),
				"value",								value,
				"min_value",						minValue,
				"max_value",						maxValue,
				"read_only",						syProperty.ReadOnly()
				// TODO Consider including security info for properties
				);
			}
		else if ( syProperty.PropertyType() == ESensrvBufferProperty )
			{
			TBuf8<32> value;
			syProperty.GetValue( value );
			
			pyProperty = Py_BuildValue(	
				"{s:N,s:i,s:i,s:i,s:s#,s:i,s:i,s:b}",
				"property_id", 					PyLong_FromUnsignedLong(
																syProperty.GetPropertyId() ),
				"property_type",				syProperty.PropertyType(),
				"property_item_index",	syProperty.PropertyItemIndex(),
				"property_array_index",	syProperty.GetArrayIndex(),
				"value", 								value.Ptr(), value.Length(),
				"min_value",						0,
				"max_value",						value.Length(), // String length
				"read_only",						syProperty.ReadOnly()
				// TODO Consider including security info for properties
				);
			}
		if ( pyProperty == NULL )
			{
			Py_DECREF( pyPropertyList );
			CleanupStack::PopAndDestroy( &syPropertyList );
			return NULL;
			}

		// fixme swap arrayindex and itemindex 
		// Create dictionary key: tuple containing (property_id, array_index, item_index).
		PyObject* pyKey = Py_BuildValue(
			"N:i:i",
			PyLong_FromUnsignedLong( syProperty.GetPropertyId() ),
			syProperty.GetArrayIndex(),
			syProperty.PropertyItemIndex()
			);
		if ( pyKey == NULL )
			{
			Py_DECREF( pyProperty );
			Py_DECREF( pyPropertyList );
			CleanupStack::PopAndDestroy( &syPropertyList );
			return NULL;
			}
		
		// Add to dictionary.
		error = PyDict_SetItem( pyPropertyList, pyKey, pyProperty );
		
		// Cleanup.
		Py_DECREF( pyKey );
		Py_DECREF( pyProperty );
		
		// Check errors.
		if ( error )
			{
			Py_DECREF( pyPropertyList );
			CleanupStack::PopAndDestroy( &syPropertyList );
			return NULL;
			}
		
		} // for
	
	// Cleanup.
	CleanupStack::PopAndDestroy( &syPropertyList );

	return pyPropertyList;
	}

// -----------------------------------------------------------------------------
// channel_get_all_properties
// -----------------------------------------------------------------------------
//
extern "C" PyObject* channel_get_all_properties( channel_object* self, 
		PyObject /**args*/ )
	{
	TInt error = KErrNone;
	PyObject* properties = NULL;
	TRAP( error, properties = GetAllPropertiesL( self->channel ) );
	if ( error != KErrNone )
		{
		return SPyErr_SetFromSymbianOSErr( error );
		}
	return properties;
	}

// -----------------------------------------------------------------------------
// GetPropertyL
// -----------------------------------------------------------------------------
//
extern "C" TSensrvProperty GetPropertyL( CSensrvChannel* aChannel, 
		TUint32 aPropertyId, TInt aItemIndex, TInt aArrayIndex )
	{
	TSensrvProperty property;

	// Create a Symbian list for found properties.
	RSensrvPropertyList propertyList;
	CleanupClosePushL( propertyList );
	
	// Get channel's properties with given property ID (all of them).
	aChannel->GetAllPropertiesL( aPropertyId, propertyList );
	
	// Select the actual property based on item index, array index.
	for ( TInt i = 0; i < propertyList.Count(); i++ )
		{
		TSensrvProperty prop = ( TSensrvProperty )propertyList[ i ];
		if ( prop.PropertyItemIndex() == aItemIndex )
			{
			if ( prop.GetArrayIndex() == aArrayIndex )
				{
				property = prop;
				break;
				}
			}
		}

	// Cleanup.
	CleanupStack::PopAndDestroy( &propertyList );

	return property;
	}

// -----------------------------------------------------------------------------
// channel_set_property
// -----------------------------------------------------------------------------
//
extern "C" PyObject* channel_set_property( channel_object* self, 
		PyObject *args )
	{
	TInt error = KErrNone;
	TInt64 tmp;
	TUint32 property_id = 0;
	TInt item_index = 0;
	TInt array_index = 0;
	TReal value = 0.0;
	TSensrvProperty property;

	// Parse property ID, item index, array index.
	if ( !PyArg_ParseTuple( args, "Liid", &tmp, &item_index, &array_index, 
			&value ) )
		{
		return NULL;
		}
	property_id = ( TUint32 )tmp;

	// Get property object.
	TRAP( error, property = GetPropertyL( 
			self->channel, property_id, item_index, array_index ) );
	if ( error != KErrNone )
		{
		return SPyErr_SetFromSymbianOSErr( error );
		}
	if ( property.GetPropertyId() != property_id )
		{
		return SPyErr_SetFromSymbianOSErr( KErrNotFound );
		}

	// Check if property is read only.
	if ( property.ReadOnly() == 1 )
		{
		return SPyErr_SetFromSymbianOSErr( KErrAccessDenied );
		}
	
	// Check property type, parse value from params, and change it.
	// TODO different data types should be handled better...
	if ( property.PropertyType() == ESensrvIntProperty )
		{
		/*
		TInt64 dummy;
		TInt dummy2;
		TInt value = 0;
		if ( !PyArg_ParseTuple( args, "Liii", &dummy, &dummy2, &dummy2, value ))
			{
			return NULL;
			}
		*/
		property.SetValue( ( TInt )value );
		}
	else if ( property.PropertyType() == ESensrvRealProperty )
		{
		/*
		TInt64 dummy;
		TInt dummy2;
		TReal value = 0.0;
		if ( !PyArg_ParseTuple( args, "Liid", &dummy, &dummy2, &dummy2, value ))
			{
			return NULL;
			}
		*/
		property.SetValue( value );
		}
	else if ( property.PropertyType() == ESensrvBufferProperty )
		{
		// TODO
		return SPyErr_SetFromSymbianOSErr( KErrNotSupported );
		}

	// Set new (changed) property.
	error = self->channel->SetProperty( property );
	if ( error != KErrNone )
		{
		return SPyErr_SetFromSymbianOSErr( error );
		}
	else
		{
		return Py_BuildValue( "i", 1 ); // Success
		}
	}

// -----------------------------------------------------------------------------
// channel_dealloc
// -----------------------------------------------------------------------------
//
extern "C" void channel_dealloc( channel_object* self )
	{
    if ( self->data_listener != NULL )
    	{
    	self->channel->StopDataListening();
	    delete self->data_listener;
    	self->data_listener = NULL;
    	}
    self->channel->CloseChannel();
    delete self->channel;
    self->channel = NULL;
    PyObject_Del( self );
    }

// -----------------------------------------------------------------------------
// "Python API"
// -----------------------------------------------------------------------------
//
extern "C"
	{

	// "Channel" type's instance methods.
	static const PyMethodDef channel_methods[] = 
		{
		{ "start_data_listening", ( PyCFunction )channel_start_data_listening, 
			METH_VARARGS, NULL },
		{ "stop_data_listening", ( PyCFunction )channel_stop_data_listening, 
			METH_NOARGS, NULL },
		{ "get_all_properties", ( PyCFunction )channel_get_all_properties, 
			METH_NOARGS, NULL },
		{ "set_property", ( PyCFunction )channel_set_property, 
			METH_VARARGS, NULL },
		{ NULL, NULL } /* sentinel */
		};

	// Returns "Channel" type's instance methods.
    static PyObject* channel_getattr( channel_object* op, char *name )
    	{
    	return Py_FindMethod( ( PyMethodDef* )channel_methods, 
    			( PyObject* )op, name );
    	}

    // "Channel" type initialization.
    static const PyTypeObject c_channel_type = 
    	{
	    PyObject_HEAD_INIT( NULL )
	    0,                                        /*ob_size*/
	    "_sensorext.Channel",                     /*tp_name*/
	    sizeof(channel_object),                   /*tp_basicsize*/
	    0,                                        /*tp_itemsize*/
	    /* methods */
	    (destructor)channel_dealloc,              /*tp_dealloc*/
	    0,                                        /*tp_print*/
	    (getattrfunc)channel_getattr,             /*tp_getattr*/
	    0,                                        /*tp_setattr*/
	    0,                                        /*tp_compare*/
	    0,                                        /*tp_repr*/
	    0,                                        /*tp_as_number*/
	    0,                                        /*tp_as_sequence*/
	    0,                                        /*tp_as_mapping*/
	    0,                                        /*tp_hash */
	    0,                                        /*tp_call*/
	    0,                                        /*tp_str*/
	    0,                                        /*tp_getattro*/
	    0,                                        /*tp_setattro*/
	    0,                                        /*tp_as_buffer*/
	    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
	    "Represents an open Sensor API channel",  /*tp_doc */
	    0,                                        /*tp_traverse */
	    0,                                        /*tp_clear */
	    0,                                        /*tp_richcompare */
	    0,                                        /*tp_weaklistoffset */
	    0,                                        /*tp_iter */
	    };

    // Module's static methods.
    static const PyMethodDef sensorext_methods[] = 
    	{
    	{ "channels", ( PyCFunction )sensor_channels, METH_NOARGS, \
    	"Returns all available sensor channels as a dictionary of (channel_id, \
    	channel_info) objects"},
    	{ "open_channel",  ( PyCFunction )sensor_channel,  METH_VARARGS, \
    	"Return a handle to an open sensor channel, specified by given channel \
    	ID" },
    	{ NULL, NULL } /* sentinel */
    	};

    // Initialization; this is the first function called from the module.
  	DL_EXPORT( void ) init_sensor( void )
	  	{
	  	// Create and init "Channel" type, set pointer into a global string.
	    PyTypeObject* channel_type = NULL;
	    channel_type = PyObject_New( PyTypeObject, &PyType_Type );
	    if ( !channel_type ) return;
	    *channel_type = c_channel_type;
	    channel_type->ob_type = &PyType_Type;
	    SPyAddGlobalString( "channel_type", ( PyObject* )channel_type );
	    
	    // Initialize SensorExt module and publish its static methods to Python.
	    PyObject* m = NULL;
	    m = Py_InitModule( "_sensorfw", ( PyMethodDef* )sensorext_methods );
	    if ( !m ) return;
	    }
	
	} // extern "C"

