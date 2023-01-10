Introdution
-----------
lib_http contains a simple Http 1 web server. 
Most of the code except in http_server is meant to be platform independent to be tested on x86. 

We do not use the wiced internal http server or other solutions to have more control over the code. 
Specifically, the wiced sdk server has bugs with partial packets. Its stream interface might report that no bytes were 
sent, altough a previous packet was ok. Further, we do want to process requests more akin to an event loop, 
and not block anything (e.g. because of one slow client)

Several request and response objects can be active at any time, while the http server continously (in own thread) polls for tcp data.
Dedicated payload classes are responsible to write (iteratively) content (json, binary, file, motion stream) of a response.

Communication and Synchronization between the Http system and the rest is performed via command queues, based on std::function


Testing
---------
all tests can be run via the lib_tests project





