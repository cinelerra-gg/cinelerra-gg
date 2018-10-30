include ../../plugin_defs

OBJS := \
	$(OBJDIR)/downsampleengine.o \
	$(OBJDIR)/motioncache-hv.o \
	$(OBJDIR)/motionscan-hv.o \
	$(OBJDIR)/motion.o \
	$(OBJDIR)/motionwindow.o

CFLAGS += -DMotionScan=MotionHVScan

PLUGIN = motion2

include ../../plugin_config

$(OBJDIR)/downsampleengine.o: downsampleengine.C
$(OBJDIR)/motion.o: motion.C
$(OBJDIR)/motionscan-hv.o: motioncache-hv.C
$(OBJDIR)/motionscan-hv.o: motionscan-hv.C
$(OBJDIR)/motionwindow.o: motionwindow.C
