#include "libzmpeg3.h"

extern "C" int
mpeg3_major()
{
  return zmpeg3_t::zmajor();
}

extern "C" int
mpeg3_minor()
{
  return zmpeg3_t::zminor();
}

extern "C" int
mpeg3_release()
{
  return zmpeg3_t::zrelease();
}

extern "C" int
mpeg3_check_sig(char *path)
{
  return zmpeg3_t::check_sig(path);
}

extern "C" zmpeg3_t*
mpeg3_open(const char *path, int *error_return)
{
  zmpeg3_t *src = new zmpeg3_t(path, *error_return);
  if( *error_return ) { delete src;  src = 0; }
  return src;
}

extern "C" zmpeg3_t*
mpeg3_open_title(const char *title_path, const char *path, int *error_return)
{
  zmpeg3_t *src = new zmpeg3_t(path, *error_return, ZIO_UNBUFFERED, title_path);
  if( *error_return ) { delete src;  src = 0; }
  return src;
}

extern "C" zmpeg3_t*
mpeg3_zopen(const char *title_path, const char *path, int *error_return, int access)
{
  zmpeg3_t *src = new zmpeg3_t(path, *error_return, access, title_path);
  if( *error_return ) { delete src;  src = 0; }
  return src;
}

extern "C" int
mpeg3_close(zmpeg3_t *zsrc)
{
  delete zsrc;
  return 0;
}

extern "C" zmpeg3_t*
mpeg3_open_copy(const char *path, zmpeg3_t *old_src, int *error_return)
{
  return old_src;
}

extern "C" int
mpeg3_set_pts_padding(zmpeg3_t *zsrc, int v)
{
  return zsrc->set_pts_padding(v);
}

extern "C" int
mpeg3_set_cpus(zmpeg3_t *zsrc, int cpus)
{
  return zsrc->set_cpus(cpus);
}

extern "C" int
mpeg3_has_audio(zmpeg3_t *zsrc)
{
  return zsrc->has_audio();
}


extern "C" int
mpeg3_total_astreams(zmpeg3_t *zsrc)
{
  return zsrc->total_astreams();
}


extern "C" int
mpeg3_audio_channels(zmpeg3_t *zsrc, int stream)
{
  return zsrc->audio_channels(stream);
}


extern "C" int
mpeg3_sample_rate(zmpeg3_t *zsrc, int stream)
{
  return zsrc->sample_rate(stream);
}


extern "C" const char *
mpeg3_audio_format(zmpeg3_t *zsrc, int stream)
{
  return zsrc->audio_format(stream);
}

extern "C" long
mpeg3_get_audio_nudge(zmpeg3_t *zsrc, int stream)
{
  return zsrc->audio_nudge(stream);
}

extern "C" long
mpeg3_audio_samples(zmpeg3_t *zsrc, int stream)
{
  return zsrc->audio_samples(stream);
}

 
extern "C" int
mpeg3_set_sample(zmpeg3_t *zsrc, long sample, int stream)
{
  return zsrc->set_sample(sample, stream);
}

extern "C" long
mpeg3_get_sample(zmpeg3_t *zsrc, int stream)
{
  return zsrc->get_sample(stream);
}

extern "C" int
mpeg3_read_audio_d(zmpeg3_t *zsrc,double *output_d,
  int channel, long samples, int stream)
{
  return zsrc->read_audio(output_d, channel, samples, stream);
}

extern "C" int
mpeg3_read_audio_f(zmpeg3_t *zsrc,float *output_f,
  int channel, long samples, int stream)
{
  return zsrc->read_audio(output_f, channel, samples, stream);
}

extern "C" int
mpeg3_read_audio_i(zmpeg3_t *zsrc,int *output_i,
  int channel, long samples, int stream)
{
  return zsrc->read_audio(output_i, channel, samples, stream);
}

extern "C" int
mpeg3_read_audio_s(zmpeg3_t *zsrc,short *output_s,
  int channel, long samples, int stream)
{
  return zsrc->read_audio(output_s, channel, samples, stream);
}

extern "C" int
mpeg3_read_audio(zmpeg3_t *zsrc,float *output_f, short *output_i,
  int channel, long samples, int stream)
{
  if( output_f != 0 )
    return zsrc->read_audio(output_f, channel, samples, stream);
  if( output_i != 0 )
    return zsrc->read_audio(output_i, channel, samples, stream);
  return 1;
}

extern "C" int
mpeg3_reread_audio(zmpeg3_t *zsrc,float *output_f, short *output_i,
  int channel, long samples, int stream)
{
  if( output_f != 0 )
    return zsrc->reread_audio(output_f, channel, samples, stream);
  if( output_i != 0 )
    return zsrc->reread_audio(output_i, channel, samples, stream);
  return 1;
}

extern "C" int
mpeg3_reread_audio_d(zmpeg3_t *zsrc,double *output_d,
  int channel, long samples, int stream)
{
  return zsrc->reread_audio(output_d, channel, samples, stream);
}

extern "C" int
mpeg3_reread_audio_f(zmpeg3_t *zsrc,float *output_f,
  int channel, long samples, int stream)
{
  return zsrc->reread_audio(output_f, channel, samples, stream);
}

extern "C" int
mpeg3_reread_audio_i(zmpeg3_t *zsrc,int *output_i,
  int channel, long samples, int stream)
{
  return zsrc->reread_audio(output_i, channel, samples, stream);
}

extern "C" int
mpeg3_reread_audio_s(zmpeg3_t *zsrc,short *output_s,
  int channel, long samples, int stream)
{
  return zsrc->reread_audio(output_s, channel, samples, stream);
}

extern "C" int
mpeg3_read_audio_chunk(zmpeg3_t *zsrc,unsigned char *output,
  long *size, long max_size, int stream)
{
  return zsrc->read_audio_chunk(output, size, max_size, stream);
}

extern "C" int
mpeg3_has_video(zmpeg3_t *zsrc)
{
  return zsrc->has_video();
}

extern "C" int
mpeg3_total_vstreams(zmpeg3_t *zsrc)
{
  return zsrc->total_vstreams();
}

extern "C" int
mpeg3_video_width(zmpeg3_t *zsrc, int stream)
{
  return zsrc->video_width(stream);
}

extern "C" int
mpeg3_video_height(zmpeg3_t *zsrc, int stream)
{
  return zsrc->video_height(stream);
}

extern "C" int
mpeg3_coded_width(zmpeg3_t *zsrc, int stream)
{
  return zsrc->coded_width(stream);
}

extern "C" int
mpeg3_coded_height(zmpeg3_t *zsrc, int stream)
{
  return zsrc->coded_height(stream);
}

extern "C" int
mpeg3_video_pid(zmpeg3_t *zsrc, int stream)
{
  return zsrc->video_pid(stream);
}

extern "C" float
mpeg3_aspect_ratio(zmpeg3_t *zsrc, int stream)
{
  return zsrc->aspect_ratio(stream);
}

extern "C" double
mpeg3_frame_rate(zmpeg3_t *zsrc, int stream)
{
  return zsrc->frame_rate(stream);
}

extern "C" long
mpeg3_video_frames(zmpeg3_t *zsrc, int stream)
{
  return zsrc->video_frames(stream);
}


extern "C" int
mpeg3_set_frame(zmpeg3_t *zsrc, long frame, int stream)
{
  return zsrc->set_frame(frame, stream);
}

#if 0
extern "C" int
mpeg3_skip_frames(zmpeg3_t *zsrc)
{
  return zsrc->skip_frames();
}
#endif

extern "C" long
mpeg3_get_frame(zmpeg3_t *zsrc, int stream)
{
  return zsrc->get_frame(stream);
}

extern "C" int64_t
mpeg3_get_bytes(zmpeg3_t *zsrc)
{
  return zsrc->get_bytes();
}

extern "C" int
mpeg3_seek_byte(zmpeg3_t *zsrc, int64_t byte)
{
  return zsrc->seek_byte(byte);
}


extern "C" int64_t
mpeg3_tell_byte(zmpeg3_t *zsrc)
{
  return zsrc->tell_byte();
}

extern "C" int
mpeg3_previous_frame(zmpeg3_t *zsrc, int stream)
{
  return zsrc->previous_frame(stream);
}


extern "C" int
mpeg3_end_of_audio(zmpeg3_t *zsrc, int stream)
{
  return zsrc->end_of_audio(stream);
}


extern "C" int
mpeg3_end_of_video(zmpeg3_t *zsrc, int stream)
{
  return zsrc->end_of_video(stream);
}

extern "C" double
mpeg3_get_time(zmpeg3_t *zsrc)
{
  return zsrc->get_time();
}

extern "C" double
mpeg3_get_audio_time(zmpeg3_t* zsrc, int stream)
{
  return zsrc->get_audio_time(stream);
}

extern "C" double
mpeg3_get_cell_time(zmpeg3_t *zsrc, int no, double *time)
{
  return zsrc->get_cell_time(no, *time);
}

extern "C" double
mpeg3_get_video_time(zmpeg3_t* zsrc, int stream)
{
  return zsrc->get_video_time(stream);
}

extern "C" int
mpeg3_read_frame(zmpeg3_t *zsrc, unsigned char **output_rows,
    int in_x, int in_y, int in_w, int in_h, 
    int out_w, int out_h, int color_model, int stream)
{
  return zsrc->read_frame(output_rows, in_x, in_y, in_w, in_h,
    out_w, out_h, color_model, stream);
}

extern "C" int
mpeg3_colormodel(zmpeg3_t *zsrc, int stream)
{
  return zsrc->colormodel(stream);
}

extern "C" int
mpeg3_set_rowspan(zmpeg3_t *zsrc, int bytes, int stream)
{
  return zsrc->set_rowspan(bytes, stream);
}

extern "C" int
mpeg3_read_yuvframe(zmpeg3_t *zsrc,
    char *y_output, char *u_output, char *v_output,
    int in_x, int in_y, int in_w, int in_h, int stream)
{
  return zsrc->read_yuvframe(y_output, u_output, v_output,
    in_x, in_y, in_w, in_h, stream);
}

extern "C" int
mpeg3_read_yuvframe_ptr(zmpeg3_t *zsrc,
    char **y_output, char **u_output, char **v_output, int stream)
{
  return zsrc->read_yuvframe_ptr(y_output, u_output, v_output, stream);
}

extern "C" int
mpeg3_drop_frames(zmpeg3_t *zsrc, long frames, int stream)
{
  return zsrc->drop_frames(frames, stream);
}

extern "C" int
mpeg3_read_video_chunk(zmpeg3_t *zsrc, unsigned char *output, 
    long *size, long max_size, int stream)
{
  return zsrc->read_video_chunk(output, size, max_size, stream);
}

extern "C" int
mpeg3_get_total_vts_titles(zmpeg3_t *zsrc)
{
  return zsrc->get_total_vts_titles();
}

extern "C" int
mpeg3_set_vts_title(zmpeg3_t *zsrc,int title)
{
  return zsrc->set_vts_title(title);
}

extern "C" int
mpeg3_get_total_interleaves(zmpeg3_t *zsrc)
{
  return zsrc->get_total_interleaves();
}

extern "C" int
mpeg3_set_interleave(zmpeg3_t *zsrc,int no)
{
  return zsrc->set_interleave(no);
}

extern "C" int
mpeg3_set_angle(zmpeg3_t *zsrc,int a)
{
  return zsrc->set_angle(a);
}

extern "C" int
mpeg3_set_program(zmpeg3_t *zsrc,int no)
{
  return zsrc->set_program(no);
}

extern "C" int64_t
mpeg3_memory_usage(zmpeg3_t *zsrc)
{
  return zsrc->memory_usage();
}

extern "C" int
mpeg3_get_thumbnail(zmpeg3_t *zsrc,
  int trk, int64_t *frn, uint8_t **t, int *w, int *h)
{
  return zsrc->get_thumbnail(trk, *frn, *t, *w, *h);
}

extern "C" int
mpeg3_set_thumbnail_callback(zmpeg3_t *zsrc, int trk,
   int skim, int thumb, zthumbnail_cb fn, void *p)
{
  return zsrc->set_thumbnail_callback(trk, skim, thumb, fn, p);
}

extern "C" int
mpeg3_set_cc_text_callback(zmpeg3_t *zsrc, int trk, zcc_text_cb fn)
{
  return zsrc->set_cc_text_callback(trk, fn);
}

extern "C" int
mpeg3_subtitle_tracks(zmpeg3_t *zsrc)
{
  return zsrc->subtitle_tracks();
}

extern "C" int
mpeg3_show_subtitle(zmpeg3_t *zsrc, int vtrk, int strk)
{
  return zsrc->show_subtitle(vtrk, strk);
}

extern "C" int
mpeg3_display_subtitle(zmpeg3_t *zsrc, int stream, int sid, int id,
   uint8_t *yp, uint8_t *up, uint8_t *vp, uint8_t *ap,
   int x, int y, int w, int h, double start_msecs, double stop_msecs)
{
  return zsrc->display_subtitle(stream, sid, id, yp, up, vp, ap,
                                x, y, w, h, start_msecs, stop_msecs);
}

extern "C" int
mpeg3_delete_subtitle(zmpeg3_t *zsrc, int stream, int sid, int id)
{
  return zsrc->delete_subtitle(stream, sid, id);
}

extern "C" zmpeg3_t*
mpeg3_start_toc(char *path, char *toc_path, int program, int64_t *total_bytes)
{
  return zmpeg3_t::start_toc(path, toc_path, program, total_bytes);
}

extern "C" void
mpeg3_set_index_bytes(zmpeg3_t *zsrc, int64_t bytes)
{
  return zsrc->set_index_bytes(bytes);
}

extern "C" int
mpeg3_do_toc(zmpeg3_t *zsrc, int64_t *bytes_processed)
{
  return zsrc->do_toc(bytes_processed);
}

extern "C" void
mpeg3_stop_toc(zmpeg3_t *zsrc)
{
  return zsrc->stop_toc();
}

extern "C" int64_t
mpeg3_get_source_date(zmpeg3_t *zsrc)
{
  return zsrc->get_source_date();
}

extern "C" int64_t
mpeg3_calculate_source_date(char *path)
{
  return zmpeg3_t::calculate_source_date(path);
}

extern "C" int
mpeg3_index_tracks(zmpeg3_t *zsrc)
{
  return zsrc->index_tracks();
}

extern "C" int
mpeg3_index_channels(zmpeg3_t *zsrc, int track)
{
  return zsrc->index_channels(track);
}

extern "C" int
mpeg3_index_zoom(zmpeg3_t *zsrc)
{
  return zsrc->index_zoom();
}

extern "C" int
mpeg3_index_size(zmpeg3_t *zsrc, int track)
{
  return zsrc->index_size(track);
}

extern "C" float*
mpeg3_index_data(zmpeg3_t *zsrc, int track, int channel)
{
  return zsrc->index_data(track, channel);
}

extern "C" int
mpeg3_has_toc(zmpeg3_t *zsrc)
{
  return zsrc->has_toc();
}

extern "C" char *
mpeg3_title_path(zmpeg3_t *zsrc, int number)
{
  return zsrc->title_path(number);
}

extern "C" int
mpeg3_is_program_stream(zmpeg3_t * zsrc)
{
  return zsrc->is_program_stream() ? 1 : 0;
}

extern "C" int
mpeg3_is_transport_stream(zmpeg3_t * zsrc)
{
  return zsrc->is_transport_stream() ? 1 : 0;
}

extern "C" int
mpeg3_is_video_stream(zmpeg3_t * zsrc)
{
  return zsrc->is_video_stream() ? 1 : 0;
}

extern "C" int
mpeg3_is_audio_stream(zmpeg3_t * zsrc)
{
  return zsrc->is_audio_stream() ? 1 : 0;
}

extern "C" int
mpeg3_is_ifo_file(zmpeg3_t * zsrc)
{
  return zsrc->is_ifo_file() ? 1 : 0;
}

extern "C" int
mpeg3_create_title(zmpeg3_t * zsrc, int full_scan)
{
  return zsrc->demuxer->create_title(full_scan);
}

extern "C" int
mpeg3audio_dolayer3(mpeg3_layer_t *zaudio,
   char *frame, int frame_size, float **output, int render)
{
  zaudio_decoder_layer_t *audio = (zaudio_decoder_layer_t *)zaudio;
  return audio->do_layer3((uint8_t*)frame, frame_size, output, render);
}

extern "C" void
mpeg3_layer_reset(mpeg3_layer_t *zlayer_data)
{
  zaudio_decoder_layer_t *layer_data = (zaudio_decoder_layer_t *)zlayer_data;
  layer_data->layer_reset();
}

extern "C" int
mpeg3_layer_header(mpeg3_layer_t *zlayer_data, unsigned char *data)
{
  zaudio_decoder_layer_t *layer_data = (zaudio_decoder_layer_t *)zlayer_data;
  return layer_data->layer3_header(data);
}

extern "C" mpeg3_layer_t*
mpeg3_new_layer()
{
  return (mpeg3_layer_t*) new zaudio_decoder_layer_t();
}

extern "C" void
mpeg3_delete_layer(mpeg3_layer_t *zaudio)
{
  zaudio_decoder_layer_t *audio = (zaudio_decoder_layer_t *)zaudio;
  delete audio;
}

extern "C" void
mpeg3_skip_video_frame(mpeg3_t *zsrc,int stream)
{
  zsrc->vtrack[stream]->demuxer->skip_video_frame();
}

extern "C" int64_t
mpeg3_video_tell_byte(mpeg3_t *zsrc, int stream)
{
  return zsrc->vtrack[stream]->demuxer->absolute_position();
}

extern "C" int64_t
mpeg3_audio_tell_byte(mpeg3_t *zsrc, int stream)
{
  return zsrc->atrack[stream]->demuxer->absolute_position();
}

#ifdef ZDVB

extern "C" int
mpeg3_dvb_channel_count(zmpeg3_t *zsrc)
{
  if( !zsrc->dvb.mgt )
	zsrc->demuxer->create_title();
  return zsrc->dvb.channel_count();
}

extern "C" int
mpeg3_dvb_get_channel(zmpeg3_t *zsrc,int n, int *major, int *minor)
{
  return zsrc->dvb.get_channel(n, *major, *minor);
}

extern "C" int
mpeg3_dvb_get_station_id(zmpeg3_t *zsrc,int n, char *name)
{
  return zsrc->dvb.get_station_id(n, name);
}

extern "C" int
mpeg3_dvb_total_astreams(zmpeg3_t *zsrc,int n, int *count)
{
  return zsrc->dvb.total_astreams(n, *count);
}

extern "C" int
mpeg3_dvb_astream_number(zmpeg3_t *zsrc,int n, int ord, int *stream, char *enc)
{
  return zsrc->dvb.astream_number(n, ord, *stream, enc);
}

extern "C" int
mpeg3_dvb_total_vstreams(zmpeg3_t *zsrc,int n, int *count)
{
  return zsrc->dvb.total_vstreams(n, *count);
}

extern "C" int
mpeg3_dvb_vstream_number(zmpeg3_t *zsrc,int n, int ord, int *stream)
{
  return zsrc->dvb.vstream_number(n, ord, *stream);
}

extern "C" int
mpeg3_dvb_get_chan_info(mpeg3_t *zsrc,int n, int ord, int i, char *cp, int len)
{
  return zsrc->dvb.get_chan_info(n, ord, i, cp, len);
}

int mpeg3_dvb_get_system_time(mpeg3_t *zsrc, int64_t *tm)
{
  return (*tm=zsrc->dvb.get_system_time()) >= 0 ? 0 : 1;
}

#endif

