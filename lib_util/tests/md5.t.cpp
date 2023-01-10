#include "../../unittest/catch.hpp"
#include "md5.h"
#include <cstring>

TEST_CASE( "md5") {
  Md5Context md5_context;
  md5_init(&md5_context);

  md5_update(&md5_context, "00aabbccddee", 12);
  unsigned char result[16];
  memset(result,0,sizeof(result));
  md5_final(result, &md5_context);

  char md5hex[64];  
  snprintf(md5hex, sizeof(md5hex), "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",result[0],result[1],result[2],result[3],result[4],result[5],result[6],result[7],result[8],result[9],result[10],result[11],result[12],result[13],result[14],result[15]);
  //printf("%s",md5hex);
  REQUIRE(strcmp(md5hex, "c49c41898b96df3e925c16a6bbf1ad0") == 0);  

}