This program shuts down Android's native display server (the hwcomposer process) and writes pixels directly to the screen via DRM/KMS, which has replaced the Linux framebuffer device on new Android OS versions. So far, it's working on regular Linux machines that use Xorg as the display server, but I'm still working to get it up and running on Android devices (namely the Google Pixel 4). It might require a small kernel patch. The "dumb buffer" is what DRM allows users to create as a sort of substitute for the old fb0. It should draw random solid colors to the screen for 100 seconds, one color per second. There will probably be some tearing because it doesn't synchronize with VSYNC (although that's coming soon). 

I know you need to be running as root to run this on a normal Linux machine, and I think you also do when running it on Android, so you'll need a rooted device. 

To run on a Linux machine, you'll first want to run something like "sudo service gdm3 stop" to shut down Xorg, then switch to a different TTY and run drm_low.c.

UPDATE (07/18/20): running on Android devices does require a kernel patch in order to access the encoder. I'm working on it but won't be able to get around to posting it for another week or so.
