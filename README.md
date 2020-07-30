This program shuts down Android's native display server (the hwcomposer process) and writes pixels directly to the screen via DRM/KMS, which has replaced the Linux framebuffer device on new Android OS versions. So far, it's working on regular Linux machines that use Xorg as the display server, but I'm still working to get it up and running on Android devices (namely the Google Pixel 4). It might require a small kernel patch. The "dumb buffer" is what DRM allows users to create as a sort of substitute for the old fb0. It should draw random solid colors to the screen for 100 seconds, one color per second. There will probably be some tearing because it doesn't synchronize with VSYNC (although that's coming soon). 

I know you need to be running as root to run this on a normal Linux machine, and I think you also do when running it on Android, so you'll need a rooted device. 

To run on a Linux machine, you'll first want to run something like "sudo service gdm3 stop" to shut down Xorg, then switch to a different TTY and run drm_low.c.

UPDATE (07/18/20): running on Android devices does require a kernel patch in order to access the encoder. I'm working on it but won't be able to get around to posting it for another week or so.

UPDATE (07/30/20): the program is now finished; everything's working. A kernel mod could do the job, but an easier way is to just do some debugging of the DRM driver using printk, or, even easier, use the modetest tool. Specifically, you need to find out what the correct encoder ID and corresponding CRTC ID are for DSI-1. (You'll see this as one of the encoder types in the modetest output.) For the Pixel, my encoder was 27, and the corresponding CRTC was 127. It seems like the latest versions of Android 10 for the Pixel 4 come with libdrm in /external; to build the modetest executable, set up the AOSP build normally (using lunch, etc.), then cd into /external/libdrm/tests/modetest and run 'mma'. When the build finishes it should tell you where it stored the output executable; mine was in /home/nodog/Documents/aosp2/working/out/target/product/flame/data/nativetest/modetest. Push the executable to /system/bin on the Pixel device, then cd into bin and run ./modetest -M msm_drm.  

You should see at the very beginning the list of encoders; you want the ID corresponding to the DSI (Display Serial Interface) encoder. For some reason, after DRM_IOCTL_MODE_GETCONNECTOR, the encoder ID for the connector was coming up null (0), so you need to override it like I did in line 298. Similarly, after DRM_IOCTL_MODE_GETENCODER, the CRTC id for the encoder was coming back as 0, so override that like I did in line 583. Getting the correct CRTC ID from modetest is a little less certain, but I'm guessing it's always the first CRTC listed under the CRTC section.  

If you have the capacity to build kernels for Android, I found debugging the DRM driver with printk was pretty helpful. You can access all of the necessary driver files in /private/msm-google/drivers/gpu/drm. If you want to verify the correct encoder ID for DSI, you can add printk statements in drm_encoder.c in the drm_encoder_init() function and look at encoder->base.id. If you want to verify the correct CRTC, you can add printk statements in drm_crtc.c in the drm_crtc_init_with_planes() function and look at crtc->base.id.     

One thing to note is that, at least on the Pixel 4 with AOSP, the rendering won't work unless you implement double buffering, as done in this example. This must have something to do with VSYNC and the fact that this DSI encoder is in command mode, so it's not flushing out the frames/refreshing correctly. Also, VERY IMPORTANT: for some reason after flashing a new AOSP build onto the device, I have to run stop vendor.hwcomposer-2-3, then start vendor.hwcomposer-2-3 and immediately run the program in order to get it working. This is only on the first run after a flash.  

I forgot to mention this before, but mmap64 is also required...some knowledgeable people tell me that has something to do with bionic?   

To build the program, run something like  

arm-linux-gnueabi-gcc -static -march=armv7-a drm_low.c -o [desired executable name]     

Then push the executable to the Android device. Don't forget, in order to push, you first need to open an ADB shell and run the following:  

mount -o rw  
reboot (possibly)  
remount /system  

(now you can make directories in /system/ and push there)  

make sure you always run  

adb root  

after a reboot.
