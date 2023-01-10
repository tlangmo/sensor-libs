// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#pragma once
#include <string>

namespace motesque
{

typedef std::pair<std::string, std::string> KeyValuePair;

/** @brief Results for http processing.
 */
enum HttpResult {

    HttpResult_Complete    =   0,  ///< the request is complete and no processing is further required.
    HttpResult_Incomplete  =  -1,  ///< the request is not complete yet and needs to be processed further.
    HttpResult_Error       =  -2,  ///< the request encountered an error during processing. It can not be further processed.
    HttpResult_Timedout    =  -3,  ///< an operation took to long to process and is aborted
    HttpResult_Undefined   =  -4   ///< undefined
};

};
