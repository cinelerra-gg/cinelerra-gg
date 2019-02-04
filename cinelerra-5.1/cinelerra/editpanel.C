
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "zoombar.h"
#include "manualgoto.h"



EditPanel::EditPanel(MWindow *mwindow,
	BC_WindowBase *subwindow,
	int window_id,
	int x,
	int y,
	int editing_mode,
	int use_editing_mode,
	int use_keyframe,
	int use_splice,   // Extra buttons
	int use_overwrite,
	int use_copy,
	int use_paste,
	int use_undo,
	int use_fit,
	int use_locklabels,
	int use_labels,
	int use_toclip,
	int use_meters,
	int use_cut,
	int use_commercial,
	int use_goto,
	int use_clk2play)
{
	this->window_id = window_id;
	this->editing_mode = editing_mode;
	this->use_editing_mode = use_editing_mode;
	this->use_keyframe = use_keyframe;
	this->use_splice = use_splice;
	this->use_overwrite = use_overwrite;
	this->use_copy = use_copy;
	this->use_paste = use_paste;
	this->use_undo = use_undo;
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->use_fit = use_fit;
	this->use_labels = use_labels;
	this->use_locklabels = use_locklabels;
	this->use_toclip = use_toclip;
	this->use_meters = use_meters;
	this->use_cut = use_cut;
	this->use_commercial = use_commercial;
	this->use_goto = use_goto;
	this->use_clk2play = use_clk2play;

	this->x = x;
	this->y = y;
	this->fit = 0;
	this->fit_autos = 0;
	this->inpoint = 0;
	this->outpoint = 0;
	this->splice = 0;
	this->overwrite = 0;
	this->clip = 0;
	this->cut = 0;
	this->commercial = 0;
	this->copy = 0;
	this->paste = 0;
	this->labelbutton = 0;
	this->prevlabel = 0;
	this->nextlabel = 0;
	this->prevedit = 0;
	this->nextedit = 0;
	this->undo = 0;
	this->redo = 0;
	this->meter_panel = 0;
	this->meters = 0;
	this->arrow = 0;
	this->ibeam = 0;
	this->keyframe = 0;
	this->mangoto = 0;
	this->click2play = 0;
	locklabels = 0;
}

EditPanel::~EditPanel()
{
}

void EditPanel::set_meters(MeterPanel *meter_panel)
{
	this->meter_panel = meter_panel;
}


void EditPanel::update()
{
	int new_editing_mode = mwindow->edl->session->editing_mode;
	if( arrow ) arrow->update(new_editing_mode == EDITING_ARROW);
	if( ibeam ) ibeam->update(new_editing_mode == EDITING_IBEAM);
	if( keyframe ) keyframe->update(mwindow->edl->session->auto_keyframes);
	if( locklabels ) locklabels->set_value(mwindow->edl->session->labels_follow_edits);
	if( click2play ) {
		int value = !is_vwindow() ?
			mwindow->edl->session->cwindow_click2play :
			mwindow->edl->session->vwindow_click2play ;
		click2play->set_value(value);
	}
	if( meters ) {
		if( is_cwindow() ) {
			meters->update(mwindow->edl->session->cwindow_meter);
			mwindow->cwindow->gui->update_meters();
		}
		else {
			meters->update(mwindow->edl->session->vwindow_meter);
		}
	}
	subwindow->flush();
}

int EditPanel::calculate_w(MWindow *mwindow, int use_keyframe, int total_buttons)
{
	int result = 0;
	int button_w = mwindow->theme->get_image_set("ibeam")[0]->get_w();
	if( use_keyframe ) {
		result += button_w + mwindow->theme->toggle_margin;
	}

	result += button_w * total_buttons;
	return result;
}

int EditPanel::calculate_h(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("ibeam")[0]->get_h();
}

void EditPanel::create_buttons()
{
	x1 = x, y1 = y;

	if( use_editing_mode ) {
		arrow = new ArrowButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(arrow);
		x1 += arrow->get_w();
		ibeam = new IBeamButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(ibeam);
		x1 += ibeam->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	if( use_keyframe ) {
		keyframe = new KeyFrameButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(keyframe);
		x1 += keyframe->get_w();
	}

	if( use_locklabels ) {
		locklabels = new LockLabelsButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(locklabels);
		x1 += locklabels->get_w();
	}

	if( use_keyframe || use_locklabels )
		x1 += mwindow->theme->toggle_margin;

// Mandatory
	inpoint = new EditInPoint(mwindow, this, x1, y1);
	subwindow->add_subwindow(inpoint);
	x1 += inpoint->get_w();
	outpoint = new EditOutPoint(mwindow, this, x1, y1);
	subwindow->add_subwindow(outpoint);
	x1 += outpoint->get_w();

	if( use_splice ) {
		splice = new EditSplice(mwindow, this, x1, y1);
		subwindow->add_subwindow(splice);
		x1 += splice->get_w();
	}

	if( use_overwrite ) {
		overwrite = new EditOverwrite(mwindow, this, x1, y1);
		subwindow->add_subwindow(overwrite);
		x1 += overwrite->get_w();
	}

	if( use_toclip ) {
		clip = new EditToClip(mwindow, this, x1, y1);
		subwindow->add_subwindow(clip);
		x1 += clip->get_w();
	}

	if( use_cut ) {
		cut = new EditCut(mwindow, this, x1, y1);
		subwindow->add_subwindow(cut);
		x1 += cut->get_w();
	}

	if( use_copy ) {
		copy = new EditCopy(mwindow, this, x1, y1);
		subwindow->add_subwindow(copy);
		x1 += copy->get_w();
	}

	if( use_paste ) {
		paste = new EditPaste(mwindow, this, x1, y1);
		subwindow->add_subwindow(paste);
		x1 += paste->get_w();
	}

	if( use_meters ) {
		if( meter_panel ) {
			meters = new MeterShow(mwindow, meter_panel, x1, y1);
			subwindow->add_subwindow(meters);
			x1 += meters->get_w();
		}
		else
			printf("EditPanel::create_objects: meter_panel == 0\n");
	}

	if( use_labels ) {
		labelbutton = new EditLabelbutton(mwindow, this, x1, y1);
		subwindow->add_subwindow(labelbutton);
		x1 += labelbutton->get_w();
		prevlabel = new EditPrevLabel(mwindow, this, x1, y1);
		subwindow->add_subwindow(prevlabel);
		x1 += prevlabel->get_w();
		nextlabel = new EditNextLabel(mwindow, this, x1, y1);
		subwindow->add_subwindow(nextlabel);
		x1 += nextlabel->get_w();
	}

// all windows except VWindow since it's only implemented in MWindow.
	if( use_cut ) {
		prevedit = new EditPrevEdit(mwindow, this, x1, y1);
		subwindow->add_subwindow(prevedit);
		x1 += prevedit->get_w();
		nextedit = new EditNextEdit(mwindow, this, x1, y1);
		subwindow->add_subwindow(nextedit);
		x1 += nextedit->get_w();
	}

	if( use_fit ) {
		fit = new EditFit(mwindow, this, x1, y1);
		subwindow->add_subwindow(fit);
		x1 += fit->get_w();
		fit_autos = new EditFitAutos(mwindow, this, x1, y1);
		subwindow->add_subwindow(fit_autos);
		x1 += fit_autos->get_w();
	}

	if( use_undo ) {
		undo = new EditUndo(mwindow, this, x1, y1);
		subwindow->add_subwindow(undo);
		x1 += undo->get_w();
		redo = new EditRedo(mwindow, this, x1, y1);
		subwindow->add_subwindow(redo);
		x1 += redo->get_w();
	}

	if( use_goto ) {
		mangoto = new EditManualGoto(mwindow, this, x1, y1);
		subwindow->add_subwindow(mangoto);
		x1 += mangoto->get_w();
	}

	if( use_clk2play ) {
		click2play = new EditClick2Play(mwindow, this, x1, y1+5);
		subwindow->add_subwindow(click2play);
		x1 += click2play->get_w();
	}

	if( use_commercial ) {
		commercial = new EditCommercial(mwindow, this, x1, y1);
		subwindow->add_subwindow(commercial);
		x1 += commercial->get_w();
	}
}

void EditPanel::reposition_buttons(int x, int y)
{
	this->x = x;
	this->y = y;
	x1 = x, y1 = y;

	if( use_editing_mode ) {
		arrow->reposition_window(x1, y1);
		x1 += arrow->get_w();
		ibeam->reposition_window(x1, y1);
		x1 += ibeam->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	if( use_keyframe ) {
		keyframe->reposition_window(x1, y1);
		x1 += keyframe->get_w();
	}

	if( use_locklabels ) {
		locklabels->reposition_window(x1,y1);
		x1 += locklabels->get_w();
	}

	if( use_keyframe || use_locklabels )
		x1 += mwindow->theme->toggle_margin;

	inpoint->reposition_window(x1, y1);
	x1 += inpoint->get_w();
	outpoint->reposition_window(x1, y1);
	x1 += outpoint->get_w();
	if( use_splice ) {
		splice->reposition_window(x1, y1);
		x1 += splice->get_w();
	}
	if( use_overwrite ) {
		overwrite->reposition_window(x1, y1);
		x1 += overwrite->get_w();
	}
	if( use_toclip ) {
		clip->reposition_window(x1, y1);
		x1 += clip->get_w();
	}
	if( use_cut ) {
		cut->reposition_window(x1, y1);
		x1 += cut->get_w();
	}
	if( use_copy ) {
		copy->reposition_window(x1, y1);
		x1 += copy->get_w();
	}
	if( use_paste ) {
		paste->reposition_window(x1, y1);
		x1 += paste->get_w();
	}

	if( use_meters ) {
		meters->reposition_window(x1, y1);
		x1 += meters->get_w();
	}

	if( use_labels ) {
		labelbutton->reposition_window(x1, y1);
		x1 += labelbutton->get_w();
		prevlabel->reposition_window(x1, y1);
		x1 += prevlabel->get_w();
		nextlabel->reposition_window(x1, y1);
		x1 += nextlabel->get_w();
	}

	if( prevedit ) {
		prevedit->reposition_window(x1, y1);
		x1 += prevedit->get_w();
	}

	if( nextedit ) {
		nextedit->reposition_window(x1, y1);
		x1 += nextedit->get_w();
	}

	if( use_fit ) {
		fit->reposition_window(x1, y1);
		x1 += fit->get_w();
		fit_autos->reposition_window(x1, y1);
		x1 += fit_autos->get_w();
	}

	if( use_undo ) {
		undo->reposition_window(x1, y1);
		x1 += undo->get_w();
		redo->reposition_window(x1, y1);
		x1 += redo->get_w();
	}

	if( use_goto ) {
		mangoto->reposition_window(x1, y1);
		x1 += mangoto->get_w();
	}
	if( use_clk2play ) {
		click2play->reposition_window(x1, y1+5);
		x1 += click2play->get_w();
	}
}

void EditPanel::create_objects()
{
	create_buttons();
}

int EditPanel::get_w()
{
	return x1 - x;
}

// toggle_label
EditLabelbutton::EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("labelbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Toggle label at current position ( l )"));
}

EditLabelbutton::~EditLabelbutton()
{
}
int EditLabelbutton::keypress_event()
{
	if( get_keypress() == 'l' && !alt_down() )
		return handle_event();
	return 0;
}
int EditLabelbutton::handle_event()
{
	panel->panel_toggle_label();
	return 1;
}

//next_label
EditNextLabel::EditNextLabel(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Next label ( ctrl -> )"));
}
EditNextLabel::~EditNextLabel()
{
}
int EditNextLabel::keypress_event()
{
	if( ctrl_down() ) {
		int key = get_keypress();
		if( (key == RIGHT || key == '.') && !alt_down() ) {
			panel->panel_next_label(0);
			return 1;
		}
		if( key == '>' && alt_down() ) {
			panel->panel_next_label(1);
			return 1;
		}
	}
	return 0;
}
int EditNextLabel::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_next_label(cut);
	return 1;
}

//prev_label
EditPrevLabel::EditPrevLabel(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Previous label ( ctrl <- )"));
}
EditPrevLabel::~EditPrevLabel()
{
}
int EditPrevLabel::keypress_event()
{
	if( ctrl_down() ) {
		int key = get_keypress();
		if( (key == LEFT || key == ',') && !alt_down() ) {
			panel->panel_prev_label(0);
			return 1;
		}
		if( key == '<' && alt_down() ) {
			panel->panel_prev_label(1);
			return 1;
		}
	}
	return 0;
}
int EditPrevLabel::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_prev_label(cut);
	return 1;
}

//prev_edit
EditPrevEdit::EditPrevEdit(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevedit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Previous edit (alt <- )"));
}
EditPrevEdit::~EditPrevEdit()
{
}
int EditPrevEdit::keypress_event()
{
	if( alt_down() ) {
		int key = get_keypress();
		if( (key == LEFT || key == ',') && !ctrl_down() ) {
			panel->panel_prev_edit(0);
			return 1;
		}
		if( key == ',' && ctrl_down() ) {
			panel->panel_prev_edit(1);
			return 1;
		}
	}
	return 0;
}
int EditPrevEdit::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_prev_edit(cut);
	return 1;
}

//next_edit
EditNextEdit::EditNextEdit(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextedit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Next edit ( alt -> )"));
}
EditNextEdit::~EditNextEdit()
{
}
int EditNextEdit::keypress_event()
{
	if( alt_down() ) {
		int key = get_keypress();
		if( (key == RIGHT || key == '.') && !ctrl_down() ) {
			panel->panel_next_edit(0);
			return 1;
		}
		if( key == '.' && ctrl_down() ) {
			panel->panel_next_edit(1);
			return 1;
		}
	}
	return 0;
}
int EditNextEdit::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_next_edit(cut);
	return 1;
}

//copy_selection
EditCopy::EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("copy"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Copy ( c )"));
}
EditCopy::~EditCopy()
{
}

int EditCopy::keypress_event()
{
	if( alt_down() ) return 0;
	if( (get_keypress() == 'c' && !ctrl_down()) ||
	    (panel->is_vwindow() && get_keypress() == 'C') ) {
		return handle_event();
	}
	return 0;
}
int EditCopy::handle_event()
{
	panel->panel_copy_selection();
	return 1;
}

//overwrite_selection
EditOverwrite::EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->overwrite_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Overwrite ( b )"));
}
EditOverwrite::~EditOverwrite()
{
}
int EditOverwrite::handle_event()
{
	panel->panel_overwrite_selection();
	return 1;
}
int EditOverwrite::keypress_event()
{
	if( alt_down() ) return 0;
	if( get_keypress() == 'b' ||
	    (panel->is_vwindow() && get_keypress() == 'B') ) {
		handle_event();
		return 1;
	}
	return 0;
}

//set_inpoint
//unset_inoutpoint
EditInPoint::EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("inbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("In point ( [ or < )"));
}
EditInPoint::~EditInPoint()
{
}
int EditInPoint::handle_event()
{
	panel->panel_set_inpoint();
	return 1;
}
int EditInPoint::keypress_event()
{
	int key = get_keypress();
	if( ctrl_down() ) {
		if( key == 't' ) {
			panel->panel_unset_inoutpoint();
			return 1;
		}
	}
	else if( !alt_down() ) {
		if( key == '[' || key == '<' ) {
			panel->panel_set_inpoint();
			return 1;
		}
	}
	return 0;
}

//set_outpoint
//unset_inoutpoint
EditOutPoint::EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("outbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Out point ( ] or > )"));
}
EditOutPoint::~EditOutPoint()
{
}
int EditOutPoint::handle_event()
{
	panel->panel_set_outpoint();
	return 1;
}
int EditOutPoint::keypress_event()
{
	int key = get_keypress();
	if( ctrl_down() ) {
		if(  key == 't' ) {
			panel->panel_unset_inoutpoint();
			return 1;
		}
	}
	else if( !alt_down() ) {
		if( key == ']' || key == '>' ) {
			panel->panel_set_outpoint();
			return 1;
		}
	}
	return 0;
}

//splice_selection
EditSplice::EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->splice_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Splice ( v )"));
}
EditSplice::~EditSplice()
{
}
int EditSplice::handle_event()
{
	panel->panel_splice_selection();
	return 1;
}
int EditSplice::keypress_event()
{
	if( alt_down() ) return 0;
	if( (get_keypress() == 'v' && !ctrl_down()) ||
	    (panel->is_vwindow() && get_keypress() == 'V') ) {
		handle_event();
		return 1;
	}
	return 0;
}

//to_clip
EditToClip::EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("toclip"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("To clip ( i )"));
}
EditToClip::~EditToClip()
{
}
int EditToClip::handle_event()
{
	panel->panel_to_clip();
	return 1;
}

int EditToClip::keypress_event()
{
	if( alt_down() ) return 0;
	if( get_keypress() == 'i' ||
	    (panel->is_vwindow() && get_keypress() == 'I') ) {
		handle_event();
		return 1;
	}
	return 0;
}

//cut
EditCut::EditCut(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("cut"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Split | Cut ( x )"));
}
EditCut::~EditCut()
{
}
int EditCut::keypress_event()
{
	if( ctrl_down() || shift_down() || alt_down() )
		return 0;
	if( get_keypress() == 'x' )
		return handle_event();
	return 0;
}

int EditCut::handle_event()
{
	panel->panel_cut();
	return 1;
}

//paste
EditPaste::EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("paste"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Paste ( v )"));
}
EditPaste::~EditPaste()
{
}

int EditPaste::keypress_event()
{
	if( get_keypress() == 'v' && !ctrl_down() )
		return handle_event();
	return 0;
}
int EditPaste::handle_event()
{
	panel->panel_paste();
	return 1;
}

//fit_selection
EditFit::EditFit(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit selection to display ( f )"));
}
EditFit::~EditFit()
{
}
int EditFit::keypress_event()
{
	if( !alt_down() && get_keypress() == 'f' ) {
		handle_event();
		return 1;
	}
	return 0;
}
int EditFit::handle_event()
{
	panel->panel_fit_selection();
	return 1;
}

//fit_autos
EditFitAutos::EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fitautos"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit all autos to display ( Alt + f )"));
}
EditFitAutos::~EditFitAutos()
{
}
int EditFitAutos::keypress_event()
{
	if( get_keypress() == 'f' && alt_down() ) {
		panel->panel_fit_autos(!ctrl_down() ? 1 : 0);
		return 1;
	}
	return 0;
}
int EditFitAutos::handle_event()
{
	panel->panel_fit_autos(1);
	return 1;
}

//set_editing_mode
ArrowButton::ArrowButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("arrow"),
	mwindow->edl->session->editing_mode == EDITING_ARROW,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Drag and drop editing mode"));
}

int ArrowButton::handle_event()
{
	update(1);
	panel->ibeam->update(0);
	panel->panel_set_editing_mode(EDITING_ARROW);
// Nothing after this
	return 1;
}

IBeamButton::IBeamButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("ibeam"),
	mwindow->edl->session->editing_mode == EDITING_IBEAM,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Cut and paste editing mode"));
}

int IBeamButton::handle_event()
{
	update(1);
	panel->arrow->update(0);
	panel->panel_set_editing_mode(EDITING_IBEAM);
// Nothing after this
	return 1;
}

//set_auto_keyframes
KeyFrameButton::KeyFrameButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("autokeyframe"),
	mwindow->edl->session->auto_keyframes,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Generate keyframes while tweeking (j)"));
}

int KeyFrameButton::handle_event()
{
	panel->panel_set_auto_keyframes(get_value());
	return 1;
}

int KeyFrameButton::keypress_event()
{
	int key = get_keypress();
	if( key == 'j' && !ctrl_down() && !shift_down() && !alt_down() ) {
		int value = get_value() ? 0 : 1;
		update(value);
		panel->panel_set_auto_keyframes(value);
		return 1;
	}
	return 0;
}

//set_labels_follow_edits
LockLabelsButton::LockLabelsButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("locklabels"),
	mwindow->edl->session->labels_follow_edits,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Lock labels from moving with edits"));
}

int LockLabelsButton::handle_event()
{
	panel->panel_set_labels_follow_edits(get_value());
	return 1;
}



EditManualGoto::EditManualGoto(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("goto"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	mangoto = new ManualGoto(mwindow, panel);
	set_tooltip(_("Manual goto ( g )"));
}
EditManualGoto::~EditManualGoto()
{
	delete mangoto;
}
int EditManualGoto::handle_event()
{
	mangoto->start();
	return 1;
}

int EditManualGoto::keypress_event()
{
	if( get_keypress() == 'g' ) {
		handle_event();
		return 1;
	}
	return 0;
}


EditClick2Play::EditClick2Play(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("playpatch_data"),
    !panel->is_vwindow() ?
	mwindow->edl->session->cwindow_click2play :
	mwindow->edl->session->vwindow_click2play)
{
        this->mwindow = mwindow;
        this->panel = panel;
        set_tooltip(_("Click to play (p)"));
}
int EditClick2Play::handle_event()
{
	int value = get_value();
	panel->set_click_to_play(value);
	return 1;
}
int EditClick2Play::keypress_event()
{
	int key = get_keypress();
	if( key == 'p' && !ctrl_down() && !shift_down() && !alt_down() ) {
		int value = get_value() ? 0 : 1;
		update(value);
		panel->set_click_to_play(value);
		return 1;
	}
	return 0;
}


EditCommercial::EditCommercial(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("commercial"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Commercial ( shift A )"));
}
EditCommercial::~EditCommercial()
{
}
int EditCommercial::keypress_event()
{
	if( ctrl_down() || !shift_down() || alt_down() )
		return 0;
	if( get_keypress() == 'A' )
		return handle_event();
	return 0;
}

int EditCommercial::handle_event()
{
	int have_mwindow_lock = mwindow->gui->get_window_lock();
	if( have_mwindow_lock )
		mwindow->gui->unlock_window();
	mwindow->commit_commercial();
	if( !mwindow->put_commercial() ) {
		mwindow->gui->lock_window("EditCommercial::handle_event 1");
		mwindow->cut();
		if( !have_mwindow_lock )
			mwindow->gui->unlock_window();
		mwindow->activate_commercial();
		return 1;
	}
	mwindow->undo_commercial();
	if( have_mwindow_lock )
		mwindow->gui->lock_window("EditCommercial::handle_event 2");
	return 1;
}


EditUndo::EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("undo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Undo ( z )"));
}
EditUndo::~EditUndo()
{
}
int EditUndo::keypress_event()
{
	if( ctrl_down() || shift_down() || alt_down() )
		return 0;
	if( get_keypress() == 'z' )
		return handle_event();
	return 0;
}
int EditUndo::handle_event()
{
	mwindow->undo_entry(panel->subwindow);
	return 1;
}

EditRedo::EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("redo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Redo ( shift Z )"));
}
EditRedo::~EditRedo()
{
}
int EditRedo::keypress_event()
{
	if( ctrl_down() || !shift_down() || alt_down() )
		return 0;
	if( get_keypress() == 'Z' )
		return handle_event();
	return 0;
}
int EditRedo::handle_event()
{
	mwindow->redo_entry(panel->subwindow);
	return 1;
}

