/**************************************************************************
 *  Spivak Karaoke PLayer - a free, cross-platform desktop karaoke player *
 *  Copyright (C) 2015-2016 George Yunaev, support@ulduzsoft.com          *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <QDir>

#include "languagedetector.h"
#include <stdio.h>
#include "cld2/public/compact_lang_det.h"

static const char * lang_table[CLD2::NUM_LANGUAGES] =
{
    "English",
    "Danish",
    "Dutch",
    "Finnish",
    "French",
    "German",
    "Hebrew",
    "Italian",
    "Japanese",
    "Korean",
    "Norwegian",
    "Polish",
    "Portuguese",
    "Russian",
    "Spanish",
    "Swedish",
    "Chinese",
    "Czech",
    "Modern Greek",
    "Icelandic",
    "Latvian",
    "Lithuanian",
    "Romanian",
    "Hungarian",
    "Estonian",
    0,
    0,
    "Bulgarian",
    "Croatian",
    "Serbian",
    "Irish",
    "Galician",
    "Tagalog",
    "Turkish",
    "Ukrainian",
    "Hindi",
    "Macedonian",
    "Bengali",
    "Indonesian",
    "Latin",
    "Malay",
    "Malayalam",
    "Welsh",
    "Nepali",
    "Telugu",
    "Albanian",
    "Tamil",
    "Belarusian",
    "Javanese",
    "Occitan ",
    "Urdu",
    "Bihari",
    "Gujarati",
    "Thai",
    "Arabic",
    "Catalan",
    "Esperanto",
    "Basque",
    "Interlingua",
    "Kannada",
    "Panjabi",
    "Scottish Gaelic",
    "Swahili",
    "Slovenian",
    "Marathi",
    "Maltese",
    "Vietnamese",
    "Western Frisian",
    "Slovak",
    "Chinese",
    "Faroese",
    "Sundanese",
    "Uzbek",
    "Amharic",
    "Azerbaijani",
    "Georgian",
    "Tigrinya",
    "Persian",
    "Bosnian",
    "Sinhala",
    "Norwegian Nynorsk",
    0,
    0,
    "Xhosa",
    "Zulu",
    "Guarani",
    "Southern Sotho",
    "Turkmen",
    "Kirghiz",
    "Breton",
    "Twi",
    "Yiddish",
    0,
    "Somali",
    "Uighur",
    "Kurdish",
    "Mongolian",
    "Armenian",
    "Lao",
    "Sindhi",
    "Romansh",
    "Afrikaans",
    "Luxembourgish",
    "Burmese",
    "Central Khmer",
    "Tibetan",
    "Dhivehi",
    "Cherokee",
    "Syriac",
    "Limbu",
    "Oriya",
    "Assamese",
    "Corsican",
    "Interlingue",
    "Kazakh",
    "Lingala",
    0,
    "Central Pashto",
    "Quechua",
    "Shona",
    "Tajik",
    "Tatar",
    "Tonga",
    "Yoruba",
    0,
    0,
    0,
    0,
    "Maori",
    "Wolof",
    "Abkhazian",
    "Afar",
    "Aymara",
    "Bashkir",
    "Bislama",
    "Dzongkha",
    "Fijian",
    "Kalaallisut",
    "Hausa",
    "Haitian",
    "Inupiaq",
    "Inuktitut",
    "Kashmiri",
    "Kinyarwanda",
    "Malagasy",
    "Nauru",
    "Oromo",
    "Rundi",
    "Samoan",
    "Sango",
    "Sanskrit",
    "Swati",
    "Tsonga",
    "Tswana",
    "Volapuk",
    "Zhuang",
    "Khasi",
    "Scots",
    "Ganda",
    "Manx",
    "Montenegrin",
    "Akan",
    "Igbo",
    "Mauritian Creole",
    "Hawaiian",
    "Cebuano",
    "Ewe",
    "Ga",
    "Hmong",
    "Krio",
    "Lozi",
    "Luba Lulua",
    "Luo",
    "Newari",
    "Nyanja",
    "Ossetian",
    "Pampanga",
    "Pedi",
    "Rajasthani",
    "Seselwa Creole French",
    "Tumbuka",
    "Venda",
    "Waray"
};

LanguageDetector::LanguageDetector()
    : QObject()
{
}

QString LanguageDetector::detectLanguage(const QByteArray &data)
{
    bool is_reliable;

    CLD2::Language lang = CLD2::DetectLanguage( data.constData(), data.length(), true,  &is_reliable );

    if ( lang >=0 && lang < CLD2::NUM_LANGUAGES )
        return lang_table[lang];

    return 0;
}

QStringList LanguageDetector::languages()
{
    QStringList langs;

    for ( int i = 0; i < CLD2::NUM_LANGUAGES; i++ )
        langs.push_back( QString::fromUtf8( lang_table[i] ) );

    return langs;
}
