// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "lw_event_trace.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include "json.hpp"

std::string output;

// // global definition for event trace system
// namespace lw_event_trace
// {
//    std::mutex mutex;
//    void BufferFullCallback()
//    {
//       // std::cout << std::this_thread::get_id() << std::endl;
//        //TRACE_EVENT_FLUSH_LOG();
//        lw_event_trace::LwEventTraceLogger::get_instance()->Flush();
//    }

//    void OutputCallback (const char* pData)
//    {
//        //std::lock_guard<std::mutex> lock(mutex);
//        output.append(pData);
//    }
// }

TEST_CASE( "trace single threaded") {
    using namespace lw_event_trace;
    output.clear();
    srand(100);
    LwEventTraceLogger::get_instance().enable(1024*128);
    for (int i=0; i< 100000; i++) {
        int idx = rand() % 3;
        switch (idx) {
            case 0:   {TRACE_EVENT0("CAT0", "name");} break;
            case 1:   {TRACE_EVENT0("CAT1", "name"); }break;
            case 2:   {TRACE_EVENT0("CAT2", "name");} break;
            default: ;
        };
    }
    LwEventTraceLogger::get_instance().disable();
    REQUIRE(LwEventTraceLogger::get_instance().get_events().size() > 0);

    //REQUIRE(output.find(",,") == std::string::npos);
    //REQUIRE(output.size() > 0);
}

// TEST_CASE( "trace multi threaded") {
//     output.clear();
//     srand(100);
//     event_trace::TraceLog::GetInstance()->SetEnabled(true);
//     event_trace::OutputCallback("[");

//     auto f0 = [](int n) {
//         for (int i=0; i < n; i++) {
//             TRACE_EVENT0("Thread0", "name0");
//         }
//     };
//     auto f1 = [](int n) {
//         for (int i=0; i < n; i++) {
//             TRACE_EVENT0("Thread1", "name1");
//         }
//     };
//     auto f2 = [](int n) {
//            for (int i=0; i < n; i++) {
//                TRACE_EVENT0("Thread2", "name2");
//                //TRACE_EVENT1("Thread2", "name2","year", 2);
//            }
//        };
//     std::thread t0(f0,11000);
//     std::thread t1(f1,10000);
//     std::thread t2(f2,12000);
//     t0.join();
//     t1.join();
//     t2.join();
//     event_trace::TraceLog::GetInstance()->SetEnabled(false);
//     // replace trainling ',' with ']'
//     output.replace(output.size()-1,1,"]");
//     if (output.find(",,") != std::string::npos) {
//         std::cout << output.substr(output.find(",,")-100,200) << std::endl;
//     }

// //    std::ofstream myfile;
// //    myfile.open ("./test.trace");
// //    myfile << output;
// //    myfile.close();

//     std::cout << output.find(",,") << std::endl;
//     REQUIRE(output.find(",,") == std::string::npos);
//     REQUIRE(output.size() > 0);
//     REQUIRE(output.find("]]") == std::string::npos);
//     // check that the result is indeed a proper json
//     json::JSON obj =  json::JSON::Load(output );
//     REQUIRE(obj.length() > 0);





// }

