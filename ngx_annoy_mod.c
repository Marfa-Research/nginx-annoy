#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "annoy.h"
#include <errno.h>
#include <uthash.h>


static void *ngx_annoy_create_conf(ngx_conf_t *cf);
static void *ngx_annoy_create_loc_conf(ngx_conf_t *cf);
static char *ngx_annoy(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_annoy_search(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_annoy_handle_search(ngx_http_request_t *r);

#define ID_MAX_LENGTH 32
#define UUID_MAX_LENGTH 36

typedef struct {
    int id;
    char uuid[UUID_MAX_LENGTH+1];
    UT_hash_handle hh1;
    UT_hash_handle hh2;
} uuid_map_t;


typedef struct {
    void *a;
    uuid_map_t *by_id;
    uuid_map_t *by_uuid;
} ngx_annoy_conf_t;

typedef struct {
    int n;
    char content_uuid[UUID_MAX_LENGTH+1];
} ngx_annoy_loc_conf_t;

static ngx_command_t ngx_annoy_commands[] =  {
   { ngx_string("annoy_init"),
     NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE2,
     ngx_annoy,
     NGX_HTTP_MAIN_CONF_OFFSET,
     0,
     NULL }, 
   { ngx_string("annoy_nns_by_uuid"),
     NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
     ngx_annoy_search,
     NGX_HTTP_LOC_CONF_OFFSET,
     0,
     NULL }, 
   ngx_null_command
};

static ngx_http_module_t ngx_annoy_mod_ctx =  { 
    NULL,
    NULL,
    ngx_annoy_create_conf,
    NULL,
    NULL,
    NULL,
    ngx_annoy_create_loc_conf,
    NULL 
};

ngx_module_t ngx_annoy_mod = { 
    NGX_MODULE_V1,
    &ngx_annoy_mod_ctx,
    ngx_annoy_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING 
};

static void ngx_annoy_release(void *data) {
    ngx_annoy_conf_t *c = data;

    uuid_map_t *entry, *tmp;
    HASH_ITER(hh1, c->by_id, entry, tmp) {
        HASH_DELETE(hh1, c->by_id, entry);
        HASH_DELETE(hh2, c->by_uuid, entry);
        free(entry);
    }

    annoy__release(c->a);
}

static void *ngx_annoy_create_conf(ngx_conf_t *cf) {
    ngx_pool_cleanup_t *cleanup;
    ngx_annoy_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_annoy_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    cleanup = ngx_pool_cleanup_add(cf->pool, 0);
    if (cleanup == NULL) {
        return NGX_CONF_ERROR;
    }
    cleanup->handler = ngx_annoy_release;
    cleanup->data = conf;

    conf->a = NULL;
    return conf;
}

static void *ngx_annoy_create_loc_conf(ngx_conf_t *cf) {
    ngx_annoy_loc_conf_t *lcf;
    lcf = ngx_pcalloc(cf->pool, sizeof(ngx_annoy_loc_conf_t));

    if (lcf == NULL) {
        return NGX_CONF_ERROR;
    }
    return lcf;
}

static char *ngx_annoy(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_str_t *value;
    value = cf->args->elts;
    ngx_annoy_conf_t *c = conf;

    if (c->a == NULL) {
        char *id_map_path = malloc(sizeof(char *)*(1+value[1].len)); 
        memcpy(id_map_path, value[1].data, value[1].len);
        id_map_path[value[1].len] = '\0';

        char *annoy_file_path = malloc(sizeof(char *)*(1+value[2].len)); 
        memcpy(annoy_file_path, value[2].data, value[2].len);
        annoy_file_path[value[2].len] = '\0';

        c->a = annoy__create();
        annoy__load(c->a, annoy_file_path);

        c->by_id = NULL;
        c->by_uuid = NULL;

        int nchars;
        long length;
        char *uuid_file = 0;
        FILE *f = fopen(id_map_path, "rb");
    
        if (f) {
            fseek(f, 0, SEEK_END);
            length = ftell(f);
            fseek(f, 0, SEEK_SET);
            uuid_file = malloc(length+sizeof(char *));
            nchars = fread(uuid_file, 1, length, f);
            uuid_file[nchars] = '\0';
    
            int i = 0;
            for (char *line = strtok(uuid_file, "\n");line;line = strtok(NULL, "\n")) {
                uuid_map_t *s = malloc(sizeof(uuid_map_t));
                s->id = i;
                strcpy(s->uuid, line);
                HASH_ADD(hh1, c->by_id, id, sizeof(int), s);
                HASH_ADD(hh2, c->by_uuid, uuid, strlen(s->uuid), s);
                ++i;
            }
            fclose(f);
        } 
        else 
            return NGX_CONF_ERROR;
    }
    return NGX_CONF_OK;
}

static char *ngx_annoy_search(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *lcf;
    lcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    lcf->handler = ngx_annoy_handle_search;

    ngx_str_t *val;
    val = cf->args->elts;
    ngx_annoy_loc_conf_t *c = conf;

    memcpy(c->content_uuid, val[1].data, val[1].len);
    c->content_uuid[val[1].len] = '\0';

    c->n = ngx_atoi(val[2].data, val[2].len);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_annoy_handle_search(ngx_http_request_t *r) {
    ngx_annoy_conf_t *conf;
    conf = ngx_http_get_module_main_conf(r, ngx_annoy_mod);

    ngx_annoy_loc_conf_t *lcf;
    lcf = ngx_http_get_module_loc_conf(r, ngx_annoy_mod);

    uuid_map_t *id_lookup;
    HASH_FIND(hh2, conf->by_uuid, lcf->content_uuid, strlen(lcf->content_uuid), id_lookup); 

    char *output = malloc(lcf->n*40+12*sizeof(char *));
    char *pos = output;

    int32_t *ids = malloc(sizeof(int32_t *));
    annoy__get_nns_by_item(conf->a, id_lookup->id, lcf->n, 10, ids, NULL);


    pos += sprintf(pos, "{\"recs\": [");
    uuid_map_t *uuid_lookup;
    int n = sizeof(ids)/sizeof(ids[0])-1;
    for (int i = 0; i < n; ++i) {
        HASH_FIND(hh1, conf->by_id, &ids[i], sizeof(int), uuid_lookup); 
        pos += sprintf(pos, "\"%s\"", uuid_lookup->uuid);
        if (i != n-1) 
            pos += sprintf(pos, ",");
    }

    if (n < lcf->n) {
        int32_t *backfill_ids = malloc(sizeof(int32_t *));
        annoy__get_nns_by_item(conf->a, ids[0], lcf->n-n, 10, backfill_ids, NULL);
        if (backfill_ids != NULL) {
            int bn = sizeof(backfill_ids)/sizeof(backfill_ids[0])-1;
            uuid_map_t *backfill_uuid_lookup;
            for (int i = 0; i < bn; ++i) {
                HASH_FIND(hh1, conf->by_id, &backfill_ids[i], sizeof(int), backfill_uuid_lookup); 
                pos += sprintf(pos, ",\"%s\"", backfill_uuid_lookup->uuid);
            }
        }
    }
    pos += sprintf(pos, "]}");
    pos = "\0";

    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;

    const char *content_type = "application/json";

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = strlen(output);
    r->headers_out.content_type.len = sizeof(content_type)-1;
    r->headers_out.content_type.data = (u_char *) content_type;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    b = ngx_calloc_buf(r->pool);
    if (b == NULL) {
        return NGX_ERROR;
    }

    b->last_buf = 1;
    b->last_in_chain = 1;
    b->memory = 1;

    b->pos = (u_char *) output;
    b->last = b->pos + strlen(output);

    out.buf = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}
