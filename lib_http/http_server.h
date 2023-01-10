// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#pragma once
#include "wiced.h"
#include <string>
#include <atomic>
#include <map>
#include <vector>
#include "command_queue.h"
#include "http_request.h"
#include "http_response.h"
#include "rtos_queue.h"
#include "event_flag_status.h"

namespace motesque {

/** @brief Our very own HttpServer.
    It parses requests and sends responses. It opens to listening sockets, one for unsecured connections, the other for tls.
    The server has its own thread, and serves the tcp connections iteratively. Up to 3 sockets can be open at any time (per tcp_server socket)
*/
class HttpServer {
public:
    /** Some bookkeeping for each active TCP connection */
    struct TcpConnection {
        wiced_tcp_socket_t* tcp_socket;
        // a dangling packet is all ready (allocated, encrypted etc) but was not able to be sent yet
        wiced_packet_t*     tcp_dangling_packet;
        wiced_ip_address_s  ip_address;
        bool                active;
        uint16_t            port;
        uint64_t            ts_last_would_block;
        std::shared_ptr<HttpResponse> response;
        std::shared_ptr<HttpRequest>  request;
    };
    HttpServer();
    virtual ~HttpServer();
    /** Creates the wiced_tcp_servers and the thread.
     *  @param port The port for the unsecure tcp server. Typically 80
     *  @param port The port for the TLS enabled tcp server. Typically 443
     *  @param routes The routes for the rest api.
     *  @param priority The thread priority. Set this carefully to avoid proper operation of the system and fastest speed
     *  */
    wiced_result_t start(uint16_t port, uint16_t port_tls, const HttpHandlerMap& routes, uint8_t priority);
    /** Stops the thread and destroys all resources. */
    wiced_result_t stop();

private:
    /** Bookkeeping for tcp connection callbacks */
    struct CallbackParams {
         bool        use_tls; ///< indicates that the connection uses TLS
         HttpServer* self;
         wiced_tcp_server_t* tcp_server;
     };

    /** @brief Callback for wiced NetworkWorker thread. Called when a client successfully connected.
     *  @param socket The Socket ptr
     *  @param arg The argument of type CallbackParams
     */
    static wiced_result_t client_connected_callback(wiced_tcp_socket_t* socket, void* arg );

    /** @brief Callback for wiced NetworkWorker thread. Called when a client disconnected.
      *  @param socket The socket ptr
      *  @param arg The argument of type CallbackParams
    */
    static wiced_result_t client_disconnected_callback(wiced_tcp_socket_t* socket, void* arg );

    /** @brief Callback for wiced NetworkWorker thread. Called when a client has sent data.
    *  @param socket The socket ptr
    *  @param arg The argument of type CallbackParams
    */
    static wiced_result_t client_data_received_callback(wiced_tcp_socket_t* socket, void* arg );

    /** @brief The main loop
    *  @param http_server_ptr Uint32 cast of void* pointer
    */
    static void tcp_handler_thread_main(uint32_t http_server_ptr);

    /** @brief TLS housekeeping for server*/
    int initialize_tls(wiced_tls_identity_t** tls_identity);
    /** @brief TLS housekeeping for server*/
    int deinitialize_tls(wiced_tls_identity_t* tls_identity);
    /** @brief Reads data from the socket and passes is on to the request
     *  @param req The request opject to build consecutively
     *  @returns Indicates the status of the process. Complete, Incomplete, Error etc.
     *  */
    HttpResult read_http_request_from_socket(wiced_tcp_socket_t* socket, HttpRequest* req);
    /** @brief Write data to the socket from response
     *  @param resp The setup up response object
     * */
    HttpResult write_http_response_to_socket(TcpConnection* tc, HttpResponse* resp);
    int num_active_connections() const {
        // this function can be called from different threads. Hence we use an atomic counter
        return m_num_connected_sockets;
    }

private:
    wiced_result_t send_dangling_packet(TcpConnection* tc);

    // the routes for the http requests
    HttpHandlerMap      m_routes;
    // the thread which actually reads & writes the tcp sockets
    wiced_thread_t      m_tcp_handler_thread;
    // semaphore to wait for m_tcpHandlerThread termination
    wiced_semaphore_t   m_tcp_handler_thread_semaphore;
    // marks whether thread should run.
    std::atomic<int>    m_tcp_handler_thread_should_run;
    // wiced server instance
    wiced_tcp_server_t  m_tcp_server;
    CallbackParams      m_tcp_server_cb_params;

    wiced_tcp_server_t     m_tcp_server_tls;
    CallbackParams         m_tcp_server_tls_cb_params;
    wiced_tls_identity_t*  m_tls_identity;

    // the command queue to communicate tcp callbacks to our tcpHandlerThreadMain
    CommandQueue<RtosQueue, 10, EventFlagStatus>  m_tcp_handler_thread_command_queue;
    // tls identity used for https.

    std::vector<TcpConnection>      m_connected_sockets;
    std::atomic<int>                m_num_connected_sockets;
};

};
