/* Copyright (c) 2005 - 2008 Nokia Corporation
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

#ifndef __STATIC_ARRAY_C_H__
#define __STATIC_ARRAY_C_H__



/*! 
  @class TStaticArrayC
  
  @discussion This templated class provides a type, and size, safe method of
  using static arrays.
  */
template <class T>
class TStaticArrayC
    {
public:

/*!
  @function operator[]
  
  @discussion Return an element from the array.
  @param aIndex the index of the element to return
  @result a reference to the object
  */
    inline const T& operator[](TInt aIndex) const;

    /*!
      @enum TPanicCode
  
      @discussion Panic code
      @value EIndexOutOfBounds index is out of bounds
      */
    enum TPanicCode
        { 
        EIndexOutOfBounds = 1 
        };

/*!
  @function Panic
  
  @discussion Generate a panic.
  @param aPanicCode the reason code for the panic
  */
    inline void Panic(TPanicCode aPanicCode) const;

public:
    /*! @var iArray the arrat of elements */
    const T* iArray;

    /*! @var iCount the number of elements */
    TInt iCount;

    };

/*!
  @function CONSTRUCT_STATIC_ARRAY_C
  
  @discussion Initalise a global constant of type TStaticArrayC<>.
  @param aValue the underlying const array of T
  */
#define CONSTRUCT_STATIC_ARRAY_C(aValue) \
        {   \
        aValue,    \
        sizeof(aValue) / sizeof(*aValue)  \
        }  \


#include "StaticArrayC.inl"

#endif //   __STATIC_ARRAY_C_H__
