#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <inttypes.h>

#define VOID2U64(x) ((uint64_t)(unsigned long)(x))
#define U642VOID(x) ((void *)(unsigned long)(x))

/*
int main (void) {
    int dri_fd  = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    printf("Open returned %d\n", dri_fd);
    //printf("Set master request code: %d\n", DRM_IOCTL_SET_MASTER);


    //DOESN'T WORK
    int ret = ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);
    //int ret = drmSetMaster(dri_fd);

    fprintf(stderr, "Set master failed with error %d: %m\n", ret, errno);

    struct drm_mode_card_res res, counts;

    int ret2 = ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
    fprintf(stderr, "Get resources failed with error %d: %m\n", ret2, errno);

    //printf("Returned %d\n", ret);
}
*/

/*for reference
struct drm_mode_card_res {
	__u64 fb_id_ptr;
	__u64 crtc_id_ptr;
	__u64 connector_id_ptr;
	__u64 encoder_id_ptr;
	__u32 count_fbs;
	__u32 count_crtcs;
	__u32 count_connectors;
	__u32 count_encoders;
	__u32 min_width, max_width;
	__u32 min_height, max_height;
};*/

int main()
{
	//kill hwcomposer process so we can get drm master
	system("stop vendor.hwcomposer-2-3");

	//guarantee shutdown and closure of dev file
	usleep(100000);

    //open up the dri device
    int dri_fd  = open("/dev/dri/card0",O_RDWR | O_CLOEXEC);
    if (dri_fd<0) {
        fprintf(stderr, "FAIL on open dev/dri/card0\n");
        return -1;
    }
    printf("Successfully opened dri card0, fd is %d\n", dri_fd);


    //kernel mode setting: set up integer arrays to store IDs of all the diff 4 types of resources
    //frambuffer, CRTC, connector, and encoder
	uint64_t res_fb_buf[10]={0}, res_crtc_buf[10]={0}, res_conn_buf[10]={0}, res_enc_buf[10]={0};

    //instantiate a drm_mode_card_res struct 
	struct drm_mode_card_res res={0};

	//Become the "master" of the DRI device
	if (ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0)!=0) {
        fprintf(stderr, "Drm set master failed with eror %d: %m\n", errno);
		return errno;
    }
	printf("Drm master success\n");


	//Get resource counts: this fills in count_fbs, count_crtcs, count_connectors, count_encoders
	if (ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res)!=0) {
		fprintf(stderr, "Drm mode getresources failed with eror %d: %m\n", errno);
		return errno;
	}
	printf("Drm get resources success\n");


	if (res.count_fbs) {
		res.fb_id_ptr = VOID2U64(calloc(res.count_fbs, sizeof(uint32_t)));
	}
	if (res.count_crtcs) {
		res.crtc_id_ptr = VOID2U64(calloc(res.count_crtcs, sizeof(uint32_t)));
	}
	if (res.count_connectors) {
		res.connector_id_ptr = VOID2U64(calloc(res.count_connectors, sizeof(uint32_t)));
	}
	if (res.count_encoders) {
		res.encoder_id_ptr = VOID2U64(calloc(res.count_encoders, sizeof(uint32_t)));
	}


	/*
	//set the id array heads to the ones we created (10 uint64_ts allocated on the stack)
	res.fb_id_ptr = (uint64_t)res_fb_buf;
	res.crtc_id_ptr = (uint64_t)res_crtc_buf;
	res.connector_id_ptr = (uint64_t)res_conn_buf;
	res.encoder_id_ptr = (uint64_t)res_enc_buf;
	*/

	//Get resource IDs
	if (ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res)!=0) {
		fprintf(stderr, "Drm mode getresources second failed with error %d: %m\n", errno);
		return errno;
	}
	printf("Drm get resources second success\n");


	//test
	printf("%" PRId64 "\n", res.crtc_id_ptr);

	//print out count information
	printf("# framebuffs: %d, # CRTCs: %d, # connectors: %d, # encoders: %d\n", res.count_fbs, res.count_crtcs, res.count_connectors, res.count_encoders);

	//array of actual pointers to starts of framebuffers in memory
	void* fb_base[10];

	//length of framebuffer
	long fb_w[10];

	//width of framebuffer
	long fb_h[10];

	int i;

	/*For reference
	struct drm_mode_get_connector {
	__u64 encoders_ptr;
	__u64 modes_ptr;
	__u64 props_ptr;
	__u64 prop_values_ptr;

	__u32 count_modes;
	__u32 count_props;
	__u32 count_encoders;

	__u32 encoder_id; // Current Encoder 
	__u32 connector_id; // Id 
	__u32 connector_type;
	__u32 connector_type_id;

	__u32 connection;
	__u32 mm_width, mm_height; // HxW in millimeters 
	__u32 subpixel;
	};



	struct drm_mode_modeinfo {
		__u32 clock;
		__u16 hdisplay, hsync_start, hsync_end, htotal, hskew;
		__u16 vdisplay, vsync_start, vsync_end, vtotal, vscan;

		__u32 vrefresh;

		__u32 flags;
		__u32 type;
		char name[DRM_DISPLAY_MODE_LEN];
	};*/


	//Loop though all available connectors
	for (i = 0; i < res.count_connectors; i++)
	{
		struct drm_mode_modeinfo conn_mode_buf[20]={0};

		uint64_t conn_prop_buf[20]={0}, conn_propval_buf[20]={0}, conn_enc_buf[20]={0};

		struct drm_mode_get_connector conn = {0};

		printf("The pointer to the connector ID list is %p, our index is %d, we're looking at address %p\n", (uint64_t*)(U642VOID(res.connector_id_ptr)), i, &((uint64_t*)(U642VOID(res.connector_id_ptr)))[i]);
		conn.connector_id = ((uint32_t*)(U642VOID(res.connector_id_ptr)))[i];

		printf("This connector ID: ");
		printf("%d\n", conn.connector_id);

		//get connector resource counts
		if (ioctl (dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn)!=0) {
			fprintf(stderr, "Drm mode getconnector failed with error %d: %m\n", errno);
			return errno;
		}
		printf("Drm get connector success\n");

		if (conn.count_props) {
			conn.props_ptr = VOID2U64(calloc(conn.count_props, sizeof(uint32_t)));
			//TODO: add malloc check
			conn.prop_values_ptr = VOID2U64(calloc(conn.count_props, sizeof(uint64_t)));
		}

		if (conn.count_modes) {
			conn.modes_ptr = VOID2U64(calloc(conn.count_modes, sizeof(struct drm_mode_modeinfo)));
		}
		//TODO: add else check??

	
		if (conn.count_encoders) {
			conn.encoders_ptr = VOID2U64(calloc(conn.count_encoders, sizeof(uint32_t)));
		}

		/*
		conn.modes_ptr=(uint64_t)conn_mode_buf;
		conn.props_ptr=(uint64_t)conn_prop_buf;
		conn.prop_values_ptr=(uint64_t)conn_propval_buf;
		conn.encoders_ptr=(uint64_t)conn_enc_buf;
		*/

		//get connector resources
		if (ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn)!=0) {
			fprintf(stderr, "Drm mode getconnector second failed with error %d: %m\n", errno);
			return errno;
		}
		printf("Drm get connector second success\n");

		//Check if the connector is OK to use (connected to something)
		if (conn.count_encoders<1 || conn.count_modes<1 || !conn.encoder_id || !conn.connection)
		{
			printf("This connector NOT connected\n");

			//skip rest of loop, continuing to try next connector
			continue;
		}

        //creating dumb buffer
		struct drm_mode_create_dumb create_dumb={0};
		struct drm_mode_map_dumb map_dumb={0};
		struct drm_mode_fb_cmd cmd_dumb={0};

		//If we create the buffer later, we can get the size of the screen first.
		//This must be a valid mode, so it's probably best to do this after we find
		//a valid crtc with modes.
		create_dumb.width = conn_mode_buf[0].hdisplay;
		create_dumb.height = conn_mode_buf[0].vdisplay;
		create_dumb.bpp = 32;
		create_dumb.flags = 0;
		create_dumb.pitch = 0;
		create_dumb.size = 0;
		create_dumb.handle = 0;
		ioctl(dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);

		cmd_dumb.width=create_dumb.width;
		cmd_dumb.height=create_dumb.height;
		cmd_dumb.bpp=create_dumb.bpp;
		cmd_dumb.pitch=create_dumb.pitch;
		cmd_dumb.depth=24;
		cmd_dumb.handle=create_dumb.handle;
		ioctl(dri_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb);

		map_dumb.handle=create_dumb.handle;
		ioctl(dri_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb);

		fb_base[i] = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, dri_fd, map_dumb.offset);
		fb_w[i]=create_dumb.width;
		fb_h[i]=create_dumb.height;

        //kernel mode
		printf("%d : mode: %d, prop: %d, enc: %d\n",conn.connection,conn.count_modes,conn.count_props,conn.count_encoders);
		printf("modes: %dx%d FB: %d\n",conn_mode_buf[0].hdisplay,conn_mode_buf[0].vdisplay,fb_base[i]);

		struct drm_mode_get_encoder enc={0};

		enc.encoder_id=conn.encoder_id;
		ioctl(dri_fd, DRM_IOCTL_MODE_GETENCODER, &enc);	//get encoder

		struct drm_mode_crtc crtc={0};

		crtc.crtc_id=enc.crtc_id;
		ioctl(dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc);

		crtc.fb_id=cmd_dumb.fb_id;
		crtc.set_connectors_ptr=(uint64_t)&res_conn_buf[i];
		crtc.count_connectors=1;
		crtc.mode=conn_mode_buf[0];
		crtc.mode_valid=1;
		ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
	}

	//Stop being the "master" of the DRI device - DROP MASTER
	ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);

	int x,y;
	for (i=0;i<100;i++)
	{
		int j;
		for (j=0;j<res.count_connectors;j++)
		{
			int col=(rand()%0x00ffffff)&0x00ff00ff;
			for (y=0;y<fb_h[j];y++)
				for (x=0;x<fb_w[j];x++)
				{
					int location=y*(fb_w[j]) + x;
					*(((uint32_t*)fb_base[j])+location)=col;
				}
		}
		usleep(100000);
	}

	return 0;
}
