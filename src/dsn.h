#ifndef LCB_DSN_H
#define LCB_DSN_H

#include <libcouchbase/couchbase.h>
#include "config.h"
#include "simplestring.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lcb_list_t llnode;
    lcb_U16 htport;
    lcb_U16 memdport;
    lcb_U16 ssl_htport;
    lcb_U16 ssl_memdport;
    char hostname[1];
} lcb_DSNHOST;

typedef struct {
    char *ctlopts; /**< Iterator for option string. opt1=val1&opt2=val2 */
    unsigned optslen; /**< Total number of bytes in ctlopts */
    char *bucket; /**< Bucket string. Free with 'free()' */
    char *username; /**< Username. Currently not used */
    char *password; /**< Password */
    char *capath; /**< Certificate path */
    lcb_SSLOPTS sslopts; /**< SSL Options */
    lcb_list_t hosts; /**< List of host information */
    char has_custom_ports; /**< Whether custom ports are specified */
    char has_no_ports; /**< Whether hosts don't have ports */
    unsigned flags; /**< Internal flags */
    lcb_config_transport_t transports[LCB_CONFIG_TRANSPORT_MAX];
} lcb_DSNPARAMS;

#define LCB_DSN_SCHEME "couchbase://"

/**
 * Compile a DSN into a structure suitable for further processing. A Couchbase
 * DSN consists of a mandatory _scheme_ (currently only `couchbase://`) is
 * recognized, an optional _authority_ section, an optional _path_ section,
 * and an optional _parameters_ section.
 */
LIBCOUCHBASE_API
lcb_error_t
lcb_dsn_parse(const char *dsn, lcb_DSNPARAMS *compiled, const char **errmsg);

/**
 * Convert an older lcb_create_st structure to an lcb_DSNPARAMS structure.
 * @param params structure to be populated
 * @param cropts structure to read from
 * @return error code on failure, LCB_SUCCESS on success.
 */
LIBCOUCHBASE_API
lcb_error_t
lcb_dsn_convert(lcb_DSNPARAMS *params, const struct lcb_create_st *cropts);

LIBCOUCHBASE_API
void
lcb_dsn_clean(lcb_DSNPARAMS *params);

/**
 * Iterate over the option pairs found in the original string. This iterates
 * over all _unrecognized_ options.
 *
 * @param params The compiled DSN context
 * @param[out] key a pointer to the option key
 * @param[out] value a pointer to the option value
 * @param[in,out] ctx iterator. This should be initialized to 0 upon the
 * first call
 * @return true if an option was fetched (and thus `key` and `value` contain
 * valid pointers) or false if there are no more options.
 */
LIBCOUCHBASE_API
int
lcb_dsn_next_option(const lcb_DSNPARAMS *params,
    const char **key, const char **value, int *ctx);


#ifdef __cplusplus
}
#endif
#endif
