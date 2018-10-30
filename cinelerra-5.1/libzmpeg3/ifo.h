#ifndef __IFO_H__
#define __IFO_H__

/*
id_PTT
  VTS_PTT_SRPT  data[id_PTT]
    nr_of_titles()
    srpt_last_byte()
    idx - use_ptt_info(title)
      0 ptt_pgcn(chapter)  Program Chain number
        ptt_pgn(chapter)   Program number
    ..  nr_of_chapters(title)
  ..  nr_of_titles()

id_TITLE_PGCI
  VTS_PGCI  data[id_TITLE_PGCI]
    nr_of_pgci_srp()
    pgci_length()
    entry_id(pgc)
    block_mode(pgc)
    block_type(pgc)
    ptl_id_mask(pgc)
    pgc_start_offset(pgc)
    idx - use_program_chain(pgc)
      0 nr_of_programs()
        nr_of_cells()
        playback_time()
        prohibited_ops()
        audio_control(0..7)
        subp_control(0..31)
        next_pgc_nr()
        prev_pgc_nr()
        goup_pgc_nr()
        pg_playback_mode()
        still_time()
        palette()
        command_tbl_offset()
        program_map_offset()
        cell_playback_offset()
        cell_position_offset()
        pgc_command_tbl()
        pgc_program_map()[program_no..nr_of_programs()]
        pgc_cell_playback()
        idx - use_playback(pcell_no)
          0 cell_block_mode()
            cell_block_type()
            cell_seamless_play()
            cell_interleaved()
            cell_stc_discontinuity()
            cell_seamless_angle()
            cell_playback_mode()
            cell_restricted()
            cell_still_time()
            cell_cmd_nr()
            cell_playback_time()
            cell_first_sector()
            cell_first_ilvu_end_sector()
            cell_last_vobu_start_sector()
            cell_last_sector()
        ..  nr_of_cells()
        pgc_cell_position()
        idx - use_position(pcell_no)
          0 vob_id_nr()
            cell_id_nr()
        ..  nr_of_programs()
    ..  nr_of_pgci_srp()
*/

uint8_t *zpgci;
uint8_t *zcell;
uint8_t *tinfo;
uint8_t *pinfo;

/* video attribute */
uint32_t vid_mpeg_version(uint8_t *p) { return (p[0]>>6) & 0x03; }
uint32_t vid_video_format(uint8_t *p) { return (p[0]>>4) & 0x03; }
uint32_t vid_display_aspect_ratio(uint8_t *p) { return (p[0]>>2) & 0x03; }
uint32_t vid_permitted_df(uint8_t *p) { return (p[0]>>0) & 0x03; }
uint32_t vid_line21_cc_1(uint8_t *p) { return (p[1]>>7) & 0x01; }
uint32_t vid_line21_cc_2(uint8_t *p) { return (p[1]>>6) & 0x01; }
uint32_t vid_bit_rate(uint8_t *p) { return (p[1]>>4) & 0x01; }
uint32_t vid_picture_size(uint8_t *p) { return (p[1]>>2) & 0x03; }
uint32_t vid_letterboxed(uint8_t *p) { return (p[1]>>1) & 0x01; }
uint32_t vid_film_mode(uint8_t *p) { return (p[1]>>0) & 0x01; }

/* audio attribute */
uint32_t aud_audio_format(uint8_t *p) { return (p[0]>>5) & 0x07; }
uint32_t aud_multichannel_extension(uint8_t *p) { return (p[0]>>4) & 0x01; }
uint32_t aud_lang_type(uint8_t *p) { return (p[0]>>2) & 0x03; }
uint32_t aud_application_mode(uint8_t *p) { return (p[0]>>0) & 0x03; }
uint32_t aud_quantization(uint8_t *p) { return (p[1]>>6) & 0x03; }
uint32_t aud_sample_frequency(uint8_t *p) { return (p[1]>>4) & 0x03; }
uint32_t aud_channels(uint8_t *p) { return (p[1]>>0) & 0x07; }
uint32_t aud_lang_code(uint8_t *p) { return get2bytes(p + 0x02); }
uint32_t aud_lang_extension(uint8_t *p) { return p[4]; }
uint32_t aud_code_extension(uint8_t *p) { return p[5]; }
uint32_t aud_kar_channel_assignment(uint8_t *p) { return (p[7]>>4) & 0x07; }
uint32_t aud_kar_version(uint8_t *p) { return (p[7]>>2) & 0x03; }
uint32_t aud_kar_mc_intro(uint8_t *p) { return (p[7]>>1) & 0x01; }
uint32_t aud_kar_mode(uint8_t *p) { return (p[7]>>7) & 0x01; }
uint32_t aud_sur_dolby_encoded(uint8_t *p) { return (p[7]>>4) & 0x01; }

/* subtitle attribute */
uint32_t sub_code_mode(uint8_t *p) { return (p[0]>>5) & 0x07; }
uint32_t sub_type(uint8_t *p) { return (p[0]>>0) & 0x03; }
uint32_t sub_lang_code(uint8_t *p) { return get2bytes(p + 0x02); }
uint32_t sub_lang_extension(uint8_t *p) { return p[4]; }
uint32_t sub_code_extension(uint8_t *p) { return p[5]; }

/* multichannel attribute */
uint32_t mch_ach0_gme(uint8_t *p) { return (p[0]>>0) & 0x01; }
uint32_t mch_ach1_gme(uint8_t *p) { return (p[1]>>0) & 0x01; }
uint32_t mch_ach2_gv1e(uint8_t *p) { return (p[2]>>3) & 0x01; }
uint32_t mch_ach2_gv2e(uint8_t *p) { return (p[2]>>2) & 0x01; }
uint32_t mch_ach2_gm1e(uint8_t *p) { return (p[2]>>1) & 0x01; }
uint32_t mch_ach2_gm2e(uint8_t *p) { return (p[2]>>0) & 0x01; }
uint32_t mch_ach3_gv1e(uint8_t *p) { return (p[3]>>3) & 0x01; }
uint32_t mch_ach3_gv2e(uint8_t *p) { return (p[3]>>2) & 0x01; }
uint32_t mch_ach3_gmAe(uint8_t *p) { return (p[3]>>1) & 0x01; }
uint32_t mch_ach3_se2e(uint8_t *p) { return (p[3]>>0) & 0x01; }
uint32_t mch_ach4_gv1e(uint8_t *p) { return (p[4]>>3) & 0x01; }
uint32_t mch_ach4_gv2e(uint8_t *p) { return (p[4]>>2) & 0x01; }
uint32_t mch_ach4_gmBe(uint8_t *p) { return (p[4]>>1) & 0x01; }
uint32_t mch_ach4_seBe(uint8_t *p) { return (p[4]>>0) & 0x01; }

/* vgm_mat */
uint8_t *vmg_identifier() { return data[id_MAT] + 0x0000; }
uint32_t vmg_last_sector() { return get4bytes(data[id_MAT] + 0x000c); }
uint32_t vmgi_last_sector() { return get4bytes(data[id_MAT] + 0x001c); }
uint32_t vgm_specification_version() { return *(data[id_MAT] + 0x0021); }
uint32_t vmg_category() { return get4bytes(data[id_MAT] + 0x0022); }
uint32_t vmg_nr_of_volumes() { return get2bytes(data[id_MAT] + 0x0026); }
uint32_t vmg_this_volume_nr() { return get2bytes(data[id_MAT] + 0x0028); }
uint32_t disc_side() { return *(data[id_MAT] + 0x002a); }
uint32_t vmg_nr_of_title_sets() { return get2bytes(data[id_MAT] + 0x003e); }
uint8_t *provider_identifier() { return data[id_MAT] + 0x0040; }
uint64_t vmg_pos_code() { return get8bytes(data[id_MAT] + 0x0060); }
uint32_t vmgi_last_byte() { return get4bytes(data[id_MAT] + 0x0080); }
uint32_t first_play_pgc() { return get4bytes(data[id_MAT] + 0x0084); }
uint32_t vmgm_vobs() { return get4bytes(data[id_MAT] + 0x00c0); }
uint32_t tt_srpt() { return get4bytes(data[id_MAT] + 0x00c4); }
uint32_t vmgm_pgci_ut() { return get4bytes(data[id_MAT] + 0x00c8); }
uint32_t ptl_mait() { return get4bytes(data[id_MAT] + 0x00cc); }
uint32_t vts_atrt() { return get4bytes(data[id_MAT] + 0x00d0); }
uint32_t txtdt_mgi() { return get4bytes(data[id_MAT] + 0x00d4); }
uint32_t vmgm_c_adt() { return get4bytes(data[id_MAT] + 0x00d8); }
uint32_t vmgm_vobu_admap() { return get4bytes(data[id_MAT] + 0x00dc); }
uint8_t *vmgm_video_attr() { return data[id_MAT] + 0x0100; }
uint32_t nr_of_vmgm_audio_streams() { return *(data[id_MAT] + 0x0103); }
uint8_t *vmgm_audio_attr() { return data[id_MAT] + 0x0104; }
uint32_t nr_of_vmgm_subp_streams() { return *(data[id_MAT] + 0x0155); }
uint8_t *vmgm_subp_attr() { return data[id_MAT] + 0x0156; }

/* vts_mat */
uint8_t *vts_identifier() { return data[id_MAT] + 0x0000; }
uint32_t vts_last_sector() { return get4bytes(data[id_MAT] + 0x000c); }
uint32_t vtsi_last_sector() { return get4bytes(data[id_MAT] + 0x001c); }
uint32_t vts_specification_version() { return *(data[id_MAT] + 0x0021); }
uint32_t vts_category() { return get4bytes(data[id_MAT] + 0x0022); }
uint32_t vtsi_last_byte() { return get4bytes(data[id_MAT] + 0x0080); }
/* sector addresses follow */
uint32_t vtsm_vobs() { return get4bytes(data[id_MAT] + 0x00c0); }
uint32_t vtstt_vobs() { return get4bytes(data[id_MAT] + 0x00c4); }
uint32_t vts_ptt_srpt() { return get4bytes(data[id_MAT] + 0x00c8); }
uint32_t vts_pgcit() { return get4bytes(data[id_MAT] + 0x00cc); }
uint32_t vtsm_pgci_ut() { return get4bytes(data[id_MAT] + 0x00d0); }
uint32_t vts_tmapt() { return get4bytes(data[id_MAT] + 0x00d4); }
uint32_t vtsm_c_adt() { return get4bytes(data[id_MAT] + 0x00d8); }
uint32_t vtsm_vobu_admap() { return get4bytes(data[id_MAT] + 0x00dc); }
/* end of sector addresses */
uint32_t vts_c_adt() { return get4bytes(data[id_MAT] + 0x00e0); }
uint32_t vts_vobu_admap() { return get4bytes(data[id_MAT] + 0x00e4); }
uint8_t *vtsm_video_attr() { return data[id_MAT] + 0x0100; }
uint32_t nr_of_vtsm_audio_streams() { return *(data[id_MAT] + 0x0103); }
uint8_t *vtsm_audio_attr() { return data[id_MAT] + 0x0104; }
uint32_t nr_of_vtsm_subp_streams() { return *(data[id_MAT] + 0x0155); }
uint8_t *vtsm_subp_attr() { return data[id_MAT] + 0x0156; }
uint32_t vts_video_attr() { return get2bytes(data[id_MAT] + 0x0200); }
uint32_t nr_of_vts_audio_streams() { return *(data[id_MAT] + 0x0203); }
uint8_t *vts_audio_attr(int i) { return data[id_MAT] + 0x0204 + 8*i; }
uint32_t nr_of_vts_subp_streams() { return *(data[id_MAT] + 0x0255); }
uint8_t *vts_subp_attr(int i) { return data[id_MAT] + 0x0256 + 6*i; }
uint8_t *vts_mu_audio_attr(int i) { return data[id_MAT] + 0x0316 + 24*i; }

/* hdr */
uint32_t hdr_entries(uint8_t *p) { return get2bytes(p + 0x00); }
uint32_t hdr_length(uint8_t *p) { return get4bytes(p + 0x04)+1; }

/* pgci header */
uint32_t nr_of_pgci_srp() { return get2bytes(data[id_TITLE_PGCI] + 0x00); }
uint32_t pgci_length() { return get4bytes(data[id_TITLE_PGCI] + 0x04)+1; }

/* pgci search ptr */
uint32_t entry_id(int pgc) {
  return *(data[id_TITLE_PGCI] + 8 + 8*pgc + 0x00);
}
uint32_t block_mode(int pgc) {
  return (*(data[id_TITLE_PGCI] + 8 + 8*pgc + 0x01)>>6) & 0x03;
}
uint32_t block_type(int pgc) {
  return (*(data[id_TITLE_PGCI] + 8 + 8*pgc + 0x01)>>4) & 0x03;
}
uint32_t ptl_id_mask(int pgc) {
  return get2bytes(data[id_TITLE_PGCI] + 8 + 8*pgc + 0x02);
}
uint32_t pgc_start_offset(int pgc) {
  return get4bytes(data[id_TITLE_PGCI] + 8 + 8*pgc + 0x04);
}
  
/* pgci unit table */
void use_program_chain(int pgc) {
  zpgci = data[id_TITLE_PGCI] + pgc_start_offset(pgc);
}
uint32_t nr_of_programs() { return *(zpgci + 0x0002); }
uint32_t nr_of_cells() { return *(zpgci + 0x0003); }
uint8_t *playback_time() { return zpgci + 0x0004; }
uint8_t *prohibited_ops() { return zpgci + 0x0008; }
uint32_t audio_control(int i) { return get2bytes(zpgci + 0x000c + 2*i); }
uint32_t subp_control(int i) { return get4bytes(zpgci + 0x001c + 4*i); }
uint32_t next_pgc_nr() { return get2bytes(zpgci + 0x009c); }
uint32_t prev_pgc_nr() { return get2bytes(zpgci + 0x009e); }
uint32_t goup_pgc_nr() { return get2bytes(zpgci + 0x00a0); }
uint32_t still_time() { return *(zpgci + 0x00a2); }
uint32_t pg_playback_mode() { return *(zpgci + 0x00a3); }
uint8_t *palette() { return zpgci + 0x00a4; }
uint32_t command_tbl_offset() { return get2bytes(zpgci + 0x00e4); }
uint32_t program_map_offset() { return get2bytes(zpgci + 0x00e6); }
uint32_t cell_playback_offset() { return get2bytes(zpgci + 0x00e8); }
uint32_t cell_position_offset() { return get2bytes(zpgci + 0x00ea); }
uint8_t *pgc_command_tbl() { return zpgci + command_tbl_offset(); }
uint8_t *pgc_program_map() { return zpgci + program_map_offset(); }
uint8_t *pgc_cell_playback() { return zpgci + cell_playback_offset(); }
uint8_t *pgc_cell_position() { return zpgci + cell_position_offset(); }

/* playback type */
/* 0: one sequential pgc title */
uint32_t multi_or_random_pgc_title(uint8_t v) { return ((v>>6) & 1); }
uint32_t jlc_exists_in_cell_cmd(uint8_t v) { return ((v>>5) & 1); }
uint32_t jlc_exists_in_prepost_cmd(uint8_t v) { return ((v>>4) & 1); }
uint32_t jlc_exists_in_button_cmd(uint8_t v) { return ((v>>3) & 1); }
uint32_t jlc_exists_in_tt_dom(uint8_t v) { return ((v>>2) & 1); }
/* uop 1 */
uint32_t chapter_search_or_play(uint8_t v) { return ((v>>1) & 1); }
/* uop 0 */
uint32_t title_or_time_play(uint8_t v) { return ((v>>0) & 1); }

/* search pointer table header */
uint32_t nr_of_titles() { return get2bytes(data[id_PTT] + 0x00); }
uint32_t srpt_last_byte() { return get4bytes(data[id_PTT] + 0x04); }

/* PTT information header */
uint32_t ptt_info_offset(int i) { return get4bytes(data[id_PTT] + 8 + 4*i); }
/* PTT information */
uint32_t nr_of_chapters(int title) {
  uint32_t n = title+1 >= (int)nr_of_titles() ?
    srpt_last_byte()+1 : ptt_info_offset(title+1);
  return (n-ptt_info_offset(title)) / 4;
}
void use_ptt_info(int title) {
  pinfo = data[id_PTT] + ptt_info_offset(title);
}
uint8_t *ptt_info(int i) { return pinfo + 4*i; }
uint32_t ptt_pgcn(int chapter) { return get2bytes(ptt_info(chapter) + 0x00); }
uint32_t ptt_pgn(int chapter) { return get2bytes(ptt_info(chapter) + 0x02); }

/* title information */
uint32_t tsp_nr_of_srpts() { return get2bytes(data[id_TSP] + 0x00); }
uint32_t tsp_last_byte() { return get4bytes(data[id_TSP] + 0x04); }
void use_title_info(int i) {
  tinfo = data[id_TSP] + 8 * 12*i;
}
uint32_t ti_pb_ty() { return *(tinfo + 0x00); }
uint32_t ti_nr_of_angles() { return *(tinfo + 0x01); }
uint32_t ti_nr_of_ptts() { return get2bytes(tinfo + 0x02); }
uint32_t ti_parental_id() { return get2bytes(tinfo + 0x04); }
uint32_t ti_title_set_nr() { return *(tinfo + 0x06); }
uint32_t ti_vts_ttn() { return *(tinfo + 0x07); }
uint32_t ti_title_set_sector() { return get4bytes(tinfo + 0x08); }

/* user ops */
uint32_t usr_video_pres_mode_change(uint8_t *p) { return (p[0]>>0) & 0x01; }
uint32_t usr_karaoke_audio_pres_mode_change(uint8_t *p) { return (p[1]>>7) & 0x01; }
uint32_t usr_angle_change(uint8_t *p) { return (p[1]>>6) & 0x01; }
uint32_t usr_subpic_stream_change(uint8_t *p) { return (p[1]>>5) & 0x01; }
uint32_t usr_audio_stream_change(uint8_t *p) { return (p[1]>>4) & 0x01; }
uint32_t usr_pause_on(uint8_t *p) { return (p[1]>>3) & 0x01; }
uint32_t usr_still_off(uint8_t *p) { return (p[1]>>2) & 0x01; }
uint32_t usr_button_select_or_activate(uint8_t *p) { return (p[1]>>1) & 0x01; }
uint32_t usr_resume(uint8_t *p) { return (p[1]>>0) & 0x01; }
uint32_t usr_chapter_menu_call(uint8_t *p) { return (p[2]>>7) & 0x01; }
uint32_t usr_angle_menu_call(uint8_t *p) { return (p[2]>>6) & 0x01; }
uint32_t usr_audio_menu_call(uint8_t *p) { return (p[2]>>5) & 0x01; }
uint32_t usr_subpic_menu_call(uint8_t *p) { return (p[2]>>4) & 0x01; }
uint32_t usr_root_menu_call(uint8_t *p) { return (p[2]>>3) & 0x01; }
uint32_t usr_title_menu_call(uint8_t *p) { return (p[2]>>2) & 0x01; }
uint32_t usr_backward_scan(uint8_t *p) { return (p[2]>>1) & 0x01; }
uint32_t usr_forward_scan(uint8_t *p) { return (p[2]>>0) & 0x01; }
uint32_t usr_next_pg_search(uint8_t *p) { return (p[3]>>7) & 0x01; }
uint32_t usr_prev_or_top_pg_search(uint8_t *p) { return (p[3]>>6) & 0x01; }
uint32_t usr_time_or_chapter_search(uint8_t *p) { return (p[3]>>5) & 0x01; }
uint32_t usr_go_up(uint8_t *p) { return (p[3]>>4) & 0x01; }
uint32_t usr_stop(uint8_t *p) { return (p[3]>>3) & 0x01; }
uint32_t usr_title_play(uint8_t *p) { return (p[3]>>2) & 0x01; }
uint32_t usr_chapter_search_or_play(uint8_t *p) { return (p[3]>>1) & 0x01; }
uint32_t usr_title_or_time_play(uint8_t *p) { return (p[3]>>0) & 0x01; }

/* cell playback */
void use_playback(int i) {
  zcell = pgc_cell_playback() + 24*i;
}
enum { BTY_NONE = 0x0, BTY_ANGLE = 0x1,
       BMD_NOT_IN_BLOCK = 0x0, BMD_FIRST_CELL = 0x1,
       BMD_IN_BLOCK = 0x2, BMD_LAST_CELL = 0x3, };
uint32_t cell_block_mode() { return (zcell[0]>>6) & 0x03; }
uint32_t cell_block_type() { return (zcell[0]>>4) & 0x03; }
uint32_t cell_seamless_play() { return (zcell[0]>>3) & 0x01; }
uint32_t cell_interleaved() { return (zcell[0]>>2) & 0x01; }
uint32_t cell_stc_discontinuity() { return (zcell[0]>>1) & 0x01; }
uint32_t cell_seamless_angle() { return (zcell[0]>>0) & 0x01; }
uint32_t cell_playback_mode() { return (zcell[1]>>7) & 0x01; }
uint32_t cell_restricted() { return (zcell[1]>>6) & 0x01; }
uint32_t cell_still_time() { return zcell[2]; }
uint32_t cell_cmd_nr() { return zcell[3]; }
uint8_t *cell_playback_time() { return zcell + 0x04; }
uint32_t cell_first_sector() { return get4bytes(zcell + 0x08); }
uint32_t cell_first_ilvu_end_sector() { return get4bytes(zcell + 0x0c); }
uint32_t cell_last_vobu_start_sector() { return get4bytes(zcell + 0x10); }
uint32_t cell_last_sector() { return get4bytes(zcell + 0x14); }

/* cell position */
void use_position(int i) {
  zcell = pgc_cell_position() + 4*i;
}
uint32_t vob_id_nr() { return get2bytes(zcell + 0x0000); }
uint32_t cell_id_nr() { return *(zcell + 0x0003); }

/* cell address */
uint32_t cadr_nr_of_vobs() {
  return get2bytes(data[id_TITLE_CELL_ADDR] + 0x0000);
}
uint32_t cadr_last_byte() {
  return get4bytes(data[id_TITLE_CELL_ADDR] + 0x0004);
}
void use_cell(int i) {
  zcell = data[id_TITLE_CELL_ADDR] + 0x0008 + 12*i;
}
uint32_t cadr_vob_id() { return get2bytes(zcell + 0x00); }
uint32_t cadr_cell_id() { return *(zcell + 0x02); }
uint32_t cadr_start_sector() { return get4bytes(zcell + 0x04); }
uint32_t cadr_last_sector() { return get4bytes(zcell + 0x08); }
uint32_t cadr_length() { return (cadr_last_byte()+1 - 8) / 12; }

#endif
