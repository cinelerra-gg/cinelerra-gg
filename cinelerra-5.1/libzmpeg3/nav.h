#ifndef __NAV_H__
#define __NAV_H__

uint8_t dsi[NAV_DSI_BYTES];
uint8_t pci[NAV_PCI_BYTES];

/* DSI General Information */
uint32_t dsi_gi_pck_scr() { return get4bytes(dsi + 0x0000); }
/* sector address of this nav pack */
uint32_t dsi_gi_pck_lbn() { return get4bytes(dsi + 0x0004); }
/* end address of this VOBU */
uint32_t dsi_gi_vobu_ea() { return get4bytes(dsi + 0x0008); }
/* end address of the 1st,2nd,3rd reference image */
uint32_t dsi_gi_vobu_1stref_ea() { return get4bytes(dsi + 0x000c); }
uint32_t dsi_gi_vobu_2ndref_ea() { return get4bytes(dsi + 0x0010); }
uint32_t dsi_gi_vobu_3rdref_ea() { return get4bytes(dsi + 0x0014); }
/* VOB Id number in which this VOBU exists */
uint32_t dsi_gi_vobu_vob_idn() { return get2bytes(dsi + 0x0018); }
/* Cell Id number in which this VOBU exists */
uint32_t dsi_gi_vobu_c_idn() { return *(dsi + 0x001b); }
/* Cell elapsed time, dvd_time_t */
uint32_t dsi_gi_elaped_time() { return get4bytes(dsi + 0x001c); }
/* Seamless Playback Information, stp/gap occurs 8 times */
uint32_t dsi_sp_category() { return get2bytes(dsi + 0x0020); }
/* end address of interleaved Unit */
uint32_t dsi_sp_ilvu_ea() { return get4bytes(dsi + 0x0022); }
/* start address of next interleaved unit */
uint32_t dsi_sp_ilvu_sa() { return get4bytes(dsi + 0x0026); }
/* size of next interleaved unit */
uint32_t dsi_sp_size() { return get2bytes(dsi + 0x002a); }
/* video start ptm in vob */
uint32_t dsi_sp_vob_v_s_s_ptm() { return get4bytes(dsi + 0x002c); }
/* video end ptm in vob */
uint32_t dsi_sp_vob_v_e_e_ptm() { return get4bytes(dsi + 0x0030); }
uint32_t dsi_sp_stp_ptm1(int i) { return get4bytes(dsi + 0x0034 + 16*i); }
uint32_t dsi_sp_stp_ptm2(int i) { return get4bytes(dsi + 0x0038 + 16*i); }
uint32_t dsi_sp_gap_len1(int i) { return get4bytes(dsi + 0x003c + 16*i); }
uint32_t dsi_sp_gap_len2(int i) { return get4bytes(dsi + 0x0040 + 16*i); }
/* Seamless Angle Information, occurs 9 times */
/* offset to next ILVU, high bit is before/after */
uint32_t dsi_sa_address(int i) { return get4bytes(dsi + 0x00b4 + 6*i); }
/* byte size of the ILVU pointed to by address */
uint32_t dsi_sa_size(int i) { return get4bytes(dsi + 0x00b8 + 6*i); }
/* VOBU Search Information, fwda occurs 19 times, bwda occurs 19 times */
/* Next vobu that contains video */
uint32_t dsi_si_next_video() { return get4bytes(dsi + 0x00ea); }
/* Forwards, time */
uint32_t dsi_si_fwda(int i) { return get4bytes(dsi + 0x00ee + 4*i); }
uint32_t dsi_si_next_vobu() { return get4bytes(dsi + 0x13a); }
uint32_t dsi_si_prev_vobu() { return get4bytes(dsi + 0x13e); }
/* Backwards, time */
uint32_t dsi_si_bwda(int i) { return get4bytes(dsi + 0x142 + 4*i); }
uint32_t dsi_si_prev_video() { return get4bytes(dsi + 0x18e); }
/* Synchronous Information, sy_a occurs 8 times, sy_sp occurs 32 times */
/* offset to first audio packet for this VOBU */
uint32_t dsi_sy_a_synca(int i) { return get2bytes(dsi + 0x192 + 4*i); }
/* offset to first subpicture packet */
uint32_t dsi_sy_sp_synca(int i) { return get4bytes(dsi + 0x1a2 + 4*i); }


/* PCI General Information */
/* sector address of this nav pack */
uint32_t pci_gi_pck_lbn() { return get4bytes(pci + 0x00); }
/* category of vobu */
uint32_t pci_gi_vobu_cat() { return get2bytes(pci + 0x04); }
/* UOP of vobu */
uint32_t pci_gi_vobu_uop_ctl() { return get4bytes(pci + 0x08); }
/* start presentation time of vobu */
uint32_t pci_gi_vobu_s_ptm() { return get4bytes(pci + 0x0c); }
/* end presentation time of vobu */
uint32_t pci_gi_vobu_e_ptm() { return get4bytes(pci + 0x10); }
/* end ptm of sequence end in vobu */
uint32_t pci_gi_vobu_se_e_ptm() { return get4bytes(pci + 0x14); }
/* Cell elapsed time */
uint32_t pci_gi_e_eltm() { return get4bytes(pci + 0x18); }
uint8_t *pci_gi_vobu_isrc() { return pci + 0x1c; }
/* Non Seamless Angle Information, occurs 9 times */
uint32_t pci_nsml_agl_dsta(int i) { return get4bytes(pci + 0x3c + 4*i); }
/* Highlight General Information */
/* status, 0:no btns, 1:!= 2:== 3:== except for btn cmds */
uint32_t pci_hgi_ss() { return get2bytes(pci + 0x60) & 0x3; }
/* start ptm of hli */
uint32_t pci_hgi_s_ptm() { return get4bytes(pci + 0x62); }
/* end ptm of hli */
uint32_t pci_hgi_e_ptm() { return get4bytes(pci + 0x66); }
/* end ptm of button select */
uint32_t pci_hgi_btn_se_e_ptm() { return get4bytes(pci + 0x6a); }
/* number of button groups 1, 2 or 3 with 36/18/12 buttons */
uint32_t pci_hgi_btngr_ns() { return (*(pci+0x6e)>>4) & 0x03; }
/* display type of subpic stream for button group 1 */
uint32_t pci_hgi_btngr1_dsp_ty() { return (*(pci+0x6e)>>0) & 0x07; }
/* display type of subpic stream for button group 2 */
uint32_t pci_hgi_btngr2_dsp_ty() { return (*(pci+0x6f)>>4) & 0x07; }
/* display type of subpic stream for button group 3 */
uint32_t pci_hgi_btngr3_dsp_ty() { return (*(pci+0x6f)>>0) & 0x07; }
/* button offset number range 0-255 */
uint32_t pci_hgi_btn_ofn() { return *(pci + 0x70); }
/* number of valid buttons  <= 36/18/12 */
uint32_t pci_hgi_btn_ns() { return *(pci + 0x71) & 0x3f; }
/* number of buttons selectable by U_BTNNi, nsl_btn_ns <= btn_ns */
uint32_t pci_hgi_nsl_btn_ns() { return *(pci + 0x72) & 0x3f; }
/**< forcedly selected button */
uint32_t pci_hgi_fosl_btnn() { return *(pci + 0x74) & 0x3f; }
/**< forcedly activated button */
uint32_t pci_hgi_foac_btnn() { return *(pci + 0x75) & 0x3f; }
/* color occurs 3 times, action occurs 2 times */
/*  each of first 4 nibs refs pgc palette, next 4 nibs are alphas */
uint32_t pci_btn_coli(int c,int i) { return get4bytes(pci + 0x76 + 8*c+4*i); }
/* Button Information, occurs 36 times */
uint32_t pci_btn_coln(int i) { /* button color number */
  return (*(pci + 0x8e + 18*i)>>6) & 0x03;
}
uint32_t pci_btn_x_start(int i) { /* x start offset within the overlay */
  uint8_t *p = pci + 0x8e + 18*i;
  return (((p[0]<<8) + p[1])>>4) & 0x3ff;
}
uint32_t pci_btn_x_end(int i) { /* x end offset within the overlay */
  uint8_t *p = pci + 0x8f + 18*i;
  return ((p[0]<<8) + p[1]) & 0x3ff;
}
uint32_t pci_btn_auto_action_mode(int i) { /* 0:no, 1:armed if selected */
  return (*(pci + 0x91 + 18*i)>>6) & 0x03;
}
uint32_t pci_btn_y_start(int i) { /**< y start offset within the overlay */
  uint8_t *p = pci + 0x91 + 18*i;
  return (((p[0]<<8) + p[1])>>4) & 0x3ff;
}
uint32_t pci_btn_y_end(int i) { /* y end offset within the overlay */
  uint8_t *p = pci + 0x93 + 18*i;
  return ((p[0]<<8) + p[1]) & 0x3ff;
}
/* button index when pressing up */
uint32_t pci_btn_up(int i) { return (*(pci + 0x94 + 18*i)>>0) & 0x3f; }
/* button index when pressing down */
uint32_t pci_btn_down(int i) { return (*(pci + 0x95 + 18*i)>>0) & 0x3f; }
/* button index when pressing left */
uint32_t pci_btn_left(int i) { return (*(pci + 0x96 + 18*i)>>0) & 0x3f; }
/* button index when pressing right */
uint32_t pci_btn_right(int i) { return (*(pci + 0x97 + 18*i)>>0) & 0x3f; }
/* command executed by button (8 bytes) */
uint8_t *pci_btn_cmd(int i) { return pci + 0x98 + 18*i; }

#endif
