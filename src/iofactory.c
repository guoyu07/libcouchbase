/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2011 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "internal.h"
#include <dlfcn.h>

typedef lcb_io_opt_t (*create_func)(struct event_base *base);

struct plugin_st {
    void *dlhandle;
    union {
        create_func create;
        void *voidptr;
    } func;
};

static lcb_error_t get_create_func(const char *image,
                                   struct plugin_st *plugin)
{
    void *dlhandle = dlopen(image, RTLD_NOW | RTLD_LOCAL);
    if (dlhandle == NULL) {
        return LCB_ERROR;
    }

    plugin->func.create = NULL;
    plugin->func.voidptr = dlsym(dlhandle, "lcb_create_libevent_io_opts");
    if (plugin->func.voidptr == NULL) {
        dlclose(dlhandle);
        dlhandle = NULL;
    } else {
        plugin->dlhandle = dlhandle;
    }
    return LCB_SUCCESS;
}

#ifdef __APPLE__
#define PLUGIN_SO(NAME) "libcouchbase_"NAME".1.dylib"
#else
#define PLUGIN_SO(NAME) "libcouchbase_"NAME".so.1"
#endif

LIBCOUCHBASE_API
lcb_error_t lcb_create_io_ops(lcb_io_opt_t *io,
                              const struct lcb_create_io_ops_st* options)
{
    lcb_error_t ret = LCB_SUCCESS;
    lcb_io_ops_type_t type = LCB_IO_OPS_DEFAULT;
    void *cookie = NULL;

    if (options != NULL) {
        if (options->version != 0) {
            return LCB_EINVAL;
        }
        type = options->v.v0.type;
        cookie = options->v.v0.cookie;
    }

    if (type == LCB_IO_OPS_DEFAULT || type == LCB_IO_OPS_LIBEVENT) {
        struct plugin_st plugin;
        memset(&plugin, 0, sizeof(plugin));
        /* search definition in main program */
        ret = get_create_func(NULL, &plugin);

        if (ret != LCB_SUCCESS) {
            return ret;
        }

#ifndef LCB_LIBEVENT_PLUGIN_EMBED
        if (plugin.func.create == NULL) {
            ret = get_create_func(PLUGIN_SO("libevent"), &plugin);
        }
#endif
        if (ret != LCB_SUCCESS) {
            return ret;
        }

        if (plugin.func.create != NULL) {
            *io = plugin.func.create(cookie);
            if (*io == NULL) {
                return LCB_CLIENT_ENOMEM;
            } else {
                (*io)->dlhandle = plugin.dlhandle;
            }
        }

    } else {
        return LCB_NOT_SUPPORTED;
    }

    return LCB_SUCCESS;
}
#undef PLUGIN_SO
