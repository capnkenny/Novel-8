//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "CPU.h"
#include <iostream>

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
		_table0xF()
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

	};

	void CPU::emulateCycle() 
	{
		//Fetch
		_opcode = (_memory[_programCounter] << 8) | (_memory[(_programCounter + 1)]);

		_programCounter += 2;
		
		//Decode and Execute
		//((*this).*(_funcTable[(_opcode & 0xF000) >> 12]))();

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
		std::ifstream file(fileName, std::ios::binary | std::ios::ate);

		if (file.is_open())
		{
			// Get size of file and allocate a buffer to hold the contents
			std::streampos size = file.tellg();
			char* buffer = new char[size];

			// Go back to the beginning of the file and fill the buffer
			file.seekg(0, std::ios::beg);
			file.read(buffer, size);
			file.close();

			// Load the ROM contents into the Chip8's memory, starting at 0x200
			for (long i = 0; i < size; ++i)
			{
				_memory[(0x200 + i)] = buffer[i];
			}

			// Free the buffer
			delete[] buffer;
		}
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
		_console.logDebugLine("Clearing Screen...");
	}

	void CPU::op00EE()
	{
		//Return
		_sp--;
		_programCounter = _stack[_sp];
	}

	void CPU::op1nnn()
	{
		//Jump to Location nnn
		auto addr = (_opcode & 0x0FFF);
		_programCounter = addr;
	}

	void CPU::op2nnn()
	{
		//Call nnn
		auto addr = (_opcode & 0x0FFF);
		_stack[_sp] = _programCounter;
		_sp++;
		_programCounter = addr;
	}

	void CPU::op3xkk()
	{
		//Skip next instr. if Vx == kk
		auto reg = (_opcode & 0x0F00) >> 8;
		auto byte = (_opcode & 0x00FF);

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
	}

	void CPU::op5xy0()
	{
		//Skip next instr. if Vx = Vy
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;

		if (_vRegister[regX] == _vRegister[regY])
		{
			_programCounter += 2;
		}
	}

	void CPU::op6xkk()
	{
		//Set Vx = kk
		auto reg = (_opcode & 0x0F00) >> 8;
		auto byte = (_opcode & 0x00FF);
		_vRegister[reg] = byte;
	}

	void CPU::op7xkk()
	{
		//Set Vx = Vx + kk
		auto reg = (_opcode & 0x0F00) >> 8;
		auto byte = (_opcode & 0x00FF);
		_vRegister[reg] += byte;
	}

	void CPU::op8xy0()
	{
		//Set Vx = Vy
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] = _vRegister[regY];
	}

	void CPU::op8xy1()
	{
		//Set Vx = Vx OR Vy
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] |= _vRegister[regY];
	}

	void CPU::op8xy2()
	{
		//Set Vx = Vx AND Vy
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] &= _vRegister[regY];
	}

	void CPU::op8xy3()
	{
		//Set Vx = Vx XOR Vy
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
		_vRegister[regX] ^= _vRegister[regY];
	}

	void CPU::op8xy4()
	{
		//Set Vx = Vx + Vy, set VF = carry
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
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
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
		
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
		auto regX = (_opcode & 0x0F00) >> 8;
		
		_vRegister[0xF] = (_vRegister[regX] & 0x1);
		_vRegister[regX] >>= 1;
	}

	void CPU::op8xy7()
	{
		//Set Vx = Vy - Vx, set VF = NOT borrow
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;

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
		auto regX = (_opcode & 0x0F00) >> 8;

		_vRegister[0xF] = (_vRegister[regX] & 0x80) >> 7;
		_vRegister[regX] <<= 1;
	}

	void CPU::op9xy0()
	{
		//Skip next instr. if Vx != Vy
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;

		if (_vRegister[regX] != _vRegister[regY])
		{
			_programCounter += 2;
		}
	}

	void CPU::opAnnn()
	{
		//Set I = nnn
		auto addr = (_opcode & 0x0FFF);
		_index = addr;
	}

	void CPU::opBnnn()
	{
		//Jump to location nnn + V0
		auto addr = (_opcode & 0xFFF);
		_programCounter = _vRegister[0x0] + addr;
	}

	void CPU::opCxkk()
	{
		//Set Vx = random byte AND kk
	}

	void CPU::opDxyn()
	{
		auto regX = (_opcode & 0x0F00) >> 8;
		auto regY = (_opcode & 0x00F0) >> 4;
		auto height = (_opcode * 0x000F);

		auto xPos = _vRegister[regX] % 1920;
		auto yPos = _vRegister[regY] % 1080;

		_vRegister[0xF] = 0;

		for (unsigned int row = 0; row < height; row++)
		{
			auto spriteByte = _memory[_index + row];

			for (unsigned int col = 0; col < 8; col++)
			{
				auto spritePixel = spriteByte & (0x80 >> col);
				auto* screenPixel = &gfx[(yPos + row) * 1920 + (xPos + col)];

				if (spritePixel)
				{
					if (*screenPixel == 0xFFFFFFFF)
					{
						_vRegister[0xF] = 1;
					}
					*screenPixel ^= 0xFFFFFFFF;
				}
			}
		}
	}

	void CPU::opEx9E()
	{
		//SKP Vx
		//Skip next instruction if key with Vx value is pressed
		auto regX = (_opcode & 0x0F00) >> 8;
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
		auto regX = (_opcode & 0x0F00) >> 8;
		auto keyCheck = _vRegister[regX];

		if (!key[keyCheck])
		{
			_programCounter += 2;
		}
	}

	void CPU::opFx07()
	{
	}

	void CPU::opFx0A()
	{
	}

	void CPU::opFx15()
	{
	}

	void CPU::opFx18()
	{
	}

	void CPU::opFx1E()
	{
	}

	void CPU::opFx29()
	{
	}

	void CPU::opFx33()
	{
	}

	void CPU::opFx55()
	{
	}

	void CPU::opFx65()
	{
	}


	void CPU::table0Function()
	{
		((*this).*(_table0x0[_opcode & 0x000F]))();
	}

	void CPU::table8Function()
	{
		((*this).*(_table0x8[_opcode & 0x000F]))();
	}

	void CPU::tableEFunction()
	{
		((*this).*(_table0xE[_opcode & 0x000F]))();
	}

	void CPU::tableFFunction()
	{
		((*this).*(_table0xF[_opcode & 0x00FF]))();
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
