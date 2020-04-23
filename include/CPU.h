//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "../build/_deps/novelrt-src/include/NovelRT.h"
#include <sstream>

namespace Chip8 {

	class CPU {

	private:
		std::weak_ptr<NovelRT::Audio::AudioService> _audio;
		ALuint _buff;
		NovelRT::LoggingService _console;
		unsigned char _delayTimer;
		unsigned short _index;
		std::weak_ptr<NovelRT::Input::InteractionService> _input;
		unsigned short _opcode;
		unsigned short _programCounter;
		NovelRT::NovelRunner* const _runner;
		unsigned char _soundTimer;
		ALuint _source;
		unsigned short _sp;

		
		std::array<unsigned char, 4096> _memory;
		std::array<unsigned short, 16> _stack;
		std::array<unsigned char, 16> _vRegister;

		unsigned char _fontset[80] =
		{
		  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		  0x20, 0x60, 0x20, 0x20, 0x70, // 1
		  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
		};

		void generateBeep();
		void beep();


	public:
		bool drawFlag;
		std::array<unsigned char, 2048> gfx;
		std::array<unsigned char, 16> key;
		
		CPU(NovelRT::NovelRunner* runner);
		~CPU();

		void cycleTimers();
		void emulateCycle();
		void loadProgram(std::string fileName);
		void setKeys();
		
		//Opcode Functions
		void op00E0();
		void op00EE();
		void op1nnn();
		void op2nnn();
		void op3xkk();
		void op4xkk();
		void op5xy0();
		void op6xkk();
		void op7xkk();
		void op8xy0();
		void op8xy1();
		void op8xy2();
		void op8xy3();
		void op8xy4();
		void op8xy5();
		void op8xy6();
		void op8xy7();
		void op8xyE();
		void op9xy0();
		void opAnnn();
		void opBnnn();
		void opCxkk();
		void opDxyn();
		void opEx9E();
		void opExA1();
		void opFx07();
		void opFx0A();
		void opFx15();
		void opFx18();
		void opFx1E();
		void opFx29();
		void opFx33();
		void opFx55();
		void opFx65();

	};
};