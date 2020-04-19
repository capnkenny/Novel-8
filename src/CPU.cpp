//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "CPU.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace Chip8 {
	CPU::CPU(NovelRT::NovelRunner* runner) :
		_programCounter(0x200),
		_opcode(0),
		_index(0),
		_sp(0),
		drawFlag(false),
		_memory(std::array<unsigned char, 4096>()),
		_vRegister(std::array<unsigned char, 16>()),
		_stack(std::array<unsigned short, 16>()),
		key(std::array<unsigned char, 16>()),
		gfx(std::array<unsigned char, 2048>()),
		_runner(runner),
		_funcTable(),
		_table0x0(),
		_table0x8(),
		_table0xE(),
		_table0xF(),
		_output("")
	{
		_delayTimer = 0;
		_soundTimer = 0;

		//Zero out arrays
		for each (auto var in gfx)
		{
			var = 0;
		}

		for each (auto var in _memory)
		{
			var = 0;
		}

		for each (auto var in key)
		{
			var = 0;
		}

		for (int i = 0; i < 80; ++i)
		{
			_memory[i] = _fontset[i];
		}

		if (!runner)
		{
			std::cerr << "Error initializing runner properly! Exiting..." << std::endl;
			exit(3);
		}
	
		_console = NovelRT::LoggingService("CPU");
		_console.logInfoLine("CPU initialized.");

	};

	void CPU::emulateCycle() 
	{
		//Fetch
		_opcode = (_memory[_programCounter] << 8) | _memory[(_programCounter + 1)];
		
		_programCounter += 2;
		
		auto op = (_opcode & 0xF000) >> 12;
		//Decode and Execute
		switch (op)
		{
		case 0x0:
		{
			table0Function();
			break;
		}
		case 0x1:
		{
			op1nnn();
			break;
		}
		case 0x2:
		{
			op2nnn();
			break;
		}
		case 0x3:
		{
			op3xkk();
			break;
		}
		case 0x4:
		{
			op4xkk();
			break;
		}
		case 0x5:
		{
			op5xy0();
			break;
		}
		case 0x6:
		{
			op6xkk();
			break;
		}
		case 0x7:
		{
			op7xkk();
			break;
		}
		case 0x8:
		{
			table8Function();
			break;
		}
		case 0x9:
		{
			op9xy0();
			break;
		}
		case 0xA:
		{
			opAnnn();
			break;
		}
		case 0xB:
		{
			opBnnn();
			break;
		}
		case 0xC:
		{
			opCxkk();
			break;
		}
		case 0xD:
		{
			opDxyn();
			break;
		}
		case 0xE:
		{
			tableEFunction();
			break;
		}
		case 0xF:
		{
			tableFFunction();
			break;
		}
		default:
		{
			std::stringstream output;
			output << "OP: " << op;
			_console.logDebugLine(output.str());
			break;
		}
		}



		//Decrement timers if it's been set
		if (_delayTimer > 0)
		{
			_delayTimer--;
		}
		if (_soundTimer > 0)
		{
			_soundTimer--;
		}
	}

	void CPU::loadProgram(std::string fileName)
	{

		FILE* pFile;
		fopen_s(&pFile, fileName.c_str(), "rb");
		if (pFile == NULL)
		{
			_console.logErrorLine("Could not open file!");
			return;
		}

		// Check file size
		fseek(pFile, 0, SEEK_END);
		long lSize = ftell(pFile);
		rewind(pFile);
		_console.logInfo("Filesize: ", (int)lSize);

		// Allocate memory to contain the whole file
		char* buffer = (char*)malloc(sizeof(char) * lSize);
		if (buffer == NULL)
		{
			_console.logErrorLine("Memory error");
			return;
		}

		// Copy the file into the buffer
		size_t result = fread(buffer, 1, lSize, pFile);
		if (result != lSize)
		{
			_console.logErrorLine("Reading error");
			return;
		}

		// Copy buffer to Chip8 memory
		if ((4096 - 512) > lSize)
		{
			for (int i = 0; i < lSize; ++i)
				_memory[i + 512] = buffer[i];
		}
		else
			_console.logErrorLine("Error: ROM too big for memory");

		// Close file, free buffer
		fclose(pFile);
		free(buffer);
	}

	void CPU::setKeys()
	{


	}

	//Defining Functions
	void CPU::op00E0()
	{
		//Clear Screen
		for each (auto var in gfx)
		{
			var = 0;
		}
		_console.logDebugLine("CLS");
	}

	void CPU::op00EE()
	{
		//Return
		_sp--;
		_programCounter = _stack[_sp];
		_console.logDebugLine("RET");
	}

	void CPU::op1nnn()
	{
		//Jump to Location nnn
		unsigned short addr = (_opcode & 0x0FFF);
		_programCounter = addr;
		_console.logDebugLine("JP nnn");
	}

	void CPU::op2nnn()
	{
		//Call nnn
		unsigned short addr = (_opcode & 0x0FFF);
		_stack[_sp] = _programCounter;
		_sp++;
		_programCounter = addr;
		_console.logDebugLine("CALL nnn");
	}

	void CPU::op3xkk()
	{
		//Skip next instr. if Vx == kk
		auto reg = (_opcode & 0x0F00) >> 8;
		auto byte = (_opcode & 0x00FF);
		_console.logDebugLine("SE Vx, byte");

		if (_vRegister[reg] == byte)
		{
			_programCounter += 2;
		}
	}

	void CPU::op4xkk()
	{
		//Skip next instr. if Vx != kk
		auto reg = (_opcode & 0x0F00) >> 8;
		auto byte = (_opcode & 0x00FF);

		if (_vRegister[reg] != byte)
		{
			_programCounter += 2;
		}
		_console.logDebugLine("SNE Vx, byte");
	}

	void CPU::op5xy0()
	{
		//Skip next instr. if Vx = Vy
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;

		if (_vRegister[regX] == _vRegister[regY])
		{
			_programCounter += 2;
		}
	}

	void CPU::op6xkk()
	{
		//Set Vx = kk
		auto reg = (_opcode & 0x0F00) >> 8;
		uint8_t byte = (_opcode & 0x00FF);
		_vRegister[reg] = byte;
	}

	void CPU::op7xkk()
	{
		//Set Vx = Vx + kk
		auto reg = (_opcode & 0x0F00) >> 8;
		uint8_t byte = (_opcode & 0x00FF);
		_vRegister[reg] += byte;
	}

	void CPU::op8xy0()
	{
		//Set Vx = Vy
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] = _vRegister[regY];
	}

	void CPU::op8xy1()
	{
		//Set Vx = Vx OR Vy
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] |= _vRegister[regY];
	}

	void CPU::op8xy2()
	{
		//Set Vx = Vx AND Vy
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] &= _vRegister[regY];
	}

	void CPU::op8xy3()
	{
		//Set Vx = Vx XOR Vy
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] ^= _vRegister[regY];
	}

	void CPU::op8xy4()
	{
		//Set Vx = Vx + Vy, set VF = carry
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;
		auto sum = _vRegister[regX] + _vRegister[regY];

		if (sum > 255)
		{
			_vRegister[0xF] = 1;
		}
		else
		{
			_vRegister[0xF] = 0;
		}
		_vRegister[regX] = (sum & 0xFF);
	}

	void CPU::op8xy5()
	{
		//Set Vx = Vx - Vy, set VF = NOT borrow
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;
		
		if (_vRegister[regX] > _vRegister[regY])
		{
			_vRegister[0xF] = 1;
		}
		else
		{
			_vRegister[0xF] = 0;
		}
		_vRegister[regX] -= _vRegister[regY];
	}

	void CPU::op8xy6()
	{
		//Set Vx = Vx SHR 1
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		
		_vRegister[0xF] = (_vRegister[regX] & 0x1);
		_vRegister[regX] >>= 1;
	}

	void CPU::op8xy7()
	{
		//Set Vx = Vy - Vx, set VF = NOT borrow
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;

		if (_vRegister[regX] > _vRegister[regY])
		{
			_vRegister[0xF] = 1;
		}
		else
		{
			_vRegister[0xF] = 0;
		}
		_vRegister[regX] = _vRegister[regY] - _vRegister[regX];
	}

	void CPU::op8xyE()
	{
		//Set Vx = Vx SHL 1
		unsigned short regX = (_opcode & 0x0F00) >> 8;

		_vRegister[0xF] = (_vRegister[regX] & 0x80) >> 7;
		_vRegister[regX] <<= 1;
	}

	void CPU::op9xy0()
	{
		//Skip next instr. if Vx != Vy
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short regY = (_opcode & 0x00F0) >> 4;

		if (_vRegister[regX] != _vRegister[regY])
		{
			_programCounter += 2;
		}
	}

	void CPU::opAnnn()
	{
		//Set I = nnn
		unsigned short addr = (_opcode & 0x0FFF);
		_index = addr;
	}

	void CPU::opBnnn()
	{
		//Jump to location nnn + V0
		unsigned short addr = (_opcode & 0xFFF);
		_programCounter = _vRegister[0x0] + addr;
	}

	void CPU::opCxkk()
	{
		//Set Vx = random byte AND kk
		unsigned short byte = (_opcode & 0x00FF);
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		std::srand(255);
		unsigned short random = static_cast<unsigned short>(std::rand());

		_vRegister[regX] = (unsigned char)(random & byte);
	}

	void CPU::opDxyn()
	{
		_console.logDebugLine("DRW Vx, Vy, nibble");
		unsigned short x = _vRegister[(_opcode & 0x0F00) >> 8];
		unsigned short y = _vRegister[(_opcode & 0x00F0) >> 4];
		unsigned short height = (_opcode & 0x000F);
		unsigned short pixel;

		_vRegister[0xF] = 0;
		for (int yLine = 0; (unsigned short)yLine < height; yLine++)
		{
			pixel = _memory[_index + yLine];

			for (int xLine = 0; xLine < 8; xLine++)
			{
				if ((pixel & (0x80 >> xLine)) != 0)
				{
					if (gfx[(x + xLine + ((y + yLine) * 64))] == 1)
					{
						_vRegister[0xF] = 1;
					}
					gfx[(x + xLine + ((y + yLine) * 64))] ^= 1;
				}
			}
		}

		drawFlag = true;
	}

	void CPU::opEx9E()
	{
		//SKP Vx
		//Skip next instruction if key with Vx value is pressed
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		auto keyCheck = _vRegister[regX];

		if (key[keyCheck])
		{
			_programCounter += 2;
		}
	}

	void CPU::opExA1()
	{
		//SKNP Vx
		//Skip next instruction if key with Vx value is not pressed
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned char keyCheck = _vRegister[regX];

		if (!key[keyCheck])
		{
			_programCounter += 2;
		}
	}

	void CPU::opFx07()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		_vRegister[regX] = _delayTimer;
	}

	void CPU::opFx0A()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;

		if (key[0])
		{
			_vRegister[regX] = 0;
		}
		else if (key[1])
		{
			_vRegister[regX] = 1;
		}
		else if (key[2])
		{
			_vRegister[regX] = 2;
		}
		else if (key[3])
		{
			_vRegister[regX] = 3;
		}
		else if (key[4])
		{
			_vRegister[regX] = 4;
		}
		else if (key[5])
		{
			_vRegister[regX] = 5;
		}
		else if (key[6])
		{
			_vRegister[regX] = 6;
		}
		else if (key[7])
		{
			_vRegister[regX] = 7;
		}
		else if (key[8])
		{
			_vRegister[regX] = 8;
		}
		else if (key[9])
		{
			_vRegister[regX] = 9;
		}
		else if (key[10])
		{
			_vRegister[regX] = 10;
		}
		else if (key[11])
		{
			_vRegister[regX] = 11;
		}
		else if (key[12])
		{
			_vRegister[regX] = 12;
		}
		else if (key[13])
		{
			_vRegister[regX] = 13;
		}
		else if (key[14])
		{
			_vRegister[regX] = 14;
		}
		else if (key[15])
		{
			_vRegister[regX] = 15;
		}
		else
		{
			_programCounter -= 2;
		}
	}

	void CPU::opFx15()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		_delayTimer = _vRegister[regX];
	}

	void CPU::opFx18()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		_soundTimer = _vRegister[regX];
	}

	void CPU::opFx1E()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		_index += _vRegister[regX];
	}

	void CPU::opFx29()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned short digit = _vRegister[regX];
		
		_index = 0x0 + (5 * digit);
	}

	void CPU::opFx33()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;
		unsigned value = _vRegister[regX];

		_memory[_index + 2] = value % 10;
		value /= 10;

		_memory[_index + 1] = value % 10;
		value /= 10;

		_memory[_index] = value % 10;
	}

	void CPU::opFx55()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;

		for (unsigned short i = 0; i <= regX; i++)
		{
			_memory[_index + i] = _vRegister[i];
		}
	}

	void CPU::opFx65()
	{
		unsigned short regX = (_opcode & 0x0F00) >> 8;

		for (unsigned short i = 0; i <= regX; i++)
		{
			_vRegister[i] = _memory[_index + i];
		}
	}


	void CPU::table0Function()
	{
		//((*this).*(_table0x0[_opcode & 0x000F]))();
		auto op = (_opcode & 0x000F);
		this->_table0x0[op];
	}

	void CPU::table8Function()
	{
		//((*this).*(_table0x8[_opcode & 0x000F]))();
		auto op = (_opcode & 0x000F);
		this->_table0x8[op];
	}

	void CPU::tableEFunction()
	{
		//((*this).*(_table0xE[_opcode & 0x000F]))();
		auto op = (_opcode & 0x000F);
		this->_table0xE[op];
	}

	void CPU::tableFFunction()
	{
		//((*this).*(_table0xF[_opcode & 0x00FF]))();
		auto op = (_opcode & 0x00FF);
		this->_table0xF[op];
	}

	void CPU::setFunctions() 
	{
		_funcTable[0x0] = &CPU::table0Function;
		_funcTable[0x1] = &CPU::op1nnn;
		_funcTable[0x2] = &CPU::op2nnn;
		_funcTable[0x3] = &CPU::op3xkk;
		_funcTable[0x4] = &CPU::op4xkk;
		_funcTable[0x5] = &CPU::op5xy0;
		_funcTable[0x6] = &CPU::op6xkk;
		_funcTable[0x7] = &CPU::op7xkk;
		_funcTable[0x8] = &CPU::table8Function;
		_funcTable[0x9] = &CPU::op9xy0;
		_funcTable[0xA] = &CPU::opAnnn;
		_funcTable[0xB] = &CPU::opBnnn;
		_funcTable[0xC] = &CPU::opCxkk;
		_funcTable[0xD] = &CPU::opDxyn;
		_funcTable[0xE] = &CPU::tableEFunction;
		_funcTable[0xF] = &CPU::tableFFunction;

		_table0x0[0x0] = &CPU::op00E0;
		_table0x0[0xE] = &CPU::op00EE;

		_table0x8[0x0] = &CPU::op8xy0;
		_table0x8[0x1] = &CPU::op8xy1;
		_table0x8[0x2] = &CPU::op8xy2;
		_table0x8[0x3] = &CPU::op8xy3;
		_table0x8[0x4] = &CPU::op8xy4;
		_table0x8[0x5] = &CPU::op8xy5;
		_table0x8[0x6] = &CPU::op8xy6;
		_table0x8[0x7] = &CPU::op8xy7;
		_table0x8[0xE] = &CPU::op8xyE;

		_table0xE[0x1] = &CPU::opExA1;
		_table0xE[0xE] = &CPU::opEx9E;

		_table0xF[0x07] = &CPU::opFx07;
		_table0xF[0x0A] = &CPU::opFx0A;
		_table0xF[0x15] = &CPU::opFx15;
		_table0xF[0x18] = &CPU::opFx18;
		_table0xF[0x1E] = &CPU::opFx1E;
		_table0xF[0x29] = &CPU::opFx29;
		_table0xF[0x33] = &CPU::opFx33;
		_table0xF[0x55] = &CPU::opFx55;
		_table0xF[0x65] = &CPU::opFx65;

	}


	CPU::~CPU()
	{}
};
