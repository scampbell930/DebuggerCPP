/*
	File containing methods for debugging software
*/

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include "..\\RV32IM\RV32IM.h"
#include "debugger.h"

using namespace std;

// Vector structures for storing breakpoint data
static vector<uint32_t> num;
static vector<uint32_t> addr;
static vector<uint32_t> instruction;
static uint32_t breakCount = 0;

void debug(uint32_t pc, uint32_t* instr, SPLP* core, uint32_t* singleStep) {

	// First check if pc is a valid C code address
	if (find_code_loc(pc, false) != -1) {
		*singleStep = 0;

		// Display all needed information
		display_header(core, pc, *instr);

		printf("\nDebugger:\n");

		string input = "";

		// Process user input
		do {
			cout << "Input an option: \nss\nsbp <address>\ncbp <1, 2, 3, ..., All>\ndisplay\nrun\n";
			getline(cin, input);

			// Parse input
			string choice, option;
			istringstream iss(input);
			iss >> choice >> option;

			if (input.compare("ss") == 0) {
				*singleStep = 1;
			}
			// Add a breakpoint at specified address
			else if (choice.compare("sbp") == 0) {
				uint32_t break_addr = stoul(option, nullptr, 16);
				num.insert(num.end(), ++breakCount);
				addr.insert(addr.end(), break_addr);
				instruction.insert(instruction.end(), (*core).mainMemory[break_addr / 4]);

				// Add ebreak to main memory
				(*core).mainMemory[break_addr / 4] = 0x00100073;
			}

			// Clear one or all break points
			else if (choice.compare("cbp") == 0) {
				if (option.compare("All") == 0) {
					// Clear all data from breakpoint table
					size_t total = num.size();
					for (size_t i = 0; i < total; i++) {

						// Check if breakpoint is current address
						if (pc == addr.back()) {
							*instr = instruction.back();
						}

						// Set memory back to original
						(*core).mainMemory[addr.back() / 4] = instruction.back();

						// Delete breakpoint
						num.pop_back();
						addr.pop_back();
						instruction.pop_back();

					}
				}
				else {
					uint32_t index = stoul(option, nullptr, 10) - 1;

					// Set memory back to original
					(*core).mainMemory[addr.at(index) / 4] = instruction.at(index);

					// If removed break is current address
					if (pc == addr.at(index)) {
						*instr = instruction.at(index);
					}

					// Delete breakpoint
					num.erase(num.begin() + index);
					addr.erase(addr.begin() + index);
					instruction.erase(instruction.begin() + index);
				}
			}
			else if (choice.compare("display") == 0) {
				if (addr.size() != 0) {
					cout << "\nCurrent breakpoints:\n";

					// Display breakpoints
					for (size_t i = 0; i < addr.size(); i++) {
						printf("Num: %d\tAddr: 0x%.8x\tInstr: 0x%.8x\n", num.at(i), addr.at(i), instruction.at(i));
					}
				}
				else {
					cout << "\nNo breakpoints set\n\n";
				}
			}

			cout << "\n\n";

		} while (input.compare("run") != 0 && input.compare("ss") != 0);

		// Pass back correct instruction if current is an ebreak
		if (*instr == 0x00100073) {

			// Check if last ebreak of program
			if (pc != 52) {
				// Check if address is stored in breakpoint table
				for (size_t i = 0; i < addr.size(); i++) {
					if (pc == addr.at(i)) {
						*instr = instruction.at(distance(addr.begin(), find(addr.begin(), addr.end(), pc)));
					}
				}
			}
		}

	}

}

// Searches debug dump file for location of running code
uint32_t find_code_loc(uint32_t pc, bool display) {
	fstream inFile;
	inFile.open("..\\Debugger\\HW05_dump.txt", ios::in);

	if (inFile.is_open()) {

		string file, lineNum, addr, line;
		string nextLine, nextFile, nextLineNum, nextAddr;

		while (getline(inFile, line)) {

			// Parse line data
			istringstream iss(line);
			iss >> file >> lineNum >> addr;

			// Check if parsed value contains data I am looking for
			if (iss) {
				uint32_t file_addr = stoul(addr, nullptr, 16);
				if (file_addr == pc) {

					// Only display line if display bool is true
					if (display) {
						display_code(file, stoi(lineNum));
					}

					// Read next line of file to determine all instructions in line of code
					getline(inFile, nextLine); // Need to account for blank line
					getline(inFile, nextLine);
					istringstream iss(nextLine);
					iss >> nextFile >> nextLineNum >> nextAddr;

					// Find difference between start and end address
					uint32_t total = stoul(nextAddr, nullptr, 16) - stoul(addr, nullptr, 16);

					inFile.close();

					return total;
				}
			}
		}

	}
	else {
		cout << "Could not open file HW05_dump.txt\n";
	}

	inFile.close();

	return -1;

}

// Display line of code based on filename and line number
void display_code(string filename, uint32_t line_num) {

	// Open correct file
	string full_filename = "..\\..\\Hw05\\" + filename;
	fstream displayFile;
	displayFile.open(full_filename, ios::in);

	if (displayFile.is_open()) {
		string curr_line;

		// Find line number containing correct code
		for (size_t i = 0; i < line_num; i++) {
			getline(displayFile, curr_line);
		}

		cout << "\nCurrently executing line of code from file: " << filename << "\n" << curr_line << "\n";
	}
	else {
		cout << "Could not open file " << full_filename << "\n";
	}
	displayFile.close();
}

// Display all needed data for the debugger
void display_header(SPLP* core, uint32_t pc, uint32_t instr) {
	cout << "\n\n";

	// Array of strings for ABI register formatting
	char* abi[] = { "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0",
		"s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
		"s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
		"t5", "t6" };

	// Display pc and registers
	printf("Current PC address: 0x%.8x\n\n", pc);

	cout << "Current Register States:\n";
	for (size_t i = 0; i < sizeof((*core).X) / sizeof((*core).X[i]); i += 4) {
		printf("\t%4s: 0x%.8x\t%4s: 0x%.8x\t%4s: 0x%.8x\t%4s: 0x%.8x\t\n", abi[i], (*core).X[i], abi[i + 1], (*core).X[i + 1],
			abi[i + 2], (*core).X[i + 2], abi[i + 3], (*core).X[i + 3]);
	}

	// Find and display C code
	uint32_t total_assembly = find_code_loc(pc, true);

	// Display assembly code
	cout << "\nCurrently Executing Assembly Instruction(s):\n";

	for (size_t j = 0; j < total_assembly; j += 4) {
		uint32_t curr_instr = (*core).mainMemory[(pc + j) / 4];

		// Check if last ebreak of program
		if (pc != 52) {
			// Find real instruction if ebreak
			if (curr_instr == 0x100073) {
				curr_instr = instruction.at(distance(addr.begin(), find(addr.begin(), addr.end(), pc + j)));
			}
		}
		core->Dissembler(pc + j, curr_instr);
	}

}
