#include <stdint.h>
#include <cstring>
#include <cstdio>
namespace motesque
{
  // write an ip4 address as dec string to provided buffer
  int ip4_to_string(uint32_t ip, char* dst, size_t dst_length);
  int ip6_to_string(uint32_t ip[4], char* dst, size_t dst_length);
}
