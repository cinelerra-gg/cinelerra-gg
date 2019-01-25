/*
 * Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV
 * Copyright (C) 2012-2013 Monty <monty@xiph.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "bluebanana.h"
#include "bluebananaconfig.h"
#include "bluebananawindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "playback3d.h"
#include "bccolors.h"
#include "vframe.h"
#include "workarounds.h"

class BluebananaMain;
class BluebananaEngine;
class BluebananaWindow;

REGISTER_PLUGIN(BluebananaMain)

BluebananaMain::BluebananaMain(PluginServer *server)
 : PluginVClient(server)
{
  engine = 0;

  /* be sure a lookup update triggers */
  lookup_cache.Hsel_lo=-999999;
  lookup_cache.Ssel_lo=-999999;
  lookup_cache.Vsel_lo=-999999;
  lookup_cache.Fsel_lo=-999999;

  lookup_cache.Sadj_lo=-999999;
  lookup_cache.Vadj_lo=-999999;
  lookup_cache.Radj_lo=-999999;
  lookup_cache.Gadj_lo=-999999;
  lookup_cache.Badj_lo=-999999;

  colormodel=-1;

  ants_counter=0;

  memset(red_histogram,0,sizeof(red_histogram));
  memset(green_histogram,0,sizeof(green_histogram));
  memset(blue_histogram,0,sizeof(blue_histogram));
  memset(hue_histogram,0,sizeof(hue_histogram));
  memset(sat_histogram,0,sizeof(sat_histogram));
  memset(value_histogram,0,sizeof(value_histogram));
}

BluebananaMain::~BluebananaMain() {

  // if ants are running, run one more pane update to hide them (gui
  // is already marked as closed)
  //if(server && server->mwindow && ants_counter>0)
  //  server->mwindow->sync_parameters();

  delete engine;
}

const char* BluebananaMain::plugin_title() { return N_("Blue Banana"); }
int BluebananaMain::is_realtime() { return 1; }

NEW_WINDOW_MACRO(BluebananaMain, BluebananaWindow)
LOAD_CONFIGURATION_MACRO(BluebananaMain, BluebananaConfig)

void BluebananaMain::render_gui(void *data){
  if(thread){
    BluebananaMain *that = (BluebananaMain *)data; // that is server-side
    BluebananaWindow *window = (BluebananaWindow *)thread->window;
    window->lock_window("BluebananaMain::render_gui");

    /* push histogram data to gui */
    window->update_histograms(that);

    /* push any colormodel update to gui */
    if(that->frame && colormodel != that->frame->get_color_model()){
      colormodel = that->frame->get_color_model();
      window->update();
    }

    window->unlock_window();
  }
}

void BluebananaMain::update_gui(){
  if(thread){
    BluebananaWindow *window = (BluebananaWindow *)thread->window;
    window->lock_window("BluebananaMain::update_gui");
    window->flush_config_change();
    int reconfigure = load_configuration();
    if(reconfigure)
      window->update();

    window->unlock_window();
  }
}

void BluebananaMain::save_data(KeyFrame *keyframe){
  FileXML output;

  // cause data to be stored directly in text
  output.set_shared_output(keyframe->xbuf);
  output.tag.set_title("BLUEBANANA");

  output.tag.set_property("ACTIVE", config.active);
  output.tag.set_property("OP", config.op);
  output.tag.set_property("INVERT_SELECTION", config.invert_selection);
  output.tag.set_property("USE_MASK", config.use_mask);
  output.tag.set_property("CAPTURE_MASK", config.capture_mask);

  output.tag.set_property("HUE_ACTIVE", config.Hsel_active);
  output.tag.set_property("HUE_LO", config.Hsel_lo);
  output.tag.set_property("HUE_HI", config.Hsel_hi);
  output.tag.set_property("HUE_OVERLAP", config.Hsel_over);

  output.tag.set_property("SATURATION_ACTIVE", config.Ssel_active);
  output.tag.set_property("SATURATION_LO", config.Ssel_lo);
  output.tag.set_property("SATURATION_HI", config.Ssel_hi);
  output.tag.set_property("SATURATION_OVERLAP", config.Ssel_over);

  output.tag.set_property("VALUE_ACTIVE", config.Vsel_active);
  output.tag.set_property("VALUE_LO", config.Vsel_lo);
  output.tag.set_property("VALUE_HI", config.Vsel_hi);
  output.tag.set_property("VALUE_OVERLAP", config.Vsel_over);

  output.tag.set_property("FILL_ACTIVE", config.Fsel_active);
  output.tag.set_property("FILL_ERODE", config.Fsel_erode);
  output.tag.set_property("FILL_LO", config.Fsel_lo);
  output.tag.set_property("FILL_MID", config.Fsel_mid);
  output.tag.set_property("FILL_HI", config.Fsel_hi);
  output.tag.set_property("FILL_FEATHER", config.Fsel_over);

  output.tag.set_property("HUE_ADJUST_ACTIVE", config.Hadj_active);
  output.tag.set_property("HUE_ADJUST", config.Hadj_val);

  output.tag.set_property("SATURATION_ADJUST_ACTIVE", config.Sadj_active);
  output.tag.set_property("SATURATION_ADJUST_GAMMA", config.Sadj_gamma);
  output.tag.set_property("SATURATION_ADJUST_LO", config.Sadj_lo);
  output.tag.set_property("SATURATION_ADJUST_HI", config.Sadj_hi);

  output.tag.set_property("VALUE_ADJUST_ACTIVE", config.Vadj_active);
  output.tag.set_property("VALUE_ADJUST_GAMMA", config.Vadj_gamma);
  output.tag.set_property("VALUE_ADJUST_LO", config.Vadj_lo);
  output.tag.set_property("VALUE_ADJUST_HI", config.Vadj_hi);

  output.tag.set_property("RED_ADJUST_ACTIVE", config.Radj_active);
  output.tag.set_property("RED_ADJUST_GAMMA", config.Radj_gamma);
  output.tag.set_property("RED_ADJUST_LO", config.Radj_lo);
  output.tag.set_property("RED_ADJUST_HI", config.Radj_hi);

  output.tag.set_property("GREEN_ADJUST_ACTIVE", config.Gadj_active);
  output.tag.set_property("GREEN_ADJUST_GAMMA", config.Gadj_gamma);
  output.tag.set_property("GREEN_ADJUST_LO", config.Gadj_lo);
  output.tag.set_property("GREEN_ADJUST_HI", config.Gadj_hi);

  output.tag.set_property("BLUE_ADJUST_ACTIVE", config.Badj_active);
  output.tag.set_property("BLUE_ADJUST_GAMMA", config.Badj_gamma);
  output.tag.set_property("BLUE_ADJUST_LO", config.Badj_lo);
  output.tag.set_property("BLUE_ADJUST_HI", config.Badj_hi);

  output.tag.set_property("OPACITY_ADJUST_ACTIVE", config.Oadj_active);
  output.tag.set_property("OPACITY_ADJUST", config.Oadj_val);
  output.tag.set_property("ALPHA_ADJUST_ACTIVE", config.Aadj_active);
  output.tag.set_property("ALPHA_ADJUST", config.Aadj_val);

  output.append_tag();
  output.append_newline();

  output.tag.set_title("/BLUEBANANA");
  output.append_tag();
  output.append_newline();

  if(keyframe->position==0){
    /* this will otherwise overwrite the nonauto information as well */
    output_nonauto(&output);
  }

  output.terminate_string();
}

// save non-auto data to the default keyframe at zero and does it
// without alerting the keyframing mechanism
void BluebananaMain::save_nonauto(){

  KeyFrame *default_keyframe=get_prev_keyframe(0);
  if(default_keyframe){
    FileXML input;
    FileXML output;
    int result = 0;

    input.read_from_string(default_keyframe->get_data());
    output.set_shared_output(default_keyframe->xbuf);

    while(!result){
      result = input.read_tag();

      if(!result &&
         !input.tag.title_is("BLUEBANANA_NONAUTO") &&
         !input.tag.title_is("/BLUEBANANA_NONAUTO")){
        input.tag.write_tag(&output);
        output.append_newline();
      }
    }

    output_nonauto(&output);
  }
}

void BluebananaMain::output_nonauto(FileXML *output){
  output->tag.set_title("BLUEBANANA_NONAUTO");
  output->tag.set_property("MARK", config.mark);
  output->append_tag();
  output->tag.set_title("/BLUEBANANA_NONAUTO");
  output->append_tag();
  output->append_newline();
  output->terminate_string();
}

void BluebananaMain::load_nonauto(){
  /* nonauto data stored in the default keyframe at position 0 */
  KeyFrame *default_keyframe=get_prev_keyframe(0);
  if(default_keyframe){
    FileXML input;
    int result = 0;
    input.set_shared_input(default_keyframe->xbuf);

    while(!result){
      result = input.read_tag();

      if(!result && input.tag.title_is("BLUEBANANA_NONAUTO")){
        config.mark = input.tag.get_property("MARK", config.mark);
      }
    }
  }
}


void BluebananaMain::read_data(KeyFrame *keyframe){
  
  int result = 0;
{ FileXML input; // release xbuf-shared_lock before nonaauto
  input.set_shared_input(keyframe->xbuf);

  while(!result){
    result = input.read_tag();

    if(!result && input.tag.title_is("BLUEBANANA")){
      config.active = input.tag.get_property("ACTIVE", config.active);
      config.op = input.tag.get_property("OP", config.op);
      config.invert_selection = input.tag.get_property("INVERT_SELECTION", config.invert_selection);
      config.use_mask = input.tag.get_property("USE_MASK", config.use_mask);
      config.capture_mask = input.tag.get_property("CAPTURE_MASK", config.capture_mask);

      config.Hsel_active = input.tag.get_property("HUE_ACTIVE", config.Hsel_active);
      config.Hsel_lo = input.tag.get_property("HUE_LO", config.Hsel_lo);
      config.Hsel_hi = input.tag.get_property("HUE_HI", config.Hsel_hi);
      config.Hsel_over = input.tag.get_property("HUE_OVERLAP", config.Hsel_over);

      config.Ssel_active = input.tag.get_property("SATURATION_ACTIVE", config.Ssel_active);
      config.Ssel_lo = input.tag.get_property("SATURATION_LO", config.Ssel_lo);
      config.Ssel_hi = input.tag.get_property("SATURATION_HI", config.Ssel_hi);
      config.Ssel_over = input.tag.get_property("SATURATION_OVERLAP", config.Ssel_over);

      config.Vsel_active = input.tag.get_property("VALUE_ACTIVE", config.Vsel_active);
      config.Vsel_lo = input.tag.get_property("VALUE_LO", config.Vsel_lo);
      config.Vsel_hi = input.tag.get_property("VALUE_HI", config.Vsel_hi);
      config.Vsel_over = input.tag.get_property("VALUE_OVERLAP", config.Vsel_over);

      config.Fsel_active = input.tag.get_property("FILL_ACTIVE", config.Fsel_active);
      config.Fsel_erode = input.tag.get_property("FILL_ERODE", config.Fsel_erode);
      config.Fsel_lo = input.tag.get_property("FILL_LO", config.Fsel_lo);
      config.Fsel_mid = input.tag.get_property("FILL_MID", config.Fsel_mid);
      config.Fsel_hi = input.tag.get_property("FILL_HI", config.Fsel_hi);
      config.Fsel_over = input.tag.get_property("FILL_FEATHER", config.Fsel_over);

      config.Hadj_active = input.tag.get_property("HUE_ADJUST_ACTIVE", config.Hadj_active);
      config.Hadj_val = input.tag.get_property("HUE_ADJUST", config.Hadj_val);

      config.Sadj_active = input.tag.get_property("SATURATION_ADJUST_ACTIVE", config.Sadj_active);
      config.Sadj_gamma = input.tag.get_property("SATURATION_ADJUST_GAMMA", config.Sadj_gamma);
      config.Sadj_lo = input.tag.get_property("SATURATION_ADJUST_LO", config.Sadj_lo);
      config.Sadj_hi = input.tag.get_property("SATURATION_ADJUST_HI", config.Sadj_hi);

      config.Vadj_active = input.tag.get_property("VALUE_ADJUST_ACTIVE", config.Vadj_active);
      config.Vadj_gamma = input.tag.get_property("VALUE_ADJUST_GAMMA", config.Vadj_gamma);
      config.Vadj_lo = input.tag.get_property("VALUE_ADJUST_LO", config.Vadj_lo);
      config.Vadj_hi = input.tag.get_property("VALUE_ADJUST_HI", config.Vadj_hi);

      config.Radj_active = input.tag.get_property("RED_ADJUST_ACTIVE", config.Radj_active);
      config.Radj_gamma = input.tag.get_property("RED_ADJUST_GAMMA", config.Radj_gamma);
      config.Radj_lo = input.tag.get_property("RED_ADJUST_LO", config.Radj_lo);
      config.Radj_hi = input.tag.get_property("RED_ADJUST_HI", config.Radj_hi);

      config.Gadj_active = input.tag.get_property("GREEN_ADJUST_ACTIVE", config.Gadj_active);
      config.Gadj_gamma = input.tag.get_property("GREEN_ADJUST_GAMMA", config.Gadj_gamma);
      config.Gadj_lo = input.tag.get_property("GREEN_ADJUST_LO", config.Gadj_lo);
      config.Gadj_hi = input.tag.get_property("GREEN_ADJUST_HI", config.Gadj_hi);

      config.Badj_active = input.tag.get_property("BLUE_ADJUST_ACTIVE", config.Badj_active);
      config.Badj_gamma = input.tag.get_property("BLUE_ADJUST_GAMMA", config.Badj_gamma);
      config.Badj_lo = input.tag.get_property("BLUE_ADJUST_LO", config.Badj_lo);
      config.Badj_hi = input.tag.get_property("BLUE_ADJUST_HI", config.Badj_hi);

      config.Oadj_active = input.tag.get_property("OPACITY_ADJUST_ACTIVE", config.Oadj_active);
      config.Oadj_val = input.tag.get_property("OPACITY_ADJUST", config.Oadj_val);
      config.Aadj_active = input.tag.get_property("ALPHA_ADJUST_ACTIVE", config.Aadj_active);
      config.Aadj_val = input.tag.get_property("ALPHA_ADJUST", config.Aadj_val);

    }
  }
}
  load_nonauto();
}

static void select_grow_h(float *hrow, float *vrow, int width){
  int i;

  /* spread left */
  for(i=0;i<width-1;i++){
    if(hrow[i]<hrow[i+1])hrow[i]=hrow[i+1];
    if(vrow[i]<hrow[i])vrow[i]=hrow[i];
  }
  /* spread right */
  for(i=width-1;i>0;i--){
    if(hrow[i]<hrow[i-1])hrow[i]=hrow[i-1];
    if(vrow[i]<hrow[i])vrow[i]=hrow[i];
  }
}

static void select_shrink_h(float *hrow, float *vrow, int width){
  int i;

  /* spread left */
  for(i=0;i<width-1;i++){
    if(hrow[i]>hrow[i+1])hrow[i]=hrow[i+1];
    if(vrow[i]>hrow[i])vrow[i]=hrow[i];
  }
  /* spread right */
  for(i=width-1;i>0;i--){
    if(hrow[i]>hrow[i-1])hrow[i]=hrow[i-1];
    if(vrow[i]>hrow[i])vrow[i]=hrow[i];
  }
}

static void select_grow_v(float *row0, float *row1, int width){
  int i;

  /* spread into 0 */
  for(i=0;i<width;i++)
    if(row0[i]<row1[i])row0[i]=row1[i];
}

static void select_shrink_v(float *row0, float *row1, int width){
  int i;

  /* spread out of 0 */
  for(i=0;i<width;i++)
    if(row0[i]>row1[i])row0[i]=row1[i];
}

static void threaded_horizontal(float *in, float *work,
                                int width, int height,
                                BluebananaEngine *e, int tasks, int passes,
                                void(*func)(float *, float *, int)){

  /* these are individually fast operations; here we do in fact make
     cache region collisions as impossible as we can.  Live with the
     overhead of the join. */
  int i,j;
  e->set_task(tasks,"H_even");
  j = e->next_task();
  {
    int row = (j*2)*height/(tasks*2);
    int end = (j*2+1)*height/(tasks*2);
    for(;row<end;row++)
      for(i=0;i<passes;i++)
        func(in+row*width,work+row*width,width);
  }
  e->wait_task();

  e->set_task(0,"H_odd");
  {
    int row = (j*2+1)*height/(tasks*2);
    int end = (j*2+2)*height/(tasks*2);
    for(;row<end;row++)
      for(i=0;i<passes;i++)
        func(in+row*width,work+row*width,width);
  }
  e->wait_task();
}

static void threaded_vertical(float *work, float *temp,
                              int width, int height,
                              BluebananaEngine *e, int tasks,
                              void(*func)(float *, float *, int)){

  /* regions overlap, so this becomes a bit more complex. */

  /* rather than on-demand task allocation, we use the task engine to
     grab a slot, then reuse this same slot through several joins */

  e->set_task(tasks,"up_odd");
  int region = e->next_task(); /* grab one slot */
  int start_row = (region*2)*height/(tasks*2);
  int mid_row = (region*2+1)*height/(tasks*2);
  int end_row = (region*2+2)*height/(tasks*2);
  int row;

  /* spread up, starting at middle row, moving down */
  /* first task is to save a copy of the un-transformed middle row, as
     we'll need it for the even pass */
  memcpy(temp,work+mid_row*width,sizeof(*temp)*width);

  /* odd interleave */
  for(row=mid_row;row<end_row-1;row++)
    func(work+row*width,work+(row+1)*width,width);
  if(end_row<height && row<end_row)
    func(work+row*width,work+(row+1)*width,width);
  e->wait_task();

  /* even interleave */
  e->set_task(0,"up_even");
  for(row=start_row;row<mid_row-1;row++)
    func(work+row*width,work+(row+1)*width,width);
  if(row<mid_row)
    func(work+row*width,temp,width);
  e->wait_task();

  /* spread down, starting at mid row and moving up */
  /* once again, grab a temp vector for the second pass overlap */
  memcpy(temp,work+mid_row*width,sizeof(*temp)*width);

  /* even interleave */
  e->set_task(0,"down_even");
  for(row=mid_row;row>start_row;row--)
    func(work+row*width,work+(row-1)*width,width);
  if(start_row>0)
    func(work+row*width,work+(row-1)*width,width);
  e->wait_task();

  /* odd interleave */
  e->set_task(0,"down_odd");
  for(row=end_row-1;row>mid_row+1;row--)
    func(work+row*width,work+(row-1)*width,width);
  if(row>mid_row)
    func(work+row*width,temp,width);
  e->wait_task();

  /* done */
}

static float *fill_one(float *in, float *work,
                      int width, int height,
                      BluebananaEngine *e, // NULL if we're not threading
                      char *pattern,
                      int n){
  int i,j;
  int tasks = e?e->get_total_packages():0;
  float temp[width];

  if(n){

    if(e){

      /* multiple memcpys running at once is occasionally a total
         cache disaster on Sandy Bridge.  So only one thread gets to
         copy, the others wait. */
      e->set_task(1,"fill_memcpy");
      while((j = e->next_task())>=0){
        memcpy(work,in,width*height*sizeof(*work));
      }

    }else{
      memcpy(work,in,width*height*sizeof(*work));
    }


    for(i=0;i<n;i++){
      switch(pattern[i]){
      case 'H': /* grow horizontal */
        if(e){
          threaded_horizontal(in, work, width, height, e, tasks, 1, select_grow_h);
        }else{
          for(j=0;j<height;j++)
            select_grow_h(in+j*width,work+j*width,width);
        }
        break;

      case 'V': /* grow vertical */
        if(e){
          threaded_vertical(work, temp, width, height, e, tasks, select_grow_v);
        }else{
          for(j=0;j<height-1;j++)
            select_grow_v(work+j*width,work+(j+1)*width,width);
          for(j=height-1;j>0;j--)
            select_grow_v(work+j*width,work+(j-1)*width,width);
        }
        break;

      case 'h': /* shrink horizontal */
        if(e){
          threaded_horizontal(in, work, width, height, e, tasks, 1, select_shrink_h);
        }else{
          for(j=0;j<height;j++)
            select_shrink_h(in+j*width,work+j*width,width);
        }
        break;

      default: /* shrink vertical */

        if(e){
          threaded_vertical(work, temp, width, height, e, tasks, select_shrink_v);
        }else{
          for(j=0;j<height-1;j++)
            select_shrink_v(work+j*width,work+(j+1)*width,width);
          for(j=height-1;j>0;j--)
            select_shrink_v(work+j*width,work+(j-1)*width,width);
        }
        break;
      }
    }
    return work;
  }else{
    return in;
  }
}

static void select_feather_h(float *p, float *dummy, int w){
  for(int x=0;x<w-1;x++)
    p[x] = (p[x]+p[x+1])*.5;
  for(int x=w-1;x>0;x--)
    p[x] = (p[x]+p[x-1])*.5;
}

static void select_feather_v(float *p0, float *p1, int w){
  for(int x=0; x<w; x++)
    p0[x] = (p0[x]+p1[x])*.5;
}

static void feather_one(float *in,
                        int width, int height,
                        BluebananaEngine *e, // NULL if we're not threading
                        int n){
  int i,j;
  int tasks = e?e->get_total_packages():0;
  float temp[width];

  if(e){
    threaded_horizontal(in, 0, width, height, e, tasks, n, select_feather_h);
    for(int i=0;i<n;i++)
      threaded_vertical(in, temp, width, height, e, tasks, select_feather_v);
  }else{
    for(j=0;j<height;j++)
      for(i=0;i<n;i++)
        select_feather_h(in+j*width,0,width);

    for(i=0;i<n;i++){
      for(j=0;j<height-1;j++)
        select_feather_v(in+j*width,in+(j+1)*width,width);
      for(j=height-1;j>0;j--)
        select_feather_v(in+j*width,in+(j-1)*width,width);
    }
  }
}

/* here and not in the engine as the GUI also uses it (without
   threading) */
float *BluebananaMain::fill_selection(float *in, float *work,
                                         int width, int height,
                                         BluebananaEngine *e){
  float *A=in;
  float *B=work;
  float *C;

  C=fill_one(A,B,width,height,e,select_one,select_one_n);
  C=fill_one(C,(C==A?B:A),width,height,e,select_two,select_two_n);
  C=fill_one(C,(C==A?B:A),width,height,e,select_three,select_three_n);

  feather_one(C,width,height,e,config.Fsel_over);

  return C;
}


int BluebananaMain::process_buffer(VFrame *frame,
                                   int64_t start_position,
                                   double frame_rate){
  ants_counter++;
  SET_TRACE
  load_configuration();
  this->frame = frame;

  SET_TRACE
  update_lookups(1);

  SET_TRACE
  read_frame(frame, 0, start_position, frame_rate, 0);

  if(!engine)
    engine = new BluebananaEngine(this, get_project_smp() + 1,
                                  get_project_smp() + 1);
  SET_TRACE
  engine->process_packages(frame);

  // push final histograms to UI if it's up
  SET_TRACE
  send_render_gui(this);

  return 0;
}

