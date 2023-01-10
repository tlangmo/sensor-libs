#pragma once
#include <string>

namespace motesque {

const std::string motesque_mpu_version = "21.03.04dev"; 

// inline 
// const std::string& get_motesque_mpu_version_str() {
//     static std::string motesque_mpu_version_str = "";
//     if (motesque_mpu_version_str.size() == 0) {
//         if (motesque_mpu_version_int > 999999) {
//             motesque_mpu_version_str = "INVALID";
//             return motesque_mpu_version_str;
//         }
//         char digits[7] = {'0','0','0','0','0','0',0};
//         uint32_t tmp = motesque_mpu_version_int;
//         for (int p=5; p >= 0; p--) {
//             int d = tmp / std::pow(10,p);
//             tmp -= d* std::pow(10,p);
//             digits[5-p] = 0xFF & (0x30+d);
//         }
//         motesque_mpu_version_str.resize(10);
//         char buffer[10];
//         memset(buffer,0,sizeof(buffer));
//         snprintf(buffer, sizeof(buffer), "%c%c.%c%c.%c%c",digits[0],digits[1],digits[2],digits[3],digits[4],digits[5]);
//         motesque_mpu_version_str.assign(buffer);
//     }
//     return motesque_mpu_version_str;
// }
};
