/**
 * ====================================================================
 * fontspec.cpp
 *
 * Copyright (c) 2007 Nokia Corporation
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

#include <aknutils.h>

/* Font flags. */
#define FONT_BOLD 1
#define FONT_ITALIC 2
#define FONT_SUBSCRIPT 4
#define FONT_SUPERSCRIPT 8
#define FONT_ANTIALIAS 16
#define FONT_NO_ANTIALIAS 32
#define FONT_FLAGS_MASK 0x3f

/*
 from graphics import * font
 font.ANTIALIAS
 font.catalog
 class font: 
   ANTIALIAS=1
   BOLD=2
 s=img.measure_text(u'foo')
 img.text((10,10),u'foo'
 f=Font(None,15,FONT_ANTIALIAS)
 img.text((20,20),u'Foo',font=(None,15,font.ANTIALIAS)
*/

static int system_font_from_label(const char * aLabel, const CFont *&aFont)
{
    CEikonEnv *env=CEikonEnv::Static();
    if (!env) {
        PyErr_SetString(
                PyExc_ValueError,
                "Sorry, but system font labels ('normal', 'title' ...) are not available in background processes.");
        return -1;
    }

   /* XXX: A better font alias system is needed.  */
#if S60_VERSION >= 28
   struct { char *name; TAknLogicalFontId id; } font_table[] = {
     // Don't want to commit to supporting these for all eternity. For
     // now, let's mostly stick to the old labels.
   //{"primary", EAknLogicalFontPrimaryFont},
    //{"secondary", EAknLogicalFontSecondaryFont},
    //{"title", EAknLogicalFontTitleFont},
    //{"primarysmall", EAknLogicalFontPrimarySmallFont},
    {"digital", EAknLogicalFontDigitalFont}, // marginally useful.
    {NULL,(TAknLogicalFontId)0}
  };
  for (int i=0; font_table[i].name; i++)
    if (!strcmp(aLabel, font_table[i].name)) {
      aFont=AknLayoutUtils::FontFromId(font_table[i].id);
      return 0;
    }
#endif
  /* Of these, "normal", "title" and "dense" are probably the only ones 
   * that sort of make sense. 
   */
    if (!strcmp(aLabel,"normal"))
        aFont = env->NormalFont();
    else if (!strcmp(aLabel,"title"))
        aFont = env->TitleFont();
    else if (!strcmp(aLabel,"dense"))
        aFont = env->DenseFont();
    else if (!strcmp(aLabel,"annotation"))
        aFont = env->AnnotationFont();
    else if (!strcmp(aLabel,"legend"))
        aFont = env->LegendFont();
    else if (!strcmp(aLabel,"symbol"))
        aFont = env->SymbolFont();
    else {
        PyErr_SetString(PyExc_ValueError, "Invalid font label");
        return -1;
    }
    return 0;
}

/*
 * Returns the TFontSpec (in twips) corresponding to the given Python form font specification object.
 * The Python font specification may be:
 *  - string: label of a font alias. 'normal', 'annotation' etc.
 *  - unicode: exact name of font to use.
 *  - tuple: font specification in the form (name, size, flags) or (name, size).
 * 
 * Return: 0 on success, -1 on error (with Python exception set)
 */
static int TFontSpec_from_python_fontspec(
    PyObject *aArg,
    TFontSpec &aFontSpec,
    MGraphicsDeviceMap &aDeviceMap)
{
    PyObject *fontname_object=Py_None, *fontsize_object=Py_None,
            *flags_object=Py_None;

    if (PyTuple_Check(aArg)) {
        switch (PyTuple_GET_SIZE(aArg)) {
        case 3:
            flags_object=PyTuple_GET_ITEM(aArg, 2);
        case 2: // Fall through
            fontsize_object=PyTuple_GET_ITEM(aArg, 1);
        case 1: // Fall through
            fontname_object=PyTuple_GET_ITEM(aArg, 0);
            break;
        default:
            PyErr_SetString(PyExc_ValueError,
                    "Invalid font specification: wrong number of elements in tuple.");
            return -1;
        }
    } else if (PyString_Check(aArg)||PyUnicode_Check(aArg)||Py_None == aArg) {
        fontname_object=aArg;
    } else {
        PyErr_SetString(PyExc_TypeError,
                "Invalid font specification: expected string, unicode or tuple");
        return -1;
    }

    /* At this point {fontname,fontsize,flags}_object are initialized with parameters,
     * no matter if they arrived as tuple or not. 
     * Now we start filling in fontspec according to these three parameters.
     */
    /* Hardcoding the defaults is a bit ugly, but the Symbian font matching
     * routines don't give us much choice.
     */
#if S60_VERSION < 28
    /* Seems like on older platforms the font name must match exactly
     * or all other parameters are ignored. On the 6600 at least this
     * would give us an ugly italic font. Querying for the system
     * fonts in CEikonEnv would tie us to only using this in a
     * foreground process, and we don't want to give up the ability
     * to run this code in a background process. Let's just use a
     * reasonable default. */
    TFontSpec fontspec(_L("LatinPlain12"),0);
#elif defined(__WINS__) && S60_VERSION == 28
    // Special case for the 2.8 emulator since the "Nokia Sans S60" font
    // doesn't exist there.
    TFontSpec fontspec(_L("Series 60 Sans"), 120);
#else
    /* Default font for S60 2.8 and above: "Nokia Sans S60" at 6 points (120 twips).
     * This exists at least on N90 and E70. 
     */
    TFontSpec fontspec(_L("Nokia Sans S60"), 120);
    /* In case the name doesn't match we set some preferences and hope that the
     * font matcher finds us something reasonable. 
     */
    fontspec.iTypeface.SetIsProportional(ETrue);
    fontspec.iTypeface.SetIsSymbol(EFalse);
    fontspec.iTypeface.SetIsSerif(EFalse);
    fontspec.iFontStyle.SetPosture(EPostureUpright);
    // Default to no antialias for efficiency.
    fontspec.iFontStyle.SetBitmapType(EMonochromeGlyphBitmap);
#endif
    /* Parse fontname_object */
    if (PyUnicode_Check(fontname_object)) {
        TPtrC fontname_des((TUint16*)PyUnicode_AS_DATA(fontname_object), PyUnicode_GetSize(fontname_object));
        fontspec.iTypeface.iName = fontname_des;
    } else if (PyString_Check(fontname_object)) {
        const CFont *font=NULL;
        if (-1 == system_font_from_label(PyString_AsString(fontname_object),
                font))
            return -1;
        fontspec = font->FontSpecInTwips();
    } else if (fontname_object == Py_None) {
        /* None is legal and means default font. */
    } else {
        PyErr_SetString(PyExc_ValueError,
                "Invalid font specification: expected unicode or None as font name");
        return -1;
    }

    /* Parse fontsize_object
     * Allow both ints and floats for convenience.
     */
    if (PyInt_Check(fontsize_object)) {
        fontspec.iHeight = PyInt_AsLong(fontsize_object);
        fontspec.iHeight = aDeviceMap.VerticalPixelsToTwips(fontspec.iHeight);
    } else if (PyFloat_Check(fontsize_object)) {
        fontspec.iHeight = (int)PyFloat_AsDouble(fontsize_object);
        fontspec.iHeight = aDeviceMap.VerticalPixelsToTwips(fontspec.iHeight);
    } else if (fontsize_object == Py_None) {
        /* None is legal and means default size. */
    } else {
        PyErr_SetString(PyExc_TypeError,
                "Invalid font size: expected int, float or None");
        return -1;
    }

    /* Parse flags. */
    if (flags_object != Py_None) {
      if (!PyInt_Check(flags_object)) {
        PyErr_SetString(PyExc_TypeError, "Invalid font flags: expected int or None");
        return -1;
      }
      int flags=PyInt_AS_LONG(flags_object);
      if (flags & FONT_BOLD) 
	fontspec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
      if (flags & FONT_ITALIC)
	fontspec.iFontStyle.SetPosture(EPostureItalic);
      if (flags & FONT_SUBSCRIPT)
	fontspec.iFontStyle.SetPrintPosition(EPrintPosSubscript);
      if (flags & FONT_SUPERSCRIPT)
	fontspec.iFontStyle.SetPrintPosition(EPrintPosSuperscript);
      if (flags & FONT_ANTIALIAS)
	fontspec.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
      if (flags & FONT_NO_ANTIALIAS)
	fontspec.iFontStyle.SetBitmapType(EMonochromeGlyphBitmap);
      if (flags & ~FONT_FLAGS_MASK) { // Any illegal bits set?
        PyErr_SetString(PyExc_ValueError, "Invalid font flags value");
        return -1;
      }
    }

    aFontSpec=fontspec;
    return 0;
}

/*
 * Returns a CFont corresponding to the given Python form font specification object.
 * See TFontSpec_from_python_fontspec for the Python form font specification format.
 *
 * Return: 0 on success, -1 on error (with Python exception set)
 */
static int CFont_from_python_fontspec(
    PyObject *aArg,
    CFont *&aFont,
    MGraphicsDeviceMap &aDeviceMap)
{
    TFontSpec fontspec;
    if (-1 == TFontSpec_from_python_fontspec(aArg, fontspec, aDeviceMap))
        return -1;
    TInt error=aDeviceMap.GetNearestFontInTwips(aFont, fontspec);
    if (error != KErrNone) {
        SPyErr_SetFromSymbianOSErr(error);
        return -1;
    }
    return 0;
}

static PyObject * python_fontspec_from_TFontSpec(
    const TFontSpec &aFontSpec,
    MGraphicsDeviceMap &aDeviceMap)
{
    int pixelsize=0;
    int flags=0;
    
    pixelsize = aDeviceMap.VerticalTwipsToPixels(aFontSpec.iHeight);
    
    if (aFontSpec.iFontStyle.Posture()==EPostureItalic) flags |= FONT_ITALIC;
    if (aFontSpec.iFontStyle.StrokeWeight()==EStrokeWeightBold) flags |= FONT_BOLD;
    if (aFontSpec.iFontStyle.PrintPosition()==EPrintPosSubscript) flags |= FONT_SUBSCRIPT;
    if (aFontSpec.iFontStyle.PrintPosition()==EPrintPosSuperscript) flags |= FONT_SUPERSCRIPT;
    if (aFontSpec.iFontStyle.BitmapType()==EAntiAliasedGlyphBitmap) flags |= FONT_ANTIALIAS;
    if (aFontSpec.iFontStyle.BitmapType()==EMonochromeGlyphBitmap) flags |= FONT_NO_ANTIALIAS;

    return Py_BuildValue("u#ii", aFontSpec.iTypeface.iName.Ptr(),
            aFontSpec.iTypeface.iName.Length(), pixelsize, flags);
}

/*
 static const char *const kwlist[] = {"name", "height", 
 "proportional", "serif", "symbol", // set via TTypeFace
 "italic", "bold", "position", "antialias", // set via TFontStyle
 NULL};
 TUint16 *aNameBuf=NULL;
 TInt aNameLength=0;
 int aHeight=16, aProportional=0, aSerif=0, aSymbol=0,
 aItalic=0, aBold=0, aPosition=0, aAntialias=0;
 if (!PyArg_ParseTupleAndKeywords(args, keywds, "|u#iiiiiiii", (char**)kwlist,
 &aNameBuf, &aNameLength,
 &aHeight,
 &aProportional, &aSerif, &aSymbol, 
 &aItalic, &aBold, &aPosition, &aAntialias))
 return NULL;
 
 if (aNameLength > KMaxTypefaceNameLength) {
 PyErr_SetString(PyExc_ValueError, "Font name too long");
 return NULL;
 }
 
 TPtr typefacename(aNameBuf, aNameLength);
 
 TTypeface typeface;
 typeface.SetIsProportional(aProportional);
 typeface.SetIsSerif(aSerif);
 typeface.SetIsSymbol(aSymbol);
 typeface.iName=typefacename;
 
 TFontStyle fontstyle;
 fontstyle.SetPosture(aItalic ? EPostureItalic : EPostureUpright);
 fontstyle.SetStrokeWeight(aBold ? EStrokeWeightBold : EStrokeWeightNormal);
 fontstyle.SetPrintPosition(aPosition == 1 ? EPrintPosSuperscript : 
 (aPosition == -1 ? EPrintPosSubscript: EPrintPosNormal));
 fontstyle.SetBitmapType(aAntialias ? EAntiAliasedGlyphBitmap : EMonochromeGlyphBitmap);
 
 TFontSpec fontspec;
 fontspec.iTypeface = typeface;
 fontspec.iHeight = aHeight;
 fontspec.iFontStyle = fontstyle;
 
 CFont *font=NULL;
 TInt error=device->GetNearestFontInTwips(font, fontspec);
 if (error) 
 return SPyErr_SetFromSymbianOSErr(error);
 
 PyObject *font_pyo = Font_FromCFont(font, _font_release, device);
 if (!font_pyo) {
 return NULL;
 }
 
 return font_pyo;
 }
 */

