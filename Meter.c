/*
htop - Meter.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Meter.h"

#include "CPUMeter.h"
#include "MemoryMeter.h"
#include "SwapMeter.h"
#include "TasksMeter.h"
#include "LoadAverageMeter.h"
#include "UptimeMeter.h"
#include "BatteryMeter.h"
#include "ClockMeter.h"
#include "HostnameMeter.h"
#include "RichString.h"
#include "Object.h"
#include "CRT.h"
#include "String.h"
#include "ListItem.h"
#include "Settings.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/time.h>

#define METER_BUFFER_LEN 128

/*{
#include "ListItem.h"
#include "ProcessList.h"

typedef struct Meter_ Meter;

typedef void(*Meter_Init)(Meter*);
typedef void(*Meter_Done)(Meter*);
typedef void(*Meter_UpdateMode)(Meter*, int);
typedef void(*Meter_SetValues)(Meter*, char*, int);
typedef void(*Meter_Draw)(Meter*, int, int, int);

typedef struct MeterClass_ {
   ObjectClass super;
   const Meter_Init init;
   const Meter_Done done;
   const Meter_UpdateMode updateMode;
   const Meter_Draw draw;
   const Meter_SetValues setValues;
   const int defaultMode;
   int items;
   const double total;
   const int* attributes;
   const char* name;
   const char* uiName;
   const char* caption;
} MeterClass;

#define As_Meter(this_)                ((MeterClass*)((this_)->super.klass))
#define Meter_initFn(this_)            As_Meter(this_)->init
#define Meter_init(this_)              As_Meter(this_)->init((Meter*)(this_))
#define Meter_done(this_)              As_Meter(this_)->done((Meter*)(this_))
#define Meter_updateModeFn(this_)      As_Meter(this_)->updateMode
#define Meter_updateMode(this_, m_)    As_Meter(this_)->updateMode((Meter*)(this_), m_)
#define Meter_drawFn(this_)            As_Meter(this_)->draw
#define Meter_doneFn(this_)            As_Meter(this_)->done
#define Meter_setValues(this_, c_, i_) As_Meter(this_)->setValues((Meter*)(this_), c_, i_)
#define Meter_defaultMode(this_)       As_Meter(this_)->defaultMode
#define Meter_getItems(this_)          As_Meter(this_)->items
#define Meter_setItems(this_, n_)      As_Meter(this_)->items = (n_)
#define Meter_attributes(this_)        As_Meter(this_)->attributes
#define Meter_name(this_)              As_Meter(this_)->name
#define Meter_uiName(this_)            As_Meter(this_)->uiName

struct Meter_ {
   Object super;
   Meter_Draw draw;
   
   char* caption;
   int mode;
   int param;
   void* drawData;
   int h;
   ProcessList* pl;
   double* values;
   double total;
};

typedef struct MeterMode_ {
   Meter_Draw draw;
   const char* uiName;
   int h;
} MeterMode;

typedef enum {
   CUSTOM_METERMODE = 0,
   BAR_METERMODE,
   TEXT_METERMODE,
   GRAPH_METERMODE,
   LED_METERMODE,
   LAST_METERMODE
} MeterModeId;

typedef struct GraphData_ {
   struct timeval time;
   double values[METER_BUFFER_LEN];
} GraphData;

}*/

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

MeterClass Meter_class = {
   .super = {
      .extends = Class(Object)
   }
};

MeterClass* Meter_types[] = {
   &CPUMeter_class,
   &ClockMeter_class,
   &LoadAverageMeter_class,
   &LoadMeter_class,
   &MemoryMeter_class,
   &SwapMeter_class,
   &TasksMeter_class,
   &UptimeMeter_class,
   &BatteryMeter_class,
   &HostnameMeter_class,
   &AllCPUsMeter_class,
   &AllCPUs2Meter_class,
   &LeftCPUsMeter_class,
   &RightCPUsMeter_class,
   &LeftCPUs2Meter_class,
   &RightCPUs2Meter_class,
   NULL
};

Meter* Meter_new(ProcessList* pl, int param, MeterClass* type) {
   Meter* this = calloc(sizeof(Meter), 1);
   Object_setClass(this, type);
   this->h = 1;
   this->param = param;
   this->pl = pl;
   this->values = calloc(sizeof(double), type->items);
   this->total = type->total;
   this->caption = strdup(type->caption);
   if (Meter_initFn(this))
      Meter_init(this);
   Meter_setMode(this, type->defaultMode);
   return this;
}

void Meter_delete(Object* cast) {
   if (!cast)
      return;
   Meter* this = (Meter*) cast;
   if (Meter_doneFn(this)) {
      Meter_done(this);
   }
   if (this->drawData)
      free(this->drawData);
   free(this->caption);
   free(this->values);
   free(this);
}

void Meter_setCaption(Meter* this, const char* caption) {
   free(this->caption);
   this->caption = strdup(caption);
}

static inline void Meter_displayBuffer(Meter* this, char* buffer, RichString* out) {
   if (Object_displayFn(this)) {
      Object_display(this, out);
   } else {
      RichString_write(out, CRT_colors[Meter_attributes(this)[0]], buffer);
   }
}

void Meter_setMode(Meter* this, int modeIndex) {
   if (modeIndex > 0 && modeIndex == this->mode)
      return;
   if (!modeIndex)
      modeIndex = 1;
   assert(modeIndex < LAST_METERMODE);
   if (Meter_defaultMode(this) == CUSTOM_METERMODE) {
      this->draw = Meter_drawFn(this);
      if (Meter_updateModeFn(this))
         Meter_updateMode(this, modeIndex);
   } else {
      assert(modeIndex >= 1);
      if (this->drawData)
         free(this->drawData);
      this->drawData = NULL;

      MeterMode* mode = Meter_modes[modeIndex];
      this->draw = mode->draw;
      this->h = mode->h;
   }
   this->mode = modeIndex;
}

ListItem* Meter_toListItem(Meter* this) {
   char mode[21];
   if (this->mode)
      snprintf(mode, 20, " [%s]", Meter_modes[this->mode]->uiName);
   else
      mode[0] = '\0';
   char number[11];
   if (this->param > 0)
      snprintf(number, 10, " %d", this->param);
   else
      number[0] = '\0';
   char buffer[51];
   snprintf(buffer, 50, "%s%s%s", Meter_uiName(this), number, mode);
   return ListItem_new(buffer, 0);
}

/* ---------- TextMeterMode ---------- */

static void TextMeterMode_draw(Meter* this, int x, int y, int w) {
   char buffer[METER_BUFFER_LEN];
   Meter_setValues(this, buffer, METER_BUFFER_LEN - 1);

   attrset(CRT_colors[METER_TEXT]);
   mvaddstr(y, x, this->caption);
   int captionLen = strlen(this->caption);
   w -= captionLen;
   x += captionLen;
   mvhline(y, x, ' ', CRT_colors[DEFAULT_COLOR]);
   attrset(CRT_colors[RESET_COLOR]);
   RichString_begin(out);
   Meter_displayBuffer(this, buffer, &out);
   RichString_printVal(out, y, x);
   RichString_end(out);
}

/* ---------- BarMeterMode ---------- */

static char BarMeterMode_characters[] = "|#*@$%&";

static void BarMeterMode_draw(Meter* this, int x, int y, int w) {
   char buffer[METER_BUFFER_LEN];
   Meter_setValues(this, buffer, METER_BUFFER_LEN - 1);

   w -= 2;
   attrset(CRT_colors[METER_TEXT]);
   int captionLen = 3;
   mvaddnstr(y, x, this->caption, captionLen);
   x += captionLen;
   w -= captionLen;
   attrset(CRT_colors[BAR_BORDER]);
   mvaddch(y, x, '[');
   mvaddch(y, x + w, ']');
   
   w--;
   x++;

   if (w < 1) {
      attrset(CRT_colors[RESET_COLOR]);
      return;
   }
   char bar[w + 1];
   
   int blockSizes[10];
   for (int i = 0; i < w; i++)
      bar[i] = ' ';

   const size_t barOffset = w - MIN((int)strlen(buffer), w);
   snprintf(bar + barOffset, w - barOffset + 1, "%s", buffer);

   // First draw in the bar[] buffer...
   int offset = 0;
   int items = Meter_getItems(this);
   for (int i = 0; i < items; i++) {
      double value = this->values[i];
      value = MAX(value, 0);
      value = MIN(value, this->total);
      if (value > 0) {
         blockSizes[i] = ceil((value/this->total) * w);
      } else {
         blockSizes[i] = 0;
      }
      int nextOffset = offset + blockSizes[i];
      // (Control against invalid values)
      nextOffset = MIN(MAX(nextOffset, 0), w);
      for (int j = offset; j < nextOffset; j++)
         if (bar[j] == ' ') {
            if (CRT_colorScheme == COLORSCHEME_MONOCHROME) {
               bar[j] = BarMeterMode_characters[i];
            } else {
               bar[j] = '|';
            }
         }
      offset = nextOffset;
   }

   // ...then print the buffer.
   offset = 0;
   for (int i = 0; i < items; i++) {
      attrset(CRT_colors[Meter_attributes(this)[i]]);
      mvaddnstr(y, x + offset, bar + offset, blockSizes[i]);
      offset += blockSizes[i];
      offset = MAX(offset, 0);
      offset = MIN(offset, w);
   }
   if (offset < w) {
      attrset(CRT_colors[BAR_SHADOW]);
      mvaddnstr(y, x + offset, bar + offset, w - offset);
   }

   move(y, x + w + 1);
   attrset(CRT_colors[RESET_COLOR]);
}

/* ---------- GraphMeterMode ---------- */

#define DrawDot(a,y,c) do { attrset(a); mvaddch(y, x+k, c); } while(0)

static int GraphMeterMode_colors[21] = {
   GRAPH_1, GRAPH_1, GRAPH_1,
   GRAPH_2, GRAPH_2, GRAPH_2,
   GRAPH_3, GRAPH_3, GRAPH_3,
   GRAPH_4, GRAPH_4, GRAPH_4,
   GRAPH_5, GRAPH_5, GRAPH_6,
   GRAPH_7, GRAPH_7, GRAPH_7,
   GRAPH_8, GRAPH_8, GRAPH_9
};

static const char* GraphMeterMode_characters = "^`'-.,_~'`-.,_~'`-.,_";

static void GraphMeterMode_draw(Meter* this, int x, int y, int w) {

   if (!this->drawData) this->drawData = calloc(sizeof(GraphData), 1);
   GraphData* data = (GraphData*) this->drawData;
   const int nValues = METER_BUFFER_LEN;
   
   struct timeval now;
   gettimeofday(&now, NULL);
   if (!timercmp(&now, &(data->time), <)) {
      struct timeval delay = { .tv_sec = (int)(DEFAULT_DELAY/10), .tv_usec = (DEFAULT_DELAY-((int)(DEFAULT_DELAY/10)*10)) * 100000 };
      timeradd(&now, &delay, &(data->time));

      for (int i = 0; i < nValues - 1; i++)
         data->values[i] = data->values[i+1];
   
      char buffer[nValues];
      Meter_setValues(this, buffer, nValues - 1);
   
      double value = 0.0;
      int items = Meter_getItems(this);
      for (int i = 0; i < items; i++)
         value += this->values[i];
      value /= this->total;
      data->values[nValues - 1] = value;
   }
   
   for (int i = nValues - w, k = 0; i < nValues; i++, k++) {
      double value = data->values[i];
      DrawDot( CRT_colors[DEFAULT_COLOR], y, ' ' );
      DrawDot( CRT_colors[DEFAULT_COLOR], y+1, ' ' );
      DrawDot( CRT_colors[DEFAULT_COLOR], y+2, ' ' );
      
      double threshold = 1.00;
      for (int j = 0; j < 21; j++, threshold -= 0.05)
         if (value >= threshold) {
            DrawDot(CRT_colors[GraphMeterMode_colors[j]], y+(j/7.0), GraphMeterMode_characters[j]);
            break;
         }
   }
   attrset(CRT_colors[RESET_COLOR]);
}

/* ---------- LEDMeterMode ---------- */

static const char* LEDMeterMode_digitsAscii[3][10] = {
   { " __ ","    "," __ "," __ ","    "," __ "," __ "," __ "," __ "," __ "},
   { "|  |","   |"," __|"," __|","|__|","|__ ","|__ ","   |","|__|","|__|"},
   { "|__|","   |","|__ "," __|","   |"," __|","|__|","   |","|__|"," __|"},
};

static const char* LEDMeterMode_digitsUtf8[3][10] = {
   { "┌──┐","  ┐ ","╶──┐","╶──┐","╷  ╷","┌──╴","┌──╴","╶──┐","┌──┐","┌──┐"},
   { "│  │","  │ ","┌──┘"," ──┤","└──┤","└──┐","├──┐","   │","├──┤","└──┤"},
   { "└──┘","  ╵ ","└──╴","╶──┘","   ╵","╶──┘","└──┘","   ╵","└──┘"," ──┘"},
};

static void LEDMeterMode_drawDigit(int x, int y, int n) {
   if (CRT_utf8) {
      for (int i = 0; i < 3; i++)
         mvaddstr(y+i, x, LEDMeterMode_digitsUtf8[i][n]);
   } else {
      for (int i = 0; i < 3; i++)
         mvaddstr(y+i, x, LEDMeterMode_digitsAscii[i][n]);
   }
}

static void LEDMeterMode_draw(Meter* this, int x, int y, int w) {
   (void) w;
   char buffer[METER_BUFFER_LEN];
   Meter_setValues(this, buffer, METER_BUFFER_LEN - 1);
   
   RichString_begin(out);
   Meter_displayBuffer(this, buffer, &out);

   int yText = CRT_utf8 ? y+1 : y+2;
   attrset(CRT_colors[LED_COLOR]);
   mvaddstr(yText, x, this->caption);
   int xx = x + strlen(this->caption);
   int len = RichString_sizeVal(out);
   for (int i = 0; i < len; i++) {
      char c = RichString_getCharVal(out, i);
      if (c >= '0' && c <= '9') {
         LEDMeterMode_drawDigit(xx, y, c-48);
         xx += 4;
      } else {
         mvaddch(yText, xx, c);
         xx += 1;
      }
   }
   attrset(CRT_colors[RESET_COLOR]);
   RichString_end(out);
}

static MeterMode BarMeterMode = {
   .uiName = "Bar",
   .h = 1,
   .draw = BarMeterMode_draw,
};

static MeterMode TextMeterMode = {
   .uiName = "Text",
   .h = 1,
   .draw = TextMeterMode_draw,
};

static MeterMode GraphMeterMode = {
   .uiName = "Graph",
   .h = 3,
   .draw = GraphMeterMode_draw,
};

static MeterMode LEDMeterMode = {
   .uiName = "LED",
   .h = 3,
   .draw = LEDMeterMode_draw,
};

MeterMode* Meter_modes[] = {
   NULL,
   &BarMeterMode,
   &TextMeterMode,
   &GraphMeterMode,
   &LEDMeterMode,
   NULL
};
