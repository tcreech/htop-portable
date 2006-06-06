/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_Meter
#define HEADER_Meter
/*
htop - Meter.h
(C) 2004-2006 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#define _GNU_SOURCE
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#include <stdarg.h>

#include "Object.h"
#include "CRT.h"
#include "ListItem.h"
#include "String.h"
#include "ProcessList.h"

#include "debug.h"
#include <assert.h>

#ifndef USE_FUNKY_MODES
#define USE_FUNKY_MODES 1
#endif

#define METER_BUFFER_LEN 128


typedef struct Meter_ Meter;
typedef struct MeterType_ MeterType;
typedef struct MeterMode_ MeterMode;

typedef void(*MeterType_Init)(Meter*);
typedef void(*MeterType_Done)(Meter*);
typedef void(*MeterType_SetMode)(Meter*, int);
typedef void(*Meter_SetValues)(Meter*, char*, int);
typedef void(*Meter_Draw)(Meter*, int, int, int);

struct MeterMode_ {
   Meter_Draw draw;
   char* uiName;
   int h;
};

struct MeterType_ {
   Meter_SetValues setValues;
   Object_Display display;
   int mode;
   int items;
   double total;
   int* attributes;
   char* name;
   char* uiName;
   char* caption;
   MeterType_Init init;
   MeterType_Done done;
   MeterType_SetMode setMode;
   Meter_Draw draw;
};

struct Meter_ {
   Object super;
   char* caption;
   MeterType* type;
   int mode;
   int param;
   Meter_Draw draw;
   void* drawBuffer;
   int h;
   ProcessList* pl;
   double* values;
   double total;
};

typedef enum {
   CUSTOM_METERMODE = 0,
   BAR_METERMODE,
   TEXT_METERMODE,
#ifdef USE_FUNKY_MODES
   GRAPH_METERMODE,
   LED_METERMODE,
#endif
   LAST_METERMODE
} MeterModeId;


#include "CPUMeter.h"
#include "MemoryMeter.h"
#include "SwapMeter.h"
#include "TasksMeter.h"
#include "LoadAverageMeter.h"
#include "UptimeMeter.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

extern char* METER_CLASS;

extern MeterType* Meter_types[];

#ifdef USE_FUNKY_MODES

#endif

extern MeterMode* Meter_modes[];

Meter* Meter_new(ProcessList* pl, int param, MeterType* type);

void Meter_delete(Object* cast);

void Meter_setCaption(Meter* this, char* caption);

void Meter_setMode(Meter* this, int modeIndex);

ListItem* Meter_toListItem(Meter* this);

/* ---------- TextMeterMode ---------- */

void TextMeterMode_draw(Meter* this, int x, int y, int w);

/* ---------- BarMeterMode ---------- */

void BarMeterMode_draw(Meter* this, int x, int y, int w);

#ifdef USE_FUNKY_MODES

/* ---------- GraphMeterMode ---------- */

#define DrawDot(a,y,c) do { attrset(a); mvaddch(y, x+k, c); } while(0)

void GraphMeterMode_draw(Meter* this, int x, int y, int w);

/* ---------- LEDMeterMode ---------- */

void LEDMeterMode_draw(Meter* this, int x, int y, int w);

#endif

#endif
