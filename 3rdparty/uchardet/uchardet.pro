# This library is retrieved from https://github.com/BYVoid/uchardet
# Licensed under LGPL license
SOURCES +=      CharDistribution.cpp JpCntx.cpp LangModels/LangArabicModel.cpp LangModels/LangBulgarianModel.cpp LangModels/LangRussianModel.cpp \
    LangModels/LangEsperantoModel.cpp LangModels/LangFrenchModel.cpp LangModels/LangGermanModel.cpp LangModels/LangGreekModel.cpp LangModels/LangHungarianModel.cpp \
    LangModels/LangHebrewModel.cpp LangModels/LangSpanishModel.cpp LangModels/LangThaiModel.cpp LangModels/LangTurkishModel.cpp nsHebrewProber.cpp nsCharSetProber.cpp \
    nsBig5Prober.cpp nsEUCJPProber.cpp nsEUCKRProber.cpp nsEUCTWProber.cpp nsEscCharsetProber.cpp nsEscSM.cpp nsGB2312Prober.cpp nsMBCSGroupProber.cpp nsMBCSSM.cpp \
    nsSBCSGroupProber.cpp nsSBCharSetProber.cpp nsSJISProber.cpp nsUTF8Prober.cpp nsLatin1Prober.cpp nsUniversalDetector.cpp uchardet.cpp
TARGET = uchardet
CONFIG += warn_on qt staticlib
TEMPLATE = lib
