//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "../build/_deps/novelrt-src/include/NovelRT.h"
#include "CPU.h"
#include <iostream>

int main(int argc, char* argv[])
{

#ifdef _DEBUG
	//Use this for debugging the emulator
	std::string fileName = "C:\\roms\\PONG";
	argc += argc;
	std::cout << argv[0] << std::endl;
#else
	if (argc > 2)
	{
		std::cerr << "Too many arguments! Quitting..." << std::endl;
		exit(2);
	}
	else if (argc == 1)
	{
		std::cout << "NovelCHIP-8! by capnkenny" << std::endl;
		std::cout << "CHIP-8 Emulator Demo made with NovelRT" << std::endl << std::endl;
		std::cout << "Usage: chip8.exe [Path to ROM]" << std::endl << std::endl;
		exit(1);
	}
	else if (argc < 1)
	{
		std::cerr << "Need ROM argument to launch properly. Quitting..." << std::endl;
		exit(-1);
	}
	
	std::string fileName = argv[1];
#endif
	//Setting CPU to cycle at 540MHz, @ 60fps
	int cyclesPerUpdate = 540 / 60;

	auto runner = NovelRT::NovelRunner(0, "NovelCHIP-8", 60U);
	auto render = runner.getRenderer();
	auto input = runner.getInteractionService();
	auto console = NovelRT::LoggingService(NovelRT::Utilities::Misc::CONSOLE_LOG_APP);
	
	console.logInfoLine("Initializing CHIP-8 CPU...");
	auto cpu = Chip8::CPU(&runner);

	//Setup gfx and input
	float screenH = 1080.0f;
	float screenW = 1920.0f;
	auto origin = NovelRT::Maths::GeoVector2<float>(screenW / 2, screenH / 2);
	
	//Get Pixel and Increment Dimensions
	auto pixelWidth = screenW / 64;			
	auto pixelHeight = screenH / 32;
	auto incrementX = 30.0f;			//X and Y work off of midpoints
	auto incrementY = 33.75f;
	
	//Black Background - NovelRT generates blue by default, so we cover it with a black one.
	auto bkgdTransform = NovelRT::Transform(origin, 0, NovelRT::Maths::GeoVector2<float>(1920, 1080));
	auto bkgd = runner.getRenderer().lock()->createBasicFillRect(bkgdTransform, 3, NovelRT::Graphics::RGBAConfig(0,0,0,255));

	//Row Major
	std::array<std::array<std::unique_ptr<NovelRT::Graphics::BasicFillRect>, 64>,32> pixels = 
		std::array<std::array<std::unique_ptr<NovelRT::Graphics::BasicFillRect>, 64>, 32>();

	//Create pixels in 2D array
	for (int y = 1; y <= 32; y++)
	{
		auto pixelsX = std::array<std::unique_ptr<NovelRT::Graphics::BasicFillRect>, 64>();
		auto pixelOrigin = NovelRT::Maths::GeoVector2<float>();
		if (y == 1)
		{
			pixelOrigin = NovelRT::Maths::GeoVector2<float>(incrementX / 2, (incrementY / 2));
		}
		else
		{
			pixelOrigin = NovelRT::Maths::GeoVector2<float>(incrementX / 2, incrementY);
		}
		for (int x = 0; x < 64; x++)
		{
			auto transform = NovelRT::Transform(pixelOrigin, 0, NovelRT::Maths::GeoVector2<float>(pixelWidth, pixelHeight));
			pixelsX[x] = render.lock()->createBasicFillRect(transform, 2, NovelRT::Graphics::RGBAConfig(255,255,255,0));
			incrementX += pixelWidth;
			//Shift the pixels into alignment with the screen
			if (x != 0)
			{
				pixelsX[x]->transform().position().setX(pixelsX[x]->transform().position().getX() - (pixelWidth / 2));
			}
			if (y != 1)
			{
				pixelsX[x]->transform().position().setY(pixelsX[x]->transform().position().getY() - (pixelHeight / 2));
			}
			pixelOrigin.setX(incrementX);
		}
		incrementX = 30.0f;
		incrementY += pixelHeight;
		auto point = y - 1;
		pixels[point] = std::move(pixelsX);
	}

	cpu.loadProgram(fileName);
	
	//To prevent unused variable errors, this is used for the delta in the update loop.
	uint64_t d;

	runner.Update += [&](NovelRT::Timing::Timestamp delta)
	{
		//Just to get rid of error of unused vars
		d = delta.getTicks();
		
		for (int i = 0; i < cyclesPerUpdate; i++)
		{
			cpu.emulateCycle();
			cpu.setKeys();

			int pixelRow = 0;
			int pixelColumn = 0;
			//Following row major as it's 64*32
			if (cpu.drawFlag)
			{
				cpu.drawFlag = false;
				for (int x = 0; x < 2048; x++)
				{
					if ((x % 64 == 0) && (x != 0))
					{
						pixelRow++;
					}
					if (cpu.gfx[x] > 0)
					{
						pixels[pixelRow][pixelColumn]->setColourConfig(NovelRT::Graphics::RGBAConfig(255, 255, 255, 255));
					}
					else
					{
						pixels[pixelRow][pixelColumn]->setColourConfig(NovelRT::Graphics::RGBAConfig(255, 255, 255, 0));
					}
					pixelColumn++;
					if (pixelColumn >= 64)
					{
						pixelColumn = 0;
					}
				}
			}
		}
			//Update timers on a 60Hz frequency / 60fps = once per update
			cpu.cycleTimers();
	};

	runner.SceneConstructionRequested += [&]
	{
		bkgd->executeObjectBehaviour();
		
		for (int i = 0; i < pixels.size(); i++)
		{
			for (int j = 0; j < pixels[i].size(); j++)
			{
				pixels[i][j]->executeObjectBehaviour();
			}
		}
	};

	runner.runNovel();

}
