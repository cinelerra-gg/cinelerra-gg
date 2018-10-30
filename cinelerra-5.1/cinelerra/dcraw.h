/* CINELERRA dcraw.c */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

class DCRaw_data;
class DCRaw;

#define CLASS DCRaw::

#if !defined(uchar)
#define uchar unsigned char
#endif
#if !defined(ushort)
#define ushort unsigned short
#endif

struct jhead;
struct tiff_tag;
struct tiff_hdr;

class DCRaw_data {
// ZEROd by DCRaw reset
public:
	FILE *ifp, *ofp;
	short order;
	const char *ifname;
	char *meta_data, xtrans[6][6], xtrans_abs[6][6];
	char cdesc[5], desc[512], make[64], model[64], model2[64], artist[64];
	float flash_used, canon_ev, iso_speed, shutter, aperture, focal_len;
	time_t timestamp;
	off_t strip_offset, data_offset;
	off_t thumb_offset, meta_offset, profile_offset;
	unsigned shot_order, kodak_cbpp, exif_cfa, unique_id;
	unsigned thumb_length, meta_length, profile_length;
	unsigned thumb_misc, *oprof, fuji_layout;
	unsigned tiff_nifds, tiff_samples, tiff_bps, tiff_compress;
	unsigned black, maximum, mix_green, raw_color, zero_is_bad;
	unsigned zero_after_ff, is_raw, dng_version, is_foveon, data_error;
	unsigned tile_width, tile_length, gpsdata[32], load_flags;
	unsigned flip, tiff_flip, filters, colors;
	ushort raw_height, raw_width, height, width, top_margin, left_margin;
	ushort shrink, iheight, iwidth, fuji_width, thumb_width, thumb_height;
	ushort *raw_image, (*image)[4], cblack[4102];
	ushort white[8][8], curve[0x10000], cr2_slice[3], sraw_mul[4];

	unsigned shot_select, multi_out;
	double pixel_aspect, aber[4], gamm[6];
	float bright, user_mul[4], threshold;
	int mask[8][4];
	int half_size, four_color_rgb, document_mode, highlight;
	int verbose, use_auto_wb, use_camera_wb, use_camera_matrix;
	int output_color, output_bps, output_tiff, med_passes;
	int no_auto_bright;
	unsigned greybox[4];
	float cam_mul[4], pre_mul[4], cmatrix[3][4], rgb_cam[3][4];
	int histogram[4][0x2000];
	void (CLASS *write_thumb)(), (CLASS *write_fun)();
	void (CLASS *load_raw)(), (CLASS *thumb_load_raw)();
	jmp_buf failure;

	struct decode {
		struct decode *branch[2];
		int leaf;
	} first_decode[2048], /* *second_decode, CINELERRA */ *free_decode;

	struct tiff_ifd {
		int width, height, bps, comp, phint, offset, flip, samples, bytes;
		int tile_width, tile_length;
		float shutter;
	} tiff_ifd[10];

	struct ph1 {
		int format, key_off, tag_21a;
		int black, split_col, black_col, split_row, black_row;
		float tag_210;
	} ph1;

// local static data
	unsigned gbh_bitbuf;
	int gbh_vbits, gbh_reset;
	uint64_t ph1_bitbuf;
	int ph1_vbits;
	float ljpeg_cs[106];
	unsigned sony_pad[128], sony_p;
	unsigned fov_huff[1024];
	float clb_cbrt[0x10000], clb_xyz_cam[3][4];
	uchar pana_buf[0x4000];  int pana_vbits;
// const static data
	static const double xyz_rgb[3][3];
	static const float d65_white[3];

};

class DCRaw : public DCRaw_data {
private:
	int fcol(int row,int col);
#if 0
	char *my_memmem(char *haystack,size_t haystacklen,char *needle,size_t needlelen);
	char *my_strcasestr(char *haystack,const char *needle);
#endif
	void merror(void *ptr,const char *where);
	void derror(void);
	ushort sget2(uchar *s);
	ushort get2(void);
	unsigned sget4(uchar *s);
	unsigned get4(void);
	unsigned getint(int type);
	float int_to_float(int i);
	double getreal(int type);
	void read_shorts(ushort *pixel,int count);
	void cubic_spline(const int *x_,const int *y_,const int len);
	void canon_600_fixed_wb(int temp);
	int canon_600_color(int ratio[2],int mar);
	void canon_600_auto_wb(void);
	void canon_600_coeff(void);
	void canon_600_load_raw(void);
	void canon_600_correct(void);
	int canon_s2is(void);
	unsigned getbithuff(int nbits,ushort *huff);
	ushort *make_decoder_ref(const uchar **source);
	ushort *make_decoder(const uchar *source);
	void crw_init_tables(unsigned table,ushort *huff[2]);
	int canon_has_lowbits(void);
	void canon_load_raw(void);
	int ljpeg_start(struct jhead *jh,int info_only);
	void ljpeg_end(struct jhead *jh);
	int ljpeg_diff(ushort *huff);
	ushort *ljpeg_row(int jrow,struct jhead *jh);
	void lossless_jpeg_load_raw(void);
	void canon_sraw_load_raw(void);
	void adobe_copy_pixel(unsigned row,unsigned col,ushort **rp);
	void ljpeg_idct(struct jhead *jh);
	void lossless_dng_load_raw(void);
	void packed_dng_load_raw(void);
	void pentax_load_raw(void);
	void nikon_load_raw(void);
	void nikon_yuv_load_raw(void);
	int nikon_e995(void);
	int nikon_e2100(void);
	void nikon_3700(void);
	int minolta_z2(void);
	void ppm_thumb(void);
	void ppm16_thumb(void);
	void layer_thumb(void);
	void rollei_thumb(void);
	void rollei_load_raw(void);
	int raw(unsigned row,unsigned col);
	void phase_one_flat_field(int is_float,int nc);
	void phase_one_correct(void);
	void phase_one_load_raw(void);
	unsigned ph1_bithuff(int nbits,ushort *huff);
	void phase_one_load_raw_c(void);
	void hasselblad_load_raw(void);
	void leaf_hdr_load_raw(void);
	void unpacked_load_raw(void);
	void sinar_4shot_load_raw(void);
	void imacon_full_load_raw(void);
	void packed_load_raw(void);
	void nokia_load_raw(void);
	void canon_rmf_load_raw(void);
	unsigned pana_bits(int nbits);
	void panasonic_load_raw(void);
	void olympus_load_raw(void);
	void canon_crx_load_raw();
	void fuji_xtrans_load_raw();
	void minolta_rd175_load_raw(void);
	void quicktake_100_load_raw(void);
	void kodak_radc_load_raw(void);
	void kodak_jpeg_load_raw(void);
	void lossy_dng_load_raw(void);
#ifndef NO_JPEG
	boolean fill_input_buffer(j_decompress_ptr cinfo);
#endif
	void kodak_dc120_load_raw(void);
	void eight_bit_load_raw(void);
	void kodak_c330_load_raw(void);
	void kodak_c603_load_raw(void);
	void kodak_262_load_raw(void);
	int kodak_65000_decode(short *out,int bsize);
	void kodak_65000_load_raw(void);
	void kodak_ycbcr_load_raw(void);
	void kodak_rgb_load_raw(void);
	void kodak_thumb_load_raw(void);
	void sony_decrypt(unsigned *data,int len,int start,int key);
	void sony_load_raw(void);
	void sony_arw_load_raw(void);
	void sony_arw2_load_raw(void);
	void samsung_load_raw(void);
	void samsung2_load_raw(void);
	void samsung3_load_raw(void);
	void smal_decode_segment(unsigned seg[2][2],int holes);
	void smal_v6_load_raw(void);
	int median4(int *p);
	void fill_holes(int holes);
	void smal_v9_load_raw(void);
	void redcine_load_raw(void);
	void foveon_decoder(unsigned size,unsigned code);
	void foveon_thumb(void);
	void foveon_sd_load_raw(void);
	void foveon_huff(ushort *huff);
	void foveon_dp_load_raw(void);
	void foveon_load_camf(void);
	const char *foveon_camf_param(const char *block,const char *param);
	void *foveon_camf_matrix(unsigned dim[3],const char *name);
	int foveon_fixed(void *ptr,int size,const char *name);
	float foveon_avg(short *pix,int range[2],float cfilt);
	short *foveon_make_curve(double max,double mul,double filt);
	void foveon_make_curves(short **curvep,float dq[3],float div[3],float filt);
	int foveon_apply_curve(short *curve,int i);
	void foveon_interpolate(void);
	void crop_masked_pixels(void);
	void remove_zeroes(void);
	void bad_pixels(const char *cfname);
	void subtract(const char *fname);
	void gamma_curve(double pwr,double ts,int mode,int imax);
	void pseudoinverse (double (*in)[3],double (*out)[3],int size);
	void cam_xyz_coeff(float rgb_cam[3][4],double cam_xyz[4][3]);
#ifdef COLORCHECK
	void colorcheck(void);
#endif
	void hat_transform(float *temp,float *base,int st,int size,int sc);
	void wavelet_denoise(void);
	void scale_colors(void);
	void pre_interpolate(void);
	void border_interpolate(int border);
	void lin_interpolate(void);
	void vng_interpolate(void);
	void ppg_interpolate(void);
	void cielab(ushort rgb[3],short lab[3]);
	void xtrans_interpolate(int passes);
	void ahd_interpolate(void);
	void median_filter(void);
	void blend_highlights(void);
	void recover_highlights(void);
	void tiff_get(unsigned base,unsigned *tag,unsigned *type,unsigned *len,unsigned *save);
	void parse_thumb_note(int base,unsigned toff,unsigned tlen);
	void parse_makernote(int base,int uptag);
	void get_timestamp(int reversed);
	void parse_exif(int base);
	void parse_gps(int base);
	void romm_coeff(float romm_cam[3][3]);
	void parse_mos(int offset);
	void linear_table(unsigned len);
	void parse_kodak_ifd(int base);
	int parse_tiff_ifd(int base);
	int parse_tiff(int base);
	void apply_tiff(void);
	void parse_minolta(int base);
	void parse_external_jpeg(void);
	void ciff_block_1030(void);
	void parse_ciff(int offset,int length,int depth);
	void parse_rollei(void);
	void parse_sinar_ia(void);
	void parse_phase_one(int base);
	void parse_fuji(int offset);
	int parse_jpeg(int offset);
	void parse_riff(void);
	void parse_crx(int end);
	void parse_qt(int end);
	void parse_smal(int offset,int fsize);
	void parse_cine(void);
	void parse_redcine(void);
	char *foveon_gets(int offset,char *str,int len);
	void parse_foveon(void);
	void adobe_coeff(const char *make,const char *model);
	void simple_coeff(int index);
	short guess_byte_order(int words);
	float find_green(int bps,int bite,int off0,int off1);
	void identify(void);
#ifndef NO_LCMS
	void apply_profile(const char *input,const char *output);
#endif
	void convert_to_rgb(void);
	void fuji_rotate(void);
	void stretch(void);
	int flip_index(int row, int col);
	void tiff_set(struct tiff_hdr *th,ushort *ntag,ushort tag,ushort type,int count,int val);
	void tiff_head(struct tiff_hdr *th,int full);
	void jpeg_thumb(void);
	void write_ppm_tiff(void);
	void write_cinelerra(void);
	void reset();
public:
	DCRaw();
	~DCRaw();
// CINELERRA
	char info[1024];
	float **data;
	int alpha;
	float matrix[9];
	int main(int argc, const char **argv);
};

