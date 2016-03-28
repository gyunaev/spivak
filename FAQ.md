## Player crashes when playing any audio or video file

And typically gst-play-1.0 crashes too, and the backtrace shows the crash is somewhere in GStreamer, mentions libproxy.so.1 and is in libkdecore.so.5

This means your system has installed libproxy both from KDE4 and KDE5. GStreamer loads libproxy by name, and the first libproxy loaded is from KDE4, while your system is running KDE5. The KDE4 libproxy loads the libkdecore from KDE5 and crashes.

To solve this, uninstall the libproxy for KDE4, such as libproxy-config-kde4 package.

## Player does not play some music or video files, notably MP3

### You're using Linux

On Linux Spivak uses the GStreamer packaged with your operating system. So the files not being played means your GStreamer installation is crippled, and the components necessary for playing those files are removed for patent or other purposes. This is a typical case with OpenSuSE. 

This is typically corrected through adding additional package repositories and reinstalling or upgrading GStreamer and/or dependent libraries. Please see the community forums for your Linux installation which you can easily find by searching for *your-distribution-name mp3". For OpenSuSE please see http://opensuse-community.org/

Once you update your GStreamer installation, please do not forget to clean the GStreamer cache by removing the ~/.cache/gstreamer* directory before you try again.

Please also try to play the target file with gst-play-1.0 tool which is part of GStreamer package. If this tool does not play your file, Spivak will not play it too.

### You are using Windows or Mac OS X

On those platforms the player uses pre-packaged GStreamer with the limited set of libraries to lower the overall installation size. You can add support for more formats by installing the regular GStreamer package for your operating system. Please download them from https://gstreamer.freedesktop.org/download/


## Background video does not play right

If background video (and foreground video for Karaoke video files) plays too fast or too slow, this might be caused by an issue in an older GStreamer VAAPI. Please try to uninstall the VAAPI drivers or gstreamer-plugins-vaapi package and try again.

