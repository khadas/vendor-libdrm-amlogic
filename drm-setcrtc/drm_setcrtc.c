#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#include "xf86drm.h"
#include "xf86drmMode.h"
#include <signal.h>

#define FORMAT  DRM_FORMAT_ARGB8888 //bpp 32

struct crtc {
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    drmModeModeInfo *mode;
};

struct encoder {
    drmModeEncoder *encoder;
};

struct connector {
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    char *name;
};

struct fb {
    drmModeFB *fb;
};

struct plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct resources {
    drmModeRes *res;
    drmModePlaneRes *plane_res;

    struct crtc *crtcs;
    struct encoder *encoders;
    struct connector *connectors;
    struct fb *fbs;
    struct plane *planes;
};

struct device {
    int fd;
    struct resources *resources;

    struct {
        unsigned int width;
        unsigned int height;
    } mode;

    drmModeAtomicReq *req;
};


static void free_resources(struct resources *res)
{
    int i;

    if (!res)
        return;

#define free_resource(_res, __res, type, Type)					\
    do {									\
        if (!(_res)->type##s)						\
        break;							\
        for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
            if (!(_res)->type##s[i].type)				\
            break;						\
            drmModeFree##Type((_res)->type##s[i].type);		\
        }								\
        free((_res)->type##s);						\
    } while (0)

#define free_properties(_res, __res, type)					\
    do {									\
        for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
            drmModeFreeObjectProperties(res->type##s[i].props);	\
            free(res->type##s[i].props_info);			\
        }								\
    } while (0)

    if (res->res) {
        free_properties(res, res, crtc);

        free_resource(res, res, crtc, Crtc);
        free_resource(res, res, encoder, Encoder);

        for (i = 0; i < res->res->count_connectors; i++)
            free(res->connectors[i].name);

        free_resource(res, res, connector, Connector);
        free_resource(res, res, fb, FB);

        drmModeFreeResources(res->res);
    }

    if (res->plane_res) {
        free_properties(res, plane_res, plane);

        free_resource(res, plane_res, plane, Plane);

        drmModeFreePlaneResources(res->plane_res);
    }

    free(res);
}

struct type_name {
    unsigned int type;
    const char *name;
};
static const struct type_name connector_type_names[] = {
    { DRM_MODE_CONNECTOR_Unknown, "unknown" },
    { DRM_MODE_CONNECTOR_VGA, "VGA" },
    { DRM_MODE_CONNECTOR_DVII, "DVI-I" },
    { DRM_MODE_CONNECTOR_DVID, "DVI-D" },
    { DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
    { DRM_MODE_CONNECTOR_Composite, "composite" },
    { DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
    { DRM_MODE_CONNECTOR_LVDS, "LVDS" },
    { DRM_MODE_CONNECTOR_Component, "component" },
    { DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
    { DRM_MODE_CONNECTOR_DisplayPort, "DP" },
    { DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
    { DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
    { DRM_MODE_CONNECTOR_TV, "TV" },
    { DRM_MODE_CONNECTOR_eDP, "eDP" },
    { DRM_MODE_CONNECTOR_VIRTUAL, "Virtual" },
    { DRM_MODE_CONNECTOR_DSI, "DSI" },
    { DRM_MODE_CONNECTOR_DPI, "DPI" },
};

static const char *util_lookup_type_name(unsigned int type,
        const struct type_name *table,
        unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++)
        if (table[i].type == type)
            return table[i].name;

    return NULL;
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
const char *util_lookup_connector_type_name(unsigned int type)
{
    return util_lookup_type_name(type, connector_type_names,
            ARRAY_SIZE(connector_type_names));
}

/*Copy from libdrm - modetest*/
static struct resources *get_resources(struct device *dev)
{
	struct resources *res;
	int i;

	res = calloc(1, sizeof(*res));
	if (res == 0)
		return NULL;

	drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

	res->res = drmModeGetResources(dev->fd);
	if (!res->res) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		goto error;
	}

	res->crtcs = calloc(res->res->count_crtcs, sizeof(*res->crtcs));
	res->encoders = calloc(res->res->count_encoders, sizeof(*res->encoders));
	res->connectors = calloc(res->res->count_connectors, sizeof(*res->connectors));
	res->fbs = calloc(res->res->count_fbs, sizeof(*res->fbs));

	if (!res->crtcs || !res->encoders || !res->connectors || !res->fbs)
		goto error;

#define get_resource(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			(_res)->type##s[i].type =				\
				drmModeGet##Type(dev->fd, (_res)->__res->type##s[i]); \
			if (!(_res)->type##s[i].type)				\
				fprintf(stderr, "could not get %s %i: %s\n",	\
					#type, (_res)->__res->type##s[i],	\
					strerror(errno));			\
		}								\
	} while (0)

	get_resource(res, res, crtc, Crtc);
	get_resource(res, res, encoder, Encoder);
	get_resource(res, res, connector, Connector);
	get_resource(res, res, fb, FB);

	/* Set the name of all connectors based on the type name and the per-type ID. */
	for (i = 0; i < res->res->count_connectors; i++) {
		struct connector *connector = &res->connectors[i];
		drmModeConnector *conn = connector->connector;
		int num;

		num = asprintf(&connector->name, "%s-%u",
			 util_lookup_connector_type_name(conn->connector_type),
			 conn->connector_type_id);
		if (num < 0)
			goto error;
	}

#define get_properties(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &res->type##s[i];			\
			unsigned int j;						\
			obj->props =						\
				drmModeObjectGetProperties(dev->fd, obj->type->type##_id, \
							   DRM_MODE_OBJECT_##Type); \
			if (!obj->props) {					\
				fprintf(stderr,					\
					"could not get %s %i properties: %s\n", \
					#type, obj->type->type##_id,		\
					strerror(errno));			\
				continue;					\
			}							\
			obj->props_info = calloc(obj->props->count_props,	\
						 sizeof(*obj->props_info));	\
			if (!obj->props_info)					\
				continue;					\
			for (j = 0; j < obj->props->count_props; ++j)		\
				obj->props_info[j] =				\
					drmModeGetProperty(dev->fd, obj->props->props[j]); \
		}								\
	} while (0)

	get_properties(res, res, crtc, CRTC);
	get_properties(res, res, connector, CONNECTOR);

	for (i = 0; i < res->res->count_crtcs; ++i)
		res->crtcs[i].mode = &res->crtcs[i].crtc->mode;

	res->plane_res = drmModeGetPlaneResources(dev->fd);
	if (!res->plane_res) {
		fprintf(stderr, "drmModeGetPlaneResources failed: %s\n",
			strerror(errno));
		return res;
	}

	res->planes = calloc(res->plane_res->count_planes, sizeof(*res->planes));
	if (!res->planes)
		goto error;

	get_resource(res, plane_res, plane, Plane);
	get_properties(res, plane_res, plane, PLANE);

	return res;

error:
	free_resources(res);
	return NULL;
}

/* -----------------------------------------------------------------------------
 * Properties: from libdrm modetest
 */

struct property_arg {
	uint32_t obj_id;
	uint32_t obj_type;
	char name[DRM_PROP_NAME_LEN+1];
	uint32_t prop_id;
	uint64_t value;
	bool optional;
};

static bool set_property(struct device *dev, struct property_arg *p)
{
	drmModeObjectProperties *props = NULL;
	drmModePropertyRes **props_info = NULL;
	const char *obj_type;
	int ret;
	int i;

	p->obj_type = 0;
	p->prop_id = 0;

#define find_object(_res, __res, type, Type)					\
	do {									\
		for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {	\
			struct type *obj = &(_res)->type##s[i];			\
			if (obj->type->type##_id != p->obj_id)			\
				continue;					\
			p->obj_type = DRM_MODE_OBJECT_##Type;			\
			obj_type = #Type;					\
			props = obj->props;					\
			props_info = obj->props_info;				\
		}								\
	} while(0)								\

	find_object(dev->resources, res, crtc, CRTC);
	if (p->obj_type == 0)
		find_object(dev->resources, res, connector, CONNECTOR);
	if (p->obj_type == 0)
		find_object(dev->resources, plane_res, plane, PLANE);
	if (p->obj_type == 0) {
		fprintf(stderr, "Object %i not found, can't set property\n",
			p->obj_id);
		return false;
	}

	if (!props) {
		fprintf(stderr, "%s %i has no properties\n",
			obj_type, p->obj_id);
		return false;
	}

	for (i = 0; i < (int)props->count_props; ++i) {
		if (!props_info[i])
			continue;
		if (strcmp(props_info[i]->name, p->name) == 0)
			break;
	}

	if (i == (int)props->count_props) {
		if (!p->optional)
			fprintf(stderr, "%s %i has no %s property\n",
				obj_type, p->obj_id, p->name);
		return false;
	}

	p->prop_id = props->props[i];

        ret = drmModeAtomicAddProperty(dev->req, p->obj_id, p->prop_id, p->value);
	if (ret < 0)
		fprintf(stderr, "failed to set %s %i property %s to %" PRIu64 ": %s\n",
			obj_type, p->obj_id, p->name, p->value, strerror(errno));

	return true;
}

static void add_property(struct device *dev, uint32_t obj_id,
			       const char *name, uint64_t value)
{
	struct property_arg p;

	p.obj_id = obj_id;
	strcpy(p.name, name);
	p.value = value;

	set_property(dev, &p);
}

static drmModeModeInfo * connector_find_mode(struct device *dev, uint32_t con_id,
                                             const char *mode_str, const unsigned int vrefresh)
{
    drmModeConnector *connector = NULL;;
    drmModeModeInfo *mode = NULL;
    bool use_preferred = false;
    int i;

    for (i = 0; i < dev->resources->res->count_connectors; i++) {
        if (dev->resources->connectors[i].connector->connector_id == con_id) {
            connector = dev->resources->connectors[i].connector;
            break;
        }
    }
    if (!connector || !connector->count_modes)
        return NULL;

    if (NULL == mode_str || 0 == strlen(mode_str)) {
        use_preferred = true;
    }
    for (i = 0; i < connector->count_modes; i++) {
        mode = &connector->modes[i];
        if (use_preferred) {
            if (mode->type & DRM_MODE_TYPE_PREFERRED)
                return mode;
        } else if (!strcmp(mode->name, mode_str)) {
            /* If the vertical refresh frequency is not specified then return the
             * first mode that match with the name. Else, return the mode that match
             * the name and the specified vertical refresh frequency.
             */
            if (vrefresh == 0)
                return mode;
            else if (mode->vrefresh == vrefresh)
                return mode;
        }
    }

    return NULL;
}


pthread_mutex_t main_mutex;
pthread_cond_t main_cond;
static bool user_stop = false;

void signal_handle(int signal) {
    (void) signal;
    fprintf(stderr, "User interrupt handling\n");
    pthread_mutex_lock(&main_mutex);
    user_stop = true;
    pthread_mutex_unlock(&main_mutex);
    pthread_cond_signal(&main_cond);
}

void wait_connection() {
    //Hark for hotplugin
    //the hot plugin not work in same platform
    sleep(3);
}

char str[] = "d:c:m:p:C:s:";
int main(int argc, char** argv) {
    int c;
    int ret = 0;
    const char* device_name = "meson";
    const char* display_mode = "";
    int display_refresh = 0;
    int connector_id = -1;
    int crtc_id = -1;
    struct device dev;
    int run_as_service = 1;

    pthread_mutex_init(&main_mutex, NULL);
    pthread_cond_init(&main_cond, NULL);
    user_stop = false;
    drmModeModeInfo *mode = NULL;

    memset(&dev, 0, sizeof dev);
    signal(SIGINT, signal_handle);
    signal(SIGTERM, signal_handle);

    while ((c = getopt(argc, argv, str)) !=-1) {
        switch (c) {
            case 'd':
                device_name = optarg;
                break;
            case 'm':
                display_mode = optarg;
                break;
            case 'p':
                display_refresh = atoi(optarg);
                break;
            case 'c':
                connector_id = atoi(optarg);
                break;
            case 'C':
                crtc_id = atoi(optarg);
                break;
            case 's':
                run_as_service = atoi(optarg);
                break;
            default:
                printf("Unknown Usage:-%c\nUsage %s: -d device_path -c connector_id -C crtc_id -m mode_str -p refresh_rate\n\tegl: %s -d %s -m 1920x1080 -p 60\n",c, argv[0], argv[0], device_name);
                exit(-1);
        }
    }
    fprintf(stderr, "Use device:%s with connector mode:%s\n", device_name, strlen(display_mode)? display_mode : "preferred");

    //The busid can be NULL.
    //If no /dev/dri, may be caused by the udev not enabed.
    //Need disable udev on libdrm build configuration.
    dev.fd = drmOpen(device_name, NULL);
    if (dev.fd < 0) {
        fprintf(stderr, "failed to open device %s\n", strerror(errno));
        exit(-1);
    }
    /*use atomic*/
    ret = drmSetClientCap(dev.fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret) {
        fprintf(stderr, "no atomic modesetting support: %s\n", strerror(errno));
        drmClose(dev.fd);
        exit(-1);
    }

    bool need_wait_connection = false;
    do {
        need_wait_connection = false;
        if (dev.resources) {
            free_resources(dev.resources);
        }
        dev.resources = get_resources(&dev);

        if (dev.resources == NULL || dev.resources->res->count_crtcs < 1 || dev.resources->res->count_connectors < 1 ) {
            fprintf(stderr, "failed get connector or crtc\n");
            ret = -1;
            need_wait_connection = true;
            wait_connection();
            continue;
        }
        fprintf(stderr, "find crtcs:%d, connectors:%d\n", dev.resources->res->count_crtcs, dev.resources->res->count_connectors );
        if (connector_id == -1) {
            //Set Default connector
            connector_id = dev.resources->connectors[0].connector->connector_id;
        }

        if (crtc_id == -1) {
            crtc_id = dev.resources->crtcs[0].crtc->crtc_id;
        }
        mode = connector_find_mode(&dev,  connector_id, display_mode, display_refresh);
        {//Dump crtc/connector info
            int i,j;
            for (j = 0; j < dev.resources->res->count_crtcs; j++) {
                struct crtc *crtc = &dev.resources->crtcs[j];
                fprintf(stderr, "Dump crtc[%d]\n", crtc->crtc->crtc_id);
            }
            for (j = 0; j < dev.resources->res->count_connectors; j++) {
                struct connector *conn = &dev.resources->connectors[j];
                fprintf(stderr, "Dump connector[%d] info:%s [have %d modes]\n", conn->connector->connector_id,  conn->name, conn->connector->count_modes);
                if (conn->connector->count_modes) {
                    for (i = 0; i < conn->connector->count_modes; i++) {
                        fprintf(stderr, "mode %d: %s@%d\n", i, conn->connector->modes[i].name, conn->connector->modes[i].vrefresh);
                    }
                }
            }
        }
        if (mode == NULL) {
            fprintf(stderr, "failed to find display-mode for connector %d\n", connector_id);
            need_wait_connection = true;
            wait_connection();
        }
    } while (need_wait_connection && !user_stop);

    if (user_stop) {
        fprintf(stderr, "User stop\n");
        goto out;
    }

    fprintf(stdout, "Set crtc[%d] with conn[%u] use displaymode:%s@%u\n", crtc_id,connector_id, mode->name, mode->vrefresh);
    /*ATOMIC to set crtc-connector, but no plane&fb specified.*/
    dev.req = drmModeAtomicAlloc();

    add_property(&dev, connector_id, "CRTC_ID", crtc_id);

    uint32_t mode_blobid = 0;
    drmModeCreatePropertyBlob(dev.fd, mode, sizeof(*mode), &mode_blobid);
    add_property(&dev, crtc_id, "MODE_ID", mode_blobid);
    add_property(&dev, crtc_id, "ACTIVE", 1);

    ret = drmModeAtomicCommit(dev.fd, dev.req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
    if (ret) {
        fprintf(stderr, "failed to set mode: %d-%s\n", ret, strerror(errno));
        goto out;
    }

    drmModeAtomicFree(dev.req);
    dev.req = NULL;

    /* not run as service */
    if (!run_as_service) {
        fprintf(stderr, "exit after set mode.\n");
        goto out;
    }

    /* Set up our event handler */
    drmEventContext evctx;
    memset(&evctx, 0, sizeof evctx);
    evctx.version = DRM_EVENT_CONTEXT_VERSION;
    evctx.vblank_handler = NULL;
    evctx.page_flip_handler = NULL;

    user_stop = true;
    /* Poll for events */
    while (!user_stop) {
#if 1
        sleep(1);
        pthread_mutex_lock(&main_mutex);
        pthread_cond_wait(&main_cond, &main_mutex);
        pthread_mutex_unlock(&main_mutex);
#else
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(dev.fd, &fds);
        ret = select(dev.fd + 1, &fds, NULL, NULL, NULL);

        if (ret <= 0) {
            fprintf(stderr, "select timed out or error (ret %d)\n",
                    ret);
            continue;
        } else if (FD_ISSET(0, &fds)) {
            break;
        }

        ret = drmHandleEvent(dev.fd, &evctx);
        if (ret != 0) {
            printf("drmHandleEvent failed: %i\n", ret);
            return -1;
        }
#endif
    }

out:
    if (dev.resources)
        free_resources(dev.resources);
    drmClose(dev.fd);
    fprintf(stderr, "Exit\n");
    return ret;
}

