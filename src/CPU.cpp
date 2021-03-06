//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "CPU.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <AL\al.h>

namespace Chip8 {
	CPU::CPU(NovelRT::NovelRunner* runner) :
		_index(0),
		_opcode(0),
		_programCounter(0x200),
		_runner(runner),
		_sp(0),
		_memory(std::array<unsigned char, 4096>()),
		_stack(std::array<unsigned short, 16>()),
		_vRegister(std::array<unsigned char, 16>()),
		drawFlag(false),
		gfx(std::array<unsigned char, 2048>()),
		key(std::array<unsigned char, 16>())
	{
		if (!runner)
		{
			std::cerr << "Error initializing runner properly! Exiting..." << std::endl;
			exit(3);
		}

		_delayTimer = 0;
		_soundTimer = 0;
		_audio = runner->getAudioService();
		_audio.lock()->initializeAudio();
		_console = NovelRT::LoggingService("CPU");
		_input = _runner->getInteractionService();
		
		//Zero out arrays
		_console.logInfoLine("Initializing Display...");
		for each (auto var in gfx)
		{
			var = 0;
		}
		_console.logInfoLine("Initializing Memory...");
		for each (auto var in _memory)
		{
			var = 0;
		}
		_console.logInfoLine("Initializing Input...");
		for each (auto var in key)
		{
			var = 0;
		}
		_console.logInfoLine("Adding Fontset...");
		for (int i = 0; i < 80; i++)
		{
			_memory[i] = _fontset[i];
		}

		generateBeep();

		_console.logInfoLine("CPU initialized.");
	};

	void CPU::cycleTimers()
	{
		//Decrement timers if it's been set
		if (_delayTimer > 0)
		{
			_delayTimer--;
		}
		if (_soundTimer > 0)
		{
			if (_soundTimer == 1)
			{
				beep();
			}
			_soundTimer--;
		}
	}

	void CPU::emulateCycle()
	{
		//Fetch
		unsigned short nextCounter = _programCounter + 1;
		_opcode = (_memory[_programCounter] << 8) | _memory[nextCounter];

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
			switch (_opcode & 0x000F)
			{
			case 0x0: op8xy0(); break;
			case 0x0001: op8xy1(); break;
			case 0x0002: op8xy2(); break;
			case 0x0003: op8xy3(); break;
			case 0x0004: op8xy4(); break;
			case 0x0005: op8xy5(); break;
			case 0x0006: op8xy6(); break;
			case 0x0007: op8xy7(); break;
			case 0x000E: op8xyE(); break;
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
			switch (_opcode & 0x00FF)
			{
			case 0x009E:opEx9E(); break;
			case 0x00A1:opExA1(); break;
			}
			break;
		}
		case 0xF:
		{
			switch (_opcode & 0x00FF)
			{
			case 0x0007: opFx07(); break;
			case 0x000A: opFx0A(); break;
			case 0x0015: opFx15(); break;
			case 0x0018: opFx18(); break;
			case 0x001E: opFx1E(); break;
			case 0x0029: opFx29(); break;
			case 0x0033: opFx33(); break;
			case 0x0055: opFx55(); break;
			case 0x0065: opFx65(); break;
			}
			break;
		}
		default:
		{
			std::stringstream output;
			output << "Opcode " << std::hex << _opcode << " unknown";
			_console.logWarningLine(output.str());
			break;
		}
		}
	}

	void CPU::loadProgram(std::string fileName)
	{
		std::stringstream loading;
		loading << "Loading " << fileName << "...";
		_console.logInfoLine(loading.str());
		FILE* pFile;
		fopen_s(&pFile, fileName.c_str(), "rb");
		_console.throwIfNullPtr(pFile, "Could not open file!");

		if (fileName == "")
		{
			_console.logErrorLine("No ROM provided!");
			throw std::runtime_error("No ROM provided!");
		}

		// Check file size
		fseek(pFile, 0, SEEK_END);
		long lSize = ftell(pFile);
		rewind(pFile);
		std::stringstream out;
		out << "Filesize: " << static_cast<int>(lSize);
		_console.logInfoLine(out.str());

		char* buffer = (char*)malloc(sizeof(char) * lSize);
		_console.throwIfNullPtr(buffer, "Unable to allocate memory for ROM!");
		
		size_t result = fread(buffer, 1, lSize, pFile);
		if (result != lSize)
		{
			_console.logErrorLine("Unable to read ROM!");
			throw std::runtime_error("Unable to read ROM!");
		}

		// Copy buffer to Chip8 memory
		if ((4096 - 512) > lSize)
		{
			auto inc = 0;
			for (int i = 0; i < lSize; ++i)
			{
				inc = i + 512;
				_memory[inc] = buffer[i];
			}
		}
		else
		{
			_console.logErrorLine("Error: ROM too big for memory");
			throw std::runtime_error("ROM too big for memory!");
		}
		
		fclose(pFile);
		free(buffer);
		_console.logInfoLine("ROM Loaded!");
	}

	void CPU::setKeys()
	{
		key[0] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::One));
		key[1] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::Two));
		key[2] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::Three));
		key[3] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::Four));
		key[4] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::Q));
		key[5] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::W));
		key[6] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::E));
		key[7] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::R));
		key[8] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::A));
		key[9] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::S));
		key[10] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::D));
		key[11] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::F));
		key[12] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::Z));
		key[13] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::X));
		key[14] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::C));
		key[15] = static_cast<unsigned char>(_input.lock()->getKeyState(NovelRT::Input::KeyCode::V));
	}

	//Defining Functions
	void CPU::op00E0()
	{
		//Clear Screen
		for (int c = 0; c < gfx.size(); c++)
		{
			gfx[c] = 0;
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
			_vRegister[0xF] = 0; //borrow
		}
		else
		{
			_vRegister[0xF] = 1;
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
		unsigned short h = (_opcode & 0x000F);
		unsigned short pixel;

		std::stringstream regHex;
		regHex << "DRW V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", V" << std::hex << ((_opcode & 0x00F0) >> 4);
		regHex << ", " << std:: hex << (_opcode & 0x000F);
		_console.logDebugLine(regHex.str());

		_vRegister[0xF] = 0;
		int mem;
		for (int yLine = 0; yLine < static_cast<int>(h); yLine++)
		{
			mem = _index + yLine;
			pixel = _memory[mem];

			for (int xLine = 0; xLine < 8; xLine++)
			{
				if ((pixel & (0x80 >> xLine)) != 0)
				{
					auto point = (x + xLine + ((y + yLine) * 64)) % 2048;
					if (gfx[point] == 1)
					{
						_vRegister[0xF] = 1;
					}
					gfx[point] ^= 1;
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
		_programCounter += 2;

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
		int indexOne = _index + 1;
		int indexTwo = _index + 2;
		_memory[_index] = _vRegister[(_opcode & 0x0F00) >> 8] / 100;
		_memory[indexOne] = (_vRegister[(_opcode & 0x0F00) >> 8] / 10) % 10;
		_memory[indexTwo] = (_vRegister[(_opcode & 0x0F00) >> 8] % 100) % 10;
		_programCounter += 2;
		
		std::stringstream regHex;
		regHex << "ADD B, V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());
	}

	void CPU::opFx55()
	{
		for (int i = 0; i <= ((_opcode & 0x0F00) >> 8); i++)
		{
			unsigned short var = _index + static_cast<unsigned short>(i);
			_memory[var] = _vRegister[i];
		}
		_index += ((_opcode & 0x0F00) >> 8) + 1u;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD [I], V" << std::hex << ((_opcode & 0x0F00) >> 8);
		_console.logDebugLine(regHex.str());

	}

	void CPU::opFx65()
	{
		for (int i = 0; i <= ((_opcode & 0x0F00) >> 8); i++)
		{
			unsigned short var = _index + static_cast<unsigned short>(i);
			_vRegister[i] = _memory[var];
		}
		_index += ((_opcode & 0x0F00) >> 8) + 1;
		_programCounter += 2;

		std::stringstream regHex;
		regHex << "LD V" << std::hex << ((_opcode & 0x0F00) >> 8);
		regHex << ", [I]";
		_console.logDebugLine(regHex.str());

	}

	void CPU::generateBeep()
	{
		if (!_audio.lock()->isInitialised)
		{
			_console.logInfoLine("BEEP");
			return;
		}

		//We're going to override NovelRT's audio implementation here
		//It's not suited for what I'm trying to do.
		
		alGenBuffers(1, &_buff);
		
		float frequency = 2000.0f;
		float seconds = 0.5f;
		float sampleRate = 44100.0f;
		auto size = seconds * sampleRate;
		ALsizei bufferSize = static_cast<ALsizei>(size);
		short* samples;
		samples = new short[bufferSize];
		for (int i = 0; i < bufferSize; i++)
		{
			samples[i] = static_cast<short>(32760 * std::sin((2.0f*float(3.14159265359)*frequency)/sampleRate * i));
		}

		alBufferData(_buff, AL_FORMAT_MONO16, samples, bufferSize, static_cast<ALsizei>(sampleRate));

		ALuint source = 0;
		alGenSources(1, &source);
		alSourcei(source, AL_BUFFER, _buff);
		_source = source;
	}

	void CPU::beep()
	{
		alSourcePlay(_source);
	}

	CPU::~CPU()
	{
		alSourcei(_source, AL_BUFFER, NULL);
		alDeleteBuffers(1, &_buff);
		alDeleteSources(1, &_source);
	}
};
