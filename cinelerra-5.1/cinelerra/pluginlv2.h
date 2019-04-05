#ifndef __PLUGINLV2_H__
#define __PLUGINLV2_H__

#define LV2_SEQ_SIZE  9624
#include "forkbase.h"
#include "pluginlv2config.h"
#include "samples.inc"

#include <lilv/lilv.h>
#define NS_EXT "http://lv2plug.in/ns/ext/"

typedef struct {
	int64_t sz;
	int samples, done;
	int nb_inputs, nb_outputs;
} shm_bfr_t;

#define PORTS_AUDIO   0x01
#define PORTS_CONTROL 0x02
#define PORTS_ATOM    0x04
#define PORTS_ALL (PORTS_AUDIO | PORTS_CONTROL | PORTS_ATOM)
#define PORTS_INPUT   0x08
#define PORTS_OUTPUT  0x10
#define PORTS_UPDATE  0x20

class PluginLV2Options : public ArrayList<LV2_Options_Option>
{
public:
	PluginLV2Options() {}
	~PluginLV2Options() {}
	void add(LV2_URID key, unsigned sz, LV2_URID typ, void *vp) {
		LV2_Options_Option *ap = &append();
		ap->context = LV2_OPTIONS_INSTANCE; ap->subject = 0;
		ap->key = key; ap->size = sz; ap->type = typ; ap->value = vp;
	}
};

class PluginLV2Work
{
public:
	PluginLV2Work();
	~PluginLV2Work();
	void load(const void *vp, unsigned size);

	PluginLV2Work *next;
	unsigned alloc, used;
	char *data;
};

class PluginLV2
{
public:
	PluginLV2();
	virtual ~PluginLV2();

	shm_bfr_t *shm_bfr;
	int use_shm, shmid;
	float **in_buffers, **out_buffers;
	int *iport, *oport;
	int nb_inputs, nb_outputs;
	int max_bufsz, ui_features;

	void reset_lv2();
	int load_lv2(const char *path,char *title=0);
	int init_lv2(PluginLV2ClientConfig &conf, int sample_rate, int bfrsz);
	virtual int is_forked() { return 0; }
	static uint32_t map_uri(LV2_URID_Map_Handle handle, const char *uri);
	static const char *unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid);
	LV2_URID_Map uri_map;
	LV2_URID_Unmap uri_unmap;
	void connect_ports(PluginLV2ClientConfig &conf, int ports);
	void del_buffer();
	void new_buffer(int64_t sz);
	shm_bfr_t *shm_buffer(int shmid);
	void init_buffer(int samples);
	void map_buffer();

	LilvWorld         *world;
	const LilvPlugin  *lilv;
	LilvUIs           *lilv_uis;

	PluginLV2UriTable  uri_table;
	LV2_URID_Map       map;
	LV2_Feature        map_feature;
	LV2_URID_Unmap     unmap;
	LV2_Feature        unmap_feature;

	PluginLV2Options   options;
	Lv2Features        features;
	LV2_Atom_Sequence  seq_in[2];
	LV2_Atom_Sequence  *seq_out;
	float samplerate, refreshrate;
	int min_block_length, block_length;
	int midi_buf_size;

	LilvInstance *inst;
	SuilInstance *sinst;
	SuilHost *ui_host;

	LilvNode *atom_AtomPort;
	LilvNode *atom_Sequence;
	LilvNode *lv2_AudioPort;
	LilvNode *lv2_CVPort;
	LilvNode *lv2_ControlPort;
	LilvNode *lv2_Optional;
	LilvNode *lv2_InputPort;
	LilvNode *lv2_OutputPort;
	LilvNode *powerOf2BlockLength;
	LilvNode *fixedBlockLength;
	LilvNode *boundedBlockLength;

	LV2_URID atom_int;
	LV2_URID atom_float;

	LV2_URID param_sampleRate;
	LV2_URID bufsz_minBlockLength;
	LV2_URID bufsz_maxBlockLength;
	LV2_URID bufsz_sequenceSize;
	LV2_URID ui_updateRate;

	pthread_t worker_thread;
	LV2_Worker_Interface *worker_iface;
	static void *worker_func(void *vp);
	void *worker_func();
	void worker_start();
	void worker_stop();
	LV2_Worker_Status worker_schedule(uint32_t inp_size, const void *inp_data);
	static LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle vp,
			uint32_t inp_size, const void *inp_data);
	LV2_Worker_Status worker_respond(uint32_t out_size, const void *out_data);
	static LV2_Worker_Status lv2_worker_respond(LV2_Worker_Respond_Handle vp,
			uint32_t out_size, const void *out_data);
	PluginLV2Work *get_work();
	void work_stop(PluginLV2Work *&work);
	void worker_responses();

	LV2_Worker_Schedule schedule;
	PluginLV2Work *work_avail, *work_input;
	PluginLV2Work *work_output, **work_tail;
	pthread_mutex_t worker_lock;
	pthread_cond_t worker_ready;
	int worker_done;
};

typedef struct { int sample_rate, bfrsz;  char path[1]; } open_bfr_t;

enum { NO_COMMAND,
	LV2_OPEN,
	LV2_LOAD,
	LV2_CONFIG,
	LV2_UPDATE,
	LV2_SHOW,
	LV2_HIDE,
	LV2_SHMID,
	NB_COMMANDS };

#endif
