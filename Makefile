#lldb: env DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib
CC=c++

WEBRTC_TRUNK_DIR=/Users/jhg/gst/master/webrtc-audio-processing
WEBRTC_CFLAGS=-I$(WEBRTC_TRUNK_DIR)/build/include -I$(WEBRTC_TRUNK_DIR) -I$(WEBRTC_TRUNK_DIR)/webrtc -I$(WEBRTC_TRUNK_DIR)/webrtc/voice_engine/include -I$(WEBRTC_TRUNK_DIR)/webrtc/video_engine/include -I$(WEBRTC_TRUNK_DIR)/webrtc/test/channel_transport/include -I$(WEBRTC_TRUNK_DIR)/webrtc/test/channel_transport -I$(WEBRTC_TRUNK_DIR)/webrtc/voice_engine/include/mock -I$(WEBRTC_TRUNK_DIR)/webrtc/system_wrappers/interface

CFLAGS=-O0 -g3 -std=c++11 -DWEBRTC_POSIX -DWEBRTC_AUDIO_PROCESSING_ONLY_BUILD -c -Wall -I./ $(WEBRTC_CFLAGS)
LDFLAGS=-Wl,-headerpad_max_install_names -headerpad_max_install_names -L$(WEBRTC_TRUNK_DIR)/webrtc -L$(WEBRTC_TRUNK_DIR)/build/lib /Users/jhg/gst/master/webrtc-audio-processing/build/lib/libwebrtc_audio_processing.a

SOURCES=demo_main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=WebRTC_VoiceEngine_Demo

all: $(SOURCES) $(EXECUTABLE)
	

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ $(EXECUTABLE)

