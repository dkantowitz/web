/* web_SockJsServer_Connection.c
 *
 * This file contains the implementation for the generated interface.
 *
 * Don't mess with the begin and end tags, since these will ensure that modified
 * code in interface functions isn't replaced when code is re-generated.
 */

#define corto_web_LIB
#include "web.h"

/* $header() */
#include "mongoose.h"
/* $end */

/* ::corto::web::SockJsServer::Connection::send(string msg) */
cx_void _web_SockJsServer_Connection_send(web_SockJsServer_Connection this, cx_string msg) {
/* $begin(::corto::web::SockJsServer::Connection::send) */
	int escapedLength;
	cx_string sockJsMsg;

	/* Escape & pack message in SockJS header */
	escapedLength = stresc(NULL, 0, msg);
	sockJsMsg = cx_alloc(escapedLength + strlen("a[\"\"]") + 1);
	sprintf(sockJsMsg, "a[\"%*s\"]", escapedLength, " ");
	stresc(sockJsMsg + 3, escapedLength, msg);

	if (this->conn) {
		mg_websocket_printf((struct mg_connection *)this->conn, WEBSOCKET_OPCODE_TEXT, sockJsMsg);
	}

	cx_dealloc(sockJsMsg);
/* $end */
}
