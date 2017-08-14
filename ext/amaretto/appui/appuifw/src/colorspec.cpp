/* Copyright (c) 2005 Nokia Corporation
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

/*
 * Parse the given color specification.
 * Store result in *rgb.
 * 
 * Return: 1 if successful, 0 if error.
 */
static int
ColorSpec_AsRgb(PyObject *color_object, TRgb *rgb)
{
  int r,g,b;  
  if (PyTuple_Check(color_object)) {
    if (PyArg_ParseTuple(color_object, "iii", &r,&g,&b)) {
      *rgb=TRgb(r,g,b);
      return 1;
    }
  } else if (PyInt_Check(color_object)) {
    long x=PyInt_AS_LONG(color_object);
    // Unfortunately the Symbian TRgb color component ordering is
    // unintuitive (0x00bbggrr), so we'll just invert that here to 0x00rrggbb.
    r=(x&0xff0000)>>16;
    g=(x&0x00ff00)>>8;
    b=(x&0x0000ff);
    *rgb=TRgb(r,g,b);
    return 1;
  }
  PyErr_SetString(PyExc_ValueError, "invalid color specification; expected int or (r,g,b) tuple");
  return 0;
}


static int 
ColorSpec_Check(PyObject *color_object)
{
  if (PyInt_Check(color_object)) 
    return 1;
  else if (PyTuple_Check(color_object)) {
    if (PyTuple_Size(color_object)!=3) 
      return 0;
    if (PyInt_Check(PyTuple_GetItem(color_object,0)) &&
	PyInt_Check(PyTuple_GetItem(color_object,1)) &&
	PyInt_Check(PyTuple_GetItem(color_object,2))) 
      return 1;
  }
  return 0;
}

static PyObject *
ColorSpec_FromRgb(TRgb color)
{
  return Py_BuildValue("(iii)", color.Red(), color.Green(), color.Blue());
}
