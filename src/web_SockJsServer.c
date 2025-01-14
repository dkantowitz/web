/* $CORTO_GENERATED
 *
 * web_SockJsServer.c
 *
 * Only code written between the begin and end tags will be preserved
 * when the file is regenerated.
 */

#include "web.h"

/* $header() */
#include "parson.h"
#include "mongoose.h"

#define WEB_SOCKJSSERVER_DEFAULT_POLL_TIMEOUT       (50)
#define WEB_SOCKJSSERVER_DEFAULT_HEARTBEAT_TIMEOUT  (25)

static void web_SockJsServer_close(web_SockJsServer this, struct mg_connection *conn) {
    web_SockJsServer_Connection c = web_SockJsServer_Connection(conn->connection_param);
    if (c) {
        if (this->onClose._parent.procedure) {
            corto_call(this->onClose._parent.procedure, NULL, this->onClose._parent.instance, c);
        }
        c->conn = 0;
        corto_delete(c);
        conn->connection_param = NULL;
    }
}

static void web_SockJsServer_open(web_SockJsServer this, struct mg_connection *conn) {
    corto_string id = web_random(17);

    web_SockJsServer_Connection c =
        web_SockJsServer_ConnectionDeclareChild(this->connections, id);

    corto_dealloc(id);
    c->conn = (corto_word)conn;
    if (web_SockJsServer_ConnectionDefine(c, NULL)) {
        goto error;
    }
    conn->connection_param = c;
    if (this->onOpen._parent.procedure) {
        corto_call(this->onOpen._parent.procedure, NULL, this->onOpen._parent.instance, c);
    }
error:;
}

static void web_SockJsServer_uri(web_SockJsServer this, struct mg_connection *conn) {
    if (this->onUri._parent.procedure) {
        web_SockJsServer_UriRequest request = {(corto_word)conn};
        corto_call(
            this->onUri._parent.procedure,
            NULL,
            this->onOpen._parent.instance,
            &request,
            conn->uri);
    }
}

static void web_SockJsServer_message(web_SockJsServer this, struct mg_connection *conn) {
    web_SockJsServer_Connection c = web_SockJsServer_Connection(conn->connection_param);
    if (this->onMessage._parent.procedure) {
        if (conn->content_len) {
            char *msg = corto_alloc(conn->content_len + 1);
            memcpy(msg, conn->content, conn->content_len);
            msg[conn->content_len] = '\0';

            /* Parse & unpack message */
            JSON_Value *root = json_parse_string(msg);
            if (!root) {
                goto error;
            }

            if (json_value_get_type(root) == JSONArray) {
                JSON_Array *messages = json_value_get_array(root);
                corto_uint32 i;
                for (i = 0; i < json_array_get_count(messages); i++) {
                    const char *message = json_array_get_string(messages, i);
                    corto_call(this->onMessage._parent.procedure, NULL, this->onMessage._parent.instance, c, message);
                }
            }

            json_value_free(root);
        }
    }
error:;
}

static int web_SockJsServer_request(struct mg_connection *conn, enum mg_event ev) {
    int result = MG_TRUE;

    web_SockJsServer this = web_SockJsServer(conn->server_param);
    switch (ev) {
    case MG_AUTH:
        break;
    case MG_WS_CONNECT:
        /* Send open message fast since SockJS uses this to determine latency */
        mg_websocket_printf(conn, WEBSOCKET_OPCODE_TEXT, "o");
        web_SockJsServer_open(this, conn);
        break;
    case MG_CLOSE:
        web_SockJsServer_close(this, conn);
        break;
    case MG_REQUEST:
        if (conn->is_websocket) {
            web_SockJsServer_message(this, conn);
        } else {
            if (!strcmp(conn->uri, "/sockjs/info")) {
                mg_send_header(conn, "Access-Control-Allow-Origin", "*");
                mg_printf_data(
                    conn,
                    "{\"websocket\":true,\"origins\":[\"*:*\"],\"cookie_needed\":false,\"entropy\":%u}",
                    10000000000 * rand());
            } else {
                web_SockJsServer_uri(this, conn);
            }
        }
        break;
    case MG_HTTP_ERROR:
        result = MG_FALSE;
        break;
    case MG_POLL:
        result = MG_FALSE;
        break;
    case MG_CONNECT:
    case MG_WS_HANDSHAKE:
    case MG_REPLY:
    case MG_RECV:
        result = MG_FALSE;
        break;
    }

    return result;
}

static void* web_SockJsServer_threadRun(void *data) {
    web_SockJsServer this = web_SockJsServer(data);
    struct mg_server *server = mg_create_server(this, web_SockJsServer_request);
    mg_set_option(server, "listening_port", "8000");
    this->server = (corto_word)server;
    web_SockJsServer_run(this);
    mg_destroy_server(&server);
    return NULL;
}
/* $end */

corto_int16 _web_SockJsServer_construct(web_SockJsServer this) {
/* $begin(::corto::web::SockJsServer::construct) */
    if (this->pollTimemoutMillis == 0) {
        this->pollTimemoutMillis = WEB_SOCKJSSERVER_DEFAULT_POLL_TIMEOUT;
    }
    this->thread = (corto_word)corto_threadNew(web_SockJsServer_threadRun, this);
    this->connections = corto_voidCreateChild(this, "__connections");
    return 0;
/* $end */
}

corto_void _web_SockJsServer_destruct(web_SockJsServer this) {
/* $begin(::corto::web::SockJsServer::destruct) */
    this->exiting = TRUE;
    corto_threadJoin((corto_thread)this->thread, NULL);
/* $end */
}

corto_void _web_SockJsServer_poll(web_SockJsServer this, corto_uint32 msec) {
/* $begin(::corto::web::SockJsServer::poll) */
    struct mg_server *server = (struct mg_server *)this->server;
    if (!msec) {
        msec = this->pollTimemoutMillis;
    }

    mg_poll_server(server, msec);
    this->timeElapsed += msec;

    /* Send heartbeats for all live connections every n seconds */
    if (this->timeElapsed >= (WEB_SOCKJSSERVER_DEFAULT_HEARTBEAT_TIMEOUT * 1000)) {
        corto_objectseq scope = corto_scopeClaim(this);
        
        corto_objectseqForeach(scope, o) {
            if (corto_instanceof(corto_type(web_SockJsServer_Connection_o), o)) {
                web_SockJsServer_Connection c = web_SockJsServer_Connection(o);
                mg_websocket_printf((struct mg_connection *)c->conn, WEBSOCKET_OPCODE_TEXT, "h");
            }
        }
        
        corto_scopeRelease(scope);
        this->timeElapsed = 0;
    }
/* $end */
}

corto_void _web_SockJsServer_run_v(web_SockJsServer this) {
/* $begin(::corto::web::SockJsServer::run) */
    while (TRUE) {
        web_SockJsServer_poll(this, 0);
        if (this->exiting) {
            break;
        }
    }
/* $end */
}
