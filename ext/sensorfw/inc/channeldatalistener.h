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

#ifndef CHANNELDATALISTENER_H
#define CHANNELDATALISTENER_H

#include <e32base.h>
#include <sensrvdatalistener.h>
#include <python.h>
#include "symbian_python_ext_util.h"


/**
 * Class CChannelDataListener
 * 
 * Implements Sensor API channel data listener object.
 * 
 * @lib -
 * @since -
 */
class CChannelDataListener : public CBase, public MSensrvDataListener
    {
    public:

        static CChannelDataListener* NewL( PyObject* aDataReceivedCallback, 
    			PyObject* aDataErrorCallback );
        static CChannelDataListener* NewLC( PyObject* aDataReceivedCallback, 
    			PyObject* aDataErrorCallback );

        ~CChannelDataListener();

    public: // Functions from MSensrvDataListener

        // @see MSensrvDataListener
    	void DataReceived( CSensrvChannel& aChannel, TInt aCount, 
			TInt aDataLost );
    	
    	// @see MSensrvDataListener
    	void DataError( CSensrvChannel& aChannel, TSensrvErrorSeverity aError );

        void GetDataListenerInterfaceL( TUid aInterfaceUid, TAny*& aInterface );    
        
    private:

    	// Convert new data from Symbian to Python format based on channel type.
    	PyObject* GetAndPackageData( CSensrvChannel& aChannel );
    	
    private:

    	CChannelDataListener( PyObject* aDataReceivedCallback, 
    			PyObject* aDataErrorCallback );

        void ConstructL();
        
    private: // Data

        /** Python callback function for received data */
    	PyObject* iDataReceivedCallback;
    	
    	/** Python callback function for data errors */
    	PyObject* iDataErrorCallback;

    };
  
#endif // CHANNELDATALISTENER_H
