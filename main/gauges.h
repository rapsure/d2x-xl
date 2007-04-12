/* $Id: gauges.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _GAUGES_H
#define _GAUGES_H

#include "fix.h"
#include "gr.h"
#include "piggy.h"
#include "object.h"
#include "hudmsg.h"
//from gauges.c

#define MAX_GAUGE_BMS 100   // increased from 56 to 80 by a very unhappy MK on 10/24/94.
#define D1_MAX_GAUGE_BMS 80   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

extern tBitmapIndex Gauges[MAX_GAUGE_BMS];      // Array of all gauge bitmaps.
extern tBitmapIndex Gauges_hires[MAX_GAUGE_BMS];    // hires gauges

// Flags for gauges/hud stuff
extern ubyte Reticle_on;

extern void InitGaugeCanvases();
extern void CloseGaugeCanvases();

extern void show_score();
extern void show_score_added();
extern void AddPointsToScore();
extern void AddBonusPointsToScore();

void RenderGauges(void);
void InitGauges(void);
extern void check_erase_message(void);

extern void HUDRenderMessageFrame();
extern void HUDClearMessages();

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
#define gauge_message HUDInitMessage

extern void DrawHUD();     // draw all the HUD stuff

extern void PlayerDeadMessage(void);
//extern void say_afterburner_status(void);

// fills in the coords of the hostage video window
void get_hostage_window_coords(int *x, int *y, int *w, int *h);

// from testgaug.c

void gauge_frame(void);
extern void UpdateLaserWeaponInfo(void);
extern void PlayHomingWarning(void);

typedef struct {
	ubyte r,g,b;
} rgb;

extern rgb player_rgb[];

#define WBU_WEAPON      0       // the weapons display
#define WBUMSL     1       // the missile view
#define WBU_ESCORT      2       // the "buddy bot"
#define WBU_REAR        3       // the rear view
#define WBU_COOP        4       // coop or team member view
#define WBU_GUIDED      5       // the guided missile
#define WBU_MARKER      6       // a dropped marker
#define WBU_STATIC      7       // playing static after missile hits
#define WBU_RADAR_TOPDOWN 8
#define WBU_RADAR_HEADSUP 9

// draws a 3d view into one of the cockpit windows.  win is 0 for
// left, 1 for right.  viewer is tObject.  NULL tObject means give up
// window user is one of the WBU_ constants.  If rearViewFlag is
// set, show a rear view.  If label is non-NULL, print the label at
// the top of the window.
void DoCockpitWindowView(int win, tObject *viewer, int rearViewFlag, int user, char *label);
void FreeInventoryIcons (void);
void FreeObjTallyIcons (void);
void HUDShowIcons (void);
int CanSeeObject(int nObject, int bCheckObjs);

#define SHOW_COCKPIT	((gameStates.render.cockpit.nMode != CM_FULL_SCREEN) && (gameStates.render.cockpit.nMode != CM_LETTERBOX))
#define SHOW_HUD		(gameOpts->render.cockpit.bHUD || !SHOW_COCKPIT)

#endif /* _GAUGES_H */
