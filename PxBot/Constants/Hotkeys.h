#include <windows.h>
//#include <iomanip>
//#include <cstdint>

//uint16_t get_word_16_bit(const std::string& hexStr) {
//    std::string hex = hexStr;
//
//    if (hex.substr(0, 2) == "0x" || hex.substr(0, 2) == "0X") {
//        hex = hex.substr(2);
//    }
//
//    uint16_t word;
//    std::stringstream ss;
//    ss << std::hex << hex;
//    ss >> word;
//
//    return word;
//}

//const WORD HEAL = get_word_16_bit("F1");
const WORD HOTKEY_HEAL = 0x70;