/* Copyright 2015 greenbytes GmbH (https://www.greenbytes.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>

#include <ap_mpm.h>

#include <httpd.h>
#include <http_core.h>
#include <http_config.h>
#include <http_log.h>
#include <http_connection.h>
#include <http_protocol.h>
#include <http_request.h>

#include "h2_private.h"
#include "h2_config.h"
#include "h2_ctx.h"
#include "h2_filter.h"
#include "h2_mplx.h"
#include "h2_session.h"
#include "h2_stream.h"
<<<<<<< HEAD
#include "h2_stream_set.h"
=======
>>>>>>> master
#include "h2_h2.h"
#include "h2_task.h"
#include "h2_worker.h"
#include "h2_workers.h"
#include "h2_conn.h"
<<<<<<< HEAD
=======
#include "h2_version.h"
>>>>>>> master

static struct h2_workers *workers;

static h2_mpm_type_t mpm_type = H2_MPM_UNKNOWN;
static module *mpm_module;
<<<<<<< HEAD
static int checked;

static void check_modules(void) 
{
    int i;
    if (!checked) {
        for (i = 0; ap_loaded_modules[i]; ++i) {
            module *m = ap_loaded_modules[i];
            if (!strcmp("event.c", m->name)) {
                mpm_type = H2_MPM_EVENT;
                mpm_module = m;
            }
            else if (!strcmp("worker.c", m->name)) {
                mpm_type = H2_MPM_WORKER;
                mpm_module = m;
=======
static int async_mpm;
static apr_socket_t *dummy_socket;

static void check_modules(int force) 
{
    static int checked = 0;
    int i;

    if (force || !checked) {
        for (i = 0; ap_loaded_modules[i]; ++i) {
            module *m = ap_loaded_modules[i];
            
            if (!strcmp("event.c", m->name)) {
                mpm_type = H2_MPM_EVENT;
                mpm_module = m;
                break;
            }
            else if (!strcmp("motorz.c", m->name)) {
                mpm_type = H2_MPM_MOTORZ;
                mpm_module = m;
                break;
            }
            else if (!strcmp("mpm_netware.c", m->name)) {
                mpm_type = H2_MPM_NETWARE;
                mpm_module = m;
                break;
>>>>>>> master
            }
            else if (!strcmp("prefork.c", m->name)) {
                mpm_type = H2_MPM_PREFORK;
                mpm_module = m;
<<<<<<< HEAD
=======
                break;
            }
            else if (!strcmp("simple_api.c", m->name)) {
                mpm_type = H2_MPM_SIMPLE;
                mpm_module = m;
                break;
            }
            else if (!strcmp("mpm_winnt.c", m->name)) {
                mpm_type = H2_MPM_WINNT;
                mpm_module = m;
                break;
            }
            else if (!strcmp("worker.c", m->name)) {
                mpm_type = H2_MPM_WORKER;
                mpm_module = m;
                break;
>>>>>>> master
            }
        }
        checked = 1;
    }
}

apr_status_t h2_conn_child_init(apr_pool_t *pool, server_rec *s)
{
    const h2_config *config = h2_config_sget(s);
    apr_status_t status = APR_SUCCESS;
<<<<<<< HEAD
    int minw = h2_config_geti(config, H2_CONF_MIN_WORKERS);
    int maxw = h2_config_geti(config, H2_CONF_MAX_WORKERS);    
    int max_threads_per_child = 0;
    int idle_secs = 0;
    int i;

    h2_config_init(pool);
    
    ap_mpm_query(AP_MPMQ_MAX_THREADS, &max_threads_per_child);
    
    for (i = 0; ap_loaded_modules[i]; ++i) {
        module *m = ap_loaded_modules[i];
        if (!strcmp("event.c", m->name)) {
            mpm_type = H2_MPM_EVENT;
            mpm_module = m;
        }
        else if (!strcmp("worker.c", m->name)) {
            mpm_type = H2_MPM_WORKER;
            mpm_module = m;
        }
        else if (!strcmp("prefork.c", m->name)) {
            mpm_type = H2_MPM_PREFORK;
            mpm_module = m;
        }
    }
    
=======
    int minw, maxw, max_tx_handles, n;
    int max_threads_per_child = 0;
    int idle_secs = 0;

    check_modules(1);
    
    ap_mpm_query(AP_MPMQ_MAX_THREADS, &max_threads_per_child);
    
    status = ap_mpm_query(AP_MPMQ_IS_ASYNC, &async_mpm);
    if (status != APR_SUCCESS) {
        /* some MPMs do not implemnent this */
        async_mpm = 0;
        status = APR_SUCCESS;
    }

    h2_config_init(pool);
    
    minw = h2_config_geti(config, H2_CONF_MIN_WORKERS);
    maxw = h2_config_geti(config, H2_CONF_MAX_WORKERS);    
>>>>>>> master
    if (minw <= 0) {
        minw = max_threads_per_child;
    }
    if (maxw <= 0) {
        maxw = minw;
    }
    
<<<<<<< HEAD
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s,
                 "h2_workers: min=%d max=%d, mthrpchild=%d", 
                 minw, maxw, max_threads_per_child);
    
    workers = h2_workers_create(s, pool, minw, maxw);
=======
    /* How many file handles is it safe to use for transfer
     * to the master connection to be streamed out? 
     * Is there a portable APR rlimit on NOFILES? Have not
     * found it. And if, how many of those would we set aside?
     * This leads all into a process wide handle allocation strategy
     * which ultimately would limit the number of accepted connections
     * with the assumption of implicitly reserving n handles for every 
     * connection and requiring modules with excessive needs to allocate
     * from a central pool.
     */
    n = h2_config_geti(config, H2_CONF_SESSION_FILES);
    if (n < 0) {
        max_tx_handles = maxw * 2;
    }
    else {
        max_tx_handles = maxw * n;
    }
    
    ap_log_error(APLOG_MARK, APLOG_TRACE3, 0, s,
                 "h2_workers: min=%d max=%d, mthrpchild=%d, tx_files=%d", 
                 minw, maxw, max_threads_per_child, max_tx_handles);
    workers = h2_workers_create(s, pool, minw, maxw, max_tx_handles);
    
>>>>>>> master
    idle_secs = h2_config_geti(config, H2_CONF_MAX_WORKER_IDLE_SECS);
    h2_workers_set_max_idle_secs(workers, idle_secs);
 
    ap_register_input_filter("H2_IN", h2_filter_core_input,
                             NULL, AP_FTYPE_CONNECTION);
   
<<<<<<< HEAD
    return status;
}

h2_mpm_type_t h2_conn_mpm_type(void) {
    check_modules();
    return mpm_type;
}

static module *h2_conn_mpm_module(void) {
    check_modules();
=======
    status = h2_mplx_child_init(pool, s);

    if (status == APR_SUCCESS) {
        status = apr_socket_create(&dummy_socket, APR_INET, SOCK_STREAM,
                                   APR_PROTO_TCP, pool);
    }

    return status;
}

h2_mpm_type_t h2_conn_mpm_type(void)
{
    check_modules(0);
    return mpm_type;
}

static module *h2_conn_mpm_module(void)
{
    check_modules(0);
>>>>>>> master
    return mpm_module;
}

apr_status_t h2_conn_setup(h2_ctx *ctx, conn_rec *c, request_rec *r)
{
    h2_session *session;
<<<<<<< HEAD
    h2_filter_core_in *in;
=======
>>>>>>> master
    
    if (!workers) {
        ap_log_cerror(APLOG_MARK, APLOG_ERR, 0, c, APLOGNO(02911) 
                      "workers not initialized");
        return APR_EGENERAL;
    }
    
    if (r) {
        session = h2_session_rcreate(r, ctx, workers);
    }
    else {
        session = h2_session_create(c, ctx, workers);
    }

    h2_ctx_session_set(ctx, session);
    
<<<<<<< HEAD
    in = apr_pcalloc(session->pool, sizeof(*in));
    in->session = session;
    ap_add_input_filter("H2_IN", in, r, c);
    
    ap_update_child_status_from_conn(c->sbh, SERVER_BUSY_READ, c);

    return APR_SUCCESS;
}

apr_status_t h2_conn_process(h2_ctx *ctx, int async)
{
    apr_status_t status;
    h2_session *session;
    
    session = h2_ctx_session_get(ctx);
    if (session->c->cs) {
        session->c->cs->sense = CONN_SENSE_DEFAULT;
    }

    status = h2_session_process(session, async);

    if (session->c->cs) {
        session->c->cs->state = CONN_STATE_WRITE_COMPLETION;
    }
    
    if (status == APR_EOF) {
        ap_log_cerror( APLOG_MARK, APLOG_DEBUG, status, session->c,
                      "h2_session(%ld): done", session->id);
        /* Make sure this connection gets closed properly. */
        ap_update_child_status_from_conn(session->c->sbh, SERVER_CLOSING, session->c);
        session->c->keepalive = AP_CONN_CLOSE;
        
        h2_session_close(session);
        /* hereafter session will be gone */
    }
    else {
        session->c->keepalive = AP_CONN_KEEPALIVE;
    }
=======
    return APR_SUCCESS;
}

apr_status_t h2_conn_run(struct h2_ctx *ctx, conn_rec *c)
{
    apr_status_t status;
    int mpm_state = 0;
    
    do {
        if (c->cs) {
            c->cs->sense = CONN_SENSE_DEFAULT;
        }
        status = h2_session_process(h2_ctx_session_get(ctx), async_mpm);
        
        if (APR_STATUS_IS_EOF(status)) {
            ap_log_cerror(APLOG_MARK, APLOG_DEBUG, status, c, APLOGNO(03045)
                          "h2_session(%ld): process, closing conn", c->id);
            c->keepalive = AP_CONN_CLOSE;
        }
        else {
            c->keepalive = AP_CONN_KEEPALIVE;
        }
        
        if (ap_mpm_query(AP_MPMQ_MPM_STATE, &mpm_state)) {
            break;
        }
    } while (!async_mpm
             && c->keepalive == AP_CONN_KEEPALIVE 
             && mpm_state != AP_MPMQ_STOPPING);
>>>>>>> master
    
    return DONE;
}

<<<<<<< HEAD
apr_status_t h2_conn_run(struct h2_ctx *ctx, conn_rec *c)
{
    do {
        h2_conn_process(ctx, 0);
    } while (c->keepalive == AP_CONN_KEEPALIVE && !c->aborted);
    
    return DONE;
}


static void fix_event_conn(conn_rec *c, conn_rec *master);

conn_rec *h2_conn_create(conn_rec *master, apr_pool_t *pool)
{
    conn_rec *c;
    
    AP_DEBUG_ASSERT(master);

    /* This is like the slave connection creation from 2.5-DEV. A
     * very efficient way - not sure how compatible this is, since
     * the core hooks are no longer run.
     * But maybe it's is better this way, not sure yet.
     */
    c = (conn_rec *) apr_palloc(pool, sizeof(conn_rec));
    if (c == NULL) {
        ap_log_perror(APLOG_MARK, APLOG_ERR, APR_ENOMEM, pool, 
=======
apr_status_t h2_conn_pre_close(struct h2_ctx *ctx, conn_rec *c)
{
    apr_status_t status;
    
    status = h2_session_pre_close(h2_ctx_session_get(ctx), async_mpm);
    if (status == APR_SUCCESS) {
        return DONE; /* This is the same, right? */
    }
    return status;
}

conn_rec *h2_slave_create(conn_rec *master, apr_pool_t *parent,
                          apr_allocator_t *allocator)
{
    apr_pool_t *pool;
    conn_rec *c;
    void *cfg;
    
    AP_DEBUG_ASSERT(master);
    ap_log_cerror(APLOG_MARK, APLOG_TRACE3, 0, master,
                  "h2_conn(%ld): create slave", master->id);
    
    /* We create a pool with its own allocator to be used for
     * processing a request. This is the only way to have the processing
     * independant of its parent pool in the sense that it can work in
     * another thread.
     */
    if (!allocator) {
        apr_allocator_create(&allocator);
    }
    apr_pool_create_ex(&pool, parent, NULL, allocator);
    apr_pool_tag(pool, "h2_slave_conn");
    apr_allocator_owner_set(allocator, pool);

    c = (conn_rec *) apr_palloc(pool, sizeof(conn_rec));
    if (c == NULL) {
        ap_log_cerror(APLOG_MARK, APLOG_ERR, APR_ENOMEM, master, 
>>>>>>> master
                      APLOGNO(02913) "h2_task: creating conn");
        return NULL;
    }
    
    memcpy(c, master, sizeof(conn_rec));
<<<<<<< HEAD
    c->id = (master->id & (long)pool);
    c->master = master;
    c->input_filters = NULL;
    c->output_filters = NULL;
    c->pool = pool;        
    return c;
}

apr_status_t h2_slave_setup(h2_task *task, apr_bucket_alloc_t *bucket_alloc,
                            apr_thread_t *thread, apr_socket_t *socket)
{
    conn_rec *master = task->mplx->c;
    
    ap_log_perror(APLOG_MARK, APLOG_TRACE3, 0, task->pool,
                  "h2_conn(%ld): created from master", master->id);
    
    /* Ok, we are just about to start processing the connection and
     * the worker is calling us to setup all necessary resources.
     * We can borrow some from the worker itself and some we do as
     * sub-resources from it, so that we get a nice reuse of
     * pools.
     */
    task->c->pool = task->pool;
    task->c->current_thread = thread;
    task->c->bucket_alloc = bucket_alloc;
    
    task->c->conn_config = ap_create_conn_config(task->pool);
    task->c->notes = apr_table_make(task->pool, 5);
    
    /* In order to do this in 2.4.x, we need to add a member to conn_rec */
    task->c->master = master;
    
    ap_set_module_config(task->c->conn_config, &core_module, socket);
    
    /* This works for mpm_worker so far. Other mpm modules have 
     * different needs, unfortunately. The most interesting one 
     * being mpm_event...
     */
    switch (h2_conn_mpm_type()) {
        case H2_MPM_WORKER:
            /* all fine */
            break;
        case H2_MPM_EVENT: 
            fix_event_conn(task->c, master);
            break;
        default:
            /* fingers crossed */
            break;
    }
    
    /* Simulate that we had already a request on this connection. */
    task->c->keepalives = 1;
    
    return APR_SUCCESS;
}

/* This is an internal mpm event.c struct which is disguised
 * as a conn_state_t so that mpm_event can have special connection
 * state information without changing the struct seen on the outside.
 *
 * For our task connections we need to create a new beast of this type
 * and fill it with enough meaningful things that mpm_event reads and
 * starts processing out task request.
 */
typedef struct event_conn_state_t event_conn_state_t;
struct event_conn_state_t {
    /** APR_RING of expiration timeouts */
    APR_RING_ENTRY(event_conn_state_t) timeout_list;
    /** the expiration time of the next keepalive timeout */
    apr_time_t expiration_time;
    /** connection record this struct refers to */
    conn_rec *c;
    /** request record (if any) this struct refers to */
    request_rec *r;
    /** is the current conn_rec suspended?  (disassociated with
     * a particular MPM thread; for suspend_/resume_connection
     * hooks)
     */
    int suspended;
    /** memory pool to allocate from */
    apr_pool_t *p;
    /** bucket allocator */
    apr_bucket_alloc_t *bucket_alloc;
    /** poll file descriptor information */
    apr_pollfd_t pfd;
    /** public parts of the connection state */
    conn_state_t pub;
};
APR_RING_HEAD(timeout_head_t, event_conn_state_t);

static void fix_event_conn(conn_rec *c, conn_rec *master) 
{
    event_conn_state_t *master_cs = ap_get_module_config(master->conn_config, 
                                                         h2_conn_mpm_module());
    event_conn_state_t *cs = apr_pcalloc(c->pool, sizeof(event_conn_state_t));
    cs->bucket_alloc = apr_bucket_alloc_create(c->pool);
    
    ap_set_module_config(c->conn_config, h2_conn_mpm_module(), cs);
    
    cs->c = c;
    cs->r = NULL;
    cs->p = master_cs->p;
    cs->pfd = master_cs->pfd;
    cs->pub = master_cs->pub;
    cs->pub.state = CONN_STATE_READ_REQUEST_LINE;
    
    c->cs = &(cs->pub);
=======
           
    /* Replace these */
    c->master                 = master;
    c->pool                   = pool;   
    c->conn_config            = ap_create_conn_config(pool);
    c->notes                  = apr_table_make(pool, 5);
    c->input_filters          = NULL;
    c->output_filters         = NULL;
    c->bucket_alloc           = apr_bucket_alloc_create(pool);
    c->data_in_input_filters  = 0;
    c->data_in_output_filters = 0;
    c->clogging_input_filters = 1;
    c->log                    = NULL;
    c->log_id                 = NULL;
    /* Simulate that we had already a request on this connection. */
    c->keepalives             = 1;
    /* We cannot install the master connection socket on the slaves, as
     * modules mess with timeouts/blocking of the socket, with
     * unwanted side effects to the master connection processing.
     * Fortunately, since we never use the slave socket, we can just install
     * a single, process-wide dummy and everyone is happy.
     */
    ap_set_module_config(c->conn_config, &core_module, dummy_socket);
    /* TODO: these should be unique to this thread */
    c->sbh                    = master->sbh;
    /* TODO: not all mpm modules have learned about slave connections yet.
     * copy their config from master to slave.
     */
    if (h2_conn_mpm_module()) {
        cfg = ap_get_module_config(master->conn_config, h2_conn_mpm_module());
        ap_set_module_config(c->conn_config, h2_conn_mpm_module(), cfg);
    }

    return c;
}

void h2_slave_destroy(conn_rec *slave, apr_allocator_t **pallocator)
{
    apr_pool_t *parent;
    apr_allocator_t *allocator = apr_pool_allocator_get(slave->pool);
    ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, slave,
                  "h2_slave_conn(%ld): destroy (task=%s)", slave->id,
                  apr_table_get(slave->notes, H2_TASK_ID_NOTE));
    /* Attache the allocator to the parent pool and return it for
     * reuse, otherwise the own is still the slave pool and it will
     * get destroyed with it. */
    parent = apr_pool_parent_get(slave->pool);
    if (pallocator && parent) {
        apr_allocator_owner_set(allocator, parent);
        *pallocator = allocator;
    }
    apr_pool_destroy(slave->pool);
}

apr_status_t h2_slave_run_pre_connection(conn_rec *slave, apr_socket_t *csd)
{
    return ap_run_pre_connection(slave, csd);
>>>>>>> master
}

