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

#ifndef BLUEBANANAWINDOW_H
#define BLUEBANANAWINDOW_H

#include "bccolors.h"
#include "pluginvclient.h"
#include "pluginserver.h"
#include "mwindow.h"

class BluebananaWindow;
class BluebananaMain;
class BluebananaThread;
class BluebananaMark;
class BluebananaActive;
class BluebananaOp;
class BluebananaA2Sel;
class BluebananaIS;
class BluebananaUnmask;

class BluebananaHActive;
class BluebananaSActive;
class BluebananaVActive;
class BluebananaFActive;
class BluebananaErode;
class BluebananaHAActive;
class BluebananaSAActive;
class BluebananaVAActive;
class BluebananaRAActive;
class BluebananaGAActive;
class BluebananaBAActive;
class BluebananaOAActive;
class BluebananaAAActive;

class BluebananaHSSlider;
class BluebananaSSSlider;
class BluebananaVSSlider;
class BluebananaFSSlider;
class BluebananaHASlider;
class BluebananaSASlider;
class BluebananaVASlider;
class BluebananaRASlider;
class BluebananaGASlider;
class BluebananaBASlider;
class BluebananaOASlider;
class BluebananaAASlider;

class BluebananaHSReadout0;
class BluebananaHSReadout1;
class BluebananaHSReadout2;
class BluebananaSSReadout0;
class BluebananaSSReadout1;
class BluebananaSSReadout2;
class BluebananaVSReadout0;
class BluebananaVSReadout1;
class BluebananaVSReadout2;
class BluebananaFSReadout0;
class BluebananaFSReadout1;
class BluebananaFSReadout2;
class BluebananaFSReadout3;
class BluebananaHAReadout;
class BluebananaSAReadout0;
class BluebananaSAReadout1;
class BluebananaSAReadout2;
class BluebananaVAReadout0;
class BluebananaVAReadout1;
class BluebananaVAReadout2;
class BluebananaRAReadout0;
class BluebananaRAReadout1;
class BluebananaRAReadout2;
class BluebananaGAReadout0;
class BluebananaGAReadout1;
class BluebananaGAReadout2;
class BluebananaBAReadout0;
class BluebananaBAReadout1;
class BluebananaBAReadout2;
class BluebananaOAReadout;
class BluebananaAAReadout;

class BluebananaPreserve;

class BB_Tumble : public BC_TextBox {
 public:
  BB_Tumble(BluebananaMain *plugin,
            BluebananaWindow *gui,
            float min,
            float mid,
            float max,
            int precision,
            float inc,
            int text_w);
  virtual ~BB_Tumble() {}

  int handle_event();
  virtual int value_event() { return 1; }
  int update(float value){ return BC_TextBox::update(value); }
  float get_value();
  void reposition_window(int x, int y){
    this->x = x;
    this->y = y;
    BC_TextBox::reposition_window(x,y,text_w,1);
  }

 protected:
  int x, y, text_w;
  float min,mid,max;
  int precision;
  float increment;
  BluebananaMain *plugin;
  BluebananaWindow *gui;

 private:
  int activate();
  int deactivate();
  int button_press_event();
  int keypress_event();
  int active;
  int suppress_events;
  float prev;
};

class BluebananaWindow : public PluginClientWindow {
public:
	BluebananaWindow(BluebananaMain *plugin);
	~BluebananaWindow();

        int windowx;
        int windowy;
        int slider_x;
        int slider_w;
        int slider_top_x_tumblers;
        int slider_top_w_tumblers;
        int slider_bottom_x_tumblers;
        int slider_bottom_w_tumblers;

	void create_objects();
	int close_event();
        void enter_config_change();
        void commit_config_change();
        void leave_config_change();
        int flush_config_change();

        int config_refcount;
        int config_change;
        int config_produce;
        int config_consume;
        int config_pending;

	void update();
	void render();
        void update_histograms(BluebananaMain *plugin);
        int repeat_event(int64_t duration);

	BluebananaMain *plugin;

        BluebananaMark *mark;
        BluebananaOp *op;
        BluebananaActive *active;
        BluebananaA2Sel *use_mask;
        BluebananaUnmask *capture_mask;
        BluebananaIS *invert_selection;

        BluebananaHActive *Hsel_active;
        BluebananaSActive *Ssel_active;
        BluebananaVActive *Vsel_active;
        BluebananaFActive *Fsel_active;
        BluebananaErode *Fsel_erode;

        BluebananaHAActive *Hadj_active;
        BluebananaSAActive *Sadj_active;
        BluebananaVAActive *Vadj_active;
        BluebananaRAActive *Radj_active;
        BluebananaGAActive *Gadj_active;
        BluebananaBAActive *Badj_active;
        BluebananaOAActive *Oadj_active;
        BluebananaAAActive *Aadj_active;

        BluebananaHSSlider *Hsel_slider;
        BluebananaHSReadout0 *Hsel_readout0;
        BluebananaHSReadout1 *Hsel_readout1;
        BluebananaHSReadout2 *Hsel_readout2;

        BluebananaSSSlider *Ssel_slider;
        BluebananaSSReadout0 *Ssel_readout0;
        BluebananaSSReadout1 *Ssel_readout1;
        BluebananaSSReadout2 *Ssel_readout2;

        BluebananaVSSlider *Vsel_slider;
        BluebananaVSReadout0 *Vsel_readout0;
        BluebananaVSReadout1 *Vsel_readout1;
        BluebananaVSReadout2 *Vsel_readout2;

        BluebananaFSSlider *Fsel_slider;
        BluebananaFSReadout0 *Fsel_readout0;
        BluebananaFSReadout1 *Fsel_readout1;
        BluebananaFSReadout2 *Fsel_readout2;
        BluebananaFSReadout3 *Fsel_readout3;

        BluebananaHASlider *Hadj_slider;
        BluebananaHAReadout *Hadj_readout;

        BluebananaSASlider *Sadj_slider;
        BluebananaSAReadout0 *Sadj_readout0;
        BluebananaSAReadout1 *Sadj_readout1;
        BluebananaSAReadout2 *Sadj_readout2;

        BluebananaVASlider *Vadj_slider;
        BluebananaVAReadout0 *Vadj_readout0;
        BluebananaVAReadout1 *Vadj_readout1;
        BluebananaVAReadout2 *Vadj_readout2;

        BluebananaRASlider *Radj_slider;
        BluebananaRAReadout0 *Radj_readout0;
        BluebananaRAReadout1 *Radj_readout1;
        BluebananaRAReadout2 *Radj_readout2;

        BluebananaGASlider *Gadj_slider;
        BluebananaGAReadout0 *Gadj_readout0;
        BluebananaGAReadout1 *Gadj_readout1;
        BluebananaGAReadout2 *Gadj_readout2;

        BluebananaBASlider *Badj_slider;
        BluebananaBAReadout0 *Badj_readout0;
        BluebananaBAReadout1 *Badj_readout1;
        BluebananaBAReadout2 *Badj_readout2;

        BluebananaOASlider *Oadj_slider;
        BluebananaOAReadout *Oadj_readout;
        BluebananaAASlider *Aadj_slider;
        BluebananaAAReadout *Aadj_readout;

        //BluebananaPreserve *preserve_luma;

        BC_Title *slider_labels[12];
        BC_Title *erode_label;
        int do_render;
};


#endif
