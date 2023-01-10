#include <stdint.h>
#include "ip_address.h"

namespace motesque
{
  // write an ip4 address as dec string to provided buffer

  int ip4_to_string(uint32_t ip, char* dst, size_t dst_length) {
     memset(dst,0,dst_length);
     unsigned char octet[4]  = {0,0,0,0};
     for (int i=0; i<4; i++)
     {
         octet[i] = ( ip >> (i*8) ) & 0xFF;
     }
     return snprintf(dst,dst_length,"%d.%d.%d.%d",octet[3],octet[2],octet[1],octet[0]);
  }


int ip6_to_string(uint32_t ip[4], char* dst, size_t dst_length) {
   memset(dst,0,dst_length);
   uint16_t sectet[8]  = {
           (uint16_t)(ip[0] >> 16),
           (uint16_t)ip[0],
           (uint16_t)(ip[1] >> 16),
           (uint16_t)ip[1],
           (uint16_t)(ip[2] >> 16),
           (uint16_t)ip[2],
           (uint16_t)(ip[3] >> 16),
           (uint16_t)ip[3],
   };

   return snprintf(dst,dst_length,"%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",sectet[0],sectet[1],sectet[2],sectet[3],
                                                            sectet[4],sectet[5],sectet[6],sectet[7]);
}
}
