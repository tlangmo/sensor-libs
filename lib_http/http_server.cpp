// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "wiced.h"
#include "wiced_tls.h"
#include <array>
#include "http_server.h"
#include "http_request.h"
#include "http_response.h"
#include "http_payload.h"
#include "lw_event_trace.h"

namespace motesque {

#define TCP_SERVER_STACK_SIZE 4*4096
//16200

#define TCP_SERVER_SEND_BACKOFF_MS  (2)
/* Keepalive will be sent every 2 seconds */
#define TCP_SERVER_KEEP_ALIVE_INTERVAL      (2)
/* Retry 15 times */
#define TCP_SERVER_KEEP_ALIVE_PROBES        (15)
/* Initiate keepalive check after 60 seconds of silence on the tcp connection */
#define TCP_SERVER_KEEP_ALIVE_TIME          (5)
#define TCP_SILENCE_DELAY                   (30)

HttpServer::HttpServer()
: m_routes(),
  m_tcp_handler_thread(),
  m_tcp_handler_thread_semaphore(),
  m_tcp_handler_thread_should_run(0),
  m_tcp_server(),
  m_tcp_server_cb_params(),
  m_tcp_server_tls(),
  m_tcp_server_tls_cb_params(),
  m_tls_identity(NULL),
  m_tcp_handler_thread_command_queue(),
  m_connected_sockets(),
  m_num_connected_sockets(0)
{
    memset(&m_tcp_handler_thread,0,sizeof(m_tcp_handler_thread));
    memset(&m_tcp_handler_thread_semaphore,0,sizeof(m_tcp_handler_thread_semaphore));
    memset(&m_tcp_server,0,sizeof(m_tcp_server));
    memset(&m_tcp_server_tls,0,sizeof(m_tcp_server_tls));
    memset(&m_tcp_server_cb_params,0,sizeof(m_tcp_server_cb_params));
    memset(&m_tcp_server_tls_cb_params,0,sizeof(m_tcp_server_tls_cb_params));
}

HttpServer::~HttpServer()
{
    //release any resources which are still
    stop();
}

HttpResult HttpServer::read_http_request_from_socket(wiced_tcp_socket_t* socket, HttpRequest* req) {
    // Note: We do not support http pipelining
    // (https://en.wikipedia.org/wiki/HTTP_pipelining#Motivation_and_limitations.
    // Once a complete request (or error) is detected, we switch to sending, the rest
    // of the data (if any, that should not happen) is ignored
    wiced_packet_t* received_packet;
    char*           fragment_data;
    uint16_t        fragment_data_size;
    uint16_t        available_data_size;
    wiced_result_t  rc;
    HttpResult http_res = HttpResult_Undefined;

    do {
        // receive packets
        rc = wiced_tcp_receive( socket, &received_packet, 0);
        if (rc == WICED_SUCCESS) {
            wiced_packet_get_data( received_packet, 0, (uint8_t**)&fragment_data, &fragment_data_size, &available_data_size );
            //WPRINT_APP_INFO(("wiced_packet_get_data, fragment_data_size=%d, available_data_size=%d",fragment_data_size,available_data_size));
            http_res = req->read_from(fragment_data, fragment_data_size);
            wiced_packet_delete( received_packet);

            if (http_res != HttpResult_Incomplete) {
                // we are done reading. Either because of error or completion
                break;
            }
            received_packet = NULL;
        }
    } while( rc == WICED_SUCCESS);
    return http_res;
}

wiced_result_t HttpServer::send_dangling_packet(TcpConnection* tc) {
    // we temporaily disable tls to avoid reencrypting the package again.
    wiced_tls_context_t* tls_context_safe = tc->tcp_socket->tls_context;
    tc->tcp_socket->tls_context = NULL;
    wiced_result_t rc = wiced_tcp_send_packet_no_wait(tc->tcp_socket, tc->tcp_dangling_packet);
    tc->tcp_socket->tls_context = tls_context_safe;
    if (rc == WICED_SUCCESS) {
        // the package was sent
        tc->tcp_dangling_packet = nullptr;
    }
    return rc;

}
HttpResult HttpServer::write_http_response_to_socket(TcpConnection* tc, HttpResponse* response)
{
    const char*     response_data;
    size_t          response_data_size;
    wiced_packet_t* packet = nullptr;
    uint8_t*        packet_data = nullptr;
    uint16_t        packet_size = 0;
    uint16_t        packet_remaining_size = 0;
    HttpResult      response_status = HttpResult_Incomplete;
   // TRACE_EVENT0("Http","write_http_response_to_socket");
    // 1. create new packet
    // 2. fill it
    // 3. send it

    // create packet
    {
        TRACE_EVENT0("HTTP","wiced_packet_create_tcp_no_wait");
        wiced_result_t rc_packet = wiced_packet_create_tcp_no_wait( tc->tcp_socket, 0 /*unused*/, &packet, &packet_data, &packet_size );
        packet_remaining_size = packet_size;
        if (WICED_SUCCESS != rc_packet) {
            // no packet could be allocated
            return HttpResult_Incomplete;
        }
    }
    // fill the packet
    {
        TRACE_EVENT0("HTTP","packet fill");
        while (response->get_read_ptr(&response_data, &response_data_size) == 0
                && response_data_size > 0
                && packet_remaining_size > 0) {
            // it is safe to assume that packet_remaining_size is always less that 65536. However, the resp data size could
            // be much larger, hence the cast is safe
            uint16_t should_send_size = (uint16_t)std::min<size_t>(response_data_size, packet_remaining_size);
            memcpy(packet_data, (uint8_t*)response_data, should_send_size);
            wiced_packet_set_data_end(packet, (uint8_t*)packet_data + should_send_size);
            packet_remaining_size -= should_send_size;
            packet_data += should_send_size;
            response_status = response->commit_read(should_send_size);
        }
    }

    if (packet_remaining_size < packet_size) {
        // packet has data, send off now
        wiced_result_t rc;
        {
            //TRACE_EVENT1("HTTP","wiced_tcp_send_packet_no_wait", "packet_size", packet_size );
            rc = wiced_tcp_send_packet_no_wait(tc->tcp_socket, packet);
        }
        if (rc != WICED_SUCCESS) {
            WPRINT_APP_DEBUG(("wiced_tcp_send_packet failed rc=%d, packet_remaining_size=%d , packet_size==%d\n",rc,
                    packet_remaining_size , packet_size));
            // differentiate between recoverable errors and fatal ones
            if (rc != WICED_TIMEOUT && rc != WICED_WOULD_BLOCK) {
                WPRINT_APP_ERROR(("wiced_tcp_send_packet_no_wait rc=%d", rc));
                wiced_packet_delete(packet);
                return HttpResult_Error;
            }
            else {
                TRACE_EVENT0("Http","tcp would block");
                // mark as dangling to send later
                tc->tcp_dangling_packet = packet;
                response_status = HttpResult_Incomplete;
            }
        }
    }
    else {
        // empty packet, delete
        wiced_packet_delete(packet);
    }
    return response_status;
}

void HttpServer::tcp_handler_thread_main(uint32_t argServerInstance) {
    HttpServer* server = (HttpServer*)argServerInstance;

    while (server->m_tcp_handler_thread_should_run == 1) {
        bool any_active_connections = std::any_of(server->m_connected_sockets.begin(),
                server->m_connected_sockets.end(), [&](const TcpConnection& tc) {
           return tc.active;
        });
        // if no active connection is there, wait until the next tcp event
        size_t timeout = any_active_connections ? 1 : WICED_WAIT_FOREVER;
        server->m_tcp_handler_thread_command_queue.process(timeout);

        std::for_each(server->m_connected_sockets.begin(), server->m_connected_sockets.end(), [&](TcpConnection& tc) {
            // decide wether we should read request or write response
            if (!tc.request && !tc.response) {
                tc.request = std::make_shared<HttpRequest>();
                tc.request->set_remote_ip(tc.ip_address.ip.v4); // this is useful information for some handlers
            }
            if (tc.request) {
                HttpResult http_res = server->read_http_request_from_socket(tc.tcp_socket, tc.request.get());
                if  (http_res == HttpResult_Complete) {
                    HttpHandler handler;
                    tc.response = std::make_shared<HttpResponse>();
                    bool is_tls = tc.tcp_socket->tls_context ? true : false;
                    if (0==find_http_handler(server->m_routes,tc.request->method(), tc.request->path(), is_tls, &handler)) {
                        handler(*tc.request, tc.response.get());
                    }
                    else {
                        // could not find any handler
                        tc.response->set_status_code(404);
                    }
                    tc.request.reset();
                } else if (http_res == HttpResult_Error) {
                    // bad request
                    tc.request.reset();
                    tc.response = std::make_shared<HttpResponse>();
                    tc.response->set_status_code(400);
                }
            }
            // we process each socket response n times to increase packet throughput.
            // Otherwise we are limited by the system ticks, which is never more that 1000/sec
            if (tc.response) {
                if (tc.tcp_dangling_packet) {
                    server->send_dangling_packet(&tc);
                }
                if (!tc.tcp_dangling_packet) {
                    HttpResult http_res = server->write_http_response_to_socket(&tc, tc.response.get());
                    if  (http_res != HttpResult_Incomplete) {
                        tc.response.reset();
                        tc.active = false;
                    }
                }
            }
        });

        // we need to yield here, otherwise we can stall other threads with the same or higher priority
        //wiced_rtos_thread_yield();
    }
    wiced_rtos_set_semaphore( &server->m_tcp_handler_thread_semaphore);
    WPRINT_APP_INFO(("info='http server thread ended'\n"));
    WICED_END_OF_CURRENT_THREAD( );
}

wiced_result_t HttpServer::client_connected_callback(wiced_tcp_socket_t* socket, void* arg ) {
    CallbackParams* cb_params = (CallbackParams*)arg;
    auto f = [cb_params,socket]() {
        if ( cb_params->use_tls ) {
            wiced_tls_context_t* context;
            if ( socket->tls_context == NULL ) {
                context =  new wiced_tls_context_t();
                socket->context_malloced = WICED_TRUE;
            }
            else {
                context = socket->tls_context;
            }
            wiced_result_t rc = wiced_tls_init_context( context,  cb_params->self->m_tls_identity, NULL );
            if (rc != WICED_SUCCESS) {
                WPRINT_APP_ERROR(("wiced_tls_init_context, rc=%d",rc));
            }
            wiced_tcp_enable_tls( socket, context );
        }
        // print ip for now
        wiced_ip_address_t  ipaddr;
        uint16_t            port;
        wiced_tcp_server_accept(cb_params->tcp_server, socket);
        wiced_tcp_enable_keepalive(socket, TCP_SERVER_KEEP_ALIVE_INTERVAL, TCP_SERVER_KEEP_ALIVE_PROBES, TCP_SERVER_KEEP_ALIVE_TIME );
        wiced_tcp_server_peer( socket, &ipaddr, &port );

        TcpConnection tc;
        tc.tcp_socket = socket;
        tc.ip_address = ipaddr;
        tc.port = port;
        tc.tcp_dangling_packet = nullptr;
        tc.active = false;
        cb_params->self->m_connected_sockets.push_back(tc);
        cb_params->self->m_num_connected_sockets++;
        WPRINT_APP_INFO(("Accepted connection from :: "));
        WPRINT_APP_INFO ( ("IP %u.%u.%u.%u : %d, active=%d\r\n", (unsigned char) ( ( GET_IPV4_ADDRESS(ipaddr) >> 24 ) & 0xff ),
                (unsigned char) ( ( GET_IPV4_ADDRESS(ipaddr) >> 16 ) & 0xff ),
                (unsigned char) ( ( GET_IPV4_ADDRESS(ipaddr) >>  8 ) & 0xff ),
                (unsigned char) ( ( GET_IPV4_ADDRESS(ipaddr) >>  0 ) & 0xff ),
                port, cb_params->self->m_connected_sockets.size() ) );
    };
    cb_params->self->m_tcp_handler_thread_command_queue.execute_async(f);

    return WICED_SUCCESS;
}

wiced_result_t HttpServer::client_data_received_callback(wiced_tcp_socket_t* socket, void* arg ) {
    CallbackParams* cb_params = (CallbackParams*)arg;
    WPRINT_APP_INFO(("Client sent data\r\n"));

    auto f = [cb_params,socket]() {
        // mark has_data
        auto it = std::find_if(cb_params->self->m_connected_sockets.begin(),cb_params->self->m_connected_sockets.end(), [&](const TcpConnection& tc) -> bool{
            return tc.tcp_socket == socket;
        });
        if (it != cb_params->self->m_connected_sockets.end()) {
            it->active = true;
        }
    };
    cb_params->self->m_tcp_handler_thread_command_queue.execute_async(f);
    WPRINT_APP_INFO(("m_tcp_handler_thread_command_queue.execute rc = \n"));
    return WICED_SUCCESS;
}

wiced_result_t HttpServer::client_disconnected_callback(wiced_tcp_socket_t* socket, void* arg ) {
    CallbackParams* cb_params = (CallbackParams*)arg;
    WPRINT_APP_INFO(("Client disconnected\r\n"));
    auto f = [cb_params,socket]() {
        wiced_tcp_server_disconnect_socket(cb_params->tcp_server, socket);
        if ( socket->tls_context != NULL && socket->context_malloced == WICED_TRUE ) {
            wiced_tls_deinit_context( socket->tls_context );
            delete socket->tls_context;
            socket->tls_context = nullptr;
        }
        // remove socket from list
        auto it = std::find_if(cb_params->self->m_connected_sockets.begin(),cb_params->self->m_connected_sockets.end(), [&](const TcpConnection& tc) -> bool{
            return tc.tcp_socket == socket;
        });
        if (it != cb_params->self->m_connected_sockets.end()) {
            cb_params->self->m_connected_sockets.erase(it);
            cb_params->self->m_num_connected_sockets--;
        }
    };
    cb_params->self->m_tcp_handler_thread_command_queue.execute_async(f);
    return WICED_SUCCESS;
}

int HttpServer::deinitialize_tls(wiced_tls_identity_t* tls_identity) {
  if (tls_identity) {
    wiced_tls_deinit_identity( tls_identity);
    delete tls_identity;
  }
  return 0;
}

int HttpServer::initialize_tls(wiced_tls_identity_t** tls_identity) {
    /* Setup TLS identity */
    *tls_identity = new wiced_tls_identity_t();
    memset(*tls_identity,0,sizeof(wiced_tls_identity_t));

    platform_dct_security_t* dct_security = NULL;
    wiced_result_t rc;
    /* Lock the DCT to allow us to access the certificate and key */
    WPRINT_APP_INFO(( "Read the certificate Key from DCT\n" ));
    rc = wiced_dct_read_lock( (void**) &dct_security, WICED_FALSE, DCT_SECURITY_SECTION, 0, sizeof( *dct_security ) );
    if ( rc != WICED_SUCCESS ) {
        WPRINT_APP_INFO(( "wiced_dct_read_lock, rc=%d\n", rc ));
        return -1;
    }

    rc = wiced_tls_init_identity( *tls_identity, dct_security->private_key, strlen( dct_security->private_key ), (uint8_t*) dct_security->certificate, strlen( dct_security->certificate ) );
    if ( rc != WICED_SUCCESS ) {
        WPRINT_APP_INFO(( "wiced_tls_init_identity, rc=%d, %s\n", rc , dct_security->private_key));
        return -1;
    }

   // wiced_tcp_server_enable_tls( &m_tcp_server, m_tls_identity );
    /* Finished accessing the certificates */
    rc = wiced_dct_read_unlock( dct_security, WICED_FALSE );
    if ( rc != WICED_SUCCESS ) {
        WPRINT_APP_INFO(( "wiced_dct_read_unlock, rc=%d\n", rc ));
        return -1;
    }
    return 0;
}

wiced_result_t HttpServer::start(uint16_t port, uint16_t port_tls, const HttpHandlerMap& routes, uint8_t priority) {
    if (m_tcp_server.port != 0 ) {
        return WICED_ALREADY_CONNECTED;
    }
    if (!wiced_network_is_up(WICED_STA_INTERFACE)) {
        return WICED_NOTUP;
    }
    m_routes = routes;
    wiced_result_t rc;
    wiced_result_t rc_tls;
    memset(&m_tcp_server,0,sizeof(m_tcp_server));
    memset(&m_tcp_server_tls,0,sizeof(m_tcp_server_tls));

    if (initialize_tls(&m_tls_identity) != 0) {
        return WICED_TLS_ERROR;
    }
    m_tcp_server_cb_params.self = this;
    m_tcp_server_cb_params.tcp_server = &m_tcp_server;
    m_tcp_server_cb_params.use_tls = false;
    rc = wiced_tcp_server_start(&m_tcp_server, WICED_STA_INTERFACE, port, 3,
            client_connected_callback, client_data_received_callback, client_disconnected_callback, &m_tcp_server_cb_params);

    m_tcp_server_tls_cb_params.self = this;
    m_tcp_server_tls_cb_params.tcp_server = &m_tcp_server_tls;
    m_tcp_server_tls_cb_params.use_tls = true;

    wiced_tcp_server_enable_tls(&m_tcp_server_tls, m_tls_identity);
    rc_tls = wiced_tcp_server_start(&m_tcp_server_tls, WICED_STA_INTERFACE, port_tls, 3,
            client_connected_callback, client_data_received_callback, client_disconnected_callback, &m_tcp_server_tls_cb_params);
    if (rc != WICED_SUCCESS || rc_tls != WICED_SUCCESS) {
        WPRINT_APP_INFO(("info='error starting tcp_server', rc=%d, rc_tls=%d\n",(int)rc,(int)rc_tls));
        // NOTE: For LwIP we need to call wiced_tcp_server_stop event after a failed start, as
        // some static resources are not getting cleared. For NetX, we MUST not do that as it causes a crash..
        if (rc == WICED_SUCCESS) {
            wiced_tcp_server_stop(&m_tcp_server);
        }
        memset(&m_tcp_server,0,sizeof(m_tcp_server));

        if (rc_tls == WICED_SUCCESS) {
            wiced_tcp_server_stop(&m_tcp_server_tls);
        }
        memset(&m_tcp_server_tls,0,sizeof(m_tcp_server_tls));
        deinitialize_tls(m_tls_identity);
        m_tls_identity = nullptr;
        return rc;
    }
    // this should be more flexible, and not require its own thread for every http server. How about scheduling it with
    // system worker thread, or using the app thread for this ?

    // Start the tcp server thread
    m_tcp_handler_thread_should_run = 1;
    wiced_rtos_init_semaphore( &m_tcp_handler_thread_semaphore);
    rc = wiced_rtos_create_thread(&m_tcp_handler_thread, priority,
            "Motesque Tcp Server", tcp_handler_thread_main, TCP_SERVER_STACK_SIZE, this);
    if (rc != WICED_SUCCESS)  {
        WPRINT_APP_ERROR(("wiced_rtos_create_thread failed, rc=%d\n",rc));
        wiced_tcp_server_stop(&m_tcp_server);
        memset(&m_tcp_server,0,sizeof(m_tcp_server));
        wiced_rtos_deinit_semaphore(&m_tcp_handler_thread_semaphore);
        return rc;
    }
    WPRINT_APP_INFO(("info='started http server', port=%d, port_tls=%d\n",port, port_tls));
    return rc;
}

wiced_result_t HttpServer::stop() {
    // check whether the server is even started
    if (m_tcp_server.port == 0 ) {
        return WICED_NOT_CONNECTED;
    }
    m_tcp_handler_thread_should_run = 0;
    if ( wiced_rtos_is_current_thread( &m_tcp_handler_thread ) != WICED_SUCCESS ) {
        /* Wakeup HTTP thread */
        wiced_rtos_thread_force_awake( &m_tcp_handler_thread);
    }
    wiced_result_t rc = wiced_rtos_get_semaphore( &m_tcp_handler_thread_semaphore, 7000 );
    if (rc != WICED_SUCCESS) {
        WPRINT_APP_ERROR(("error='m_tcp_handler_thread_semaphore wait failed', rc=%d\n",rc));
    }
    wiced_rtos_delete_thread(&m_tcp_handler_thread);
    memset(&m_tcp_handler_thread,0,sizeof(m_tcp_handler_thread));
    wiced_rtos_deinit_semaphore(&m_tcp_handler_thread_semaphore);
    memset(&m_tcp_handler_thread_semaphore,0,sizeof(m_tcp_handler_thread_semaphore));

    rc = wiced_tcp_server_stop(&m_tcp_server);
    WPRINT_APP_INFO(("info='stopped http server', port=%d\n",m_tcp_server.port));
    memset(&m_tcp_server,0,sizeof(m_tcp_server));
    memset(&m_tcp_server_cb_params,0,sizeof(m_tcp_server_cb_params));

    rc = wiced_tcp_server_stop(&m_tcp_server_tls);
    WPRINT_APP_INFO(("info='stopped https server', port=%d\n",m_tcp_server_tls.port));
    memset(&m_tcp_server_tls,0,sizeof(m_tcp_server_tls));
    memset(&m_tcp_server_tls_cb_params,0,sizeof(m_tcp_server_tls_cb_params));
    deinitialize_tls(m_tls_identity);
    m_tls_identity = nullptr;
    m_connected_sockets.clear();
    m_num_connected_sockets = 0;

    return rc;
}


} // end ns
