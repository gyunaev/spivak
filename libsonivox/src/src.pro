TARGET = sonivox
CONFIG += warn_on staticlib
TEMPLATE = lib

DEFINES = NUM_OUTPUT_CHANNELS=2 _SAMPLE_RATE_44100 MAX_SYNTH_VOICES=64 EAS_WT_SYNTH \
        _8_BIT_SAMPLES _FILTER_ENABLED _WAVE_PARSER MAX_SMF_STREAMS=32 _REVERB_ENABLED \
        _CHORUS_ENABLED DLS_SYNTHESIZER MMAPI_SUPPORT UNIFIED_DEBUG_MESSAGES \
        _IMELODY_PARSER _RTTTL_PARSER _OTA_PARSER _XMF_PARSER

SOURCES += eas_mididata.c eas_pan.c eas_wavefiledata.c \
       eas_reverb.c eas_mdls.c eas_mixbuf.c eas_tcdata.c eas_chorus.c eas_pcmdata.c \
       eas_math.c eas_tonecontrol.c eas_voicemgt.c eas_public.c eas_dlssynth.c eas_midi.c \
       eas_flog.c eas_wtengine.c eas_imaadpcm.c eas_wtsynth.c eas_pcm.c eas_mixer.c \
       eas_wavefile.c eas_ima_tables.c eas_data.c eas_config.c eas_report.c eas_hostmm.c \
       eas_smf.c eas_smfdata.c eas_imelody.c eas_imelodydata.c eas_ota.c eas_otadata.c \
       eas_rtttl.c  eas_rtttldata.c eas_xmf.c eas_xmfdata.c

INCLUDEPATH += ../include
unix: QMAKE_CFLAGS += -Wno-unused
