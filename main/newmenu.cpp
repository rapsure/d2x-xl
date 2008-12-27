/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#endif
#ifdef __macosx__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "error.h"
#include "inferno.h"
#include "key.h"
#include "text.h"
#include "findfile.h"
#include "menu.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gamepal.h"
#include "network.h"
#include "network_lib.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "endlevel.h"
#include "screens.h"
#include "config.h"
#include "newdemo.h"
#include "kconfig.h"
#include "strutil.h"
#include "ogl_lib.h"
#include "ogl_bitmap.h"
#include "render.h"
#include "input.h"
#include "netmenu.h"
#include "menubackground.h"

#if defined (TACTILE)
 #include "tactile.h"
#endif

#define LHX(x)      (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)

char NORMAL_RADIO_BOX [2] = {(char) 127, 0};
char CHECKED_RADIO_BOX [2] = {(char) 128, 0};
char NORMAL_CHECK_BOX [2] = {(char) 129, 0};
char CHECKED_CHECK_BOX [2] = {(char) 130, 0};
char CURSOR_STRING [2] = {'_', 0};
char SLIDER_LEFT [2] = {(char) 131, 0};
char SLIDER_RIGHT [2] = {(char) 132, 0};
char SLIDER_MIDDLE [2] = {(char) 133, 0};
char SLIDER_MARKER [2] = {(char) 134, 0};
char UP_ARROW_MARKER [2] = {(char) 135, 0};
char DOWN_ARROW_MARKER [2] = {(char) 136, 0};

typedef struct tMenuProps {
	int	scWidth,
			scHeight,
			x, 
			y, 
			xOffs, 
			yOffs,
			width,
			height,
			w, 
			h, 
			aw, 
			tw, 
			th, 
			ty,
			twidth, 
			right_offset, 
			nStringHeight,
			bTinyMode,
			nMenus, 
			nOthers, 
			nMaxNoScroll, 
			nMaxOnMenu,
			nMaxDisplayable,
			nScrollOffset,
			bIsScrollBox,
			nDisplayMode,
			bValid;
} tMenuProps;

int ExecMenu4 (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
					 int (*menuCallback) (int nItems, CMenuItem *itemP, int *lastKeyP, int nItem), 
					 int *nCurItemP, char *filename, int width, int height, int bTinyMode);

//------------------------------------------------------------------------------

#define MESSAGEBOX_TEXT_SIZE 10000		// How many characters in messagebox
#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

char bPauseableMenu = 0;
char bAlreadyShowingInfo = 0;

//------------------------------------------------------------------------------

inline void NMFreeTextBm (CMenuItem *itemP)
{
	int	i;

for (i = 0; i < 2; i++) {
	delete itemP->text_bm [i];
	itemP->text_bm [i] = NULL;
	}
}

//------------------------------------------------------------------------------

void NMFreeAllTextBms (CMenuItem *itemP, int nItems)
{
for (; nItems; nItems--, itemP++)
	NMFreeTextBm (itemP);
}

//------------------------------------------------------------------------------

short NMSetItemColor (CMenuItem *itemP, int bIsCurrent, int bTiny) 
{
if (bTiny) {
	if (!gameData.menu.bValid) {
		gameData.menu.warnColor = RED_RGBA;
		gameData.menu.keyColor = RGBA_PAL (57, 49, 20);
		gameData.menu.tabbedColor = WHITE_RGBA;
		gameData.menu.tinyColors [0][0] = RGBA_PAL (29, 29, 47);
		gameData.menu.tinyColors [0][1] = RED_RGBA;
		gameData.menu.tinyColors [1][0] = RGBA_PAL (57, 49, 20);
		gameData.menu.tinyColors [1][1] = ORANGE_RGBA;
		gameData.menu.bValid = 1;
		}
	if (gameData.menu.colorOverride)
		fontManager.SetColorRGBi (gameData.menu.colorOverride, 1, 0, 0);
	else if (itemP->text [0] == '\t')
		fontManager.SetColorRGBi (gameData.menu.tabbedColor, 1, 0, 0);
	else 
		fontManager.SetColorRGBi (gameData.menu.tinyColors [bIsCurrent][itemP->unavailable], 1, 0, 0);
	}
else {
	if (bIsCurrent)
		fontManager.SetCurrent (SELECTED_FONT);
	else
		fontManager.SetCurrent (NORMAL_FONT);
	}
return CCanvas::Current ()->FontColor (0).index;
}

//------------------------------------------------------------------------------

int nTabIndex = -1;
int nTabs [] = {15, 87, 124, 162, 228, 253};

void NMHotKeyString (CMenuItem *itemP, int bIsCurrent, int bTiny, int bCreateTextBms, int nDepth)
{
#if 1
	CBitmap	*bmP = itemP->text_bm [bIsCurrent];

if (!*itemP->text)
	return;
if (itemP->color)
	fontManager.SetColorRGBi (itemP->color, 1, 0, 0);
else
	NMSetItemColor (itemP, bIsCurrent, bTiny);
if (bCreateTextBms && gameOpts->menus.bFastMenus &&
	 (bmP || (bmP = CreateStringBitmap (itemP->text, MENU_KEY (itemP->key, -1), 
												   gameData.menu.keyColor,
												   nTabs, itemP->centered, itemP->w, 0)))) {
	bmP->Render (CCanvas::Current (), itemP->x, itemP->y, bmP->Width (), bmP->Height (), 0, 0, bmP->Width (), bmP->Height (), 1, 0);
	itemP->text_bm [bIsCurrent] = bmP;
	}
else 
#endif
	{
	int	w, h, aw, l, i, 
			x = itemP->x, 
			y = itemP->y;
	char	*t, *ps = itemP->text, s [256], ch = 0, ch2;

if (!nDepth)
	itemP->textSave = itemP->text;
if ((t = strchr (ps, '\n'))) {
	strncpy (s, ps, sizeof (s));
	itemP->text = s;
	fontManager.Current ()->StringSize (s, w, h, aw);
	do {
		if ((t = strchr (itemP->text, '\n')))
			*t = '\0';
		NMHotKeyString (itemP, 0, bTiny, 0, nDepth + 1);
		if (!t)
			break;
		itemP->text = t + 1;
		itemP->y += h / 2;
		itemP->x = itemP->xSave;
		nTabIndex = -1;
		} while (*(itemP->text));
	}
else if ((t = strchr (ps, '\t'))) {
	strncpy (s, ps, sizeof (s));
	itemP->text = s;
	fontManager.Current ()->StringSize (s, w, h, aw);
	do {
		if ((t = strchr (itemP->text, '\t')))
			*t = '\0';
		NMHotKeyString (itemP, 0, bTiny, 0, nDepth + 1);
		if (!t)
			break;
		itemP->text = t + 1;
		nTabIndex++;
	} while (*(itemP->text));
	nTabIndex = -1;
	itemP->text = ps;
	itemP->y = y;
	}
else {
	l = (int) strlen (itemP->text);
	if (bIsCurrent || !itemP->key)
		i = l;
	else {
		ch = MENU_KEY (itemP->key, *ps);
		for (i = 0; ps [i]; i++)
			if (ps [i] == ch)
				break;
		}
	strncpy (s, ps, sizeof (s));
	s [i] = '\0';
	fontManager.Current ()->StringSize (s, w, h, aw);
	if (nTabIndex >= 0) {
		x += LHX (nTabs [nTabIndex]);
		if (!gameStates.multi.bSurfingNet)
			x += itemP->w - w;
		}
	//itemP->x = x + w;
	if (i) {
		GrString (x, y, s, NULL);
#if DBG
		//GrUpdate (0);
#endif
		}
	if (i < l) {	// print the hotkey
		x += w;
		s [i] = ch;
		ch2 = s [++i];
		s [i] = '\0';
		NMSetItemColor (itemP, 1, bTiny);
		GrString (x, y, s + i - 1, NULL);
#if DBG
		//GrUpdate (0);
#endif
		NMSetItemColor (itemP, 0, bTiny);
		if (i < l) { // print text following the hotkey
			fontManager.Current ()->StringSize (s + i - 1, w, h, aw);
			x += w;
			s [i] = ch2;
			GrString (x, y, s + i, NULL);
			}
		}
	}
#if 0
if (!nDepth) {
	itemP->text = itemP->textSave;
	itemP->x = itemP->xSave;
	itemP->y = itemP->ySave;
	}
#endif
}
}

//------------------------------------------------------------------------------
// Draw a left justfied string
void NMString (CMenuItem *itemP, int bIsCurrent, int bTiny)
{
	int w1 = itemP->w, x = itemP->x, y = itemP->y;
	int l, w, h, aw, tx=0, t=0, i;
	char *p, *s1, measure [2] , *s = itemP->text;
	int XTabs [6];
	static char s2 [1024];

p = s1 = NULL;
l = (int) strlen (s);
memcpy (s2, s, l + 1);

for (i = 0; i < 6; i++)
	XTabs [i]= (LHX (nTabs [i])) + x;

measure [1] = 0;

if (!gameStates.multi.bSurfingNet) {
	p = strchr (s2, '\t');
	if (p &&(w1>0)) {
		*p = '\0';
		s1 = p+1;
		}
	}
if (w1 > 0)
	w = w1;
fontManager.Current ()->StringSize (s2, w, h, aw);
// CHANGED
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), 5, y - 1, backgroundManager.Current ()->Width () - 15, h + 2, 5, y - 1);
if (0 && gameStates.multi.bSurfingNet) {
	for (i = 0;i < l;i++) {
		if (s2 [i]=='\t' && gameStates.multi.bSurfingNet) {
			x=XTabs [t];
			t++;
			continue;
			}
		measure [0]=s2 [i];
		fontManager.Current ()->StringSize (measure, tx, h, aw);
		GrString (x, y, measure, NULL);
		x += tx;
		}
	}
else {
	NMHotKeyString (itemP, bIsCurrent, bTiny, 1, 0);
	return;
	}         

if (!gameStates.multi.bSurfingNet && p && (w1 > 0)) {
	fontManager.Current ()->StringSize (s1, w, h, aw);
	GrString (x+w1-w, y, s1, NULL);
	*p = '\t';
	}
}

//------------------------------------------------------------------------------
// Draw a slider and it's string
void NMStringSlider (CMenuItem *itemP, int bIsCurrent, int bTiny)
{
char* s1 = NULL;
char* p = strchr (itemP->savedText, '\t');
if (p) {
	*p = '\0';
	s1 = p + 1;
	}

int w, h, aw;
fontManager.Current ()->StringSize (itemP->savedText, w, h, aw);

int y = itemP->y;
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), 5, y, backgroundManager.Current ()->Width () - 15, h, 5, y);
char* t = itemP->text;
itemP->text = itemP->savedText;
NMHotKeyString (itemP, bIsCurrent, bTiny, 1, 0);
itemP->text = t;
if (p) {
	fontManager.Current ()->StringSize (s1, w, h, aw);
	int x = itemP->x + itemP->w - w;
	if (gameStates.ogl.nDrawBuffer != GL_BACK) {
		backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x, y, w, 1, x, y);
		backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x, y + h - 1, w, 1, x, y);
		}
	GrString (x, y, s1, NULL);
	*p = '\t';
	}
}


//------------------------------------------------------------------------------
// Draw a left justfied string with black background.
void NMStringBlack (int w1, int x, int y, const char *s)
{
	int w, h, aw;

fontManager.Current ()->StringSize (s, w, h, aw);
if (w1 == 0) 
	w1 = w;

CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
GrRect (x-1, y-1, x-1, y+h-1);
GrRect (x-1, y-1, x+w1-1, y-1);
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
GrRect (x, y+h, x+w1, y+h);
GrRect (x+w1, y-1, x+w1, y+h);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
GrRect (x, y, x+w1-1, y+h-1);
GrString (x+1, y+1, s, NULL);
}

//------------------------------------------------------------------------------
// Draw a right justfied string
void NMRString (CMenuItem *itemP, int bIsCurrent, int bTiny, char *s)
{
	int	w, h, aw;
	int	w1 = itemP->right_offset, 
			x = itemP->x, 
			y = itemP->y;
	char	*hs;

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
hs = itemP->text;
itemP->text = s;
h = itemP->x;
itemP->x = x - w;
NMHotKeyString (itemP, bIsCurrent, bTiny, 0, 0);
itemP->text = hs;
itemP->x = h;
}

//------------------------------------------------------------------------------

void NMRStringWXY (int w1, int x, int y, const char *s)
{
	int	w, h, aw;

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
GrString (x-w, y, s, NULL);
}

//------------------------------------------------------------------------------

#include "timer.h"

//for text itemP, constantly redraw cursor (to achieve flash)
void NMUpdateCursor (CMenuItem *itemP)
{
	int w, h, aw;
	fix time = TimerGetApproxSeconds ();
	int x, y;
	char *text = itemP->text;

	Assert (itemP->nType == NM_TYPE_INPUT_MENU || itemP->nType == NM_TYPE_INPUT);

while (*text)	{
	fontManager.Current ()->StringSize (text, w, h, aw);
	if (w > itemP->w-10)
		text++;
	else
		break;
	}
if (!*text) 
	w = 0;
x = itemP->x+w; 
y = itemP->y;
if (time & 0x8000)
	GrString (x, y, CURSOR_STRING, NULL);
else {
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
	GrRect (x, y, x+CCanvas::Current ()->Font ()->Width ()-1, y+CCanvas::Current ()->Font ()->Height ()-1);
	}
}

//------------------------------------------------------------------------------

void NMStringInputBox (int w, int x, int y, char *text, int current)
{
	int w1, h1, aw;

while (*text) {
	fontManager.Current ()->StringSize (text, w1, h1, aw);
	if (w1 > w-10)
		text++;
	else
		break;
	}
if (!*text)
	w1 = 0;
  NMStringBlack (w, x, y, text);
if (current)	
	GrString (x+w1+1, y, CURSOR_STRING, NULL);
}

//------------------------------------------------------------------------------

void NMGauge (int w, int x, int y, int val, int maxVal, int current)
{
	int w1, h, aw;

fontManager.Current ()->StringSize (" ", w1, h, aw);
if (!w) 
	w = w1 * 30;
w1 = w * val / maxVal;
if (w1 < w) {
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
	GrURect (x + w1 + 1, y, x + w, y + h - 2);
	}
CCanvas::Current ()->SetColorRGB (200, 0, 0, 255);
if (w1)
	GrURect (x + 1, y, x + w1, y + h - 2);
GrUBox (x, y, x + w - 1, y + h - 1);
}

//------------------------------------------------------------------------------

void NMDrawItem (CMenuItem *itemP, int bIsCurrent, int bTiny)
{
NMSetItemColor (itemP, bIsCurrent, bTiny);
if (itemP->rebuild) {
	NMFreeTextBm (itemP);
	itemP->rebuild = 0;
	}
switch (itemP->nType)	{
	case NM_TYPE_TEXT:
      // CCanvas::Current ()->Font ()=TEXT_FONT);
		// fall through on purpose

	case NM_TYPE_MENU:
		NMString (itemP, bIsCurrent, bTiny);
		break;

	case NM_TYPE_SLIDER:	{
		int h, l;
		char *psz = itemP->savedText;

		if (itemP->value < itemP->minValue) 
			itemP->value = itemP->minValue;
		else if (itemP->value > itemP->maxValue) 
			itemP->value = itemP->maxValue;
		sprintf (psz, "%s\t%s", itemP->text, SLIDER_LEFT);
#if 1
		l = (int) strlen (psz);
		h = itemP->maxValue - itemP->minValue + 1;
		memset (psz + l, SLIDER_MIDDLE [0], h);
		psz [l + h] = SLIDER_RIGHT [0];
		psz [l + h + 1] = '\0';
#else
		for (j = 0; j < (itemP->maxValue - itemP->minValue + 1); j++) {
			sprintf (psz, "%s%s", psz, SLIDER_MIDDLE);
			}
		sprintf (itemP->savedText, "%s%s", itemP->savedText, SLIDER_RIGHT);
#endif
		psz [itemP->value + 1 + strlen (itemP->text) + 1] = SLIDER_MARKER [0];
		NMStringSlider (itemP, bIsCurrent, bTiny);
		}
		break;

	case NM_TYPE_INPUT_MENU:
		if (itemP->group)
			NMStringInputBox (itemP->w, itemP->x, itemP->y, itemP->text, bIsCurrent);
		else
			NMString (itemP, bIsCurrent, bTiny);
		break;

	case NM_TYPE_INPUT:
		NMStringInputBox (itemP->w, itemP->x, itemP->y, itemP->text, bIsCurrent);
		break;

	case NM_TYPE_GAUGE:
		NMGauge (itemP->w, itemP->x, itemP->y, itemP->value, itemP->maxValue, bIsCurrent);
		break;

	case NM_TYPE_CHECK:
		NMString (itemP, bIsCurrent, bTiny);
		if (itemP->value)
			NMRString (itemP, bIsCurrent, bTiny, CHECKED_CHECK_BOX);
		else														  
			NMRString (itemP, bIsCurrent, bTiny, NORMAL_CHECK_BOX);
		break;

	case NM_TYPE_RADIO:
		NMString (itemP, bIsCurrent, bTiny);
		if (itemP->value)
			NMRString (itemP, bIsCurrent, bTiny, CHECKED_RADIO_BOX);
		else
			NMRString (itemP, bIsCurrent, bTiny, NORMAL_RADIO_BOX);
		break;

	case NM_TYPE_NUMBER:
		{
		char text [10];
		if (itemP->value < itemP->minValue) 
			itemP->value=itemP->minValue;
		if (itemP->value > itemP->maxValue) 
			itemP->value=itemP->maxValue;
		NMString (itemP, bIsCurrent, bTiny);
		sprintf (text, "%d", itemP->value);
		NMRString (itemP, bIsCurrent, bTiny, text);
		}
		break;
	}

}

//------------------------------------------------------------------------------

char *nmAllowedChars = NULL;

//returns true if char is bAllowed
int NMCharAllowed (char c)
{
	char *p = nmAllowedChars;

	if (!p)
		return 1;

	while (*p) {
		Assert (p [1]);

		if (c>=p [0] && c<=p [1])
			return 1;

		p += 2;
	}

	return 0;
}

//------------------------------------------------------------------------------

void NMTrimWhitespace (char *text)
{
	int i, l = (int) strlen (text);
	for (i = l-1; i>=0; i--)	{
		if (::isspace (text [i]))
			text [i] = 0;
		else
			return;
	}
}

//------------------------------------------------------------------------------

int ExecMenu (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
					int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem),
					char *filename)
{
return ExecMenu3 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, NULL, filename, -1, -1);
}

int ExecMenuTiny (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
						int (*menuCallback) (int nItems, CMenuItem *itemP, int *lastKeyP, int nItem))
{
return ExecMenu4 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, NULL, NULL, LHX (310), -1, 1);
}

//------------------------------------------------------------------------------

int ExecMenutiny2 (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
						  int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem))
{
return ExecMenu4 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, 0, NULL, -1, -1, 1);
}

//------------------------------------------------------------------------------

int ExecMenu1 (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
					int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem), 
					int *nCurItemP)
{
return ExecMenu3 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, nCurItemP, NULL, -1, -1);
}

//------------------------------------------------------------------------------

int ExecMenu2 (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
					int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem), 
					int *nCurItemP, char *filename)
{
return ExecMenu3 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, nCurItemP, filename, -1, -1);
}

//------------------------------------------------------------------------------

int ExecMenu3 (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
					int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem), 
					int *nCurItemP, char *filename, int width, int height)
 {
 return ExecMenu4 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, nCurItemP, filename, width, height, 0);
 }

//------------------------------------------------------------------------------

int ExecMenuFixedFont (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
							  int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem), 
							  int *nCurItemP, char *filename, int width, int height){
SetScreenMode (SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
//	return ExecMenu3_real (pszTitle, pszSubTitle, nItems, itemP, menuCallback, nItem, filename, width, height, GAME_FONT, GAME_FONT, GAME_FONT, GAME_FONT);
return ExecMenu4 (pszTitle, pszSubTitle, nItems, itemP, menuCallback, nCurItemP, filename, width, height, 0);
}

//------------------------------------------------------------------------------

//returns 1 if a control device button has been pressed
int NMCheckButtonPress ()
{
	int i;

	switch (gameConfig.nControlType) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
	case	CONTROL_GRAVIS_GAMEPAD:
		for (i = 0; i < 4; i++)
	 		if (JoyGetButtonDownCnt (i) > 0) 
				return 1;
		break;
	case	CONTROL_MOUSE:
	case	CONTROL_CYBERMAN:
	case	CONTROL_WINJOYSTICK:
		break;
	case	CONTROL_NONE:		//keyboard only
		#ifdef APPLE_DEMO
			if (KeyCheckChar ())	return 1;		
		#endif

		break;
	default:
		Error ("Bad control nType (gameConfig.nControlType):%i", gameConfig.nControlType);
	}

	return 0;
}

//------------------------------------------------------------------------------

extern int bRestoringMenu;

ubyte bHackDblClickMenuMode = 0;

# define JOYDEFS_CALIBRATING 0

#define CLOSE_X     (gameStates.menus.bHires?15:7)
#define CLOSE_Y     (gameStates.menus.bHires?15:7)
#define CLOSE_SIZE  (gameStates.menus.bHires?10:5)

void NMDrawCloseBox (int x, int y)
{
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
GrRect (x + CLOSE_X, y + CLOSE_Y, x + CLOSE_X + CLOSE_SIZE, y + CLOSE_Y + CLOSE_SIZE);
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (21, 21, 21));
GrRect (x + CLOSE_X + LHX (1), y + CLOSE_Y + LHX (1), x + CLOSE_X + CLOSE_SIZE - LHX (1), y + CLOSE_Y + CLOSE_SIZE - LHX (1));
}

//------------------------------------------------------------------------------

int NMDrawTitle (const char *pszTitle, CFont *font, uint color, int ty)
{
if (pszTitle && *pszTitle)	{
		int w, h, aw;

	fontManager.SetCurrent (font);
	fontManager.SetColorRGBi (color, 1, 0, 0);
	fontManager.Current ()->StringSize (pszTitle, w, h, aw);
	GrPrintF (NULL, 0x8000, ty, pszTitle);
	ty += h;
	}
return ty;
}

//------------------------------------------------------------------------------

void NMGetTitleSize (const char *pszTitle, CFont *font, int *tw, int *th)
{
if (pszTitle && *pszTitle)	{
		int nStringWidth, nStringHeight, nAverageWidth;

	fontManager.SetCurrent (font);
	fontManager.Current ()->StringSize (pszTitle, nStringWidth, nStringHeight, nAverageWidth);
	if (nStringWidth > *tw)
		*tw = nStringWidth;
	*th += nStringHeight;
	}
}

//------------------------------------------------------------------------------

int NMGetMenuSize (CMenuItem *itemP, int nItems, int *w, int *h, int *aw, int *nMenus, int *nOthers)
{
	int	nStringWidth = 0, nStringHeight = 0, nAverageWidth = 0;
	int	i, j;

#if 0
if ((gameOpts->menus.nHotKeys > 0) && !gameStates.app.bEnglish)
	gameOpts->menus.nHotKeys = -1;
#endif
for (i = 0; i < nItems; i++, itemP++) {
	if (!gameStates.app.bEnglish)
		itemP->key = *(itemP->text - 1);
	if (itemP->key) {
		if (gameOpts->menus.nHotKeys < 0) {
			if ((itemP->key < KEY_1) || (itemP->key > KEY_0))
				itemP->key = -1;
			}
		else if (gameOpts->menus.nHotKeys == 0)
			itemP->key = 0;
		}
	itemP->redraw = 1;
	itemP->y = *h;
	fontManager.Current ()->StringSize (itemP->text, nStringWidth, nStringHeight, nAverageWidth);
	itemP->right_offset = 0;

	if (gameStates.multi.bSurfingNet)
		nStringHeight+=LHY (3);

	itemP->savedText [0] = '\0';
	if (itemP->nType == NM_TYPE_SLIDER) {
		int w1, h1, aw1;
		(*nOthers)++;
		sprintf (itemP->savedText, "%s", SLIDER_LEFT);
		for (j = 0; j< (itemP->maxValue - itemP->minValue+1); j++)	{
			sprintf (itemP->savedText, "%s%s", itemP->savedText, SLIDER_MIDDLE);
			}
		sprintf (itemP->savedText, "%s%s", itemP->savedText, SLIDER_RIGHT);
		fontManager.Current ()->StringSize (itemP->savedText, w1, h1, aw1);
		nStringWidth += w1 + *aw;
		}
	else if (itemP->nType == NM_TYPE_MENU)	{
		(*nMenus)++;
		}
	else if (itemP->nType == NM_TYPE_CHECK)	{
		int w1, h1, aw1;
		(*nOthers)++;
		fontManager.Current ()->StringSize (NORMAL_CHECK_BOX, w1, h1, aw1);
		itemP->right_offset = w1;
		fontManager.Current ()->StringSize (CHECKED_CHECK_BOX, w1, h1, aw1);
		if (w1 > itemP->right_offset)
			itemP->right_offset = w1;
		}
	else if (itemP->nType == NM_TYPE_RADIO) {
		int w1, h1, aw1;
		(*nOthers)++;
		fontManager.Current ()->StringSize (NORMAL_RADIO_BOX, w1, h1, aw1);
		itemP->right_offset = w1;
		fontManager.Current ()->StringSize (CHECKED_RADIO_BOX, w1, h1, aw1);
		if (w1 > itemP->right_offset)
			itemP->right_offset = w1;
		}
	else if  (itemP->nType==NM_TYPE_NUMBER)	{
		int w1, h1, aw1;
		char test_text [20];
		(*nOthers)++;
		sprintf (test_text, "%d", itemP->maxValue);
		fontManager.Current ()->StringSize (test_text, w1, h1, aw1);
		itemP->right_offset = w1;
		sprintf (test_text, "%d", itemP->minValue);
		fontManager.Current ()->StringSize (test_text, w1, h1, aw1);
		if (w1 > itemP->right_offset)
			itemP->right_offset = w1;
		}
	else if (itemP->nType == NM_TYPE_INPUT)	{
		Assert (strlen (itemP->text) < NM_MAX_TEXT_LEN);
		strncpy (itemP->savedText, itemP->text, NM_MAX_TEXT_LEN);
		(*nOthers)++;
		nStringWidth = itemP->nTextLen*CCanvas::Current ()->Font ()->Width ()+ ((gameStates.menus.bHires?3:1)*itemP->nTextLen);
		if (nStringWidth > MAX_TEXT_WIDTH) 
			nStringWidth = MAX_TEXT_WIDTH;
		itemP->value = -1;
		}
	else if (itemP->nType == NM_TYPE_INPUT_MENU)	{
		Assert (strlen (itemP->text) < NM_MAX_TEXT_LEN);
		strncpy (itemP->savedText, itemP->text, NM_MAX_TEXT_LEN);
		(*nMenus)++;
		nStringWidth = itemP->nTextLen*CCanvas::Current ()->Font ()->Width ()+ ((gameStates.menus.bHires?3:1)*itemP->nTextLen);
		itemP->value = -1;
		itemP->group = 0;
		}
	itemP->w = nStringWidth;
	itemP->h = nStringHeight;

	if (nStringWidth > *w)
		*w = nStringWidth;		// Save maximum width
	if (nAverageWidth > *aw)
		*aw = nAverageWidth;
	*h += nStringHeight + 1;		// Find the height of all strings
	}
return nStringHeight;
}

//------------------------------------------------------------------------------

void NMSetItemPos (CMenuItem *itemP, int nItems, int twidth, int xOffs, int yOffs, int right_offset)
{
	int	i, j;

for (i = 0; i < nItems; i++)	{
	if (gameOpts->menus.nStyle && ((itemP [i].x == (short) 0x8000) || itemP [i].centered)) {
		itemP [i].centered = 1;
		itemP [i].x = fontManager.Current ()->GetCenteredX (itemP [i].text);
		}
	else
		itemP [i].x = xOffs + twidth + right_offset;
	itemP [i].y += yOffs;
	itemP [i].xSave = itemP [i].x;
	itemP [i].ySave = itemP [i].y;
	if (itemP [i].nType == NM_TYPE_RADIO)	{
		int fm = -1;	// ffs first marked one
		for (j = 0; j < nItems; j++)	{
			if ((itemP [j].nType == NM_TYPE_RADIO) && (itemP [j].group == itemP [i].group)) {
				if ((fm == -1) && itemP [j].value)
					fm = j;
				itemP [j].value = 0;
				}
			}
		if ( fm >= 0)
			itemP [fm].value = 1;
		else
			itemP [i].value = 1;
		}
	}
}

//------------------------------------------------------------------------------

int NMInitCtrl (tMenuProps *ctrlP, const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP)
{
if ((ctrlP->scWidth == screen.Width ()) && (ctrlP->scHeight == screen.Height ()))
	return 0;

	tMenuProps	ctrl = *ctrlP;
	int			i, gap, haveTitle;

ctrl.scWidth = screen.Width ();
ctrl.scHeight = screen.Height ();
ctrl.nDisplayMode = gameStates.video.nDisplayMode;
NMGetTitleSize (pszTitle, TITLE_FONT, &ctrl.tw, &ctrl.th);
NMGetTitleSize (pszSubTitle, SUBTITLE_FONT, &ctrl.tw, &ctrl.th);

haveTitle = ((pszTitle && *pszTitle) || (pszSubTitle && *pszSubTitle));
gap = haveTitle ? (int) LHY (8) : 0;
ctrl.th += gap;		//put some space between pszTitles & body
fontManager.SetCurrent (ctrl.bTinyMode ? SMALL_FONT : NORMAL_FONT);

ctrl.h = ctrl.th;
ctrl.nMenus = ctrl.nOthers = 0;
ctrl.nStringHeight = NMGetMenuSize (itemP, nItems, &ctrl.w, &ctrl.h, &ctrl.aw, &ctrl.nMenus, &ctrl.nOthers);
ctrl.nMaxOnMenu = (((gameOpts->menus.nStyle && (ctrl.scHeight > 480)) ? ctrl.scHeight * 4 / 5 : 480) - ctrl.th - LHY (8)) / ctrl.nStringHeight - 2;
if (/*!ctrl.bTinyMode &&*/ (ctrl.h > (ctrl.nMaxOnMenu * (ctrl.nStringHeight+1)) + gap)) {
  ctrl.bIsScrollBox = 1;
  ctrl.h = (ctrl.nMaxOnMenu * (ctrl.nStringHeight + haveTitle) + haveTitle * LHY (ctrl.bTinyMode ? 12 : 8));
 }
else
	ctrl.bIsScrollBox = 0;
ctrl.nMaxDisplayable = (ctrl.nMaxOnMenu < nItems) ? ctrl.nMaxOnMenu : nItems;
ctrl.right_offset = 0;
if (ctrl.width > -1)
	ctrl.w = ctrl.width;
if (ctrl.height > -1)
	ctrl.h = ctrl.height;

for (i = 0; i < nItems; i++) {
	itemP [i].w = ctrl.w;
	if (itemP [i].right_offset > ctrl.right_offset)
		ctrl.right_offset = itemP [i].right_offset;
	if (itemP [i].noscroll && (i == ctrl.nMaxNoScroll))
		ctrl.nMaxNoScroll++;
	}
ctrl.nScrollOffset = ctrl.nMaxNoScroll;
if (ctrl.right_offset > 0)
	ctrl.right_offset += 3;

ctrl.w += ctrl.right_offset;
ctrl.twidth = 0;
if (ctrl.tw > ctrl.w)	{
	ctrl.twidth = (ctrl.tw - ctrl.w) / 2;
	ctrl.w = ctrl.tw;
}

if (bRestoringMenu) { 
	ctrl.right_offset = 0; 
	ctrl.twidth = 0;
	}

if (ctrl.w > ctrl.scWidth)
	ctrl.w = ctrl.scWidth;
if (ctrl.h > ctrl.scHeight)
	ctrl.h = ctrl.scHeight;

ctrl.xOffs = (gameStates.menus.bHires ? 30 : 15);
i = (ctrl.scWidth - ctrl.w) / 2;
if (i < ctrl.xOffs)
	ctrl.xOffs = i / 2;
ctrl.yOffs = (gameStates.menus.bHires ? 30 : 15);
if (ctrl.scHeight - ctrl.h < 2 * ctrl.yOffs)
	ctrl.h = ctrl.scHeight - 2 * ctrl.yOffs;
ctrl.w += 2 * ctrl.xOffs;
ctrl.h += 2 * ctrl.yOffs;
ctrl.x = (ctrl.scWidth - ctrl.w) / 2;
ctrl.y = (ctrl.scHeight - ctrl.h) / 2;
if (gameOpts->menus.nStyle) {
	ctrl.xOffs += ctrl.x;
	ctrl.yOffs += ctrl.y;
	}
ctrl.bValid = 1;
*ctrlP = ctrl;
NMSetItemPos (itemP, nItems, ctrl.twidth, ctrl.xOffs, ctrl.yOffs, ctrl.right_offset);
return 1;
}

//------------------------------------------------------------------------------

void NMSaveScreen (CCanvas **gameCanvasP)
{
*gameCanvasP = CCanvas::Current ();
CCanvas::Push ();
CCanvas::SetCurrent (NULL);
}

//------------------------------------------------------------------------------

void NMRestoreScreen (char *filename, int bDontRestore)
{
//CCanvas::SetCurrent (bg->menu_canvas);
backgroundManager.Remove ();
if (gameStates.app.bGameRunning)
	backgroundManager.Remove ();
GrUpdate (0);
CCanvas::SetCurrent (NULL);		
CCanvas::Pop ();
GrabMouse (1, 0);
}

//------------------------------------------------------------------------------

void ShowMenuHelp (char *szHelp)
{
if (szHelp && *szHelp) {
	int nInMenu = gameStates.menus.nInMenu;
	int bFastMenus = gameOpts->menus.bFastMenus;
	gameStates.menus.nInMenu = 0;
	//gameOpts->menus.bFastMenus = 0;
	gameData.menu.helpColor = RGBA_PAL (47, 47, 47);
	gameData.menu.colorOverride = gameData.menu.helpColor;
	ExecMessageBox ("D2X-XL online help", NULL, -3, szHelp, " ", TXT_CLOSE);
	gameData.menu.colorOverride = 0;
	gameOpts->menus.bFastMenus = bFastMenus;
	gameStates.menus.nInMenu = nInMenu;
	}
}

//------------------------------------------------------------------------------

#define REDRAW_ALL	for (i = 0; i < nItems; i++) itemP [i].redraw = 1; bRedrawAll = 1

int ExecMenu4 (const char *pszTitle, const char *pszSubTitle, int nItems, CMenuItem *itemP, 
					 int (*menuCallback) (int nItems, CMenuItem *itemP, int *lastKeyP, int nItem), 
					 int *nCurItemP, char *filename, int width, int height, int bTinyMode)
{
	int			bKeyRepeat, done, nItem = nCurItemP ? *nCurItemP : 0;
	int			choice, old_choice, i;
	tMenuProps	ctrl;
	int			k, nLastScrollCheck = -1, sx, sy;
	int			bAllText=0;		//set true if all text itemP
	int			sound_stopped=0, time_stopped=0;
   int			topChoice;   // Is this a scrolling box? Set to 0 at init
	int			bDontRestore = 0;
	int			bRedraw = 0, bRedrawAll = 0, bStart = 1;
	CCanvas*		gameCanvasP;
	int			bWheelUp, bWheelDown, nMouseState, nOldMouseState, bDblClick = 0;
	int			mx=0, my=0, x1 = 0, x2, y1, y2;
	int			bLaunchOption = 0;
	int			bCloseBox = 0;

if (gameStates.menus.nInMenu)
	return -1;
memset (&ctrl, 0, sizeof (ctrl));
ctrl.width = width;
ctrl.height = height;
ctrl.bTinyMode = bTinyMode;
FlushInput ();

if (nItems < 1)
	return -1;
if (gameStates.app.bGameRunning)
	backgroundManager.LoadStars ();
SDL_ShowCursor (0);
SDL_EnableKeyRepeat(60, 30);
gameStates.menus.nInMenu++;
if (!gameOpts->menus.nStyle && (gameStates.app.nFunctionMode == FMODE_GAME) && !(gameData.app.nGameMode & GM_MULTI)) {
	DigiPauseDigiSounds ();
	sound_stopped = 1;
	}

if (!(gameOpts->menus.nStyle || (IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME) &&(!gameStates.app.bEndLevelSequence)))) {
	time_stopped = 1;
	StopTime ();
	#ifdef TACTILE 
	  if (TactileStick)
		  DisableForces ();
	#endif
	}

if (gameStates.app.bGameRunning && IsMultiGame)
	gameData.multigame.nTypingTimeout = 0;

SetPopupScreenMode ();
if (!gameOpts->menus.nStyle) {
	OglSetDrawBuffer (GL_FRONT, 1);
	if (gameStates.menus.bNoBackground || gameStates.app.bGameRunning) {
		//NMLoadBackground (NULL, NULL, 0);
		GrUpdate (0);
		}
	}
NMSaveScreen (&gameCanvasP);
bKeyRepeat = gameStates.input.keys.bRepeat;
gameStates.input.keys.bRepeat = 1;
if (nItem == -1)
	choice = -1;
else {
	if (nItem < 0) 
		nItem = 0;
	else 
		nItem %= nItems;
	choice = nItem;
	bDblClick = 1;
	while (itemP [choice].nType == NM_TYPE_TEXT) {
		choice++;
		if (choice >= nItems)
			choice = 0; 
		if (choice == nItem) {
			choice = 0; 
			bAllText = 1;
			break; 
			}
		}
	} 

done = 0;
topChoice = 0;
while (itemP [topChoice].nType == NM_TYPE_TEXT) {
	topChoice++;
	if (topChoice >= nItems)
		topChoice = 0; 
	if (topChoice == nItem) {
		topChoice = 0; 
		break; 
		}
	}

//GrUpdate (0);
// Clear mouse, joystick to clear button presses.
GameFlushInputs ();
nMouseState = nOldMouseState = 0;
bCloseBox = !(filename || gameStates.menus.bReordering);

if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING) {
	SDL_ShowCursor (1);
	}
GrabMouse (0, 0);
while (!done) {
	if (nCurItemP)
		*nCurItemP = choice;
	if (gameStates.app.bGameRunning && IsMultiGame) {
		gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer] = 1;
		MultiSendTyping ();
		}
	if (!JOYDEFS_CALIBRATING)
		SDL_ShowCursor (1);      // possibly hidden
	nOldMouseState = nMouseState;
	if (!gameStates.menus.bReordering) {
		int b = gameOpts->legacy.bInput;
		gameOpts->legacy.bInput = 1;
		nMouseState = MouseButtonState (0);
		gameOpts->legacy.bInput = b;
		}
	bWheelUp = MouseButtonState (3);
	bWheelDown = MouseButtonState (4);
	//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();
	//NetworkListen ();
	if (bWheelUp)
		k = KEY_UP;
	else if (bWheelDown)
		k = KEY_DOWN;
	else
		k = KeyInKey ();
	if (mouseData.bDoubleClick)
		k = KEY_ENTER;
	if ((ctrl.scWidth != screen.Width ()) || (ctrl.scHeight != screen.Height ())) {
		memset (&ctrl, 0, sizeof (ctrl));
		ctrl.width = width;
		ctrl.height = height;
		ctrl.bTinyMode = bTinyMode;
		}
#if 1
	if (ctrl.bValid && (ctrl.nDisplayMode != gameStates.video.nDisplayMode)) {
		NMFreeAllTextBms (itemP, nItems);
		backgroundManager.Remove (); //bDontRestore?
		SetScreenMode (SCREEN_MENU);
		NMSaveScreen (&gameCanvasP);
//		RemapFontsAndMenus (1);
		memset (&ctrl, 0, sizeof (ctrl));
		ctrl.width = width;
		ctrl.height = height;
		ctrl.bTinyMode = bTinyMode;
		}
#endif
	if (NMInitCtrl (&ctrl, pszTitle, pszSubTitle, nItems, itemP)) {
		backgroundManager.Setup (filename, ctrl.x, ctrl.y, ctrl.w, ctrl.h);
		if (!gameOpts->menus.nStyle)
			CCanvas::SetCurrent (backgroundManager.Canvas ());
		bRedraw = 0;
		ctrl.ty = ctrl.yOffs;
		if (choice > ctrl.nScrollOffset + ctrl.nMaxOnMenu - 1)
			ctrl.nScrollOffset = choice - ctrl.nMaxOnMenu + 1;
		}
	if (!gameOpts->menus.nStyle) {
		if (menuCallback)
			(*menuCallback) (nItems, itemP, &k, choice);
		}
	else {
		if (gameStates.app.bGameRunning) {
			CCanvas::Push ();
			CCanvas::SetCurrent (gameCanvasP);
			//GrPaletteStepLoad (paletteManager.Game ());
			//GrCopyPalette (grPalette, paletteManager.Game (), sizeof (grPalette));
			if (!gameStates.app.bShowError) {
				if (gameData.app.bGamePaused /*|| timer_paused*/) {
					GameRenderFrame ();
					gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
					G3_SLEEP (0);
					}
				else {
					GameLoop (1, 0);
					}
				}
			CCanvas::Pop ();
			}
		}

	if (!bRedraw || gameOpts->menus.nStyle) {
		int t;
		backgroundManager.Draw ();
		if (!gameStates.app.bGameRunning)
			console.Draw();
		if (menuCallback)
     		choice = (*menuCallback) (nItems, itemP, &k, choice);
		t = NMDrawTitle (pszTitle, TITLE_FONT, RGBA_PAL (31, 31, 31), ctrl.yOffs);
		NMDrawTitle (pszSubTitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
		if (!bRedraw)
			ctrl.ty = t;
		fontManager.SetCurrent (bTinyMode ? SMALL_FONT : NORMAL_FONT);
		}
	if (!time_stopped){
		// Save current menu box
		if (MultiMenuPoll () == -1)
			k = -2;
		}

	if (k < -1) {
		bDontRestore = (k == -3);		//-3 means don't restore
		if (choice >= 0)
			choice = k;
		else {
			choice = -choice - 1;
			*nCurItemP = choice;
			}
		k = -1;
		done = 1;
		}
old_choice = choice;
if (k && (console.Events (k) || bWheelUp || bWheelDown))
	switch (k)	{

		case KEY_SHIFTED + KEY_ESC:
			console.Show ();
			k = -1;
			break;

		case KEY_I:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 ShowNetGameInfo (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo)
			{
			 done = 1;
			 choice = -1;
			}
		 break;
		 
		case KEY_U:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 NetworkRequestPlayerNames (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done=1;
			 choice=-1;
			}
		 break;

		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;

		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;

		case KEY_COMMAND + KEY_T:
		case KEY_CTRLED + KEY_T:
			gameData.menu.alpha = (gameData.menu.alpha + 16) & 0xFF;
			break;

		case KEY_COMMAND + KEY_P:
		case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
		 if (bPauseableMenu) {
			 bPauseableMenu=0;
			 done=1;
		 	 choice=-1;
			}
		 else
			 gameData.app.bGamePaused = !gameData.app.bGamePaused;
		 break;
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (bAllText) 
				break;
			do {
				choice--;
      		if (ctrl.bIsScrollBox) {
           		nLastScrollCheck=-1;
              	if (choice < topChoice) { 
						choice = nItems - 1; 
						ctrl.nScrollOffset = nItems - ctrl.nMaxDisplayable + ctrl.nMaxNoScroll;
						if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
							ctrl.nScrollOffset = ctrl.nMaxNoScroll;
						REDRAW_ALL;
						break; 
						}
             	else if (choice < ctrl.nScrollOffset) {
						REDRAW_ALL;
                  ctrl.nScrollOffset--;
						if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
							ctrl.nScrollOffset = ctrl.nMaxNoScroll;
					   }
	           	}
           	else {
           		if (choice >= nItems) 
						choice=0;
            	else if (choice < 0) 
						choice = nItems - 1;
           		}
				} while (itemP [choice].nType == NM_TYPE_TEXT);
			if ((itemP [choice].nType==NM_TYPE_INPUT) &&(choice != old_choice))
				itemP [choice].value = -1;
			if ((old_choice>-1) &&(itemP [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice != choice))	{
				itemP [old_choice].group=0;
				strcpy (itemP [old_choice].text, itemP [old_choice].savedText);
				itemP [old_choice].value = -1;
			}
			if (old_choice>-1) 
				itemP [old_choice].redraw = 1;
			itemP [choice].redraw=1;
			break;

		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
      	// ((0, "Pressing down! ctrl.bIsScrollBox=%d", ctrl.bIsScrollBox);
     		if (bAllText) 
				break;
			do {
				choice++;
        		if (ctrl.bIsScrollBox) {
            	nLastScrollCheck = -1;
					if (choice == nItems) {
						choice = 0;
						if (ctrl.nScrollOffset) {
							REDRAW_ALL;
			            ctrl.nScrollOffset = ctrl.nMaxNoScroll;
							}
						}
              	if (choice >= ctrl.nMaxOnMenu + ctrl.nScrollOffset - ctrl.nMaxNoScroll) {
						REDRAW_ALL;
                  ctrl.nScrollOffset++;
						}
           		}
            else {
         		if (choice < 0) 
						choice=nItems-1;
           		else if (choice >= nItems) 
						choice=0;
           		}
				} while (itemP [choice].nType==NM_TYPE_TEXT);
                                                      
			if ((itemP [choice].nType == NM_TYPE_INPUT) && (choice != old_choice))
				itemP [choice].value = -1;
			if ((old_choice>-1) &&(itemP [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice != choice))	{
				itemP [old_choice].group=0;
				strcpy (itemP [old_choice].text, itemP [old_choice].savedText);
				itemP [old_choice].value = -1;
			}
			if (old_choice>-1)
				itemP [old_choice].redraw=1;
			itemP [choice].redraw=1;
			break;

		case KEY_SPACEBAR:
checkOption:
radioOption:
			if (choice > -1)	{
				switch (itemP [choice].nType)	{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if (itemP [choice].value)
						itemP [choice].value = 0;
					else
						itemP [choice].value = 1;
					if (ctrl.bIsScrollBox) {
						if (choice== (ctrl.nMaxOnMenu+ctrl.nScrollOffset-1) || choice==ctrl.nScrollOffset)
							nLastScrollCheck=-1;				
						 }
			
					itemP [choice].redraw=1;
					if (bLaunchOption)
						goto launchOption;
					break;
				case NM_TYPE_RADIO:
					for (i = 0; i < nItems; i++)	{
						if ((i != choice) &&(itemP [i].nType == NM_TYPE_RADIO) && (itemP [i].group == itemP [choice].group) &&(itemP [i].value))	{
							itemP [i].value = 0;
							itemP [i].redraw = 1;
						}
					}
					itemP [choice].value = 1;
					itemP [choice].redraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				}
			}
			break;

      case KEY_SHIFTED+KEY_UP:
         if (gameStates.menus.bReordering && choice != topChoice) {
            Swap (itemP [choice].text, itemP [choice - 1].text);
            Swap (itemP [choice].value, itemP [choice - 1].value);
            itemP [choice].rebuild =
            itemP [choice - 1].rebuild = 1;
            choice--;
				}
         break;

      case KEY_SHIFTED+KEY_DOWN:
         if (gameStates.menus.bReordering && choice !=  (nItems-1)) {
            Swap (itemP [choice].text, itemP [choice + 1].text);
            Swap (itemP [choice].value, itemP [choice + 1].value);
            itemP [choice].rebuild =
            itemP [choice + 1].rebuild = 1;
            choice++;
				}
         break;
                
		case KEY_ALTED + KEY_ENTER: {
			//int bLoadCustomBg = NMFreeCustomBg ();
			NMFreeAllTextBms (itemP, nItems);
			backgroundManager.Restore ();
			//NMRestoreScreen (filename, & bDontRestore);
			GrToggleFullScreenGame ();
			GrabMouse (0, 0);
			SetScreenMode (SCREEN_MENU);
			NMSaveScreen (&gameCanvasP);
			RemapFontsAndMenus (1);
			memset (&ctrl, 0, sizeof (ctrl));
			ctrl.width = width;
			ctrl.height = height;
			ctrl.bTinyMode = bTinyMode;
			continue;
			}

		case KEY_ENTER:
		case KEY_PADENTER:
launchOption:
			if ((choice > -1) && (itemP [choice].nType == NM_TYPE_INPUT_MENU) && (itemP [choice].group == 0)) {
				itemP [choice].group = 1;
				itemP [choice].redraw = 1;
				if (!strnicmp (itemP [choice].savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
					itemP [choice].text [0] = 0;
					itemP [choice].value = -1;
					}
				else {
					NMTrimWhitespace (itemP [choice].text);
					}
				}
			else {
				if (nCurItemP)
					*nCurItemP = choice;
				done = 1;
				}
			break;

		case KEY_ESC:
			if ((choice > -1) &&(itemP [choice].nType==NM_TYPE_INPUT_MENU) &&(itemP [choice].group == 1)) {
				itemP [choice].group=0;
				strcpy (itemP [choice].text, itemP [choice].savedText);
				itemP [choice].redraw=1;
				itemP [choice].value = -1;
				}
			else {
				done = 1;
				if (nCurItemP)
					*nCurItemP = choice;
				choice = -1;
			}
			break;

		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_ALTED + KEY_F9:
			gameStates.app.bSaveScreenshot = 1;
			SaveScreenShot (NULL, 0);
			for (i = 0;i < nItems;i++)
				itemP [i].redraw=1;
		
			break;

		#if DBG
		case KEY_BACKSP:
			if ((choice > -1) && (itemP [choice].nType != NM_TYPE_INPUT) && (itemP [choice].nType != NM_TYPE_INPUT_MENU))
				Int3 (); 
			break;
		#endif

		case KEY_F1:
			ShowMenuHelp (itemP [choice].szHelp);
			break;
		}

		if (!done && nMouseState && !nOldMouseState && !bAllText) {
			MouseGetPos (&mx, &my);
			for (i = 0; i < nItems; i++)	{
				x1 = CCanvas::Current ()->Left () + itemP [i].x - itemP [i].right_offset - 6;
				x2 = x1 + itemP [i].w;
				y1 = CCanvas::Current ()->Top () + itemP [i].y;
				y2 = y1 + itemP [i].h;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					if (i + ctrl.nScrollOffset - ctrl.nMaxNoScroll != choice) {
						if (bHackDblClickMenuMode) 
							bDblClick = 0; 
					}
				
					choice = i + ctrl.nScrollOffset - ctrl.nMaxNoScroll;
					if (choice >= 0 && choice < nItems)
						switch (itemP [choice].nType)	{
							case NM_TYPE_CHECK:
								itemP [choice].value = !itemP [choice].value;
								itemP [choice].redraw=1;
								if (ctrl.bIsScrollBox)
									nLastScrollCheck=-1;
								break;
							case NM_TYPE_RADIO:
								for (i = 0; i < nItems; i++)	{
									if ((i!=choice) &&(itemP [i].nType==NM_TYPE_RADIO) &&(itemP [i].group==itemP [choice].group) &&(itemP [i].value))	{
										itemP [i].value = 0;
										itemP [i].redraw = 1;
									}
								}
								itemP [choice].value = 1;
								itemP [choice].redraw = 1;
								break;
							}
					itemP [old_choice].redraw=1;
					break;
				}
			}
		}

		if (nMouseState && bAllText) {
			if (nCurItemP)
				*nCurItemP = choice;
			done = 1;
			}
	
		if (!done && nMouseState && !bAllText) {
			MouseGetPos (&mx, &my);
		
			// check possible scrollbar stuff first
			if (ctrl.bIsScrollBox) {
				int i, arrowWidth, arrowHeight, aw;
			
				if (ctrl.nScrollOffset > ctrl.nMaxNoScroll) {
					fontManager.Current ()->StringSize (UP_ARROW_MARKER, arrowWidth, arrowHeight, aw);
					x2 = CCanvas::Current ()->Left () + itemP [ctrl.nScrollOffset].x- (gameStates.menus.bHires?24:12);
		         y1 = CCanvas::Current ()->Top () + itemP [ctrl.nScrollOffset].y- ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
					x1 = x2 - arrowWidth;
					y2 = y1 + arrowHeight;
					if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
						choice--;
		           	nLastScrollCheck=-1;
		            if (choice < ctrl.nScrollOffset) {
							REDRAW_ALL;
		               ctrl.nScrollOffset--;
			            }
						}
					}
				if ((i = ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll) < nItems) {
					fontManager.Current ()->StringSize (DOWN_ARROW_MARKER, arrowWidth, arrowHeight, aw);
					x2 = CCanvas::Current ()->Left () + itemP [i-1].x - (gameStates.menus.bHires?24:12);
					y1 = CCanvas::Current ()->Top () + itemP [i-1].y - ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
					x1 = x2 - arrowWidth;
					y2 = y1 + arrowHeight;
					if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
						choice++;
		            nLastScrollCheck=-1;
		            if (choice >= ctrl.nMaxOnMenu + ctrl.nScrollOffset) {
							REDRAW_ALL;
		               ctrl.nScrollOffset++;
							}
						}
					}
				}
		
			for (i = ctrl.nScrollOffset; i < nItems; i++)	{
				x1 = CCanvas::Current ()->Left () + itemP [i].x - itemP [i].right_offset - 6;
				x2 = x1 + itemP [i].w;
				y1 = CCanvas::Current ()->Top () + itemP [i].y;
				y1 -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
				y2 = y1 + itemP [i].h;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) &&(itemP [i].nType != NM_TYPE_TEXT)) {
					if (i /*+ ctrl.nScrollOffset - ctrl.nMaxNoScroll*/ != choice) {
						if (bHackDblClickMenuMode) 
							bDblClick = 0; 
					}

					choice = i/* + ctrl.nScrollOffset - ctrl.nMaxNoScroll*/;

					if (itemP [choice].nType == NM_TYPE_SLIDER) {
						char slider_text [NM_MAX_TEXT_LEN+1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
					
						strcpy (slider_text, itemP [choice].savedText);
						p = strchr (slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p+1;
						}
						if (p) {
							fontManager.Current ()->StringSize (s1, slider_width, height, aw);
							fontManager.Current ()->StringSize (SLIDER_LEFT, sleft_width, height, aw);
							fontManager.Current ()->StringSize (SLIDER_RIGHT, sright_width, height, aw);
							fontManager.Current ()->StringSize (SLIDER_MIDDLE, smiddle_width, height, aw);

							x1 = CCanvas::Current ()->Left () + itemP [choice].x + itemP [choice].w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ((mx > x1) &&(mx < (x1 + sleft_width)) &&(itemP [choice].value != itemP [choice].minValue)) {
								itemP [choice].value = itemP [choice].minValue;
								itemP [choice].redraw = 2;
							} else if ((mx < x2) &&(mx > (x2 - sright_width)) &&(itemP [choice].value != itemP [choice].maxValue)) {
								itemP [choice].value = itemP [choice].maxValue;
								itemP [choice].redraw = 2;
							} else if ((mx > (x1 + sleft_width)) &&(mx < (x2 - sright_width))) {
								int numValues, value_width, newValue;
							
								numValues = itemP [choice].maxValue - itemP [choice].minValue + 1;
								value_width = (slider_width - sleft_width - sright_width) / numValues;
								newValue = (mx - x1 - sleft_width) / value_width;
								if (itemP [choice].value != newValue) {
									itemP [choice].value = newValue;
									itemP [choice].redraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == old_choice)
						break;
					if ((itemP [choice].nType==NM_TYPE_INPUT) &&(choice != old_choice))
						itemP [choice].value = -1;
					if ((old_choice>-1) &&(itemP [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice != choice))	{
						itemP [old_choice].group=0;
						strcpy (itemP [old_choice].text, itemP [old_choice].savedText);
						itemP [old_choice].value = -1;
					}
					if (old_choice>-1) 
						itemP [old_choice].redraw = 1;
					itemP [choice].redraw=1;
					break;
				}
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && !bAllText &&(choice != -1) &&(itemP [choice].nType == NM_TYPE_MENU)) {
			MouseGetPos (&mx, &my);
			x1 = CCanvas::Current ()->Left () + itemP [choice].x;
			x2 = x1 + itemP [choice].w;
			y1 = CCanvas::Current ()->Top () + itemP [choice].y;
			if (choice >= ctrl.nScrollOffset)
				y1 -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
			y2 = y1 + itemP [choice].h;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (bHackDblClickMenuMode) {
					if (bDblClick) {
						if (nCurItemP)
							*nCurItemP = choice;
						done = 1;
						}
					else 
						bDblClick = 1;
				}
				else {
					done = 1;
					if (nCurItemP)
						*nCurItemP = choice;
				}
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && (choice > -1) &&
			 (itemP [choice].nType == NM_TYPE_INPUT_MENU) && (itemP [choice].group == 0)) {
			itemP [choice].group = 1;
			itemP [choice].redraw = 1;
			if (!strnicmp (itemP [choice].savedText, TXT_EMPTY, strlen (TXT_EMPTY)))	{
				itemP [choice].text [0] = 0;
				itemP [choice].value = -1;
			} else {
				NMTrimWhitespace (itemP [choice].text);
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && bCloseBox) {
			MouseGetPos (&mx, &my);
			x1 = (gameOpts->menus.nStyle ? ctrl.x : CCanvas::Current ()->Left ()) + CLOSE_X;
			x2 = x1 + CLOSE_SIZE;
			y1 = (gameOpts->menus.nStyle ? ctrl.y : CCanvas::Current ()->Top ()) + CLOSE_Y;
			y2 = y1 + CLOSE_SIZE;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (nCurItemP)
					*nCurItemP = choice;
				choice = -1;
				done = 1;
				}
			}

//	 HACK! Don't redraw loadgame preview
		if (bRestoringMenu) 
			itemP [0].redraw = 0;

		if (choice > -1)	{
			int ascii;

			if (((itemP [choice].nType == NM_TYPE_INPUT) || 
				 ((itemP [choice].nType == NM_TYPE_INPUT_MENU) && (itemP [choice].group == 1))) && (old_choice == choice)) {
				if (k == KEY_LEFT || k == KEY_BACKSP || k == KEY_PAD4)	{
					if (itemP [choice].value  == -1) 
						itemP [choice].value = (int) strlen (itemP [choice].text);
					if (itemP [choice].value > 0)
						itemP [choice].value--;
					itemP [choice].text [itemP [choice].value] = 0;
					itemP [choice].redraw = 1;
					}
				else {
					ascii = KeyToASCII (k);
					if ((ascii < 255) && (itemP [choice].value < itemP [choice].nTextLen)) {
						int bAllowed;

						if (itemP [choice].value  == -1)
							itemP [choice].value = 0;
						bAllowed = NMCharAllowed ((char) ascii);
						if (!bAllowed && ascii == ' ' && NMCharAllowed ('_')) {
							ascii = '_';
							bAllowed=1;
							}
						if (bAllowed) {
							itemP [choice].text [itemP [choice].value++] = ascii;
							itemP [choice].text [itemP [choice].value] = 0;
							itemP [choice].redraw=1;
							}
						}
					}
				}
			else if ((itemP [choice].nType != NM_TYPE_INPUT) && (itemP [choice].nType != NM_TYPE_INPUT_MENU)) {
				ascii = KeyToASCII (k);
				if (ascii < 255) {
					int choice1 = choice;
					ascii = toupper (ascii);
					do {
						int i, ch = 0, t;

						if (++choice1 >= nItems) 
							choice1=0;
						t = itemP [choice1].nType;
						if (gameStates.app.bNostalgia)
							ch = itemP [choice1].text [0];
						else if (itemP [choice1].key > 0)
							ch = MENU_KEY (itemP [choice1].key, 0);
						else if (itemP [choice1].key < 0) //skip any leading blanks
							for (i = 0; (ch = itemP [choice1].text [i]) && ch == ' '; i++)
								;
						else
							continue;
								;
						if (((t == NM_TYPE_MENU) ||
							  (t == NM_TYPE_CHECK) ||
							  (t == NM_TYPE_RADIO) ||
							  (t == NM_TYPE_NUMBER) ||
							  (t == NM_TYPE_SLIDER))
								&& (ascii == toupper (ch))) {
							k = 0;
							choice = choice1;
							if (old_choice>-1)
								itemP [old_choice].redraw = 1;
							itemP [choice].redraw = 1;
							if (choice < ctrl.nScrollOffset) {
								ctrl.nScrollOffset = choice;
								REDRAW_ALL;
								}
							else if (choice > ctrl.nScrollOffset + ctrl.nMaxDisplayable - 1) {
								ctrl.nScrollOffset = choice;
								if (ctrl.nScrollOffset + ctrl.nMaxDisplayable >= nItems) {
									ctrl.nScrollOffset = nItems - ctrl.nMaxDisplayable;
									if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
										ctrl.nScrollOffset = ctrl.nMaxNoScroll;
									}
								REDRAW_ALL;
								}
							if (t == NM_TYPE_CHECK)
								goto checkOption;
							else if (t == NM_TYPE_RADIO)
								goto radioOption;
							else if ((t != NM_TYPE_SLIDER) && (itemP [choice1].key != 0))
								if (gameOpts->menus.nHotKeys > 0)
									goto launchOption;
						}
					} while (choice1 != choice);
				}
			}

			if ((itemP [choice].nType==NM_TYPE_NUMBER) || (itemP [choice].nType==NM_TYPE_SLIDER)) 	{
				int ov=itemP [choice].value;
				switch (k) {
				case KEY_PAD4:
			  	case KEY_LEFT:
			  	case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					itemP [choice].value -= 1;
					break;
			  	case KEY_RIGHT:
				case KEY_PAD6:
			  	case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					itemP [choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					itemP [choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					itemP [choice].value -= 10;
					break;
				}
				if (ov!=itemP [choice].value)
					itemP [choice].redraw=1;
			}

		}


    	// Redraw everything...
#if 1
		bRedraw = 0;
		if (bRedrawAll && !gameOpts->menus.nStyle) {
			int t;
			backgroundManager.Draw ();
			t = NMDrawTitle (pszTitle, TITLE_FONT, RGBA_PAL (31, 31, 31), ctrl.yOffs);
			NMDrawTitle (pszSubTitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
			bRedrawAll = 0;
			}
		if (!gameOpts->menus.nStyle)
			CCanvas::SetCurrent (backgroundManager.Canvas ());
		fontManager.SetCurrent (ctrl.bTinyMode ? SMALL_FONT : NORMAL_FONT);
     	for (i = 0; i < ctrl.nMaxDisplayable + ctrl.nScrollOffset - ctrl.nMaxNoScroll; i++) {
			if ((i >= ctrl.nMaxNoScroll) && (i < ctrl.nScrollOffset))
				continue;
			if (!(gameOpts->menus.nStyle || (itemP [i].text && *itemP [i].text)))
				continue;
			if (bStart || (gameStates.ogl.nDrawBuffer == GL_BACK) || itemP [i].redraw || itemP [i].rebuild) {// warning! ugly hack below                  
				bRedraw = 1;
				if (itemP [i].rebuild && itemP [i].centered)
					itemP [i].x = fontManager.Current ()->GetCenteredX (itemP [i].text);
				if (i >= ctrl.nScrollOffset)
         		itemP [i].y -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
				if (!gameOpts->menus.nStyle) 
					SDL_ShowCursor (0);
           	NMDrawItem (itemP + i, (i == choice) && !bAllText, bTinyMode);
				itemP [i].redraw = 0;
				if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
					SDL_ShowCursor (1);
				if (i >= ctrl.nScrollOffset)
	            itemP [i].y += ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
	        	}   
         if ((i == choice) && 
				 ((itemP [i].nType == NM_TYPE_INPUT) || 
				 ((itemP [i].nType == NM_TYPE_INPUT_MENU) && itemP [i].group)))
				NMUpdateCursor (&itemP [i]);
			}
#endif
      if (ctrl.bIsScrollBox) {
      	//fontManager.SetCurrent (NORMAL_FONT);
        	if (bRedraw || (nLastScrollCheck != ctrl.nScrollOffset)) {
          	nLastScrollCheck=ctrl.nScrollOffset;
          	fontManager.SetCurrent (SELECTED_FONT);
          	sy=itemP [ctrl.nScrollOffset].y - ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
          	sx=itemP [ctrl.nScrollOffset].x - (gameStates.menus.bHires?24:12);
          	if (ctrl.nScrollOffset > ctrl.nMaxNoScroll)
           		NMRStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, UP_ARROW_MARKER);
          	else
           		NMRStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, "  ");
				i = ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll - 1;
          	sy=itemP [i].y - ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
          	sx=itemP [i].x - (gameStates.menus.bHires ? 24 : 12);
          	if (ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll < nItems)
           		NMRStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, DOWN_ARROW_MARKER);
          	else
	           	NMRStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, "  ");
		    	}
     		}   
		if (bCloseBox && (bStart || gameOpts->menus.nStyle)) {
			if (gameOpts->menus.nStyle)
				NMDrawCloseBox (ctrl.x, ctrl.y);
			else
				NMDrawCloseBox (ctrl.x - CCanvas::Current ()->Left (), ctrl.y - CCanvas::Current ()->Top ());
			bCloseBox = 1;
			}
		if (bRedraw || !gameOpts->menus.nStyle)
			GrUpdate (0);
		bRedraw = 1;
		bStart = 0;
		if (!bDontRestore && paletteManager.EffectDisabled ()) {
			paletteManager.EnableEffect ();
		}
	}
SDL_ShowCursor (0);
// Restore everything...
NMRestoreScreen (filename, bDontRestore);
NMFreeAllTextBms (itemP, nItems);
gameStates.input.keys.bRepeat = bKeyRepeat;
GameFlushInputs ();
if (time_stopped) {
	StartTime (0);
#ifdef TACTILE
		if (TactileStick)
			EnableForces ();
#endif
  }
if (sound_stopped)
	DigiResumeDigiSounds ();
gameStates.menus.nInMenu--;
paletteManager.SetEffect (0, 0, 0);
SDL_EnableKeyRepeat (0, 0);
if (gameStates.app.bGameRunning && IsMultiGame)
	MultiSendMsgQuit();
return choice;
}

//------------------------------------------------------------------------------

int _CDECL_ ExecMessageBox1 (
					const char *pszTitle, 
					int (*menuCallback) (int nItems, CMenuItem * itemP, int * lastKeyP, int nItem), 
					char *filename, int nChoices, ...)
{
	int i;
	char *format;
	va_list args;
	char *s;
	char nm_text [MESSAGEBOX_TEXT_SIZE];
	CMenuItem nmMsgItems [5];

va_start (args, nChoices);
Assert (nChoices <= 5);
memset (nmMsgItems, 0, sizeof (nmMsgItems));
for (i = 0; i < nChoices; i++)	{
   s = va_arg (args, char *);
   nmMsgItems [i].nType = NM_TYPE_MENU; 
	nmMsgItems [i].text = s;
	nmMsgItems [i].key = -1;
	}
format = va_arg (args, char *);
strcpy (nm_text, "");
vsprintf (nm_text, format, args);
va_end (args);
Assert (strlen (nm_text) < MESSAGEBOX_TEXT_SIZE);
return ExecMenu (pszTitle, nm_text, nChoices, nmMsgItems, menuCallback, filename);
}

//------------------------------------------------------------------------------

int _CDECL_ ExecMessageBox (const char *pszTitle, char *filename, int nChoices, ...)
{
	int				h, i, l, bTiny, nInMenu;
	char				*format, *s;
	va_list			args;
	char				nm_text [MESSAGEBOX_TEXT_SIZE];
	CMenuItem		nmMsgItems [10];


if (!nChoices)
	return -1;
if ((bTiny = (nChoices < 0)))
	nChoices = -nChoices;
va_start (args, nChoices);
memset (nmMsgItems, 0, h = nChoices * sizeof (CMenuItem));
for (i = l = 0; i < nChoices; i++) {
	s = va_arg (args, char *);
	h = (int) strlen (s);
	if (l + h > MESSAGEBOX_TEXT_SIZE)
		break;
	l += h;
	if (!bTiny || i) 
		nmMsgItems [i].nType = NM_TYPE_MENU; 
	else {
		nmMsgItems [i].nType = NM_TYPE_TEXT; 
		nmMsgItems [i].unavailable = 1; 
		}
	nmMsgItems [i].text = s;
	nmMsgItems [i].key = (bTiny && !i) ? 0 : -1;
	if (bTiny)
		nmMsgItems [i].centered = (i != 0);
	}
if (!bTiny) {
	format = va_arg (args, char *);
	vsprintf (nm_text, format, args);
	va_end (args);
	}
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
i = bTiny ? 
	 ExecMenuTiny (NULL, pszTitle, nChoices, nmMsgItems, NULL) :
	 ExecMenu (pszTitle, nm_text, nChoices, nmMsgItems, NULL, filename);
gameStates.menus.nInMenu = nInMenu;
return i;
}

//------------------------------------------------------------------------------

void NMFileSort (int n, char *list)
{
	int i, j, incr;
	char t [FILENAME_LEN];

	incr = n / 2;
	while (incr > 0)		{
		for (i = incr; i < n; i++)		{
			j = i - incr;
			while (j>=0)			{
				if (strncmp (&list [j* (FILENAME_LEN + 1)], &list [ (j+incr)* (FILENAME_LEN + 1)], FILENAME_LEN - 1) > 0) {
               memcpy (t, &list [j* (FILENAME_LEN + 1)], FILENAME_LEN);
               memcpy (&list [j* (FILENAME_LEN + 1)], &list [ (j+incr)* (FILENAME_LEN + 1)], FILENAME_LEN);
               memcpy (&list [ (j+incr)* (FILENAME_LEN + 1)], t, FILENAME_LEN);
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}

//------------------------------------------------------------------------------

void DeletePlayerSavedGames (char *name)
{
	int i;
	char filename [16];

for (i = 0; i < 10; i++) {
	sprintf (filename, "%s.sg%d", name, i);
	CFile::Delete (filename, gameFolders.szSaveDir);
	}
}

//------------------------------------------------------------------------------

#define MAX_FILES 300

int ExecMenuFileSelector (const char *pszTitle, const char *filespec, char *filename, int bAllowAbort)
{
	int i;
	FFS ffs;
	int nFileCount = 0, key, done, nItem, ocitem;
	char *filenames = NULL;
	int nFilesDisplayed = 8;
	int nFirstItem = -1, nPrevItem;
	int bKeyRepeat = gameStates.input.keys.bRepeat;
	int bPlayerMode = 0;
	int bDemoMode = 0;
	int bDemosDeleted = 0;
	int bInitialized = 0;
	int exitValue = 0;
	int w_x, w_y, w_w, w_h, nTitleHeight;
	int box_x, box_y, box_w, box_h;
	int mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;
	int mouse2State, omouse2State, bWheelUp, bWheelDown;
	int bDblClick = 0;
	char szPattern [40];
	int nPatternLen = 0;
	char *pszFn;

w_x = w_y = w_w = w_h = nTitleHeight = 0;
box_x = box_y = box_w = box_h = 0;
if (!(filenames = new char [MAX_FILES * (FILENAME_LEN + 1)]))
	return 0;
nItem = 0;
gameStates.input.keys.bRepeat = 1;

if (strstr (filespec, "*.plr"))
	bPlayerMode = 1;
else if (strstr (filespec, "*.dem"))
	bDemoMode = 1;

ReadFileNames:

done = 0;
nFileCount = 0;

#if !defined (APPLE_DEMO)		// no new pilots for special apple oem version
if (bPlayerMode)	{
	strncpy (filenames + nFileCount * (FILENAME_LEN + 1), TXT_CREATE_NEW, FILENAME_LEN);
	nFileCount++;
	}
#endif

if (!FFF (filespec, &ffs, 0))	{
	pszFn = filenames + nFileCount * (FILENAME_LEN + 1);
	do	{
		if (nFileCount < MAX_FILES)	{
         strncpy (pszFn, ffs.name, FILENAME_LEN);
			if (bPlayerMode)	{
				char *p = strchr (pszFn, '.');
				if (p) 
					*p = '\0';
			}
			if (*pszFn) {
				strlwr (pszFn);
				nFileCount++;
				}
		} else {
			break;
		}
	pszFn += (FILENAME_LEN + 1);
	} while (!FFN (&ffs, 0));
	FFC (&ffs);
	}
if (bDemoMode && gameFolders.bAltHogDirInited) {
	char filespec2 [PATH_MAX + FILENAME_LEN];
	sprintf (filespec2, "%s/%s", gameFolders.szAltHogDir, filespec);
	if (!FFF (filespec2, &ffs, 0)) {
		do {
			if (nFileCount<MAX_FILES)	{
				strncpy (filenames+nFileCount* (FILENAME_LEN + 1), ffs.name, FILENAME_LEN);
				nFileCount++;
			} else {
				break;
			}
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	}

if ((nFileCount < 1) && bDemosDeleted)	{
	exitValue = 0;
	goto exitFileMenu;
	}
if ((nFileCount < 1) && bDemoMode) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
	exitValue = 0;
	goto exitFileMenu;
	}

if (nFileCount < 1) {
#ifndef APPLE_DEMO
	ExecMessageBox (NULL, NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
#endif
	exitValue = 0;
	goto exitFileMenu;
	}

if (!bInitialized) {
//		SetScreenMode (SCREEN_MENU);
	SetPopupScreenMode ();
	CCanvas::SetCurrent (NULL);
	fontManager.SetCurrent (SUBTITLE_FONT);
	w_w = 0;
	w_h = 0;

	for (i = 0; i < nFileCount; i++) {
		int w, h, aw;
		fontManager.Current ()->StringSize (filenames+i* (FILENAME_LEN+1), w, h, aw);	
		if (w > w_w)
			w_w = w;
		}
	if (pszTitle) {
		int w, h, aw;
		fontManager.Current ()->StringSize (pszTitle, w, h, aw);	
		if (w > w_w)
			w_w = w;
		nTitleHeight = h + (CCanvas::Current ()->Font ()->Height ()*2);		// add a little space at the bottom of the pszTitle
		}

	box_w = w_w;
	box_h = ((CCanvas::Current ()->Font ()->Height () + 2) * nFilesDisplayed);

	w_w += (CCanvas::Current ()->Font ()->Width () * 4);
	w_h = nTitleHeight + box_h + (CCanvas::Current ()->Font ()->Height () * 2);		// more space at bottom

	if (w_w > CCanvas::Current ()->Width ()) 
		w_w = CCanvas::Current ()->Width ();
	if (w_h > CCanvas::Current ()->Height ()) 
		w_h = CCanvas::Current ()->Height ();
	if (w_w > 640)
		w_w = 640;
	if (w_h > 480)
		w_h = 480;

	w_x = (CCanvas::Current ()->Width ()-w_w)/2;
	w_y = (CCanvas::Current ()->Height ()-w_h)/2;

	if (w_x < 0) 
		w_x = 0;
	if (w_y < 0) 
		w_y = 0;

	box_x = w_x + (CCanvas::Current ()->Font ()->Width ()*2);			// must be in sync with w_w!!!
	box_y = w_y + nTitleHeight;

// save the screen behind the menu.

	CCanvas::Push ();
	backgroundManager.Setup (NULL, w_x, w_y, w_w, w_h);
	CCanvas::Pop ();
	GrString (0x8000, w_y+10, pszTitle, NULL);
	bInitialized = 1;
	}

if (!bPlayerMode)
	NMFileSort (nFileCount, filenames);
else {
	NMFileSort (nFileCount-1, filenames+ (FILENAME_LEN+1));		// Don't sort first one!
	for (i = 0; i < nFileCount; i++)	{
		if (!stricmp (LOCALPLAYER.callsign, filenames+i* (FILENAME_LEN+1)))	{
			bDblClick = 1;
			nItem = i;
			}
	 	}
	}

nMouseState = nOldMouseState = 0;
mouse2State = omouse2State = 0;
NMDrawCloseBox (w_x, w_y);
SDL_ShowCursor (1);

SDL_EnableKeyRepeat(60, 30);
while (!done)	{
	ocitem = nItem;
	nPrevItem = nFirstItem;
	GrUpdate (0);
	nOldMouseState = nMouseState;
	omouse2State = mouse2State;
	nMouseState = MouseButtonState (0);
	mouse2State = MouseButtonState (1);
	bWheelUp = MouseButtonState (3);
	bWheelDown = MouseButtonState (4);
	//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();
	if (bWheelUp)
		key = KEY_UP;
	else if (bWheelDown)
		key = KEY_DOWN;
	else
		key = KeyInKey ();
	switch (key) {
	case KEY_CTRLED+KEY_F1:
		SwitchDisplayMode (-1);
		break;
		
	case KEY_CTRLED+KEY_F2:
		SwitchDisplayMode (1);
		break;
		
	case KEY_COMMAND+KEY_SHIFTED+KEY_P:
	case KEY_ALTED + KEY_F9:
		gameStates.app.bSaveScreenshot = 1;
		SaveScreenShot (NULL, 0);
		break;

	case KEY_CTRLED+KEY_S:
		if (gameStates.app.bNostalgia)
			gameOpts->menus.bSmartFileSearch = 0;
		else
			gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
		break;

	case KEY_CTRLED+KEY_D:
		if (((bPlayerMode) && (nItem > 0)) || ((bDemoMode) && (nItem >= 0))) {
			int x = 1;
			char *pszFile = filenames + nItem * (FILENAME_LEN+1);
			if (*pszFile == '$')
				pszFile++;
			SDL_ShowCursor (0);
			if (bPlayerMode)
				x = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, pszFile);
			else if (bDemoMode)
				x = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, pszFile);
			SDL_ShowCursor (1);
			if (!x)	{
				char *p;
				int ret;

				p = pszFile + strlen (pszFile);
				if (bPlayerMode)
					*p = '.';
				ret = CFile::Delete (pszFile, bPlayerMode ? gameFolders.szProfDir : gameFolders.szDemoDir);
				if (bPlayerMode) {
					if (!ret) {
						p [3] = 'x';	//turn ".plr" to ".plx"
						CFile::Delete (pszFile, gameFolders.szProfDir);
						*p = 0;
						DeletePlayerSavedGames (pszFile);
						}
					*p = 0;
					}

				if (ret) {
					if (bPlayerMode)
						ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, pszFile);
					else if (bDemoMode)
						ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, pszFile);
					}
				else if (bDemoMode)
					bDemosDeleted = 1;
				nFirstItem = -1;
				goto ReadFileNames;
			}
		}
		break;
	case KEY_HOME:
	case KEY_PAD7:
		nItem = 0;
		break;
	case KEY_END:
	case KEY_PAD1:
		nItem = nFileCount-1;
		break;
	case KEY_UP:
	case KEY_PAD8:
		nItem--;		
		break;
	case KEY_DOWN:
	case KEY_PAD2:
		nItem++;		
		break;
	case KEY_PAGEDOWN:
	case KEY_PAD3:
		nItem += nFilesDisplayed;
		break;
	case KEY_PAGEUP:
	case KEY_PAD9:
		nItem -= nFilesDisplayed;
		break;
	case KEY_ESC:
		if (bAllowAbort) {
			nItem = -1;
			done = 1;
		}
		break;
	case KEY_ENTER:
	case KEY_PADENTER:
		done = 1;
		break;

	case KEY_BACKSP:
		if (!gameOpts->menus.bSmartFileSearch)
			break;
		if (nPatternLen > 0)
			szPattern [--nPatternLen] = '\0';
			
	default:
		if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < (int) sizeof (szPattern) - 1)) {
			int nStart, ascii = KeyToASCII (key);
			if ((key == KEY_BACKSP) || (ascii < 255)) {
				int cc, bFound = 0;
				if (!gameOpts->menus.bSmartFileSearch) {
					cc = nItem + 1;
					nStart = nItem;
					if (cc < 0)  
						cc = 0;
					else if (cc >= nFileCount)  
						cc = 0;
					}
				else {
					cc = 0;
					nStart = 0;
					if (key != KEY_BACKSP) {
						szPattern [nPatternLen++] = tolower (ascii);
						szPattern [nPatternLen] = '\0';
						}
					}
				do {
					pszFn = filenames + cc * (FILENAME_LEN + 1);
					if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == toupper (ascii)) {
						nItem = cc;
						bFound = 1;
						break;
					}
					cc++;
					cc %= nFileCount;
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound)
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
	if (done) 
		break;

	if (nItem < 0)
		nItem = 0;
	if (nItem >= nFileCount)
		nItem = nFileCount - 1;

	if (nItem < nFirstItem)
		nFirstItem = nItem;

	if (nItem >= (nFirstItem + nFilesDisplayed))
		nFirstItem = nItem - nFilesDisplayed + 1;
	if (nFileCount <= nFilesDisplayed)
		 nFirstItem = 0;
	if (nFirstItem>nFileCount-nFilesDisplayed)
		nFirstItem = nFileCount-nFilesDisplayed;
	if (nFirstItem < 0) 
		nFirstItem = 0;

	if (nMouseState || mouse2State) {
		int w, h, aw;

		MouseGetPos (&mx, &my);
		for (i = nFirstItem; i < nFirstItem+nFilesDisplayed; i++)	{
			fontManager.Current ()->StringSize (&filenames [i* (FILENAME_LEN+1)], w, h, aw);
			x1 = box_x;
			x2 = box_x + box_w - 1;
			y1 = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
			y2 = y1+h+1;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (i == nItem && !mouse2State)
					break;
				nItem = i;
				bDblClick = 0;
				break;
				}
			}
		}

	if (!nMouseState && nOldMouseState) {
		int w, h, aw;

		fontManager.Current ()->StringSize (&filenames [nItem* (FILENAME_LEN+1)], w, h, aw);
		MouseGetPos (&mx, &my);
		x1 = box_x;
		x2 = box_x + box_w - 1;
		y1 = (nItem-nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
		y2 = y1+h+1;
		if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
			if (bDblClick) 
				done = 1;
			else 
				bDblClick = 1;
			}
		}

	if (!nMouseState && nOldMouseState) {
		MouseGetPos (&mx, &my);
		x1 = w_x + CLOSE_X + 2;
		x2 = x1 + CLOSE_SIZE - 2;
		y1 = w_y + CLOSE_Y + 2;
		y2 = y1 + CLOSE_SIZE - 2;
		if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
			nItem = -1;
			done = 1;
			}
		}

	if ((nPrevItem != nFirstItem) || gameOpts->menus.nStyle) {
		if (!gameOpts->menus.nStyle) 
			SDL_ShowCursor (0);
		backgroundManager.Draw ();
		fontManager.SetCurrent (NORMAL_FONT);
		GrString (0x8000, w_y+10, pszTitle, NULL);
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		for (i = nFirstItem; i < nFirstItem + nFilesDisplayed; i++) {
			int w, h, aw, y;
			y = (i-nFirstItem) * (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
	
			if (i >= nFileCount)	{
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
				GrRect (box_x + box_w, y-1, box_x + box_w, y + CCanvas::Current ()->Font ()->Height () + 1);
				//GrRect (box_x, y + CCanvas::Current ()->Font ()->Height () + 2, box_x + box_w, y + CCanvas::Current ()->Font ()->Height () + 2);
			
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
				GrRect (box_x - 1, y - 1, box_x - 1, y + CCanvas::Current ()->Font ()->Height () + 2);
			
				CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
				GrRect (box_x, y - 1, box_x + box_w - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
				} 
			else {
				fontManager.SetCurrent ((i == nItem) ? SELECTED_FONT : NORMAL_FONT);
				fontManager.Current ()->StringSize (&filenames [i* (FILENAME_LEN+1)], w, h, aw);
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
				GrRect (box_x + box_w, y - 1, box_x + box_w, y + h + 1);
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
				GrRect (box_x - 1, y - 1, box_x - 1, y + h + 1);
				CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
				GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
				GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((bPlayerMode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0), NULL);
				}
			}	 
		SDL_ShowCursor (1);
		}
	else if (nItem != ocitem)	{
		int w, h, aw, y;

		if (!gameOpts->menus.nStyle) 
			SDL_ShowCursor (0);
		i = ocitem;
		if ((i >= 0) && (i < nFileCount))	{
			y = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height ()+2)+box_y;
			if (i == nItem)
				fontManager.SetCurrent (SELECTED_FONT);
			else
				fontManager.SetCurrent (NORMAL_FONT);
			fontManager.Current ()->StringSize (&filenames [i* (FILENAME_LEN+1)], w, h, aw);
			GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
			GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((bPlayerMode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0), NULL);
			}
		i = nItem;
		if ((i>=0) &&(i < nFileCount))	{
			y = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height ()+2)+box_y;
			if (i == nItem)
				fontManager.SetCurrent (SELECTED_FONT);
			else
				fontManager.SetCurrent (NORMAL_FONT);
			fontManager.Current ()->StringSize (&filenames [i* (FILENAME_LEN+1)], w, h, aw);
			GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
			GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((bPlayerMode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0), NULL);
			}
		GrUpdate (0);
		SDL_ShowCursor (1);
		}
	}

//exitFileMenuEarly:
if (nItem < 0)
	exitValue = 0;
else {
	strncpy (filename, (&filenames [nItem* (FILENAME_LEN+1)])+ ((bPlayerMode && filenames [nItem* (FILENAME_LEN+1)]=='$')?1:0), FILENAME_LEN);
	exitValue = 1;
	}										 

exitFileMenu:
gameStates.input.keys.bRepeat = bKeyRepeat;

if (bInitialized) {
	backgroundManager.Remove ();
	GrUpdate (0);
	}

if (filenames)
	delete [] filenames;

SDL_EnableKeyRepeat(0, 0);
return exitValue;
}

//------------------------------------------------------------------------------

#define LB_ITEMS_ON_SCREEN 8

int ExecMenuListBox (const char *pszTitle, int nItems, char *itemP [], int bAllowAbort, 
							int (*listbox_callback) (int *nItemP, int *nItems, char *itemP [], int *keypress))
{
return ExecMenuListBox1 (pszTitle, nItems, itemP, bAllowAbort, 0, listbox_callback);
}

//------------------------------------------------------------------------------

int ExecMenuListBox1 (const char *pszTitle, int nItems, char *itemP [], int bAllowAbort, int nDefaultItem, 
							 int (*listbox_callback) (int *nItemP, int *nItems, char *itemP [], int *keypress))
{
	int i;
	int done, ocitem, nItem, nPrevItem, nFirstItem, key, redraw;
	int bKeyRepeat = gameStates.input.keys.bRepeat;
	int width, height, wx, wy, nTitleHeight, border_size;
	int total_width, total_height;
	int mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;	//, bDblClick;
	int close_x, close_y, bWheelUp, bWheelDown;
	int nItemsOnScreen;
	char szPattern [40];
	int nPatternLen = 0;
	char *pszFn;

	gameStates.input.keys.bRepeat = 1;

//	SetScreenMode (SCREEN_MENU);
	SetPopupScreenMode ();
	CCanvas::SetCurrent (NULL);
	fontManager.SetCurrent (SUBTITLE_FONT);

	width = 0;
	for (i = 0; i < nItems; i++)	{
		int w, h, aw;
		fontManager.Current ()->StringSize (itemP [i], w, h, aw);	
		if (w > width)
			width = w;
	}
	nItemsOnScreen = LB_ITEMS_ON_SCREEN * CCanvas::Current ()->Height () / 480;
	height = (CCanvas::Current ()->Font ()->Height () + 2) * nItemsOnScreen;
	{
		int w, h, aw;
		fontManager.Current ()->StringSize (pszTitle, w, h, aw);	
		if (w > width)
			width = w;
		nTitleHeight = h + 5;
	}

	border_size = CCanvas::Current ()->Font ()->Width ();
	
	width += (CCanvas::Current ()->Font ()->Width ());
	if (width > CCanvas::Current ()->Width () - (CCanvas::Current ()->Font ()->Width () * 3))
		width = CCanvas::Current ()->Width () - (CCanvas::Current ()->Font ()->Width () * 3);

	wx = (CCanvas::Current ()->Width ()-width)/2;
	wy = (CCanvas::Current ()->Height ()- (height+nTitleHeight))/2 + nTitleHeight;
	if (wy < nTitleHeight)
		wy = nTitleHeight;

	total_width = width+2*border_size;
	total_height = height+2*border_size+nTitleHeight;

	CCanvas::Push ();
	backgroundManager.Setup (NULL, wx - border_size, wy - nTitleHeight - border_size, 
									 width + border_size * 2, height + nTitleHeight + border_size * 2);
	GrUpdate (0);
	CCanvas::Pop ();
	GrString (0x8000, wy - nTitleHeight, pszTitle, NULL);
	done = 0;
	nItem = nDefaultItem;
	if (nItem < 0) 
		nItem = 0;
	if (nItem >= nItems) 
		nItem = 0;

	nFirstItem = -1;

	nMouseState = nOldMouseState = 0;	//bDblClick = 0;
	close_x = wx-border_size;
	close_y = wy-nTitleHeight-border_size;
	NMDrawCloseBox (close_x, close_y);
	SDL_ShowCursor (1);

	SDL_EnableKeyRepeat(60, 30);
	while (!done)	{
		ocitem = nItem;
		nPrevItem = nFirstItem;
		nOldMouseState = nMouseState;
		nMouseState = MouseButtonState (0);
		bWheelUp = MouseButtonState (3);
		bWheelDown = MouseButtonState (4);
		//see if redbook song needs to be restarted
		SongsCheckRedbookRepeat ();

		if (bWheelUp)
			key = KEY_UP;
		else if (bWheelDown)
			key = KEY_DOWN;
		else
			key = KeyInKey ();

		if (listbox_callback)
			redraw = (*listbox_callback) (&nItem, &nItems, itemP, &key);
		else
			redraw = 0;

		if (key<-1) {
			nItem = key;
			key = -1;
			done = 1;
		}


		switch (key)	{
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_CTRLED+KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_PRINT_SCREEN: 	
			SaveScreenShot (NULL, 0); 
			break;
		case KEY_HOME:
		case KEY_PAD7:
			nItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			nItem = nItems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			nItem--;		
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			nItem++;		
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			nItem += nItemsOnScreen;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			nItem -= nItemsOnScreen;
			break;
		case KEY_ESC:
			if (bAllowAbort) {
				nItem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		case KEY_BACKSP:
			if (!gameOpts->menus.bSmartFileSearch)
				break;
			if (nPatternLen > 0)
				szPattern [--nPatternLen] = '\0';
				
		default:
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < (int) sizeof (szPattern) - 1)) {
				int nStart,
					 ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						nStart = nItem;
						cc = nItem + 1;
						if (cc < 0)  
							cc = 0;
						else if (cc >= nItems)  
							cc = 0;
						}
					else {
						nStart = 0;
						cc = 0;
						if (key != KEY_BACKSP) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = itemP [cc];
						if (itemP [cc][0] == '[') {
							if (((itemP [cc][1] == '1') || (itemP [cc][1] == '2')) && (itemP [cc][2] == ']'))
								pszFn += 4;
							else
								pszFn++;
							}
						strlwr (pszFn);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == tolower (ascii)) {
							nItem = cc;
							bFound = 1;
							break;
							}
						cc++;
						cc %= nItems;
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound && (nPatternLen > 0))
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
		if (done) break;

		if (nItem < 0)
			nItem = nItems - 1;
		else if (nItem >= nItems)
			nItem = 0;
		if (nItem < nFirstItem)
			nFirstItem = nItem;
		else if (nItem >= (nFirstItem + nItemsOnScreen))
			nFirstItem = nItem - nItemsOnScreen + 1;
		if (nItems <= nItemsOnScreen)
			 nFirstItem = 0;
		if (nFirstItem > nItems - nItemsOnScreen)
			nFirstItem = nItems - nItemsOnScreen;
		if (nFirstItem < 0) 
			nFirstItem = 0;

		if (nMouseState) {
			int w, h, aw;

			MouseGetPos (&mx, &my);
			for (i = nFirstItem; i < nFirstItem + nItemsOnScreen; i++) {
				if (i > nItems)
					break;
				fontManager.Current ()->StringSize (itemP [i], w, h, aw);
				x1 = wx;
				x2 = wx + width;
				y1 = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height ()+2)+wy;
				y2 = y1+h+1;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					//if (i == nItem) {
					//	break;
					//}
					//bDblClick= 0;
					nItem = i;
					done = 1;
					break;
				}
			}
		}

		//check for close box clicked
		if (!nMouseState && nOldMouseState) {
			MouseGetPos (&mx, &my);
			x1 = close_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = close_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				nItem = -1;
				done = 1;
			}
		}

		if ((nPrevItem != nFirstItem) || redraw || gameOpts->menus.nStyle) {
			if (gameOpts->menus.nStyle) 
				backgroundManager.Draw ();
				//NMDrawBackground (wx-border_size, wy-nTitleHeight-border_size, wx+width+border_size-1, wy+height+border_size-1,1);
			else
				SDL_ShowCursor (0);
			if (gameOpts->menus.nStyle) {
				fontManager.SetCurrent (NORMAL_FONT);
				GrString (0x8000, wy - nTitleHeight, pszTitle, NULL);
				}

			CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
			for (i = nFirstItem; i < nFirstItem + nItemsOnScreen; i++)	{
				int w, h, aw, y;
				y = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height ()+2)+wy;
				if (i >= nItems)	{
					CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
					GrRect (wx, y-1, wx+width-1, y+CCanvas::Current ()->Font ()->Height () + 1);
				} else {
					if (i == nItem)
						fontManager.SetCurrent (SELECTED_FONT);
					else
						fontManager.SetCurrent (NORMAL_FONT);
					fontManager.Current ()->StringSize (itemP [i], w, h, aw);
					GrRect (wx, y-1, wx+width-1, y+h+1);
					GrString (wx+5, y, itemP [i], NULL);
				}
			}	

			
			// If Win95 port, draw up/down arrows on left CSide of menu
			SDL_ShowCursor (1);
			GrUpdate (0);
		} else if (nItem != ocitem)	{
			int w, h, aw, y;

			if (!gameOpts->menus.nStyle) 
				SDL_ShowCursor (0);

			i = ocitem;
			if ((i>=0) &&(i < nItems))	{
				y = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height ()+2)+wy;
				if (i == nItem)
					fontManager.SetCurrent (SELECTED_FONT);
				else
					fontManager.SetCurrent (NORMAL_FONT);
				fontManager.Current ()->StringSize (itemP [i], w, h, aw);
				GrRect (wx, y-1, wx+width-1, y+h+1);
				GrString (wx+5, y, itemP [i], NULL);

			}
			i = nItem;
			if ((i>=0) &&(i < nItems))	{
				y = (i-nFirstItem)* (CCanvas::Current ()->Font ()->Height ()+2)+wy;
				if (i == nItem)
					fontManager.SetCurrent (SELECTED_FONT);
				else
					fontManager.SetCurrent (NORMAL_FONT);
				fontManager.Current ()->StringSize (itemP [i], w, h, aw);
				GrRect (wx, y-1, wx+width-1, y+h);
				GrString (wx+5, y, itemP [i], NULL);
			}
			SDL_ShowCursor (1);
			GrUpdate (0);
		}
	}

gameStates.input.keys.bRepeat = bKeyRepeat;
backgroundManager.Remove ();
GrUpdate (0);
SDL_EnableKeyRepeat(0, 0);
return nItem;
}

//------------------------------------------------------------------------------

static char FilenameText [MAX_FILES][ (FILENAME_LEN+1)];

int ExecMenuFileList (char *pszTitle, char *filespec, char *filename)
{
	int i, NumFiles;
	char *Filenames [MAX_FILES];
	FFS  ffs;

	NumFiles = 0;
	if (!FFF (filespec, &ffs, 0))	{
		do	{
			if (NumFiles<MAX_FILES)	{
            strncpy (FilenameText [NumFiles], ffs.name, FILENAME_LEN);
				Filenames [NumFiles] = FilenameText [NumFiles];
				NumFiles++;
			} else {
				break;
			}
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
	}

	i = ExecMenuListBox (pszTitle, NumFiles, Filenames, 1, NULL);
	if (i > -1)	{
		strcpy (filename, Filenames [i]);
		return 1;
	} 
	return 0;
}

//------------------------------------------------------------------------------
//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int _CDECL_ NMMsgBoxFixedFont (char *pszTitle, int nChoices, ...)
{
	int i;
	char *format;
	va_list args;
	char *s;
	char nm_text [MESSAGEBOX_TEXT_SIZE];
	CMenuItem nmMsgItems [5];

	va_start (args, nChoices);

	Assert (nChoices <= 5);

	memset (nmMsgItems, 0, sizeof (nmMsgItems));
	for (i = 0; i < nChoices; i++)	{
		s = va_arg (args, char *);
		nmMsgItems [i].nType = NM_TYPE_MENU; nmMsgItems [i].text = s;
	}
	format = va_arg (args, char *);
	//sprintf (	  nm_text, ""); // adb: ?
	vsprintf (nm_text, format, args);
	va_end (args);

	Assert (strlen (nm_text) < MESSAGEBOX_TEXT_SIZE);

   return ExecMenuFixedFont (pszTitle, nm_text, nChoices, nmMsgItems, NULL, 0, NULL, -1, -1);
}
//end this section addition - Victor Rachels

//------------------------------------------------------------------------------
/* Spiffy word wrap string formatting function */

void NMWrapText (char *dbuf, char *sbuf, int line_length)
{
	int col;
	char *wordptr;
	char *tbuf;

	tbuf = new char [strlen (sbuf)+1];
	strcpy (tbuf, sbuf);

	wordptr = strtok (tbuf, " ");
	if (!wordptr) return;
	col = 0;
	dbuf [0] = 0;

while (wordptr) {
	col = col + (int) strlen (wordptr)+1;
	if (col >=line_length) {
		col = 0;
		sprintf (dbuf, "%s\n%s ", dbuf, wordptr);
		}
	else {
		sprintf (dbuf, "%s%s ", dbuf, wordptr);
		}
	wordptr = strtok (NULL, " ");
	}
delete [] tbuf;
}

//------------------------------------------------------------------------------

void NMProgressBar (const char *szCaption, int nCurProgress, int nMaxProgress, 
						  int (*doProgress) (int nItems, CMenuItem *itemP, int *lastKeyP, int nCurItemP))
{
	CMenuItem	m [3];
	int			i, nInMenu;

memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, nMaxProgress); 
m [0].centered = 1;
m [0].value = nCurProgress;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameData.app.bGamePaused = 1;
do {
	i = ExecMenu2 (NULL, szCaption, 1, m, doProgress, 0, NULL);
	} while (i >= 0);
gameData.app.bGamePaused = 0;
gameStates.menus.nInMenu = nInMenu;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CMenuManager::AddCheck (char* szText, int nValue, int nKey, char* szHelp)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_CHECK;
item.text = szText;
item.value = NMBOOL (nValue);
item.key = nKey;
item.szHelp = szHelp;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddRadio (char* szText, int nValue, int nGroup, int nKey, char* szHelp)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_RADIO;
item.text = szText;
item.value = nValue;
item.key = nKey;
item.group=nGroup;
item.szHelp = szHelp;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddMenu (char* szText, int nKey, char* szHelp)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_MENU;
item.text = szText;
item.key = nKey;
item.szHelp = szHelp;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddText (char* szText, int nKey)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_TEXT;
item.text = szText;
item.key = nKey;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddSlider (char* szText, int nValue, int nMin, int nMax, int nKey, char* szHelp)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_SLIDER;
item.text = szText;
item.value = NMCLAMP (nValue, nMin, nMax);
item.minValue = nMin;
item.maxValue = nMax;
item.key = nKey;
item.szHelp = szHelp;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddInput (char* szText, int nLen, char* szHelp)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_INPUT;
item.text = szText;
item.nTextLen = nLen;
item.szHelp = szHelp;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddInputBox (char* szText, int nLen, int nKey, char* szHelp)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_INPUT_MENU;
item.text = szText;
item.nTextLen = nLen;
item.key = nKey;
item.szHelp = szHelp;
return m_opt++;
}

//------------------------------------------------------------------------------

int CMenuManager::AddGauge (char* szText, int nValue, int nMax)
{
CMenuItem& item = Item ();
item.nType = NM_TYPE_GAUGE;
item.text = szText;
item.nTextLen = *szText ? (int) strlen (szText) : 20;
item.value = NMCLAMP (nValue, 0, nMax);
item.maxValue = nMax;
return m_opt++;
}

//------------------------------------------------------------------------------
