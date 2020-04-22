//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "../build/_deps/novelrt-src/include/NovelRT.h"
#include "CPU.h"
#include <iostream>

int main(int argc, char* /*argv[]*/)
{

	if (argc > 1)
	{
		std::cerr << "Too many arguments! Quitting..." << std::endl;
		//exit(2);
	}
	else if (argc < 1)
	{
		std::cerr << "Need ROM argument to launch properly. Quitting..." << std::endl;
		//exit(-1);
	}
	
	//std::string fileName = romPath.string();
	//fileName.append("\\");
	//fileName.append(argv[0]);

	auto runner = NovelRT::NovelRunner(0, "NovelChip-8", 60U);
	auto render = runner.getRenderer();
	auto console = NovelRT::LoggingService(NovelRT::Utilities::Misc::CONSOLE_LOG_APP);
	auto timer = NovelRT::Timing::StepTimer();
	auto input = runner.getInteractionService();
	
	console.logInfoLine("Initializing CHIP-8 CPU");
	auto cpu = Chip8::CPU(&runner);

	//setup gfx and input
	float screenH = 1080.0f;
	float screenW = 1920.0f;
	auto origin = NovelRT::Maths::GeoVector2<float>(screenW / 2, screenH / 2);
	
	//Get Pixel and Increment Dimensions
	auto pixelWidth = screenW / 64;
	auto pixelHeight = screenH / 32;
	auto incrementX = 30.0f;
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
		pixels[(y-1)] = std::move(pixelsX);
	}
	
	//fileName.append("stars.ch8");
	cpu.loadProgram("C:\\roms\\PONG");
	
	int cyclesPerUpdate = 540 / 60;
	//cpu.stepMode = true;
	runner.Update += [&](NovelRT::Timing::Timestamp delta)
	{
		//Just to get rid of error of unused vars
		auto d = delta.getTicks();
		d += d;

		if (cpu.stepMode)
		{
			if (input.lock()->getKeyState(NovelRT::Input::KeyCode::Spacebar) == NovelRT::Input::KeyState::KeyDown)
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
						/*if (pixelRow >= 32)
						{
							pixelRow = 31;
						}*/
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
				//Update timers on a 60Hz frequency / 60fps = once per update
				cpu.cycleTimers();
			}
		}
		else
		{
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
						/*if (pixelRow >= 32)
						{
							pixelRow = 31;
						}*/
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
		}
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
