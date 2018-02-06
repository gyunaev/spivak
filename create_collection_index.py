#!/usr/bin/python
# -*- coding: utf-8 -*-

# Index file is a simple vertical dash-separated text file in UTF8, containing per each line:
# <artist> <title> <filepathfromroot> <musicpathifneeded> <type> [language]

import re, os, sys, sys


def createIndex( src ):

    # Iterate through all dirs in the sourcepath
    filesfound = dict()
    
    # dash-separated
    #artisttitlepattern = ( ".*\/(.*) - (.*)\.(\w+)", ".*\/(.*)-(.*)\.(\w+)" )
    
    # dir-separated
    artisttitlepattern = ( ".*\/(.*)\/(.*)\.(.+)$", )

    # File extensions containing Karaoke media in a single file (i.e. mp4, kfn or zip)
    karaoke_ext_standalone = ("mp4", "mkv", "avi", "kfn", "zip", "mid" )
    
    # Supported audio file extensions - would only be used if the appropriate lyric file was found
    karaoke_ext_music = ("mp3", "m4a", "ogg")
    
    # Lyric file extensions
    karaoke_ext_lyrics = ("cdg", "lrc", "lyric", "txt" )
    
    # Output index file
    out = open( os.path.join( src, "index.dat"), "wb" )
    
    # Use the queue-based lookup to avoid recursion
    lookupdirs = list( [ src ] )
    
    srclen = len(src) - 1
    
    if src.endswith("/") or src.endswith("\\"):
        srclen += 1
    
    while lookupdirs:
        dire = lookupdirs.pop()
        
        files = os.listdir( dire )
        for entry in sorted( files ):
        
            fullpath = os.path.join( dire, entry )

            if os.path.isdir( fullpath ):
                lookupdirs.append( fullpath )
                continue
                        
            # Find if it matches our pattern
            for p in artisttitlepattern:
                match = re.search( p, fullpath )
                
                if match != None:
                    break
                
            if match == None:
                print ("Cannot determine artist and/or title for the file ", fullpath )
                continue
                
            artist = match.group( 1 )
            title = match.group( 2 )
            extension = match.group( 3 ).lower()
            
            # Check if this is a music karaoke file so we can check if lyrics are there
            if extension in karaoke_ext_music:
                
                # Look through the files and see if we can find matching lyrics
                lyricsfile = None
                
                basename = os.path.splitext(entry)[0]
                
                for f in files:
                    
                    (nf, ne) = os.path.splitext(f)

                    if nf == basename and ne.lower()[1:] in karaoke_ext_lyrics:
                        lyricsfile = os.path.join( dire, f )[ srclen: ]
                        break
                
                if lyricsfile == None:
                    print ("Music file", fullpath, "has no matching lyrics file, skipped" )
                    continue
                
                musicfile = fullpath[ srclen: ]

            elif extension in karaoke_ext_standalone:
                
                lyricsfile = fullpath[ srclen: ]
                musicfile = ""
            
            else:

                # Ignore files which are not music with matching lyrics or standalone
                continue

            out.write( (artist + "|" + title + "|" + lyricsfile + "|" + musicfile + "|" + extension + "|\n").encode( "utf-8") )
            
    out.close()
        
    
if len(sys.argv) < 2:
    print ("Usage: " + sys.argv[0] + " <dir>\n")
    sys.exit(1)
    
createIndex( sys.argv[1] )
