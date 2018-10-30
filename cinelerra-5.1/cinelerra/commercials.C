
#include "arraylist.h"
#include "asset.h"
#include "clip.h"
#include "commercials.h"
#include "cache.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "indexable.h"
#include "indexfile.h"
#include "linklist.h"
#include "mainmenu.h"
#include "mediadb.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginset.h"
#include "preferences.h"
#include "record.h"
#include "track.h"
#include "tracks.h"
#include "vframe.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>



Commercials::
Commercials(MWindow *mwindow)
 : Garbage("Commercials")
{
	this->mwindow = mwindow;
	this->scan_status = 0;
  	mdb = new MediaDb();
	scan_file = 0;
	cancelled = 0;
	muted = 0;
	openDb();
	detachDb();
}

Commercials::
~Commercials()
{
	closeDb();
	delete mdb;
	tracks.remove_all_objects();
}

int Commercials::
newDb()
{
        return mdb->newDb();
}

void Commercials::
closeDb()
{
        tracks.remove_all_objects();
	mdb->closeDb();
}

int Commercials::
openDb()
{
	if( !mwindow->has_commercials() )
		return -1;
        if( !mdb->is_open() && mdb->openDb() ) {
		printf("Commercials::openDb failed\n");
                return -1;
        }
        return 0;
}

int Commercials::
resetDb()
{
	mdb->closeDb();
	if( !mwindow->has_commercials() )
		return -1;
	return mdb->resetDb();
}

void Commercials::
commitDb()
{
	mdb->commitDb();
}

void Commercials::
undoDb()
{
	mdb->undoDb();
}

int Commercials::
attachDb(int rw)
{
	return mdb->attachDb(rw);
}

int Commercials::
detachDb()
{
	return mdb->detachDb();
}

int Commercials::
put_weight(VFrame *frame, int no)
{
	int w = frame->get_w(), h = frame->get_h(), rsz = w;
	uint8_t *tp = frame->get_y();
	int64_t wt = 0;
	for( int y=h; --y>=0; tp+=rsz ) {
		uint8_t *bp = tp;
		for( int x=w; --x>=0; ++bp ) wt += *bp;
	}
	clip_weights[no] = (double)wt / (w*h);
	return 0;
}

int Commercials::
put_frame(VFrame *frame, int no, int group, double offset)
{
	int iw = frame->get_w(), ih = frame->get_h();
	int sw = SWIDTH, sh = SHEIGHT;
	int slen = sw*sh;  uint8_t skey[slen];
	Scale(frame->get_y(),0,iw,ih,0,0,iw,ih).scale(skey,sw,sh,0,0,sw,sh);
	int ret = mdb->new_frame(clip_id, skey, no, group, offset);
	if( ret < 0 ) ret = 0;  // ignore forbidden frames
	return ret;
}

int Commercials::
put_clip(File *file, int track, double position, double length)
{
	if( file->asset->format != FILE_MPEG ) return -1;
	double framerate;  int pid, width, height;  char title[BCTEXTLEN];
	if( file->get_video_info(track, pid, framerate,
					width, height, title) ) return -1;
	if( file->set_layer(track) ) return -1;
	int64_t pos = position * framerate;
	if( file->set_video_position(pos, 0) ) return 1;
	time_t ct;  time(&ct);
	int64_t creation_time = (int64_t)ct, system_time;
	if( file->get_system_time(system_time) ) system_time = 0;
	int frames = length * framerate;
	int prefix_size = 2*framerate, length2 = frames/2;
	if( prefix_size > length2 ) prefix_size = length2;
	int suffix_size = prefix_size;

	if( mdb->new_clip_set(title, file->asset->path, position,
		framerate, frames, prefix_size, suffix_size,
		creation_time, system_time) ) return 1;

	clip_id = mdb->clip_id();
	cancelled = 0;
	scan_status = new ScanStatus(this, 30, 30, 1, 1,
		cancelled, _("Cutting Ads"));
	scan_status->update_length(0, frames);
	scan_status->update_position(0, 0);
	update_cut_info(track+1, position);

	clip_weights = mdb->clip_weights();
	frame_period = 1. / framerate;
	VFrame frame(width, height, BC_YUV420P);

	int i = 0, n = 0, result = 0;
	// first 2 secs of frame data and weights
	while( i < prefix_size && !result ) {
		if( (result=file->read_frame(&frame)) != 0 ) break;
		if( (result=put_frame(&frame, n++, 1, i/framerate)) != 0 ) break;
		if( (result=put_weight(&frame, i)) != 0 ) break;
		result = scan_status->update_position(0, ++i);
	}
	int suffix_start = frames - suffix_size;
	while( i < suffix_start && !result ) {
		if( (result=file->read_frame(&frame)) != 0 ) break;
		if( (result=put_weight(&frame, i)) != 0 ) break;
		result = scan_status->update_position(0, ++i);
		++n;
	}
	// last 2 secs of frame data and weights
	while( i < frames && !result ) {
		if( (result=file->read_frame(&frame)) != 0 ) break;
		if( (result=put_frame(&frame, n++, 2, i/framerate)) != 0 ) break;
		if( (result=put_weight(&frame, i)) != 0 ) break;
		result = scan_status->update_position(0, ++i);
	}

	double wt = 0;
	for( i=0; i<frames; ++i ) wt += clip_weights[i];
	mdb->clip_average_weight(wt/frames);

	delete scan_status;
	scan_status = 0;
	return result;
}


Clips *Commercials::
find_clips(int pid)
{
	Clips *clips = 0;
	int trk = tracks.size();
	while( --trk >= 0 && (clips=tracks.get(trk))->pid != pid );
	return trk >= 0 ? clips : 0;
}

int Commercials::
get_frame(File *file, int pid, double position,
	uint8_t *tp, int mw, int mh, int ww, int hh)
{
	if( file->asset->format != FILE_MPEG ) return -1;
	int sw = SWIDTH, sh = SHEIGHT;
	int slen= sw*sh;   uint8_t skey[slen];
	Scale(tp,0,mw,mh,0,0,ww/8,hh/8).scale(skey,sw,sh,0,0,sw,sh);
	Clips *clips = find_clips(pid);
	if( !clips ) tracks.append(clips = new Clips(pid));
	int fid, result = mdb->get_frame_key(skey, fid, clips, position);
	return result;
}

double Commercials::
frame_weight(uint8_t *tdat, int rowsz, int width, int height)
{
	int64_t weight = 0;
	for( int y=height; --y>=0; tdat+=rowsz ) {
		uint8_t *bp = tdat;
		for( int x=width; --x>=0; ++bp ) weight += *bp;
	}
	return (double)weight / (width*height);
}

int Commercials::
skim_frame(Snips *snips, uint8_t *dat, double position)
{
	int fid, result = mdb->get_frame_key(dat, fid, snips, position, 1);
	double weight = frame_weight(dat, SWIDTH, SWIDTH, SHEIGHT);
	for( Clip *next=0,*clip=snips->first; clip; clip=next ) {
		next = clip->next;
		result = verify_snip((Snip*)clip, weight, position);
		if( !result ) {
			if( clip->votes > 2 )
				mute_audio(clip);
		}
		else if( result < 0 || --clip->votes < 0 ) {
			--snips->count;
			delete clip;
			if( !snips->first )
				unmute_audio();
		}
	}
	return 0;
}

double Commercials::
abs_err(Snip *snip, double *ap, int j, int len, double iframerate)
{
	double vv = 0;
	int i, k, sz = snip->weights.size(), n = 0;
	for( i=0; i<sz; ++i ) {
		k = j + (snip->positions[i] - snip->start) * iframerate + 0.5;
		if( k < 0 ) continue;
		if( k >= len ) break;
		double a = ap[k], b =  snip->weights[i];
		double dv = fabs(a - b);
		vv += dv;
		++n;
	}
	return !n ? MEDIA_WEIGHT_ERRLMT : vv / n;
}

int Commercials::
verify_snip(Snip *snip, double weight, double position)
{
	int cid = snip->clip_id;
	if( mdb->clip_id(cid) ) return -1;
	double iframerate = mdb->clip_framerate();
	int iframes = mdb->clip_frames();
	double pos = position - snip->start;
	int iframe_no = pos * iframerate + 0.5;
	if( iframe_no >= iframes ) return -1;
	snip->weights.append(weight);
	snip->positions.append(position);
	double *iweights = mdb->clip_weights();
	double errlmt = MEDIA_WEIGHT_ERRLMT;
	double err = errlmt, kerr = err;
	int k = 0;
	int tmargin = TRANSITION_MARGIN * iframerate + 0.5;
	for( int j=-2*tmargin; j<=tmargin; ++j ) {
		err = abs_err(snip, iweights, j, iframes, iframerate);
		if( err < kerr ) { kerr = err;  k = j; }
	}
	if( kerr >= errlmt ) return 1;
	if( iframe_no + k >= iframes ) return -1;
	return 0;
}


int Commercials::
mute_audio(Clip *clip)
{
	Record *record = mwindow->gui->record;
	if( !clip->muted ) {
		clip->muted = 1;
		mdb->access_clip(clip->clip_id);
		mdb->commitDb();
		char clip_title[BCSTRLEN];
		sprintf(clip_title,"%d",clip_id);
		record->display_video_text(10, 10, clip_title,
			BIGFONT, DKGREEN, LTYELLOW, -1, 300., 1.);
	}
	if( !muted ) {
		muted = 1;
		record->set_mute_gain(0);
		printf(_("***MUTE***\n"));
	}
	return 0;
}

int Commercials::
unmute_audio()
{
	Record *record = mwindow->gui->record;
	if( muted ) {
		muted = 0;
		record->set_mute_gain(1);
		printf(_("***UNMUTE***\n"));
	}
	record->undisplay_vframe();
	return 0;
}

int Commercials::skim_weight(int track)
{
	if( cancelled ) return 1;
	int64_t framenum; uint8_t *tdat; int mw, mh;
	if( scan_file->get_thumbnail(track, framenum, tdat, mw, mh) ) return 1;
	double framerate;  int pid, width, height;
	if( scan_file->get_video_info(track, pid, framerate,
					width, height, 0) ) return 1;
	if( !framerate ) return 1;
	width /= 8;  height /= 8;
//write_pbm(tdat,width,height,"/tmp/dat/r%05ld.pbm",framenum);
	if( height > mh ) height = mh;
	if( !width || !height ) return 1;
	weights.append(frame_weight(tdat, mw, width, height));
	int64_t frame_no = framenum - frame_start;
	offsets.append(frame_no / framerate);
	if( scan_status->update_position(1,frame_no) ) return 1;
	// return < 0, continue.  = 0, complete.  > 0 error
	return frame_no < frame_total ? -1 : 0;
}

int Commercials::skim_weight(void *vp, int track)
{
	return ((Commercials *)vp)->skim_weight(track);
}


int Commercials::
skim_weights(int track, double position, double iframerate, int iframes)
{
	double framerate;  int pid, width, height;  char title[BCTEXTLEN];
	if( scan_file->get_video_info(track, pid, framerate,
					width, height, title) ) return 1;
	if( scan_file->set_layer(track) ) return 1;
	frame_start = framerate * position;
	if( scan_file->set_video_position(frame_start, 0) ) return 1;
	// skimming limits
	double length = iframes / iframerate;
	frame_total = length * framerate;
	scan_status->update_length(1,frame_total);
	scan_status->update_position(1, 0);
	weights.remove_all();
	offsets.remove_all();
	return scan_file->skim_video(track, this, skim_weight);
}

double Commercials::
abs_err(double *ap, int len, double iframerate)
{
	double vv = 0;
	int i, k, sz = weights.size(), n = 0;
	for( i=0; i<sz; ++i ) {
		k = offsets[i] * iframerate + 0.5;
		if( k < 0 ) continue;
		if( k >= len ) break;
		double a = ap[k], b = weights[i];
		double dv = fabs(a - b);
		vv += dv;
		++n;
	}
        return !n ? MEDIA_WEIGHT_ERRLMT : vv / n;
}


int Commercials::
verify_clip(int clip_id, int track, double position, double &pos)
{
	if( mdb->clip_id(clip_id) ) return 1;
	double iframerate = mdb->clip_framerate();
	if( iframerate <= 0. ) return 1;
	int iframes = mdb->clip_frames();
	int tmargin = TRANSITION_MARGIN * iframerate;
	int mframes = iframes - 2*tmargin;
	if( mframes <= 0 ) return 1;
	double tposition = position + TRANSITION_MARGIN;
	if( skim_weights(track, tposition, iframerate, mframes) ) return 1;
	double *iweights = mdb->clip_weights();
	double err = abs_err(iweights, mframes, iframerate);
	int k = 0;  double kerr = err;
	int n = 2*tmargin;
	for( int j=1; j<n; ++j ) {
		err = abs_err(iweights+j, mframes, iframerate);
		if( err < kerr ) { kerr = err;  k = j; }
	}
//printf("** kerr %f, k=%d\n", kerr, k);
	if( kerr >= MEDIA_WEIGHT_ERRLMT ) return -1;
	pos = position + (k - tmargin) / iframerate;
	return 0;
}

int Commercials::
write_ads(const char *filename)
{
	char index_filename[BCTEXTLEN], source_filename[BCTEXTLEN];
	IndexFile::get_index_filename(source_filename,
		mwindow->preferences->index_directory, index_filename,
		filename, ".ads");

	FILE *fp = fopen(index_filename, "wb");
	if( fp != 0 ) {
		FileXML xml;
		xml.tag.set_title("ADS");
		xml.append_tag();
		xml.append_newline();
		int trks = tracks.size();
		for( int trk=0; trk<trks; ++trk )
			tracks.get(trk)->save(xml);
		xml.tag.set_title("/ADS");
		xml.append_tag();
		xml.append_newline();
		xml.terminate_string();
		xml.write_to_file(fp);
		fclose(fp);
	}
	return 0;
}

int Commercials::
read_ads(const char *filename)
{
	char index_filename[BCTEXTLEN], source_filename[BCTEXTLEN];
	IndexFile::get_index_filename(source_filename,
		mwindow->preferences->index_directory, index_filename,
		filename, ".ads");
	tracks.remove_all_objects();
	FileXML xml;
	if( xml.read_from_file(index_filename, 1) ) return 1;

	do {
		if( xml.read_tag() ) return 1;
	} while( !xml.tag.title_is("ADS") );

	for(;;) {
		if( xml.read_tag() ) return 1;
		if( xml.tag.title_is("/ADS") ) break;
		if( xml.tag.title_is("CLIPS") ) {
			int pid = xml.tag.get_property("PID", (int)0);
			Clips *clips = new Clips(pid);
			tracks.append(clips);
			if( clips->load(xml) ) return 1;
			if( !xml.tag.title_is("/CLIPS") ) break;
		}
	}

	return 0;
}

void Commercials::
dump_ads()
{
	for( int i=0; i<tracks.size(); ++i )
	{
		printf("clip %i:\n", i);
		for( Clip *clip=tracks.get(i)->first; clip; clip=clip->next )
			printf("  clip_id %d: votes %d, index %d, groups %d, start %f, end %f\n",
				clip->clip_id, clip->votes, clip->index, clip->groups,
				clip->start, clip->end);
	}
}

int Commercials::
verify_edit(Track *track, Edit *edit, double start, double end)
{
	int64_t clip_start = track->to_units(start,0);
	int64_t clip_end = track->to_units(end,0);
	int64_t edit_start = edit->startsource;
	if( edit_start >= clip_end ) return 1;
	int64_t edit_end = edit_start + edit->length;
	if( edit_end <= clip_start ) return 1;
	return 0;
}

Edit *Commercials::
cut_edit(Track *track, Edit *edit, int64_t clip_start, int64_t clip_end)
{
	int64_t edit_start = edit->startsource;
	int64_t edit_end = edit_start + edit->length;
	int64_t cut_start = clip_start < edit_start ? edit_start : clip_start;
	int64_t cut_end = clip_end >= edit_end ? edit_end : clip_end;
	int64_t edit_offset = edit->startproject - edit->startsource;
	// shift from source timeline to project timeline
	cut_start += edit_offset;  cut_end += edit_offset;
	// cut autos and plugins
	track->automation->clear(cut_start, cut_end, 0, 1);
	if( mwindow->edl->session->plugins_follow_edits ) {
		int sz = track->plugin_set.size();
		for( int i=0; i<sz; ++i ) {
			PluginSet *plugin_set = track->plugin_set.values[i];
			plugin_set->clear(cut_start, cut_end, 0);
		}
	}
	// cut edit
	Edit *next_edit = edit->edits->split_edit(cut_start);
	int64_t cut_length = cut_end - cut_start;
	next_edit->length -= cut_length;
	next_edit->startsource += cut_length;
	for( Edit *ep=next_edit->next; ep;  ep=ep->next )
		ep->startproject -= cut_length;
	return next_edit;
}

int Commercials::
scan_audio(int vstream, double start, double end)
{
	int64_t channel_mask = 0;
	int astrm = scan_file->get_audio_for_video(vstream, -1, channel_mask);
	if( astrm < 0 || !channel_mask ) return -1;
	Tracks *tracks = mwindow->edl->tracks;
	for(Track *atrk=tracks->first; !cancelled && atrk; atrk=atrk->next) {
		if( atrk->data_type != TRACK_AUDIO ) continue;
		if( !atrk->record ) continue;
		Edits *edits = atrk->edits;  Edit *next = 0;
		for( Edit *edit=edits->first; !cancelled && edit; edit=next ) {
			next = edit->next;
			if( ((channel_mask>>edit->channel) & 1) == 0 ) continue;
			Indexable *indexable = edit->get_source();
			if( !indexable || !indexable->is_asset ) continue;
			Asset *asset = (Asset *)indexable;
			if( !scan_file->asset->equivalent(*asset,0,0,mwindow->edl) ) continue;
			if( verify_edit(atrk, edit, start, end) ) continue;
			next = cut_edit(atrk, edit,
				atrk->to_units(start,0),
				atrk->to_units(end,0));
		}
		edits->optimize();
	}
	return 0;
}

int Commercials::
scan_media()
{
	cancelled = 0;
	scan_status = new ScanStatus(this, 30, 30, 2, 2,
		cancelled, _("Cutting Ads"));
	if( !openDb() ) {
		scan_video();
		commitDb();
		closeDb();
	}
	delete scan_status;
	scan_status = 0;
	return 0;
}

int Commercials::
scan_video()
{
	Tracks *tracks = mwindow->edl->tracks;
	for( Track *vtrk=tracks->first; !cancelled && vtrk; vtrk=vtrk->next) {
		if( vtrk->data_type != TRACK_VIDEO ) continue;
		if( !vtrk->record ) continue;
		Edits *edits = vtrk->edits;  Edit *next = 0;
		for( Edit *edit=edits->first; !cancelled && edit; edit=next ) {
			next = edit->next;
			Indexable *indexable = edit->get_source();
			if( !indexable || !indexable->is_asset ) continue;
			Asset *asset = (Asset *)indexable;
			if( update_caption(edit->channel+1,
				edits->number_of(edit), asset->path) ) break;
			if( read_ads(asset->path) ) continue;
			if( scan_asset(asset, vtrk, edit) ) continue;
			next = edits->first;
		}
		edits->optimize();
	}
	return 0;
}

int Commercials::
scan_asset(Asset *asset, Track *vtrk, Edit *edit)
{
	int result = 1;
	scan_file = mwindow->video_cache->check_out(asset, mwindow->edl);
	if( scan_file && scan_file->asset->format == FILE_MPEG ) {
		result = scan_clips(vtrk, edit);
		mwindow->video_cache->check_in(asset);
		scan_file = 0;
	}
	return result;
}

int Commercials::
scan_clips(Track *vtrk, Edit *edit)
{
	int pid = scan_file->get_video_pid(edit->channel);
	if( pid < 0 ) return 1;
	Clips *clips = find_clips(pid);
	if( !clips ) return 1;
	double last_end = -CLIP_MARGIN;
	scan_status->update_length(0, clips->total());
	scan_status->update_position(0, 0);
	for( Clip *clip=clips->first; !cancelled && clip; clip=clip->next ) {
		if( verify_edit(vtrk, edit, clip->start, clip->end) ) continue;
		if( update_status(clips->number_of(clip),
			clip->start, clip->end) ) break;
		scan_status->update_position(0, clips->number_of(clip));
		double start = clip->start;
		if( verify_clip(clip->clip_id, edit->channel,
				clip->start, start) ) continue;
 		double length = clip->end - clip->start;
		if( start < 0 ) {
			if( (length+=start) <= 0 ) continue;
			start = 0;
		}
		double end = start + length;
printf(_("cut clip %d in edit @%f %f-%f, clip @%f-%f\n"), clip->clip_id,
  vtrk->from_units(edit->startsource), vtrk->from_units(edit->startproject),
  vtrk->from_units(edit->startproject+edit->length),start,end);
		if( last_end+CLIP_MARGIN > start ) start = last_end;
		edit = cut_edit(vtrk, edit,
				vtrk->to_units(start,0),
				vtrk->to_units(end,0));
		scan_audio(edit->channel, start, end);
		mdb->access_clip(clip->clip_id);
		last_end = end;
	}
	return 1;
}


int Commercials::
update_cut_info(int trk, double position)
{
	if( cancelled ) return 1;
	char string[BCTEXTLEN];  EDLSession *session = mwindow->edl->session;
	Units::totext(string, position, session->time_format, session->sample_rate,
		session->frame_rate, session->frames_per_foot);
	char text[BCTEXTLEN];  sprintf(text,_("ad: trk %d@%s  "),trk,string);
	scan_status->update_text(0, text);
	return 0;
}

int Commercials::
update_caption(int trk, int edt, const char *path)
{
	if( cancelled ) return 1;
	char text[BCTEXTLEN];
	snprintf(text,sizeof(text),_("trk%d edt%d asset %s"), trk, edt, path);
	scan_status->update_text(0, text);
	return 0;
}

int Commercials::
update_status(int clip, double start, double end)
{
	if( cancelled ) return 1;
	char text[BCTEXTLEN];
	snprintf(text,sizeof(text),_("scan: clip%d %f-%f"), clip, start, end);
	scan_status->update_text(1, text);
	return 0;
}


ScanStatusGUI::
ScanStatusGUI(ScanStatus *sswindow, int x, int y, int nlines, int nbars)
 : BC_Window(_("Scanning"), x, y, 340,
	40 + BC_CancelButton::calculate_h() +
		(BC_Title::calculate_h((BC_WindowBase*) sswindow->
			commercials->mwindow->gui, _("My")) + 5) * nlines +
		(BC_ProgressBar::calculate_h() + 5) * nbars, 0, 0, 0)
{
	this->sswindow = sswindow;
	this->nlines = nlines;
	this->nbars = nbars;
	this->texts = new BC_Title *[nlines];
	this->bars = new ScanStatusBar *[nbars];
}

ScanStatusGUI::
~ScanStatusGUI()
{
	delete [] texts;
	delete [] bars;
}

void ScanStatusGUI::
create_objects(const char *text)
{
	lock_window("ScanStatusGUI::create_objects");
	int x = 10, y = 10;
	int dy = BC_Title::calculate_h((BC_WindowBase*) sswindow->
		commercials->mwindow->gui, "My") + 5;
	for( int i=0; i<nlines; ++i, y+=dy, text="" )
		add_tool(texts[i] = new BC_Title(x, y, text));
	y += 10;
	dy = BC_ProgressBar::calculate_h() + 5;
	for( int i=0; i<nbars; ++i, y+=dy )
		add_tool(bars[i] = new ScanStatusBar(x, y, get_w() - 20, 100));

	add_tool(new BC_CancelButton(this));
	unlock_window();
}

ScanStatus::
ScanStatus(Commercials *commercials, int x, int y,
	int nlines, int nbars, int &status, const char *text)
 : Thread(1, 0, 0),
   status(status)
{
	this->commercials = commercials;
	gui = new ScanStatusGUI(this, x, y, nlines, nbars);
	gui->create_objects(text);
	start();
	gui->init_wait();
};

ScanStatus::
~ScanStatus()
{
	stop();
	delete gui;
}


int ScanStatus::
update_length(int i, int64_t length)
{
	if( status ) return 1;
	gui->bars[i]->update_length(length);
	return 0;
}

int ScanStatus::
update_position(int i, int64_t position)
{
	if( status ) return 1;
	gui->bars[i]->update(position);
	return 0;
}

int ScanStatus::
update_text(int i, const char *text)
{
	if( status ) return 1;
	gui->texts[i]->update(text);
	return 0;
}

void ScanStatus::
stop()
{
	status = 1;
	if( running() ) {
		if( gui ) gui->set_done(1);
		cancel();
	}
	join();
}

void ScanStatus::
run()
{
	gui->create_objects(_("Cutting Ads"));
	int result = gui->run_window();
	if( result ) status = 1;
}



void SdbPacketQueue::
put_packet(SdbPacket *p)
{
	lock("SdbPacketQueue::put_packet");
	append(p);
	unlock();
}

SdbPacket *SdbPacketQueue::
get_packet()
{
	lock("SdbPacketQueue::get_packet");
	SdbPacket *p = first;
	remove_pointer(p);
	unlock();
	return p;
}


SkimDbThread::
SkimDbThread()
 : Thread(1, 0, 0)
{
	input_lock = new Condition(0, "SkimDbThread::input_lock");
	for( int i=32; --i>=0; ) skim_frames.append(new SdbSkimFrame(this));
	snips = new Snips();
	commercials = 0;
	done = 1;
}

SkimDbThread::
~SkimDbThread()
{
	stop();
	delete snips;
	delete input_lock;
}


void SkimDbThread::
start(Commercials *commercials)
{
	commercials->add_user();
	this->commercials = commercials;
	if( commercials->openDb() ) return;
	if( commercials->detachDb() ) return;
	done = 0;
	Thread::start();
}

void SkimDbThread::
stop()
{
	if( running() ) {
		done = 1;
		input_lock->unlock();
		cancel();
	}
	join();
	if( commercials && !commercials->remove_user() )
		commercials = 0;
}

int SkimDbThread::
skim(int pid,int64_t framenum,double framerate,
	uint8_t *idata,int mw,int mh,int iw,int ih)
{
	SdbSkimFrame *sf = (SdbSkimFrame *)skim_frames.get_packet();
	if( !sf ) { printf("SkimDbThread::skim no packet\n");  return 1; }
	sf->load(pid,framenum,framerate, idata,mw,mh,iw,ih);
	sf->start();
	return 0;
}


void SdbPacket::start()
{
	thread->put_packet(this);
}

void SkimDbThread::
put_packet(SdbPacket *p)
{
	active_packets.put_packet(p);
	input_lock->unlock();
}

void SkimDbThread::
run()
{
	while( !done ) {
		input_lock->lock("SkimDbThread::run");
		if( done ) break;
		SdbPacket *p = active_packets.get_packet();
		if( !p ) continue;
		commercials->attachDb();
		p->run();
		commercials->detachDb();
	}
}


void SdbSkimFrame::
load(int pid,int64_t framenum,double framerate,
	uint8_t *idata,int mw,int mh,int iw,int ih)
{
	int sw=SWIDTH, sh=SHEIGHT;
	this->pid = pid;
	this->framenum = framenum;
	this->framerate = framerate;
	Scale(idata,0,mw,mh,0,0,iw/8,ih/8).scale(dat,sw,sh,0,0,sw,sh);
}

void SdbSkimFrame::
run()
{
	double position = framenum / framerate;
	thread->commercials->skim_frame(thread->snips, dat, position);
	thread->skim_frames.put_packet(this);
}


void run_that_puppy(const char *fn)
{
	FILE *fp = fopen(fn,"r");  double position, length;
	if( !fp ) { perror("fopen"); return; }
	MWindow::commercials->resetDb();
	MWindow *mwindow = MWindow::commercials->mwindow;
	File *file = mwindow->video_cache->first->file;
	int result = 0;
	while( !result && fscanf(fp,"%lf %lf\n",&position, &length) == 2 ) {
		int result = MWindow::commercials->put_clip(file, 0, position, length);
		printf(_("cut %f/%f = %d\n"),position,length, result);
	}
	MWindow::commercials->commitDb();
	MWindow::commercials->closeDb();
	fclose(fp);
}

