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
		_opcode = (_memory[_programCounter] << 8) | _memory[(_programCounter + 1u)];

		//Decode and Execute
		switch ((_opcode & 0xF000) >> 12)
		{
		case 0x0:
		{
			switch (_opcode & 0x000F)
			{
			case 0x0000: op00E0(); break;
			case 0x000E: op00EE(); break;
			}
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
			switch (_opcode & 0x000F >> 12)
			{
			case 0x0: op8xy0(); break;
			case 0x1: op8xy1(); break;
			case 0x2: op8xy2(); break;
			case 0x3: op8xy3(); break;
			case 0x4: op8xy4(); break;
			case 0x5: op8xy5(); break;
			case 0x6: op8xy6(); break;
			case 0x7: op8xy7(); break;
			case 0x8: op8xyE(); break;
			}
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
			switch (_opcode & 0x00FF >> 8)
			{
			case 0x9E:opEx9E(); break;
			case 0xA1:opExA1(); break;
			}
			break;
		}
		case 0xF:
		{
			switch (_opcode & 0x00FF >> 8)
			{
			case 0x07: opFx07(); break;
			case 0x0A: opFx0A(); break;
			case 0x15: opFx15(); break;
			case 0x18: opFx18(); break;
			case 0x1E: opFx1E(); break;
			case 0x29: opFx29(); break;
			case 0x33: opFx33(); break;
			case 0x55: opFx55(); break;
			case 0x65: opFx65(); break;
			}
			break;
		}
		default:
		{
			std::stringstream output;
			output << "OP: " << std::hex << _opcode;
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
			if (_soundTimer == 1)
			{
				_console.logInfoLine("BEEP");
			}
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
		auto input = _runner->getInteractionService();

		key[0] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::One));
		key[1] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::Two));
		key[2] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::Three));
		key[3] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::Four));
		key[4] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::Q));
		key[5] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::W));
		key[6] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::E));
		key[7] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::R));
		key[8] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::A));
		key[9] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::S));
		key[10] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::D));
		key[11] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::F));
		key[12] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::Z));
		key[13] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::X));
		key[14] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::C));
		key[15] = static_cast<unsigned char>(input.lock()->getKeyState(NovelRT::Input::KeyCode::V));

	}

	//Defining Functions
	void CPU::op00E0()
	{
		//Clear Screen
		for each (auto var in gfx)
		{
			var = 0;
		}
		drawFlag = true;
		_console.logDebugLine("CLS");
		_programCounter += 2;
	}

	void CPU::op00EE()
	{
		//Return
		_sp--;
		_programCounter = _stack[_sp];
		_programCounter += 2;
		_console.logDebugLine("RET");
	}

	void CPU::op1nnn()
	{
		//Jump to Location nnn
		_programCounter = (_opcode & 0x0FFF);

		std::stringstream hex;
		hex << std::hex << (_opcode & 0x0FFF);
		_console.logDebug("JP ", hex.str());
	}

	void CPU::op2nnn()
	{
		//Call nnn
		_stack[_sp] = _programCounter;
		_sp++;
		_programCounter = (_opcode & 0x0FFF);
		std::stringstream hex;
		hex << std::hex << (_opcode & 0x0FFF);
		_console.logDebug("CALL $" + hex.str());
	}

	void CPU::op3xkk()
	{
		//Skip next instr. if Vx == kk
		if (_vRegister[(_opcode & 0x0F00) >> 8] == (_opcode & 0x00FF))
		{
			_programCounter += 4;
		}
		else
		{
			_programCounter += 2;
		}

		std::stringstream hex, regHex;
		hex << std::hex << (_opcode & 0x00FF);
		regHex << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine("SE V" + regHex.str() + ", " + hex.str());

	}

	void CPU::op4xkk()
	{
		//Skip next instr. if Vx != kk
		if (_vRegister[(_opcode & 0x0F00) >> 8] != (_opcode & 0x00FF))
		{
			_programCounter += 4;
		}
		else
		{
			_programCounter += 2;
		}

		std::stringstream hex, regHex;
		hex << std::hex << (_opcode & 0x00FF);
		regHex << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine("SNE V" + regHex.str() + ", " + hex.str());
	}

	void CPU::op5xy0()
	{
		//Skip next instr. if Vx = Vy
		if (_vRegister[(_opcode & 0x0F00) >> 8] == _vRegister[(_opcode & 0x00F0) >> 4])
		{
			_programCounter += 4;
		}
		else
		{
			_programCounter += 2;
		}

		std::stringstream hex, regHex;
		hex << std::hex << ((_opcode & 0x00F0) >> 4);
		regHex << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebug("SE V" + regHex.str() + ", " + hex.str());
	}

	void CPU::op6xkk()
	{
		//Set Vx = kk
		_vRegister[(_opcode & 0x0F00) >> 8] = (_opcode & 0x00FF);
		_programCounter += 2;

		std::stringstream hex, regHex;
		hex << std::hex << (_opcode & 0x00FF);
		regHex << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine("LD V" + regHex.str() + ", " + hex.str());
	}

	void CPU::op7xkk()
	{
		//Set Vx = Vx + kk
		_vRegister[(_opcode & 0x0F00) >> 8] += (_opcode & 0x00FF);
		_programCounter += 2;

		std::stringstream hex, regHex;
		hex << std::hex << (_opcode & 0x00FF);
		regHex << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine("ADD V" + regHex.str() + ", " + hex.str());
	}

	void CPU::op8xy0()
	{
		//Set Vx = Vy
		_vRegister[(_opcode & 0x0F00) >> 8] = _vRegister[(_opcode & 0x00F0) >> 4];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());

	}

	void CPU::op8xy1()
	{
		//Set Vx = Vx OR Vy
		_vRegister[(_opcode & 0x0F00) >> 8] |= _vRegister[(_opcode & 0x00F0) >> 4];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "OR V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op8xy2()
	{
		//Set Vx = Vx AND Vy
		_vRegister[(_opcode & 0x0F00) >> 8] &= _vRegister[(_opcode & 0x00F0) >> 4];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "AND V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op8xy3()
	{
		//Set Vx = Vx XOR Vy
		_vRegister[(_opcode & 0x0F00) >> 8] ^= _vRegister[(_opcode & 0x00F0) >> 4];
		_programCounter += 2;


		std::stringstream regHex;
		regHex << "XOR V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op8xy4()
	{
		//Set Vx = Vx + Vy, set VF = carry
		if (_vRegister[(_opcode & 0x00F0) >> 4] > (0xFF - _vRegister[(_opcode & 0x0F00) >> 8]))
		{
			_vRegister[0xF] = 1; //carry flag
		}
		else
		{
			_vRegister[0xF] = 0;
		}
		_vRegister[(_opcode & 0x0F00) >> 8] += _vRegister[(_opcode & 0x00F0) >> 4];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "ADD V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}
	
	void CPU::op8xy5()
	{
		//Set Vx = Vx - Vy, set VF = NOT borrow
		if (_vRegister[(_opcode & 0x00F0) >> 4] > _vRegister[(_opcode & 0x0F00) >> 8])
		{
			_vRegister[0xF] = 1; //borrow
		}
		else
		{
			_vRegister[0xF] = 0;
		}
		_vRegister[(_opcode & 0x0F00) >> 8] -= _vRegister[(_opcode & 0x00F0) >> 4];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "SUB V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op8xy6()
	{
		//Set Vx = Vx SHR 1
		_vRegister[0xF] = (_vRegister[(_opcode & 0x0F00) >> 8] & 0x1);
		_vRegister[(_opcode & 0x0F00) >> 8] >>= 1;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "SHR V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op8xy7()
	{
		//Set Vx = Vy - Vx, set VF = NOT borrow
		if (_vRegister[(_opcode & 0x0F00) >> 8] > _vRegister[(_opcode & 0x00F0) >> 4])
		{
			_vRegister[0xF] = 0; //borrow
		}
		else
		{
			_vRegister[0xF] = 1;
		}
		_vRegister[(_opcode & 0x0F00) >> 8] = _vRegister[(_opcode & 0x00F0) >> 4] - _vRegister[(_opcode & 0x0F00) >> 8];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "SUBN V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op8xyE()
	{
		//Set Vx = Vx SHL 1
		_vRegister[0xF] = (_vRegister[(_opcode & 0x0F00) >> 8]) >> 7;
		_vRegister[(_opcode & 0x0F00) >> 8] <<= 1;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "SHL V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::op9xy0()
	{
		//Skip next instr. if Vx != Vy
		if (_vRegister[(_opcode & 0x0F00) >> 8] != _vRegister[(_opcode & 0x00F0) >> 4])
		{
			_programCounter += 4;
		}
		else
		{
			_programCounter += 2;
		}

		std::stringstream regHex;
		regHex << "SNE V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opAnnn()
	{
		//Set I = nnn
		_index = (_opcode & 0x0FFF);
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD I, " << std::hex << (_opcode & 0x0FFF);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opBnnn()
	{
		//Jump to location nnn + V0
		_programCounter = (_opcode & 0x0FFF) + _vRegister[0x0];
		
		std::stringstream regHex;
		regHex << "JP V0, $" << std::hex << (_opcode & 0x0FFF);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opCxkk()
	{
		//Set Vx = random byte AND kk
		_vRegister[(_opcode & 0x0F00) >> 8] = (std::rand() % 0xFF) & (_opcode & 0x00FF);
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "RND V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opDxyn()
	{
		unsigned short x = _vRegister[(_opcode & 0x0F00) >> 8];
		unsigned short y = _vRegister[(_opcode & 0x00F0) >> 4];
		unsigned short height = (_opcode & 0x000F);
		unsigned short pixel;

		std::stringstream regHex;
		regHex << "DRW V" << std::hex << x;
		regHex << ", V" << std::hex << y;
		regHex << ", " << std:: hex << height;
		_console.logDebugLine(regHex.str());


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
		_programCounter += 2;
	}

	void CPU::opEx9E()
	{
		//SKP Vx
		//Skip next instruction if key with Vx value is pressed
		if (key[_vRegister[(_opcode & 0x0F00) >> 8]] != 0)
		{
			_programCounter += 4;
		}
		else
		{
			_programCounter += 2;
		}
			
		std::stringstream regHex;
		regHex << "SKP V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());

	}

	void CPU::opExA1()
	{
		//SKNP Vx
		//Skip next instruction if key with Vx value is not pressed
		if (key[_vRegister[(_opcode & 0x0F00) >> 8]] == 0)
		{
			_programCounter += 4;
		}
		else
			_programCounter += 2;

		std::stringstream regHex;
		regHex << "SKNP V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opFx07()
	{
		_vRegister[(_opcode & 0x0F00) >> 8] = _delayTimer;

		std::stringstream regHex;
		regHex << "LD V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", T";
		_console.logDebugLine(regHex.str());
	}

	void CPU::opFx0A()
	{
		bool pressed = false;

		for (int i = 0; i < 16; i++)
		{
			if (key[i] != 0)
			{
				_vRegister[(_opcode & 0x0F00) >> 8] = 1;
				pressed = true;
			}
		}

		std::stringstream regHex;
		regHex << "LD V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", K";
		_console.logDebugLine(regHex.str());

		if (!pressed) return;

		_programCounter += 2;
	}

	void CPU::opFx15()
	{
		_delayTimer = _vRegister[(_opcode & 0x0F00) >> 8];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD DT, V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());

	}

	void CPU::opFx18()
	{
		_soundTimer = _vRegister[(_opcode & 0x0F00) >> 8];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD ST, V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opFx1E()
	{
		if (_index + _vRegister[(_opcode & 0x0F00) >> 8] > 0xFFF)
		{
			_vRegister[0xF] = 1; //overflow
		}
		else
		{
			_vRegister[0xF] = 0;
		}

		_index += _vRegister[(_opcode & 0x0F00) >> 8];
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "ADD I, V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opFx29()
	{
		_index = _vRegister[(_opcode & 0x0F00) >> 8] * 0x5;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD F, V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());

	}

	void CPU::opFx33()
	{
		_memory[_index] = _vRegister[(_opcode & 0x0F00) >> 8] / 100;
		_memory[_index + 1u] = (_vRegister[(_opcode & 0x0F00) >> 8] / 10) % 10;
		_memory[_index + 1u] = (_vRegister[(_opcode & 0x0F00) >> 8] % 100) % 10;
		_programCounter += 2;
		
		std::stringstream regHex;
		regHex << "ADD B, V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opFx55()
	{
		for (int i = 0; i <= ((_opcode & 0x0F00) >> 8); i++)
		{
			_memory[_index + i] = _vRegister[i];
		}
		_index += ((_opcode & 0x0F00) >> 8) + 1;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD [I], V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());

	}

	void CPU::opFx65()
	{
		for (unsigned short i = 0; i <= ((_opcode & 0x0F00) >> 8); i++)
		{
			_vRegister[i] = _memory[_index + i];
		}
		_index += ((_opcode & 0x0F00) >> 8) + 1;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", [I]";
		_console.logDebugLine(regHex.str());

	}


	void CPU::table0Function()
	{
		unsigned short op = (_opcode & 0x000F);
		this->_table0x0[op];
	}

	void CPU::table8Function()
	{
		this->_table0x8[(_opcode & 0x000F)];
	}

	void CPU::tableEFunction()
	{
		//((*this).*(_table0xE[_opcode & 0x000F]))();
		unsigned short op = (_opcode & 0x000F);
		this->_table0xE[op];
	}

	void CPU::tableFFunction()
	{
		//((*this).*(_table0xF[_opcode & 0x00FF]))();
		unsigned short op = (_opcode & 0x00FF);
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
