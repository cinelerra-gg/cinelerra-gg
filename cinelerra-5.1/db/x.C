#include "tdb.h"

// Video_frame
DbObj(Video_frame)
  basic_def(double,Frame_mean);
  basic_def(double,Frame_std_dev);
  basic_def(double,Frame_cx);
  basic_def(double,Frame_cy);
  basic_def(double,Frame_moment);
  varray_def(unsigned char,Frame_data);
};

DbLoc(Video_frame)
  basic_ref(double,Frame_mean);
  basic_ref(double,Frame_std_dev);
  basic_ref(double,Frame_cx);
  basic_ref(double,Frame_cy);
  basic_ref(double,Frame_moment);
  varray_ref(unsigned char,Frame_data);

  class ikey_Frame_weight : public Db::iKey { public:
    Video_frameObj::t_Frame_mean v_Frame_mean;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Frame_weight(ObjectLoc &loc,
        double Frame_mean, int id=-1)
    : iKey("Frame_weight",loc,cmpr),
      v_Frame_mean(Frame_mean),
      v_id(id) {}
  };
  class rkey_Frame_weight : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Frame_weight(ObjectLoc &loc) : rKey("Frame_weight",loc,cmpr) {}
  };

  class ikey_Frame_center : public Db::iKey { public:
    Video_frameObj::t_Frame_moment v_Frame_moment;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Frame_center(ObjectLoc &loc,
        double Frame_moment, int id=-1)
    : iKey("Frame_center",loc,cmpr),
      v_Frame_moment(Frame_moment),
      v_id(id) {}
  };
  class rkey_Frame_center : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Frame_center(ObjectLoc &loc) : rKey("Frame_center",loc,cmpr) {}
  };

  int Allocate();
  int Construct();
  int Destruct();
  void Deallocate();
};
// Timeline
DbObj(Timeline)
  basic_def(unsigned int,Clip_id);
  basic_def(unsigned int,Sequence_no);
  basic_def(unsigned int,Frame_id);
  basic_def(unsigned int,Group);
  basic_def(double,Time_offset);
};

DbLoc(Timeline)
  basic_ref(unsigned int,Clip_id);
  basic_ref(unsigned int,Sequence_no);
  basic_ref(unsigned int,Frame_id);
  basic_ref(unsigned int,Group);
  basic_ref(double,Time_offset);

  class ikey_Timelines : public Db::iKey { public:
    TimelineObj::t_Frame_id v_Frame_id;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Timelines(ObjectLoc &loc,
        unsigned int Frame_id, int id=-1)
    : iKey("Timelines",loc,cmpr),
      v_Frame_id(Frame_id),
      v_id(id) {}
  };
  class rkey_Timelines : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Timelines(ObjectLoc &loc) : rKey("Timelines",loc,cmpr) {}
  };

  class ikey_Sequences : public Db::iKey { public:
    TimelineObj::t_Clip_id v_Clip_id;
    TimelineObj::t_Sequence_no v_Sequence_no;
    static int cmpr(char *a, char *b);
    ikey_Sequences(ObjectLoc &loc,
        unsigned int Clip_id,
        unsigned int Sequence_no)
    : iKey("Sequences",loc,cmpr),
      v_Clip_id(Clip_id),
      v_Sequence_no(Sequence_no) {}
  };
  class rkey_Sequences : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Sequences(ObjectLoc &loc) : rKey("Sequences",loc,cmpr) {}
  };

  int Allocate();
  int Construct();
  int Destruct();
  void Deallocate();
};
// Clip_set
DbObj(Clip_set)
  sarray_def(char,Title);
  sarray_def(char,Asset_path);
  basic_def(double,Position);
  basic_def(double,Framerate);
  basic_def(double,Average_weight);
  basic_def(unsigned int,Frames);
  basic_def(unsigned int,Prefix_size);
  basic_def(unsigned int,Suffix_size);
  varray_def(unsigned char,Weights);
  basic_def(long,System_time);
  basic_def(long,Creation_time);
};

DbLoc(Clip_set)
  sarray_ref(char,Title);
  sarray_ref(char,Asset_path);
  basic_ref(double,Position);
  basic_ref(double,Framerate);
  basic_ref(double,Average_weight);
  basic_ref(unsigned int,Frames);
  basic_ref(unsigned int,Prefix_size);
  basic_ref(unsigned int,Suffix_size);
  varray_ref(unsigned char,Weights);
  basic_ref(long,System_time);
  basic_ref(long,Creation_time);

  class ikey_Clip_title : public Db::iKey { public:
    Clip_setObj::t_Title v_Title;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Clip_title(ObjectLoc &loc,
        const Clip_setObj::t_Title &Title, int id=-1)
    : iKey("Clip_title",loc,cmpr),
      v_Title(Title),
      v_id(id) {}
  };
  class rkey_Clip_title : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Clip_title(ObjectLoc &loc) : rKey("Clip_title",loc,cmpr) {}
  };

  class ikey_Clip_system_time : public Db::iKey { public:
    Clip_setObj::t_System_time v_System_time;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Clip_system_time(ObjectLoc &loc,
        long System_time, int id=-1)
    : iKey("Clip_system_time",loc,cmpr),
      v_System_time(System_time),
      v_id(id) {}
  };
  class rkey_Clip_system_time : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Clip_system_time(ObjectLoc &loc) : rKey("Clip_system_time",loc,cmpr) {}
  };

  class ikey_Clip_creation_time : public Db::iKey { public:
    Clip_setObj::t_Creation_time v_Creation_time;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Clip_creation_time(ObjectLoc &loc,
        long Creation_time, int id=-1)
    : iKey("Clip_creation_time",loc,cmpr),
      v_Creation_time(Creation_time),
      v_id(id) {}
  };
  class rkey_Clip_creation_time : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Clip_creation_time(ObjectLoc &loc) : rKey("Clip_creation_time",loc,cmpr) {}
  };

  int Allocate();
  int Construct();
  int Destruct();
  void Deallocate();
};
// Clip_views
DbObj(Clip_views)
  basic_def(unsigned int,Access_clip_id);
  basic_def(long,Access_time);
  basic_def(unsigned int,Access_count);
};

DbLoc(Clip_views)
  basic_ref(unsigned int,Access_clip_id);
  basic_ref(long,Access_time);
  basic_ref(unsigned int,Access_count);

  class ikey_Clip_access : public Db::iKey { public:
    Clip_viewsObj::t_Access_clip_id v_Access_clip_id;
    static int cmpr(char *a, char *b);
    ikey_Clip_access(ObjectLoc &loc,
        unsigned int Access_clip_id)
    : iKey("Clip_access",loc,cmpr),
      v_Access_clip_id(Access_clip_id) {}
  };
  class rkey_Clip_access : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Clip_access(ObjectLoc &loc) : rKey("Clip_access",loc,cmpr) {}
  };

  class ikey_Last_view : public Db::iKey { public:
    Clip_viewsObj::t_Access_time v_Access_time;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Last_view(ObjectLoc &loc,
        long Access_time, int id=-1)
    : iKey("Last_view",loc,cmpr),
      v_Access_time(Access_time),
      v_id(id) {}
  };
  class rkey_Last_view : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Last_view(ObjectLoc &loc) : rKey("Last_view",loc,cmpr) {}
  };

  class ikey_Total_views : public Db::iKey { public:
    Clip_viewsObj::t_Access_count v_Access_count;
    Clip_viewsObj::t_Access_clip_id v_Access_clip_id;
    int v_id;
    static int cmpr(char *a, char *b);
    ikey_Total_views(ObjectLoc &loc,
        unsigned int Access_count,
        unsigned int Access_clip_id, int id=-1)
    : iKey("Total_views",loc,cmpr),
      v_Access_count(Access_count),
      v_Access_clip_id(Access_clip_id),
      v_id(id) {}
  };
  class rkey_Total_views : public Db::rKey { public:
    static int cmpr(char *a, char *b);
    rkey_Total_views(ObjectLoc &loc) : rKey("Total_views",loc,cmpr) {}
  };

  int Allocate();
  int Construct();
  int Destruct();
  void Deallocate();
};

int Video_frameLoc::ikey_Frame_weight::
cmpr(char *a, char *b)
{
  ikey_Frame_weight *kp = (ikey_Frame_weight *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Video_frameLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_double( kp->v_Frame_mean.addr(), kp->v_Frame_mean.size(),
                  vloc._Frame_mean(), vloc->v_Frame_mean.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Video_frameLoc::rkey_Frame_weight::
cmpr(char *a, char *b)
{
  rkey_Frame_weight *kp = (rkey_Frame_weight *)a;
  Video_frameLoc &kloc = (Video_frameLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Video_frameLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_double( kloc._Frame_mean(), kloc->v_Frame_mean.size(),
                   vloc._Frame_mean(), vloc->v_Frame_mean.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Video_frameLoc::ikey_Frame_center::
cmpr(char *a, char *b)
{
  ikey_Frame_center *kp = (ikey_Frame_center *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Video_frameLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_double( kp->v_Frame_moment.addr(), kp->v_Frame_moment.size(),
                  vloc._Frame_moment(), vloc->v_Frame_moment.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Video_frameLoc::rkey_Frame_center::
cmpr(char *a, char *b)
{
  rkey_Frame_center *kp = (rkey_Frame_center *)a;
  Video_frameLoc &kloc = (Video_frameLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Video_frameLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_double( kloc._Frame_moment(), kloc->v_Frame_moment.size(),
                   vloc._Frame_moment(), vloc->v_Frame_moment.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Video_frameLoc::Allocate()
{
  if_err( allocate() );
  if( !addr_wr() ) return err_(Db::errNoMemory);
  v_init();
  return 0;
}

int Video_frameLoc::Construct()
{
  if_err( insertProhibit() );
  if_err( construct() );
  int id = this->id();
  { rkey_Frame_weight rkey(*this);
    if_err( entity->index("Frame_weight")->Insert(rkey,&id) ); }
  { rkey_Frame_center rkey(*this);
    if_err( entity->index("Frame_center")->Insert(rkey,&id) ); }
  if_err( insertCascade() );
  return 0;
}

int Video_frameLoc::Destruct()
{
  if_err( deleteProhibit() );
  { rkey_Frame_weight rkey(*this);
    if_err( entity->index("Frame_weight")->Delete(rkey) ); }
  { rkey_Frame_center rkey(*this);
    if_err( entity->index("Frame_center")->Delete(rkey) ); }
  if_err( destruct() );
  if_err( deleteCascade() );
  return 0;
}

void Video_frameLoc::Deallocate()
{
  v_del();
  deallocate();
}


int TimelineLoc::ikey_Timelines::
cmpr(char *a, char *b)
{
  ikey_Timelines *kp = (ikey_Timelines *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  TimelineLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_uint( kp->v_Frame_id.addr(), kp->v_Frame_id.size(),
                  vloc._Frame_id(), vloc->v_Frame_id.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int TimelineLoc::rkey_Timelines::
cmpr(char *a, char *b)
{
  rkey_Timelines *kp = (rkey_Timelines *)a;
  TimelineLoc &kloc = (TimelineLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  TimelineLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_uint( kloc._Frame_id(), kloc->v_Frame_id.size(),
                   vloc._Frame_id(), vloc->v_Frame_id.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int TimelineLoc::ikey_Sequences::
cmpr(char *a, char *b)
{
  ikey_Sequences *kp = (ikey_Sequences *)a;
  int v = *(int*)b;
  TimelineLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_uint( kp->v_Clip_id.addr(), kp->v_Clip_id.size(),
                  vloc._Clip_id(), vloc->v_Clip_id.size());
  if( v != 0 ) return v;
  v = cmpr_uint( kp->v_Sequence_no.addr(), kp->v_Sequence_no.size(),
                  vloc._Sequence_no(), vloc->v_Sequence_no.size());
  if( v != 0 ) return v;
  return 0;
}

int TimelineLoc::rkey_Sequences::
cmpr(char *a, char *b)
{
  rkey_Sequences *kp = (rkey_Sequences *)a;
  TimelineLoc &kloc = (TimelineLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  TimelineLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_uint( kloc._Clip_id(), kloc->v_Clip_id.size(),
                   vloc._Clip_id(), vloc->v_Clip_id.size());
  if( v != 0 ) return v;
  v = cmpr_uint( kloc._Sequence_no(), kloc->v_Sequence_no.size(),
                   vloc._Sequence_no(), vloc->v_Sequence_no.size());
  if( v != 0 ) return v;
  return 0;
}

int TimelineLoc::Allocate()
{
  if_err( allocate() );
  if( !addr_wr() ) return err_(Db::errNoMemory);
  return 0;
}

int TimelineLoc::Construct()
{
  if_err( insertProhibit() );
  if_err( construct() );
  int id = this->id();
  { rkey_Timelines rkey(*this);
    if_err( entity->index("Timelines")->Insert(rkey,&id) ); }
  { rkey_Sequences rkey(*this);
    if_err( entity->index("Sequences")->Insert(rkey,&id) ); }
  if_err( insertCascade() );
  return 0;
}

int TimelineLoc::Destruct()
{
  if_err( deleteProhibit() );
  { rkey_Timelines rkey(*this);
    if_err( entity->index("Timelines")->Delete(rkey) ); }
  { rkey_Sequences rkey(*this);
    if_err( entity->index("Sequences")->Delete(rkey) ); }
  if_err( destruct() );
  if_err( deleteCascade() );
  return 0;
}

void TimelineLoc::Deallocate()
{
  deallocate();
}


int Clip_setLoc::ikey_Clip_title::
cmpr(char *a, char *b)
{
  ikey_Clip_title *kp = (ikey_Clip_title *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Clip_setLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_char( kp->v_Title.addr(), kp->v_Title.size(),
                  vloc._Title(), vloc->v_Title.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Clip_setLoc::rkey_Clip_title::
cmpr(char *a, char *b)
{
  rkey_Clip_title *kp = (rkey_Clip_title *)a;
  Clip_setLoc &kloc = (Clip_setLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Clip_setLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_char( kloc._Title(), kloc->v_Title.size(),
                   vloc._Title(), vloc->v_Title.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_setLoc::ikey_Clip_system_time::
cmpr(char *a, char *b)
{
  ikey_Clip_system_time *kp = (ikey_Clip_system_time *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Clip_setLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_long( kp->v_System_time.addr(), kp->v_System_time.size(),
                  vloc._System_time(), vloc->v_System_time.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Clip_setLoc::rkey_Clip_system_time::
cmpr(char *a, char *b)
{
  rkey_Clip_system_time *kp = (rkey_Clip_system_time *)a;
  Clip_setLoc &kloc = (Clip_setLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Clip_setLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_long( kloc._System_time(), kloc->v_System_time.size(),
                   vloc._System_time(), vloc->v_System_time.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_setLoc::ikey_Clip_creation_time::
cmpr(char *a, char *b)
{
  ikey_Clip_creation_time *kp = (ikey_Clip_creation_time *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Clip_setLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_long( kp->v_Creation_time.addr(), kp->v_Creation_time.size(),
                  vloc._Creation_time(), vloc->v_Creation_time.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Clip_setLoc::rkey_Clip_creation_time::
cmpr(char *a, char *b)
{
  rkey_Clip_creation_time *kp = (rkey_Clip_creation_time *)a;
  Clip_setLoc &kloc = (Clip_setLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Clip_setLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_long( kloc._Creation_time(), kloc->v_Creation_time.size(),
                   vloc._Creation_time(), vloc->v_Creation_time.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_setLoc::Allocate()
{
  if_err( allocate() );
  if( !addr_wr() ) return err_(Db::errNoMemory);
  v_init();
  Title((char *)"",0);
  Asset_path((char *)"",0);
  return 0;
}

int Clip_setLoc::Construct()
{
  if_err( insertProhibit() );
  if_err( construct() );
  int id = this->id();
  { rkey_Clip_title rkey(*this);
    if_err( entity->index("Clip_title")->Insert(rkey,&id) ); }
  { rkey_Clip_system_time rkey(*this);
    if_err( entity->index("Clip_system_time")->Insert(rkey,&id) ); }
  { rkey_Clip_creation_time rkey(*this);
    if_err( entity->index("Clip_creation_time")->Insert(rkey,&id) ); }
  if_err( insertCascade() );
  return 0;
}

int Clip_setLoc::Destruct()
{
  if_err( deleteProhibit() );
  { rkey_Clip_title rkey(*this);
    if_err( entity->index("Clip_title")->Delete(rkey) ); }
  { rkey_Clip_system_time rkey(*this);
    if_err( entity->index("Clip_system_time")->Delete(rkey) ); }
  { rkey_Clip_creation_time rkey(*this);
    if_err( entity->index("Clip_creation_time")->Delete(rkey) ); }
  if_err( destruct() );
  if_err( deleteCascade() );
  return 0;
}

void Clip_setLoc::Deallocate()
{
  v_del();
  deallocate();
}


int Clip_viewsLoc::ikey_Clip_access::
cmpr(char *a, char *b)
{
  ikey_Clip_access *kp = (ikey_Clip_access *)a;
  int v = *(int*)b;
  Clip_viewsLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_uint( kp->v_Access_clip_id.addr(), kp->v_Access_clip_id.size(),
                  vloc._Access_clip_id(), vloc->v_Access_clip_id.size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_viewsLoc::rkey_Clip_access::
cmpr(char *a, char *b)
{
  rkey_Clip_access *kp = (rkey_Clip_access *)a;
  Clip_viewsLoc &kloc = (Clip_viewsLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Clip_viewsLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_uint( kloc._Access_clip_id(), kloc->v_Access_clip_id.size(),
                   vloc._Access_clip_id(), vloc->v_Access_clip_id.size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_viewsLoc::ikey_Last_view::
cmpr(char *a, char *b)
{
  ikey_Last_view *kp = (ikey_Last_view *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Clip_viewsLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_long( kp->v_Access_time.addr(), kp->v_Access_time.size(),
                  vloc._Access_time(), vloc->v_Access_time.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Clip_viewsLoc::rkey_Last_view::
cmpr(char *a, char *b)
{
  rkey_Last_view *kp = (rkey_Last_view *)a;
  Clip_viewsLoc &kloc = (Clip_viewsLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Clip_viewsLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_long( kloc._Access_time(), kloc->v_Access_time.size(),
                   vloc._Access_time(), vloc->v_Access_time.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_viewsLoc::ikey_Total_views::
cmpr(char *a, char *b)
{
  ikey_Total_views *kp = (ikey_Total_views *)a;
  int v = *(int*)b;
  if( kp->v_id == v ) return 0;
  Clip_viewsLoc vloc(kp->loc.entity);
  if( vloc.FindId(v) )
    vloc.err_(Db::errCorrupt);
  v = cmpr_uint( kp->v_Access_count.addr(), kp->v_Access_count.size(),
                  vloc._Access_count(), vloc->v_Access_count.size());
  if( v != 0 ) return v;
  v = cmpr_uint( kp->v_Access_clip_id.addr(), kp->v_Access_clip_id.size(),
                  vloc._Access_clip_id(), vloc->v_Access_clip_id.size());
  if( v != 0 ) return v;
  if( kp->v_id >= 0 ) {
    v = cmpr_int(&kp->v_id, sizeof(kp->v_id),
                 vloc._id(), vloc._id_size());
    if( v != 0 ) return v;
  }
  return 0;
}

int Clip_viewsLoc::rkey_Total_views::
cmpr(char *a, char *b)
{
  rkey_Total_views *kp = (rkey_Total_views *)a;
  Clip_viewsLoc &kloc = (Clip_viewsLoc&)kp->loc;
  int v = kloc->id, b_id = *(int*)b;
  if( v == b_id ) return 0;
  Clip_viewsLoc vloc(kloc.entity);
  if( vloc.FindId(b_id) )
    kloc.err_(Db::errCorrupt);
  v = cmpr_uint( kloc._Access_count(), kloc->v_Access_count.size(),
                   vloc._Access_count(), vloc->v_Access_count.size());
  if( v != 0 ) return v;
  v = cmpr_uint( kloc._Access_clip_id(), kloc->v_Access_clip_id.size(),
                   vloc._Access_clip_id(), vloc->v_Access_clip_id.size());
  if( v != 0 ) return v;
  v = cmpr_int(kloc._id(), kloc._id_size(),
               vloc._id(), vloc._id_size());
  if( v != 0 ) return v;
  return 0;
}

int Clip_viewsLoc::Allocate()
{
  if_err( allocate() );
  if( !addr_wr() ) return err_(Db::errNoMemory);
  return 0;
}

int Clip_viewsLoc::Construct()
{
  if_err( insertProhibit() );
  if_err( construct() );
  int id = this->id();
  { rkey_Clip_access rkey(*this);
    if_err( entity->index("Clip_access")->Insert(rkey,&id) ); }
  { rkey_Last_view rkey(*this);
    if_err( entity->index("Last_view")->Insert(rkey,&id) ); }
  { rkey_Total_views rkey(*this);
    if_err( entity->index("Total_views")->Insert(rkey,&id) ); }
  if_err( insertCascade() );
  return 0;
}

int Clip_viewsLoc::Destruct()
{
  if_err( deleteProhibit() );
  { rkey_Clip_access rkey(*this);
    if_err( entity->index("Clip_access")->Delete(rkey) ); }
  { rkey_Last_view rkey(*this);
    if_err( entity->index("Last_view")->Delete(rkey) ); }
  { rkey_Total_views rkey(*this);
    if_err( entity->index("Total_views")->Delete(rkey) ); }
  if_err( destruct() );
  if_err( deleteCascade() );
  return 0;
}

void Clip_viewsLoc::Deallocate()
{
  deallocate();
}

class theDb : public Db {
  int dfd, dkey;
  int db_create();
  int db_open();
  int db_access();
public:
  Objects objects;
  Entity Video_frame;  Video_frameLoc video_frame;
  Entity Timeline;  TimelineLoc timeline;
  Entity Clip_set;  Clip_setLoc clip_set;
  Entity Clip_views;  Clip_viewsLoc clip_views;

  int create(const char *dfn);
  int open(const char *dfn, int key=-1);
  int access(const char *dfn, int key=-1, int rw=0);
  void close();
  int attach(int rw=0) { return Db::attach(rw); }
  int detach() { return Db::detach(); }

  theDb();
  ~theDb() { finit(objects); }
};



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int theDb::
create(const char *dfn)
{
  dfd = ::open(dfn,O_RDWR+O_CREAT+O_TRUNC+O_NOATIME,0666);
  if( dfd < 0 ) { perror(dfn); return -1; }
  int ret = db_create();
  close();
  return ret;
}

int theDb::
db_create()
{
  if_ret( Db::make(dfd) );
  if_ret( Video_frame.new_entity("Video_frame", sizeof(Video_frameObj)) );
  if_ret( Video_frame.add_kindex("Frame_weight") );
  if_ret( Video_frame.add_kindex("Frame_center") );

  if_ret( Timeline.new_entity("Timeline", sizeof(TimelineObj)) );
  if_ret( Timeline.add_kindex("Timelines") );
  if_ret( Timeline.add_kindex("Sequences") );

  if_ret( Clip_set.new_entity("Clip_set", sizeof(Clip_setObj)) );
  if_ret( Clip_set.add_kindex("Clip_title") );
  if_ret( Clip_set.add_kindex("Clip_system_time") );
  if_ret( Clip_set.add_kindex("Clip_creation_time") );

  if_ret( Clip_views.new_entity("Clip_views", sizeof(Clip_viewsObj)) );
  if_ret( Clip_views.add_kindex("Clip_access") );
  if_ret( Clip_views.add_kindex("Last_view") );
  if_ret( Clip_views.add_kindex("Total_views") );

  if_ret( Db::commit(1) );
  return 0;
}

theDb::
theDb()
 : dfd(-1), dkey(-1), objects(0),
   Video_frame(this), video_frame(Video_frame),
   Timeline(this), timeline(Timeline),
   Clip_set(this), clip_set(Clip_set),
   Clip_views(this), clip_views(Clip_views)
{
  objects = new ObjectList(objects, video_frame);
  objects = new ObjectList(objects, timeline);
  objects = new ObjectList(objects, clip_set);
  objects = new ObjectList(objects, clip_views);
  Video_frame.add_vref((vRef)&Video_frameObj::v_Frame_data);
  Clip_set.add_vref((vRef)&Clip_setObj::v_Title);
  Clip_set.add_vref((vRef)&Clip_setObj::v_Asset_path);
  Clip_set.add_vref((vRef)&Clip_setObj::v_Weights);
}

int theDb::
open(const char *dfn, int key)
{
  dfd = ::open(dfn,O_RDWR+O_NOATIME);
  if( dfd < 0 ) { perror(dfn); return errNotFound; }
  if( (dkey=key) >= 0 ) Db::use_shm(1);
  int ret = Db::open(dfd, dkey);
  if( !ret ) ret = db_open();
  if( ret ) close();
  return ret;
}

int theDb::
db_open()
{
  if_ret( Video_frame.get_entity("Video_frame") );
  if_ret( Video_frame.key_index("Frame_weight") );
  if_ret( Video_frame.key_index("Frame_center") );

  if_ret( Timeline.get_entity("Timeline") );
  if_ret( Timeline.key_index("Timelines") );
  if_ret( Timeline.key_index("Sequences") );

  if_ret( Clip_set.get_entity("Clip_set") );
  if_ret( Clip_set.key_index("Clip_title") );
  if_ret( Clip_set.key_index("Clip_system_time") );
  if_ret( Clip_set.key_index("Clip_creation_time") );

  if_ret( Clip_views.get_entity("Clip_views") );
  if_ret( Clip_views.key_index("Clip_access") );
  if_ret( Clip_views.key_index("Last_view") );
  if_ret( Clip_views.key_index("Total_views") );

  if_ret( Db::start_transaction() );
  return 0;
}

void theDb::
close()
{
  Db::close();
  if( dfd >= 0 ) { ::close(dfd); dfd = -1; }
}

int theDb::
access(const char *dfn, int key, int rw)
{
  if( key < 0 ) return Db::errInvalid;
  dfd = ::open(dfn,O_RDWR+O_NOATIME);
  if( dfd < 0 ) { perror(dfn); return Db::errNotFound; }
  dkey = key;  Db::use_shm(1);
  int ret = Db::attach(dfd, dkey, rw);
  if( !ret ) ret = db_access();
  else if( ret == errNotFound ) {
    ret = Db::open(dfd, dkey);
    if( !ret ) ret = db_open();
    if( !ret ) ret = Db::attach(rw);
  }
  if( ret ) close();
  return ret;
}

int theDb::
db_access()
{
  if_ret( Video_frame.get_entity("Video_frame") );
  if_ret( Timeline.get_entity("Timeline") );
  if_ret( Clip_set.get_entity("Clip_set") );
  if_ret( Clip_views.get_entity("Clip_views") );
  return 0;
}


