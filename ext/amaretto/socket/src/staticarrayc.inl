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

template <class T>
inline const T& TStaticArrayC<T>::operator[](TInt aIndex) const
    {
    if ((aIndex >= iCount) || (aIndex < 0))
        {
        Panic(EIndexOutOfBounds);
        }

    return  iArray[aIndex];
    }

template <class T>
inline void TStaticArrayC<T>::Panic(TPanicCode aPanicCode) const
    {
    User::Panic(_L("StaticArray"), aPanicCode);
    }
