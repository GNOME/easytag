/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ET_GENRES_H_
#define ET_GENRES_H_

/* GENRE_MAX is the last genre number that can be used */
#define GENRE_MAX ( sizeof(id3_genres)/sizeof(id3_genres[0]) - 1 )

/**
    \def genre_no(IndeX) 
    \param IndeX number of genre using in id3v1 
    \return pointer to genre as string
*/
#define genre_no(IndeX) ( IndeX < (sizeof(id3_genres)/sizeof(*id3_genres) ) ? id3_genres[IndeX] : "Unknown" )

/* 
 * Do not sort genres!!
 * Last Update: 2014-05-05
 * https://en.wikipedia.org/wiki/ID3#List_of_genres
 */
static const char *id3_genres[] =
{
    "Blues", /* 0: Core ID3 support */
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",             /* 5 */
    "Grunge",
    "Hip-Hop", 
    "Jazz",
    "Metal",
    "New Age",          /* 10 */
    "Oldies",
    "Other", 
    "Pop",
    "R&B",
    "Rap",              /* 15 */
    "Reggae", 
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",      /* 20 */
    "Ska",
    "Death Metal", 
    "Pranks",
    "Soundtrack",
    "Euro-Techno",      /* 25 */
    "Ambient",
    "Trip-Hop", 
    "Vocal",
    "Jazz+Funk", 
    "Fusion",           /* 30 */
    "Trance",
    "Classical",
    "Instrumental", 
    "Acid",
    "House",            /* 35 */
    "Game",
    "Sound Clip", 
    "Gospel",
    "Noise",
    "Altern Rock",      /* 40 */
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",       /* 45 */
    "Instrumental Pop",
    "Instrumental Rock", 
    "Ethnic",
    "Gothic",
    "Darkwave",         /* 50 */
    "Techno-Industrial", 
    "Electronic", 
    "Pop-Folk",
    "Eurodance", 
    "Dream",            /* 55 */
    "Southern Rock", 
    "Comedy", 
    "Cult",
    "Gangsta",
    "Top 40",           /* 60 */
    "Christian Rap", 
    "Pop/Funk", 
    "Jungle",
    "Native American", 
    "Cabaret",          /* 65 */
    "New Wave",
    "Psychedelic",
    "Rave",
    "Showtunes", 
    "Trailer",          /* 70 */
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz", 
    "Polka",            /* 75 */
    "Retro",
    "Musical",
    "Rock & Roll", 
    "Hard Rock", 
    "Folk", /* 80: Winamp extensions */
    "Folk/Rock",
    "National Folk", 
    "Swing",
    "Fast Fusion",
    "Bebob",            /* 85 */
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",       /* 90 */
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock", 
    "Symphonic Rock", 
    "Slow Rock",        /* 95 */
    "Big Band", 
    "Chorus",
    "Easy Listening", 
    "Acoustic", 
    "Humour",           /* 100 */
    "Speech",
    "Chanson", 
    "Opera",
    "Chamber Music", 
    "Sonata",           /* 105 */
    "Symphony",
    "Booty Bass", 
    "Primus",
    "Porn Groove", 
    "Satire",           /* 110 */
    "Slow Jam", 
    "Club",
    "Tango",
    "Samba",
    "Folklore",         /* 115 */
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",             /* 120 */
    "Punk Rock",
    "Drum Solo",
    "A Capella",
    "Euro-House",
    "Dance Hall",       /* 125 */
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",           /* 130 */
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",             /* 135 */
    "Christian Gangsta Rap",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary Christian",/* 140 */
    "Christian Rock",
    "Merengue", /* Winamp 1.91 */
    "Salsa",
    "Thrash Metal",
    "Anime",            /* 145 */
    "JPop",
    "Synthpop",
    "Abstract", /* Winamp 5.6 */
    "Art Rock",
    "Baroque", /* 150 */
    "Bhangra",
    "Big Beat",
    "Breakbeat",
    "Chillout",
    "Downtempo", /* 155 */
    "Dub",
    "EBM",
    "Eclectic",
    "Electro",
    "Electroclash", /* 160 */
    "Emo",
    "Experimental",
    "Garage",
    "Global",
    "IDM", /* 165 */
    "Illbient",
    "Industro-Goth",
    "Jam Band",
    "Krautrock",
    "Leftfield", /* 170 */
    "Lounge",
    "Math Rock",
    "New Romantic",
    "Nu-Breakz",
    "Post-Punk", /* 175 */
    "Post-Rock",
    "Psytrance",
    "Shoegaze",
    "Space Rock",
    "Trop Rock", /* 180 */
    "World Music",
    "Neoclassical",
    "Audiobook",
    "Audio Theatre",
    "Neue Deutsche Welle", /* 185 */
    "Podcast",
    "Indie Rock",
    "G-Funk",
    "Dubstep",
    "Garage Rock", /* 190 */
    "Psybient"
};

#endif /* ET_GENRES_H_ */
