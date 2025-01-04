TARGET = cld2
CONFIG = warn_on staticlib c++11
TEMPLATE = lib
SOURCES += internal/cldutil.cc \
  internal/cldutil_shared.cc \
  internal/compact_lang_det.cc \
  internal/compact_lang_det_hint_code.cc \
  internal/compact_lang_det_impl.cc  \
  internal/debug.cc \
  internal/fixunicodevalue.cc \
  internal/generated_entities.cc  \
  internal/generated_language.cc  \
  internal/generated_ulscript.cc  \
  internal/getonescriptspan.cc \
  internal/lang_script.cc \
  internal/offsetmap.cc  \
  internal/scoreonescriptspan.cc \
  internal/tote.cc \
  internal/utf8statetable.cc  \
  internal/cld_generated_cjk_uni_prop_80.cc \
  internal/cld2_generated_cjk_compatible.cc  \
  internal/cld_generated_cjk_delta_bi_4.cc \
  internal/generated_distinct_bi_0.cc  \
  internal/cld2_generated_quadchrome_2.cc \
  internal/cld2_generated_deltaoctachrome.cc \
  internal/cld2_generated_distinctoctachrome.cc  \
  internal/cld_generated_score_quad_octa_2.cc

HEADERS += \
    internal/cld2tablesummary.h \
    internal/cldutil.h \
    internal/cldutil_shared.h \
    internal/compact_lang_det_impl.h \
    internal/compact_lang_det_hint_code.h \
    internal/debug.h \
    internal/fixunicodevalue.h \
    internal/generated_language.h \
    internal/generated_ulscript.h \
    internal/getonescriptspan.h \
    internal/integral_types.h \
    internal/lang_script.h \
    internal/langspan.h \
    internal/offsetmap.h \
    internal/port.h \
    internal/stringpiece.h \
    internal/tote.h \
    internal/utf8acceptinterchange.h \
    internal/utf8repl_lettermarklower.h \
    internal/utf8prop_lettermarkscriptnum.h \
    internal/utf8scannot_lettermarkspecial.h \
    internal/utf8statetable.h \
    public/compact_lang_det.h \
    public/encodings.h
