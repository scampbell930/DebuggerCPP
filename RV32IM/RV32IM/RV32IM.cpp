
/*---------------------------------------------------------------------------*\
 SPLP		 : Software PipeLined Processor
 Composed By : Sim, Mong Tee
 Dated		 : 26th September 2018
 Copyright   : (C) 2018--2022, Mong
 ==============================================================================
 History
 =======

 Author Date       Description
 ------ ---------- ------------------------------------------------------------
  Mong  09/26/2018 Initial 
  Mong  10/10/2018 Add all logical and boolean operation 
  Mong  01/09/2022 Change pipelined CPU to single cycle CPU (RV32IM)

\*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include <Windows.h>
#include <conio.h>
#include "..\\Debugger\debugger.h"

//-----------------------------------------------------------------------------
// SPLP
//-----------------------------------------------------------------------------
SPLP::SPLP(void)
{
	//--- CPU Methods
	//
	memset(&PC_Val,0,sizeof(PROGRAM_COUNTER)); 
	memset(&X,0,sizeof(X));

	//--- CPU Stages
	//
	memset(&IF_StageVal ,0,sizeof(IF_STAGE_REGISTER)); 
	memset(&ID_StageVal ,0,sizeof(ID_STAGE_REGISTER)); 
	memset(&EX_StageVal ,0,sizeof(EX_STAGE_REGISTER)); 
	memset(&MEM_StageVal,0,sizeof(MEM_STAGE_REGISTER)); 
	memset(&WB_StageVal ,0,sizeof(WB_STAGE_REGISTER)); 

	//---System Flag
	//
	pgmBREAK		= NORMAL;
	Dissembler_flag = 0;
}

//-----------------------------------------------------------------------------
// ~SPLP
//-----------------------------------------------------------------------------
SPLP::~SPLP(void)
{
}

//-----------------------------------------------------------------------------
// ~SPLP
//-----------------------------------------------------------------------------
void SPLP::ClearFlag(void)
{
}

//-----------------------------------------------------------------------------
// Core
//-----------------------------------------------------------------------------
uint32_t SPLP::Core(uint32_t rst)
{
	uint32_t haltFlag = 0;
	uint32_t startFlag = 1;

	while(1)
	{
		PF_Cnt.instrCnt++;
		PF_Cnt.rdCycle++;			// number of cycle cycle executed by the processor
		PF_Cnt.rdTime++;			// wall-clock 
		PF_Cnt.rdInstret++;			// number of executed instruction
 
		if (ID_StageVal.brnInstr)
		{
			switch(ID_StageVal.brnInstr)
			{
				case 1: PF_Cnt.jalCnt++; break;
				case 2: PF_Cnt.jalrCnt++; break;
				case 3:
						switch(EX_StageVal.branch)
						{
							case 0: PF_Cnt.brnNotTaken++; break;
							case 1: PF_Cnt.brnTaken++; break;
						}
					break;
			}
		}

		ProgramCounter	(rst);		
		IF_Stage		();

		// Debugger call if startFlag, keyboard hit, or ebreak instruction
		if (IF_StageVal.instr == 0x00100073 || _kbhit() || startFlag) {
			debug(IF_StageVal.pc, &IF_StageVal.instr, this, &startFlag);
		}

		ID_Stage		();
		EX_Stage		();
		MEM_Stage		();
		WB_Stage		();

		if (rst == RESET) rst = 0;

		/*
		if (pgmBREAK==HALT)
		{
			if (haltFlag==1)
			{
				haltFlag = 1;
				PF_Cnt.instrCnt			= 0;
				PF_Cnt.brnNotTaken		= 0;
				PF_Cnt.brnTaken			= 0; 
				PF_Cnt.jalCnt			= 0;
				PF_Cnt.jalrCnt			= 0;
				PF_Cnt.dataHazardCnt	= 0;
			}
			else
			{
				PrintDateTime();
				printf("\n\n");
				printf("   Program Statistics\n");
				printf("   ------------------\n");
				printf("   Number of Clock Cycle(s)       = %llu\n", PF_Cnt.instrCnt);
				printf("   Number of Branch-Not-Taken(s)  = %llu\n", PF_Cnt.brnNotTaken);
				printf("   Number of Branch-Taken(s)      = %llu\n", PF_Cnt.brnTaken);
				printf("   Number of JAL(s)               = %llu\n", PF_Cnt.jalCnt);
				printf("   Number of JALR(s)              = %llu\n", PF_Cnt.jalrCnt);
			}

			pgmBREAK = NORMAL;
		}
		*/
	}

	return 0;
}

//-----------------------------------------------------------------------------
// ProgramCounter
//-----------------------------------------------------------------------------
void SPLP::ProgramCounter(uint32_t rst)
{ 
	if (rst==RESET)
	{
		PC_Val.pc		= 0;
		PC_Val.pcPlus4	= 4;
		return;
	}

	if (EX_StageVal.branch)
	{
		PC_Val.pc	= EX_StageVal.aluOut;
	}
	else
	{
		PC_Val.pc	= PC_Val.pcPlus4;
	}

	PC_Val.pcPlus4	= PC_Val.pc + 4;
}

//-----------------------------------------------------------------------------
// IF_Stage
//-----------------------------------------------------------------------------
void SPLP::IF_Stage(void)
{
	memset(&IF_StageVal, 0, sizeof(IF_STAGE_REGISTER));

	IF_StageVal.instr	= mainMemory[PC_Val.pc >> 2];
	IF_StageVal.pc		= PC_Val.pc;
	IF_StageVal.pcPlus4 = PC_Val.pcPlus4;

	if (Dissembler_flag==1)
		Dissembler(PC_Val.pc, IF_StageVal.instr);
}

//-----------------------------------------------------------------------------
// ID_Stage
//-----------------------------------------------------------------------------
void SPLP::ID_Stage(void)
{
	memset(&ID_StageVal, 0, sizeof(ID_STAGE_REGISTER));

	ID_StageVal.instr	= IF_StageVal.instr;
	ID_StageVal.pc		= IF_StageVal.pc;
	ID_StageVal.pcPlus4 = IF_StageVal.pcPlus4;

	R_TYPE* r_type = (R_TYPE*)&ID_StageVal.instr;
	J_TYPE* j_type = (J_TYPE*)&ID_StageVal.instr;

	Control();
	ImmediateExt();
	SystemRegister();
}

//-----------------------------------------------------------------------------
// Control
//-----------------------------------------------------------------------------
void SPLP::Control(void)
{
	R_TYPE* r_type = (R_TYPE*)&ID_StageVal.instr;
	U_TYPE* u_type = (U_TYPE*)&ID_StageVal.instr;
	uint32_t opcode = r_type->opcode;
	TC_TYPE* tc_type = (TC_TYPE*)&ID_StageVal.instr;

	uint32_t null			= 0;
	uint32_t NotSupported	= 0;

	switch (opcode)
	{
	case RV32I_NOP:
		break;

	case RV32I_LUI:
		ID_StageVal.CEX = ALU_AUI; ID_StageVal.CWB = WB_ALUOUT;
		ID_StageVal.formatType = UFORMAT;
		break;

	case RV32I_AUIPC:
		ID_StageVal.CEX = ALU_AUIPC; ID_StageVal.CWB = WB_ALUOUT;
		ID_StageVal.formatType = UFORMAT;
		break;

	case RV32I_JAL:
		ID_StageVal.CEX = ALU_JAL; ID_StageVal.CWB = WB_PCPLUS4;
		ID_StageVal.formatType = JFORMAT; ID_StageVal.signExt = ENABLE;
		ID_StageVal.brnInstr = 1;
		break;

	case RV32I_JALR:
		ID_StageVal.CEX = ALU_JALR; ID_StageVal.CWB = WB_PCPLUS4;
		ID_StageVal.formatType = IFORMAT; ID_StageVal.signExt = ENABLE;
		ID_StageVal.brnInstr = 2;
		break;

	case RV32I_BRANCH:
		ID_StageVal.formatType = BFORMAT; ID_StageVal.signExt = ENABLE; ID_StageVal.brnInstr = 3;
		switch (r_type->funct3)
		{
		case RV32I_BEQ:		ID_StageVal.CEX = ALU_BEQ;	break;
		case RV32I_BNE:		ID_StageVal.CEX = ALU_BNE;	break;
		case RV32I_BLT:		ID_StageVal.CEX = ALU_BLT;	break;
		case RV32I_BGE:		ID_StageVal.CEX = ALU_BGE;	break;
		case RV32I_BLTU:	ID_StageVal.CEX = ALU_BLTU;	break;
		case RV32I_BGEU:    ID_StageVal.CEX = ALU_BGEU; break;
		default: NotSupported = 1; break;
		}
		break;

	case RV32I_LOAD:
		ID_StageVal.CWB = WB_MDATA; ID_StageVal.formatType = IFORMAT; ID_StageVal.signExt = ENABLE; ID_StageVal.rdFlag = 1;

		switch (r_type->funct3)
		{
		case RV32I_LB:	ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_LB;	break;
		case RV32I_LH:	ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_LH;	break;
		case RV32I_LW:  ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_LW;	break;
		case RV32I_LBU:	ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_LBU; break;
		case RV32I_LHU:	ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_LHU; break;
		default: NotSupported = 2; break;
		}
		break;

	case RV32I_STORE:
		ID_StageVal.formatType = SFORMAT; ID_StageVal.signExt = ENABLE;
		switch (r_type->funct3)
		{
		case RV32I_SB: ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_SB;  break;
		case RV32I_SH: ID_StageVal.CEX = ALU_ADDI; ID_StageVal.CMEM = MEM_SH;  break;
		case RV32I_SW: ID_StageVal.CEX = ALU_SW;   ID_StageVal.CMEM = MEM_SW;  break;
		default: NotSupported = 3;	break;
		}
		break;

	case RV32I_MATHI:
		ID_StageVal.CWB = WB_ALUOUT; ID_StageVal.signExt = ENABLE; ID_StageVal.formatType = IFORMAT;
		switch (r_type->funct3)
		{
		case RV32I_ADDI:	ID_StageVal.CEX = ALU_ADDI; break;
		case RV32I_SLTI:	ID_StageVal.CEX = ALU_SLTI; break;
		case RV32I_SLTIU:   ID_StageVal.CEX = ALU_SLTI; ID_StageVal.signExt = DISABLE; break;

		case RV32I_XORI:	ID_StageVal.CEX = ALU_XORI; break;
		case RV32I_ORI:		ID_StageVal.CEX = ALU_ORI;	break;
		case RV32I_ANDI:	ID_StageVal.CEX = ALU_ANDI; break;
		case RV32I_SLLI:	ID_StageVal.CEX = ALU_SLLI; ID_StageVal.formatType = RFORMAT; break; //error???

		case RV32I_SHIFT:
			switch (r_type->funct7)
			{
			case RV32I_SRLI: ID_StageVal.CEX = ALU_SRLI; ID_StageVal.formatType = RFORMAT;	break;
			case RV32I_SRAI: ID_StageVal.CEX = ALU_SRAI; ID_StageVal.formatType = RFORMAT;	break;
			default:		 NotSupported = 4;												break;
			}
			break;
		default: NotSupported = 5; break;
		}
		break;

	case RV32I_MATH:

		if (r_type->funct7 == 0)
		{
			ID_StageVal.CWB = WB_ALUOUT; ID_StageVal.formatType = RFORMAT;
			switch (r_type->funct3)
			{
			case ADD_SUB:		ID_StageVal.CEX = ALU_ADD;	break;
			case RV32I_SLL:		ID_StageVal.CEX = ALU_SLL;	break;
			case RV32I_SLT:		ID_StageVal.CEX = ALU_SLT;	break;
			case RV32I_SLTU:	ID_StageVal.CEX = ALU_SLTU;	break;

			case RV32I_XOR:		ID_StageVal.CEX = ALU_XOR;	break;
			case RV32I_SRL_SRA:	ID_StageVal.CEX = ALU_SRL;	break;
			case RV32I_OR:		ID_StageVal.CEX = ALU_OR;	break;
			case RV32I_AND:		ID_StageVal.CEX = ALU_AND;	break;
			default: NotSupported = 6;	break;
			}
		}
		else if (r_type->funct7 == RV32I_SRA)
		{
			ID_StageVal.CWB = WB_ALUOUT; ID_StageVal.formatType = RFORMAT;
			switch (r_type->funct3)
			{
			case RV32I_SRL_SRA: ID_StageVal.CEX = ALU_SRA;	break;
			case ADD_SUB:		ID_StageVal.CEX = ALU_SUB;	break;
			default: NotSupported = 7;	break;
			}
		}
		else if (r_type->funct7 == 0x01)
		{
			ID_StageVal.CWB = WB_ALUOUT; ID_StageVal.formatType = RFORMAT;
			switch (r_type->funct3)
			{
			case RV32M_MUL:		ID_StageVal.CEX = ALU_MUL;	break;
			case RV32M_MULH:	ID_StageVal.CEX = ALU_MULH; break;
			case RV32M_MULHSU:	ID_StageVal.CEX = ALU_MULHSU;	break;
			case RV32M_MULHU:	ID_StageVal.CEX = ALU_MULHU;	break;
			case RV32M_DIV:		ID_StageVal.CEX = ALU_DIV;		break;
			case RV32M_DIVU: 	ID_StageVal.CEX = ALU_DIVU;		break;
			case RV32M_REM: 	ID_StageVal.CEX = ALU_REM;		break;
			case RV32M_REMU: 	ID_StageVal.CEX = ALU_REMU;		break;
			default: NotSupported = 8;	break;
			}
		}
		break;

	case RV32I_FINSTR:
		switch (r_type->funct3)
		{
		case RV32I_FENCE: break;
		case RV32I_FENCE_I: break;
		default: NotSupported = 9;	break;
		}
		break;

	case RV32I_SPECIAL:
		switch (r_type->funct3)
		{
		case RV32I_ECOMMAND:
			switch (r_type->rs2)
			{
			case RV32I_ECALL:	break;
			case RV32I_EBREAK:	ID_StageVal.ebreak = 1; break;
			default: NotSupported = 10;	break;
			}
			break;

		case RV32I_CSRRW: ID_StageVal.CEX = null; ID_StageVal.CMEM = null; ID_StageVal.CWB = null; break;

		case RV32I_CSRRS:
			ID_StageVal.CMEM = null; ID_StageVal.CWB = WB_ALUOUT;
			switch (tc_type->csr)
			{
			case RV32I_CCL:		ID_StageVal.CEX	= ALU_CCL; break;
			case RV32I_CCH:		ID_StageVal.CEX = ALU_CCH; break;
			case RV32I_TML:		ID_StageVal.CEX = ALU_TML; break;
			case RV32I_TMH:		ID_StageVal.CEX = ALU_TMH; break;
			case RV32I_INSTL:	ID_StageVal.CEX = ALU_INSTL; break;
			case RV32I_INSTH:	ID_StageVal.CEX = ALU_INSTH; break;
			default: NotSupported = 11;	break;
			}
			break;

		case RV32I_CSRRC:  ID_StageVal.CEX = null; ID_StageVal.CMEM = null; ID_StageVal.CWB = null; break;
		case RV32I_CSRRWI: ID_StageVal.CEX = null; ID_StageVal.CMEM = null; ID_StageVal.CWB = null; break;
		case RV32I_CSRRSI: ID_StageVal.CEX = null; ID_StageVal.CMEM = null; ID_StageVal.CWB = null; break;
		case RV32I_CSRRCI: ID_StageVal.CEX = null; ID_StageVal.CMEM = null; ID_StageVal.CWB = null; break;
		default: NotSupported = 12;	break;
		}

		break;

	default: NotSupported = 13;	break;
	}

	if (NotSupported == 0) return;

	memset(&ID_StageVal, 0, sizeof(ID_STAGE_REGISTER));
}

//-----------------------------------------------------------------------------
// ImmediateExt
//-----------------------------------------------------------------------------
void SPLP::ImmediateExt(void)
{
	R_TYPE* r_Type = (R_TYPE*)&ID_StageVal.instr;
	B_TYPE* b_Type = (B_TYPE*)&ID_StageVal.instr;
	I_TYPE* i_Type = (I_TYPE*)&ID_StageVal.instr;
	J_TYPE* j_Type = (J_TYPE*)&ID_StageVal.instr;
	S_TYPE* s_Type = (S_TYPE*)&ID_StageVal.instr;
	U_TYPE* u_Type = (U_TYPE*)&ID_StageVal.instr;

	uint32_t mask = 0;
	uint32_t val1 = 0;
	uint32_t val2 = 0;
	uint32_t val3 = 0;
	uint32_t val4 = 0;

	ID_StageVal.rs1 = r_Type->rs1;
	ID_StageVal.rs2 = r_Type->rs2;
	ID_StageVal.rd	= r_Type->rd;

	switch (ID_StageVal.formatType)
	{
	case BFORMAT:
		val1 = (b_Type->imm4_1 << 1);
		val2 = (b_Type->imm10_5 << 5);
		val3 = (b_Type->imm11 << 11);
		val4 = (b_Type->imm12 << 12);
		mask = (b_Type->imm12 & ID_StageVal.signExt) ? 0xFFFFE000 : 0x00000000;
		ID_StageVal.extData = (mask | val4 | val3 | val2 | val1);
		ID_StageVal.rd = 0;
		break;

	case IFORMAT:
		val1 = i_Type->imm10_0;
		val2 = i_Type->imm11 << 11;
		mask = (i_Type->imm11 & ID_StageVal.signExt) ? 0xFFFFF000 : 0x00000000;
		ID_StageVal.extData = mask | val2 | val1;
		ID_StageVal.rs2 = 0;
		break;

	case JFORMAT:
		val1 = (j_Type->imm10_1 << 1);
		val2 = (j_Type->imm11 << 11);
		val3 = (j_Type->imm19_12 << 12);
		val4 = (j_Type->imm20 << 20);
		mask = (j_Type->imm20 & ID_StageVal.signExt) ? 0xFFE00000 : 0x00000000;
		ID_StageVal.extData = (mask | val4 | val3 | val2 | val1);
		ID_StageVal.rs1 = 0;
		ID_StageVal.rs2 = 0;
		break;

	case SFORMAT:
		val1 = s_Type->imm4_0;
		val2 = s_Type->imm10_5 << 5;
		val3 = s_Type->imm11 << 11;
		mask = (s_Type->imm11 & ID_StageVal.signExt) ? 0xFFFFF000 : 0x00000000;
		ID_StageVal.extData = (mask | val3 | val2 | val1);
		ID_StageVal.rd = 0;
		break;

	case UFORMAT:
		val1 = u_Type->imm30_12;
		val2 = u_Type->imm31 << 19;
		mask = (u_Type->imm31 & ID_StageVal.signExt) ? 0xFFF00000 : 0x00000000;
		ID_StageVal.extData = (mask | val2 | val1);
		ID_StageVal.rs1 = 0;
		ID_StageVal.rs2 = 0;
		break;

	default:
		ID_StageVal.extData = 0;
		break;
	}
}

//-----------------------------------------------------------------------------
// SystemRegister
//-----------------------------------------------------------------------------
uint32_t SPLP::SystemRegister(void)
{
	X[0] = 0;
	ID_StageVal.rs1Data = X[ID_StageVal.rs1];
	ID_StageVal.rs2Data = X[ID_StageVal.rs2];

	return 0;
}

//-----------------------------------------------------------------------------
// EX_Stage
//-----------------------------------------------------------------------------
void SPLP::EX_Stage(void)
{
	uint32_t inpA = 0;
	uint32_t inpB = 0;
	uint32_t inpC = 0;
	int32_t  sinpA = 0;
	int32_t  sinpB = 0;
	int32_t  sextIMM = 0;
	uint32_t extIMM = 0;
	uint32_t shamt = 0;
	uint32_t mask = 0;
	int64_t  s64A = 0;
	int64_t  s64B = 0;
	uint64_t u64A = 0;
	uint64_t u64B = 0;

	memset(&EX_StageVal, 0, sizeof(EX_STAGE_REGISTER));

	EX_StageVal.instr	= ID_StageVal.instr;
	EX_StageVal.pc		= ID_StageVal.pc;
	EX_StageVal.pcPlus4 = ID_StageVal.pcPlus4;
	EX_StageVal.CEX		= ID_StageVal.CEX;
	EX_StageVal.CMEM	= ID_StageVal.CMEM;
	EX_StageVal.CWB		= ID_StageVal.CWB;
	EX_StageVal.rs2Data = ID_StageVal.rs2Data;
	EX_StageVal.mData	= ID_StageVal.rs2Data;
	EX_StageVal.rd		= ID_StageVal.rd;

	EX_StageVal.ebreak  = ID_StageVal.ebreak;

	extIMM				= ID_StageVal.extData;
	sextIMM				= (int32_t)extIMM;
	shamt				= ID_StageVal.rs2;

	//---RS1
	//
	if (ID_StageVal.rs1 == 0)
	{
		inpA = 0;
	}
	else
	{
		inpA = ID_StageVal.rs1Data;
	}

	//---RS2
	//
	if (ID_StageVal.rs2 == 0)
	{
		inpB = 0;
	}
	else
	{
		inpB = ID_StageVal.rs2Data;
	}

	inpC = ID_StageVal.pc;
	sinpA = (int32_t)inpA;
	sinpB = (int32_t)inpB;

	EX_StageVal.aluOut = 0;
	EX_StageVal.branch = 0;
	//EX_StageVal.rs2Data = inpB;

	switch (EX_StageVal.CEX)
	{
	case ALU_AUI:		EX_StageVal.aluOut = extIMM << 12;  break;
	case ALU_AUIPC:		EX_StageVal.aluOut = inpC + (extIMM << 12);  break;

	case ALU_JAL:
		EX_StageVal.aluOut = inpC + sextIMM;
		EX_StageVal.branch = MEM_JMP_BR;
		break;
	case ALU_JALR:
		EX_StageVal.aluOut = sinpA + sextIMM;
		EX_StageVal.branch = MEM_JMP_BR;
		break;
	case ALU_BEQ:
		EX_StageVal.branch = inpA == inpB;
		EX_StageVal.aluOut = inpC + sextIMM;
		break;
	case ALU_BNE:
		EX_StageVal.branch = inpA != inpB;
		EX_StageVal.aluOut = inpC + sextIMM;
		break;
	case ALU_BLT:
		EX_StageVal.branch = sinpA < sinpB;
		EX_StageVal.aluOut = inpC + sextIMM;
		break;
	case ALU_BGE:
		EX_StageVal.branch = sinpA >= sinpB;
		EX_StageVal.aluOut = inpC + sextIMM;
		break;
	case ALU_BLTU:
		EX_StageVal.branch = inpA < inpB;
		EX_StageVal.aluOut = inpC + sextIMM;
		break;
	case ALU_BGEU:
		EX_StageVal.branch = inpA >= inpB;
		EX_StageVal.aluOut = inpC + sextIMM;
		break;

	case ALU_SW:		EX_StageVal.aluOut = (uint32_t)(sinpA + sextIMM);	break;
	case ALU_ADDI:		EX_StageVal.aluOut = (uint32_t)(sinpA + sextIMM);	break;
	case ALU_SLTI:		EX_StageVal.aluOut = sinpA < sextIMM;				break;
	case ALU_SLTIU:		EX_StageVal.aluOut = inpA < extIMM;					break;
	case ALU_XORI:		EX_StageVal.aluOut = inpA ^ extIMM;					break;
	case ALU_ORI:		EX_StageVal.aluOut = inpA | extIMM;					break;
	case ALU_ANDI:		EX_StageVal.aluOut = inpA & extIMM;					break;

		// shamt
	case ALU_SLLI:		EX_StageVal.aluOut = inpA << shamt;					break;
	case ALU_SRLI:		EX_StageVal.aluOut = inpA >> shamt;					break;

	case ALU_SRAI:
		u64A = (inpA & 0x80000000) ? 0xFFFFFFFF00000000 | inpA : inpA;
		EX_StageVal.aluOut = (uint32_t)(u64A >> shamt);

		break;

	case ALU_ADD:		EX_StageVal.aluOut = inpA + inpB; break;
	case ALU_SUB:		EX_StageVal.aluOut = inpA - inpB; break;
	case ALU_SLL:		EX_StageVal.aluOut = inpA << inpB; break;
	case ALU_SLT:		EX_StageVal.aluOut = sinpA < sinpB; break;
	case ALU_SLTU:		EX_StageVal.aluOut = inpA < inpB; break;
	case ALU_XOR:		EX_StageVal.aluOut = inpA ^ inpB; break;
	case ALU_SRL:		EX_StageVal.aluOut = inpA >> inpB; break;

	case ALU_SRA:
		u64A = (inpA & 0x80000000) ? 0xFFFFFFFF00000000 | inpA : inpA;
		EX_StageVal.aluOut = (u64A >> (sinpB & 0x1F)) & 0x0FFFFFFFF;

		break;
	case ALU_OR:		EX_StageVal.aluOut = inpA | inpB; break;
	case ALU_AND:		EX_StageVal.aluOut = inpA & inpB; break;

		// RV32M Standard Extension
	case ALU_MUL:
		s64A = (sinpA & 0x80000000) ? 0xFFFFFFFF00000000 | sinpA : sinpA;
		s64B = (sinpB & 0x80000000) ? 0xFFFFFFFF00000000 | sinpB : sinpB;
		EX_StageVal.aluOut = (s64A * s64B) & 0x0FFFFFFFF;

		break;

	case ALU_MULH:
		s64A = (sinpA & 0x80000000) ? 0xFFFFFFFF00000000 | sinpA : sinpA;
		s64B = (sinpB & 0x80000000) ? 0xFFFFFFFF00000000 | sinpB : sinpB;
		EX_StageVal.aluOut = (s64A * s64B) >> 32;

		break;

	case ALU_MULHSU:
		s64A = (sinpA & 0x80000000) ? 0xFFFFFFFF00000000 | sinpA : sinpA;
		EX_StageVal.aluOut = (s64A * inpB) >> 32;

		break;

	case ALU_MULHU:
		u64A = inpA;
		u64B = inpB;
		EX_StageVal.aluOut = (u64A * u64B) >> 32;
		break;

	case ALU_DIV:
		if (inpB == 0)
		{
			EX_StageVal.aluOut = 0xFFFFFFFF;
		}
		else
			if (sinpA == 0x80000000 && sinpB == -1)
			{
				EX_StageVal.aluOut = 0x80000000;
			}
			else
			{
				EX_StageVal.aluOut = sinpA / sinpB;
			}
		break;

	case ALU_DIVU:
		EX_StageVal.aluOut = (inpB == 0) ? 0xFFFFFFFF : inpA / inpB;

		break;

	case ALU_REM:
		if (sinpB == 0)
		{
			EX_StageVal.aluOut = sinpA;
		}
		else
			if (sinpA == 0x80000000 && sinpB == -1)
			{
				EX_StageVal.aluOut = 0;
			}
			else
				EX_StageVal.aluOut = sinpA % sinpB;

		break;
	case ALU_REMU:
		EX_StageVal.aluOut = (inpB == 0) ? inpA : inpA % inpB;

		break;

	case ALU_CCL:		EX_StageVal.aluOut = (uint32_t)(PF_Cnt.rdCycle & 0x0FFFFFFFF);		break;
	case ALU_CCH:		EX_StageVal.aluOut = (uint32_t)(PF_Cnt.rdCycle >> 32);				break;
	case ALU_TML:		EX_StageVal.aluOut = (uint32_t)(PF_Cnt.rdTime & 0x0FFFFFFFF);		break;
	case ALU_TMH:		EX_StageVal.aluOut = (uint32_t)(PF_Cnt.rdTime >> 32);				break;
	case ALU_INSTL:		EX_StageVal.aluOut = (uint32_t)(PF_Cnt.rdInstret & 0x0FFFFFFFF);	break;
	case ALU_INSTH:		EX_StageVal.aluOut = (uint32_t)(PF_Cnt.rdInstret >> 32);			break;

	default:			EX_StageVal.aluOut = 0;	break;
	}
}

//-----------------------------------------------------------------------------
// MEM_Stage
//-----------------------------------------------------------------------------
void SPLP::MEM_Stage(void)
{
	memset(&MEM_StageVal, 0, sizeof(MEM_STAGE_REGISTER));

	MEM_StageVal.instr		= EX_StageVal.instr;
	MEM_StageVal.pcPlus4	= EX_StageVal.pcPlus4;
	MEM_StageVal.rd			= EX_StageVal.rd;
	MEM_StageVal.CWB		= EX_StageVal.CWB;
	MEM_StageVal.aluOut		= EX_StageVal.aluOut;

	DataBus(EX_StageVal.rs2Data, &MEM_StageVal.mData, EX_StageVal.aluOut, EX_StageVal.CMEM);

	pgmBREAK = (EX_StageVal.ebreak) ? HALT : NORMAL;
}

//-----------------------------------------------------------------------------
// DataBus
//-----------------------------------------------------------------------------
void SPLP::DataBus(uint32_t dataIn, uint32_t* dataOut, uint32_t addr, uint32_t ctrl)
{
	uint32_t tmpData	= 0;
	uint32_t dataAddr	= addr;
	uint32_t memAddress = addr;
	uint8_t  byte       = (uint8_t)dataIn;

	if (ctrl >= MEM_LB && ctrl <= MEM_SW)
	{
		if (memAddress >= UART0_START && memAddress < UART0_END)
		{
			UART0(&byte, memAddress);
			return;
		}

		if (memAddress >= DMEM_START && memAddress < DMEM_END)
		{
			DCacheMemory(dataAddr, ctrl);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// DCacheMemory
//-----------------------------------------------------------------------------
void SPLP::DCacheMemory(uint32_t addr, uint32_t ctrl)
{	
	uint32_t tmpData	= 0;
	uint32_t dataAddr	= (addr >> 2);
	uint32_t memAddress = addr;
	 
	if (memAddress > 0xFFF)
	{
		memAddress=memAddress;
	}

	switch (ctrl)
	{		
		case MEM_LB:	 
			switch(memAddress & 0x03)
			{
				case 0: tmpData =  mainMemory[dataAddr]        & 0x000000FF; break;					
				case 1: tmpData = (mainMemory[dataAddr] >> 8)  & 0x000000FF; break;					
				case 2: tmpData = (mainMemory[dataAddr] >> 16) & 0x000000FF; break;
				case 3: tmpData = (mainMemory[dataAddr] >> 24) & 0x000000FF; break;	
			} 

			MEM_StageVal.mData = (tmpData & 0x80) ? 0xFFFFFF00 | tmpData : tmpData;
			break;

		case MEM_LH: 
			switch(memAddress & 0x02)
			{
				case 0: tmpData = mainMemory[dataAddr] & 0x0000FFFF;break;			
				case 2: tmpData = (mainMemory[dataAddr]>>16) & 0x0000FFFF;break;				
			} 
			
			MEM_StageVal.mData = (tmpData & 0x8000) ? 0xFFFF0000 | tmpData : tmpData;
			break;

		case MEM_LW: MEM_StageVal.mData = mainMemory[dataAddr]; break;

		case MEM_LBU:  
			switch(memAddress & 0x03)
			{
				case 0: tmpData =  mainMemory[dataAddr]       ; break;
				case 1: tmpData = (mainMemory[dataAddr] >> 8) ; break;		
				case 2: tmpData = (mainMemory[dataAddr] >> 16); break;
				case 3: tmpData = (mainMemory[dataAddr] >> 24); break;
			} 

			MEM_StageVal.mData = tmpData & 0x000000FF;
			break;

		case MEM_LHU:  
			switch(memAddress & 0x02)
			{
				case 0: tmpData = mainMemory[dataAddr]      ; break;
				case 2:tmpData =( mainMemory[dataAddr]) >>16; break;	
			} 
			
			MEM_StageVal.mData = tmpData & 0x0000FFFF;
			break;

		case MEM_SB: 
			tmpData = mainMemory[dataAddr];

			switch(memAddress & 0x03)
			{
				case 0: tmpData = (mainMemory[dataAddr] & 0xFFFFFF00) |  (0x000000FF & EX_StageVal.mData)     ; break;
				case 1: tmpData = (mainMemory[dataAddr] & 0xFFFF00FF) | ((0x000000FF & EX_StageVal.mData) << 8) ; break;
				case 2: tmpData = (mainMemory[dataAddr] & 0xFF00FFFF) | ((0x000000FF & EX_StageVal.mData) << 16); break;
				case 3: tmpData = (mainMemory[dataAddr] & 0x00FFFFFF) | ((0x000000FF & EX_StageVal.mData) << 24); break;
			} 

			mainMemory[dataAddr] = tmpData;
			break;

		case MEM_SH: 	
			switch(memAddress & 0x02)
			{
				case 0: tmpData = (mainMemory[dataAddr] & 0xFFFF0000) | (0x0000FFFF & EX_StageVal.mData);break;
				case 2: tmpData = (mainMemory[dataAddr] & 0x0000FFFF) | (0xFFFF0000 & (EX_StageVal.mData << 16));break;
			} 
			
			mainMemory[dataAddr] = tmpData;
			break;

		case MEM_SW:	
				mainMemory[dataAddr] = EX_StageVal.mData;
			break;

		default: MEM_StageVal.mData = 0; break;
	}	 
}

//-----------------------------------------------------------------------------
// WB_Stage
//-----------------------------------------------------------------------------
void SPLP::WB_Stage(void)
{
	memset(&WB_StageVal, 0, sizeof(WB_STAGE_REGISTER));

	WB_StageVal.pcPlus4 = MEM_StageVal.pcPlus4;
	WB_StageVal.CWB		= MEM_StageVal.CWB;

	WB_StageVal.write	= NORMAL;
	WB_StageVal.wbReg	= MEM_StageVal.rd;

	WB_StageVal.instr	= MEM_StageVal.instr;

	switch (MEM_StageVal.CWB)
	{
	case WB_ALUOUT:		WB_StageVal.wbData = MEM_StageVal.aluOut;  WB_StageVal.write = WRITE;  break;
	case WB_MDATA:		WB_StageVal.wbData = MEM_StageVal.mData;   WB_StageVal.write = WRITE;  break;
	case WB_PCPLUS4:	WB_StageVal.wbData = MEM_StageVal.pcPlus4; WB_StageVal.write = WRITE;  break;
	default:			WB_StageVal.wbData = 0;                    WB_StageVal.write = NORMAL; break;
	}

	// Writeback stage
	//
	if (WB_StageVal.write == WRITE)
	{
		X[WB_StageVal.wbReg] = WB_StageVal.wbData;
		X[0] = 0;
	}
}

//-----------------------------------------------------------------------------
// a
//-----------------------------------------------------------------------------
void SPLP::Dissembler(uint32_t pc, uint32_t instr)
{
	// Get opcode
	unsigned int opcode = (instr >> 0) & ((1 << 7) - 1);
	unsigned int rd;
	unsigned int imm;
	unsigned int funct;
	unsigned int rs1;
	unsigned int rs2;
	unsigned int imm2;

	// Array of strings for ABI register formatting
	char* abi[] = { "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0",
		"s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
		"s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
		"t5", "t6" };

	// Display instruction
	switch (opcode)
	{
	case RV32I_NOP:
		printf("\t%x:\t%08x\t\tnop\n", pc, instr);
		break;

	case RV32I_NOPF:
		printf("\t%x:\t%08x\t\tnopf\n", pc, instr);
		break;

	case RV32I_JAL:
	{
		J_TYPE j_type;
		memcpy(&j_type, &instr, sizeof(uint32_t));

		uint32_t val1 = (j_type.imm10_1 << 1);
		uint32_t val2 = (j_type.imm11 << 11);
		uint32_t val3 = (j_type.imm19_12 << 12);
		uint32_t val4 = (j_type.imm20 << 20);
		uint32_t mask = (j_type.imm20 & 1) ? 0xFFE00000 : 0x00000000;
		int value = (mask | val4 | val3 | val2 | val1) + pc;

		printf("\t%x:\t%08x\t\tjal\t%s,%x\n", pc, instr, abi[j_type.rd], value);
		break;
	}

	case RV32I_AUIPC:
		rd = (instr >> 7) & ((1 << 5) - 1);
		imm = instr >> 12;
		printf("\t%x:\t%08x\t\tauipc\t%s,0x%x\n", pc, instr, abi[rd], imm);
		break;

	case RV32I_LUI:
		rd = (instr >> 7) & ((1 << 5) - 1);
		imm = instr >> 12;
		printf("\t%x:\t%08x\t\tlui\t%s,0x%x\n", pc, instr, abi[rd], imm);
		break;

		// BROKEN ----------------------------------------------------------------------------
	case RV32I_BRANCH:
	{
		B_TYPE b_type;
		memcpy(&b_type, &instr, sizeof(uint32_t));

		uint32_t val1 = (b_type.imm4_1 << 1);
		uint32_t val2 = (b_type.imm10_5 << 5);
		uint32_t val3 = (b_type.imm11 << 11);
		uint32_t val4 = (b_type.imm12 << 12);
		uint32_t mask = (b_type.imm12 & 1) ? 0xFFFFE000 : 0x00000000;
		int value = (mask | val4 | val3 | val2 | val1) + pc;

		// BRANCH OPTIONS
		switch (b_type.funct3)
		{
		case RV32I_BEQ:
			printf("\t%x:\t%08x\t\tbeq\t%s,%s,%x\n", pc, instr, abi[b_type.rs1], abi[b_type.rs2], value);
			break;

		case RV32I_BNE:
			printf("\t%x:\t%08x\t\tbne\t%s,%s,%x\n", pc, instr, abi[b_type.rs1], abi[b_type.rs2], value);
			break;

		case RV32I_BLT:
			printf("\t%x:\t%08x\t\tblt\t%s,%s,%x\n", pc, instr, abi[b_type.rs1], abi[b_type.rs2], value);
			break;

		case RV32I_BGE:
			printf("\t%x:\t%08x\t\tbge\t%s,%s,%x\n", pc, instr, abi[b_type.rs1], abi[b_type.rs2], value);
			break;

		case RV32I_BLTU:
			printf("\t%x:\t%08x\t\tbltu\t%s,%s,%x\n", pc, instr, abi[b_type.rs1], abi[b_type.rs2], value);
			break;

		case RV32I_BGEU:
			printf("\t%x:\t%08x\t\tbgeu\t%s,%s,%x\n", pc, instr, abi[b_type.rs1], abi[b_type.rs2], value);
			break;
		}
		break;


	}

	case RV32I_MATHI:
	{
		I_TYPE i_type;
		memcpy(&i_type, &instr, sizeof(uint32_t));
		int value = i_type.imm10_0 - (i_type.imm11 << 11);

		// MATHI OPTIONS
		switch (i_type.funct3)
		{
		case RV32I_ADDI:
			printf("\t%x:\t%08x\t\taddi\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], value);
			break;

		case RV32I_SLTI:
			printf("\t%x:\t%08x\t\tslti\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], ((i_type.imm11 << 11) + (i_type.imm10_0)));
			break;

		case RV32I_SLTIU:
			printf("\t%x:\t%08x\t\tsltiu\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], ((i_type.imm11 << 11) + (i_type.imm10_0)));
			break;

		case RV32I_XORI:
			printf("\t%x:\t%08x\t\txori\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], ((i_type.imm11 << 11) + (i_type.imm10_0)));
			break;

		case RV32I_ORI:
			printf("\t%x:\t%08x\t\tori\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], ((i_type.imm11 << 11) + (i_type.imm10_0)));
			break;

		case RV32I_ANDI:
			printf("\t%x:\t%08x\t\tandi\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], ((i_type.imm11 << 11) + (i_type.imm10_0)));
			break;

		case RV32I_SLLI:
			printf("\t%x:\t%08x\t\tslli\t%s,%s,%d\n", pc, instr, abi[i_type.rd], abi[i_type.rs1], ((i_type.imm11 << 11) + (i_type.imm10_0)));
			break;
		}
		break;
	}

	case RV32I_SHIFT:
	{
		R_TYPE r_type;
		memcpy(&r_type, &instr, sizeof(uint32_t));

		switch (r_type.funct3) {
		case RV32I_SRLI:
			printf("\t%x:\t%08x\t\tsrli\t%s,%s,0x%x\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], r_type.rs2);
			break;

		case RV32I_SRAI:
			printf("\t%x:\t%08x\t\tsrai\t%s,%s,0x%x\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], r_type.rs2);
			break;
		}
		break;
	}

	case RV32I_LOAD:
		rd = (instr >> 7) & ((1 << 5) - 1);
		funct = (instr >> 12) & ((1 << 3) - 1);
		rs1 = (instr >> 15) & ((1 << 5) - 1);
		imm = instr >> 20;

		// LOAD OPTIONS
		switch (funct)
		{
		case RV32I_LB:
			printf("\t%x:\t%08x\t\tlb\t%s,%d(%s)\n", pc, instr, abi[rd], imm, abi[rs1]);
			break;

		case RV32I_LH:
			printf("\t%x:\t%08x\t\tlh\t%s,%d(%s)\n", pc, instr, abi[rd], imm, abi[rs1]);
			break;

		case RV32I_LW:
			printf("\t%x:\t%08x\t\tlw\t%s,%d(%s)\n", pc, instr, abi[rd], imm, abi[rs1]);
			break;

		case RV32I_LBU:
			printf("\t%x:\t%08x\t\tlbu\t%s,%d(%s)\n", pc, instr, abi[rd], imm, abi[rs1]);
			break;

		case RV32I_LHU:
			printf("\t%x:\t%08x\t\tlhu\t%s,%d(%s)\n", pc, instr, abi[rd], imm, abi[rs1]);
			break;
		}
		break;

	case RV32I_STORE:
		S_TYPE s_type;
		memcpy(&s_type, &instr, sizeof(uint32_t));
		unsigned int value;

		// STORE OPTIONS
		switch (s_type.funct3) {
		case RV32I_SB:
			// Assemble imm
			value = (s_type.imm11 << 11) + (s_type.imm10_5 << 5) + (s_type.imm4_0);
			printf("\t%x:\t%08x\t\tsb\t%s,%d(%s)\n", pc, instr, abi[s_type.rs2], value, abi[s_type.rs1]);
			break;

		case RV32I_SH:
			// Assemble imm
			value = (s_type.imm11 << 11) + (s_type.imm10_5 << 5) + (s_type.imm4_0);
			printf("\t%x:\t%08x\t\tsh\t%s,%d(%s)\n", pc, instr, abi[s_type.rs2], value, abi[s_type.rs1]);
			break;

		case RV32I_SW:
			// Assemble imm
			value = (s_type.imm11 << 11) + (s_type.imm10_5 << 5) + (s_type.imm4_0);
			printf("\t%x:\t%08x\t\tsw\t%s,%d(%s)\n", pc, instr, abi[s_type.rs2], value, abi[s_type.rs1]);
			break;
		}
		break;

	case RV32I_MATH:
		R_TYPE r_type;
		memcpy(&r_type, &instr, sizeof(uint32_t));

		switch (r_type.funct7) {
		case 1:
			S_TYPE s_type;
			memcpy(&s_type, &instr, sizeof(uint32_t));

			switch (s_type.funct3) {
			case RV32M_MUL:
				printf("\t%x:\t%08x\t\tmul\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_MULH:
				printf("\t%x:\t%08x\t\tmulh\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_MULHSU:
				printf("\t%x:\t%08x\t\tmulhsu\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_MULHU:
				printf("\t%x:\t%08x\t\tmulhu\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_DIV:
				printf("\t%x:\t%08x\t\tdiv\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_DIVU:
				printf("\t%x:\t%08x\t\tdivu\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_REM:
				printf("\t%x:\t%08x\t\trem\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;

			case RV32M_REMU:
				printf("\t%x:\t%08x\t\tremu\t%s,%s,%s\n", pc, instr, abi[s_type.imm4_0], abi[s_type.rs1], abi[s_type.rs2]);
				break;
			}
			break;

		default:
			switch (r_type.funct3) {
			case ADD_SUB:
				switch (r_type.funct7) {
				case RV32I_ADD:
					printf("\t%x:\t%08x\t\tadd\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
					break;

				case RV32I_SUB:
					printf("\t%x:\t%08x\t\tsub\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
					break;
				}
				break;

			case RV32I_SLTU:
				printf("\t%x:\t%08x\t\tsll\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
				break;

			case RV32I_SRL_SRA:
				switch (r_type.funct7) {
				case RV32I_SRL:
					printf("\t%x:\t%08x\t\tsrl\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
					break;

				case RV32I_SRA:
					printf("\t%x:\t%08x\t\tsra\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
					break;
				}
				break;

			case RV32I_OR:
				printf("\t%x:\t%08x\t\tor\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
				break;

			case RV32I_AND:
				printf("\t%x:\t%08x\t\tand\t%s,%s,%s\n", pc, instr, abi[r_type.rd], abi[r_type.rs1], abi[r_type.rs2]);
				break;
			}
			break;
		}
		break;

	case RV32I_SPECIAL:
	{
		S_TYPE s_type;
		memcpy(&s_type, &instr, sizeof(uint32_t));

		switch (s_type.funct3) {
		case RV32I_ECOMMAND:
			switch ((s_type.imm11 << 11) + (s_type.imm10_5 << 5) + s_type.rs2) {
			case RV32I_EBREAK:
				printf("\t%x:\t%08x\t\tebreak\n", pc, instr);
				break;
			}
			break;
		}
		break;
	}
	}
	
}

//-----------------------------------------------------------------------------
// UART0
//-----------------------------------------------------------------------------
void SPLP::UART0(uint8_t* byte, uint32_t address)
{
	uint32_t deviceAddr = 0;

	if (address >= UART0_START && address < UART0_END)
	{
		deviceAddr = ( address & MASK128K) >> 2;

		if (address==UART0_TXRDY)
		{
			*byte = 0x01; 
			return;
		}
		if (address==UART0_RXRDY)
		{
			*byte = 0x01; 
			return;
		}

		if (address==UART0_DATA)
		{
			printf("%c",*byte);
		}
	}
}

//-----------------------------------------------------------------------------
// ImmSignExt
//-----------------------------------------------------------------------------
int32_t SPLP::ImmSignExt(uint32_t signBit, uint32_t immExt,uint32_t pc)
{
	int32_t signExt;
	
	if (signBit==1) 
	{
		signExt = (0xFFFFF000 | immExt) + (pc);
	}
	else
	{
		signExt = (0x00000FFF & immExt) + (pc);
	}

	return signExt;
}

//-----------------------------------------------------------------------------
// LoadInstr
//-----------------------------------------------------------------------------
void SPLP::LoadInstr(char* fname)
{
	uint32_t length = 0;

	std::ifstream is (fname, std::ifstream::binary);

	if (is) 
	{
		// get length of file:
		is.seekg (0, is.end);
		length = (uint32_t)is.tellg();
		is.seekg (0, is.beg);

		memset(mainMemory,0,MAINMEM);
		is.read ((char*)mainMemory,length);
	}

    if (is)
	{
		printf("\nFile: %s : size = %d bytes\n\n",fname,length);
	}
	else
	{
		printf("Cannot open File: %s : \n\n",fname);
		exit(0);
	}
    is.close();   
}	 

//-----------------------------------------------------------------------------
// PrintDateTime
//-----------------------------------------------------------------------------
void SPLP::PrintDateTime(void)
{
    char		buffer[26];
	struct		tm newtime;
    time_t		now = time(0);

    localtime_s(&newtime,&now);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", &newtime);
    puts(buffer);
}