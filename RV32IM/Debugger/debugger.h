#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include <stdint.h>
#include <string>

// Function prototypes
void debug(uint32_t pc, uint32_t* instr, SPLP* core, uint32_t* singleStep);
uint32_t find_code_loc(uint32_t pc, bool display);
void display_code(std::string filename, uint32_t line_num);
void display_header(SPLP* core, uint32_t pc, uint32_t instr);

#endif