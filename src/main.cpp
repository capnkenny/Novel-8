//Ken Johnson (capnkenny) - 4/14/2020
//Based off of the CHIP-8 tutorial from multigesture.net

#include "../build/_deps/novelrt-src/include/NovelRT.h"
#include "CPU.h"
#include <iostream>

int main(int argc, char* argv[])
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
	
	std::filesystem::path executableDirPath = NovelRT::Utilities::Misc::getExecutableDirPath();
	std::filesystem::path romPath = executableDirPath / "roms";
	std::string fileName = romPath.string();
	fileName.append("/");
	fileName.append(argv[0]);

	auto runner = NovelRT::NovelRunner(0, "NovelChip8", 60U);
	auto render = runner.getRenderer();
	auto console = NovelRT::LoggingService(NovelRT::Utilities::Misc::CONSOLE_LOG_APP);
	
	console.logInfoLine("Initializing CHIP-8 CPU");
	auto cpu = Chip8::CPU(&runner);

	//setup gfx and input
	float screenH = 1080.0f;
	float screenW = 1920.0f;
	auto origin = NovelRT::Maths::GeoVector2<float>(screenW / 2, screenH / 2);
	
	//Generate Pixels
	auto pixelBlockX = screenW / 64;
	auto pixelBlockY = screenH / 32;
	auto incrementX = 30.0f;
	auto incrementY = 33.75f;
	

	//Black Background
	auto bkgdTransform = NovelRT::Transform(origin, 0, NovelRT::Maths::GeoVector2<float>(1920, 1080));
	auto bkgd = runner.getRenderer().lock()->createBasicFillRect(bkgdTransform, 3, NovelRT::Graphics::RGBAConfig(0,0,0,255));

	//Row Major
	std::array<std::array<std::unique_ptr<NovelRT::Graphics::BasicFillRect>, 64>,32> pixels = 
		std::array<std::array<std::unique_ptr<NovelRT::Graphics::BasicFillRect>, 64>, 32>();

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
			auto transform = NovelRT::Transform(pixelOrigin, 0, NovelRT::Maths::GeoVector2<float>(pixelBlockX, pixelBlockY));
			pixelsX[x] = render.lock()->createBasicFillRect(transform, 2, NovelRT::Graphics::RGBAConfig(255,255,255,255));
			incrementX += pixelBlockX;
			if (x != 0)
			{	//what a terrible hack
				pixelsX[x]->transform().position().setX(pixelsX[x]->transform().position().getX() - (pixelBlockX / 2));
			}
			pixelOrigin.setX(incrementX);
		}
		incrementY += pixelBlockY;
		pixels[(y-1)] = std::move(pixelsX);
	}
	
	pixels[0][0]->setColourConfig(NovelRT::Graphics::RGBAConfig(255, 0, 0, 255));
	
	cpu.loadProgram("stars.ch8");

	runner.Update += [&](NovelRT::Timing::Timestamp delta)
	{
		//cpu.emulateCycle();
	
		//cpu.setKeys();

		auto d = delta.getTicks();
		d += d;
		//update rendering
	};

	runner.SceneConstructionRequested += [&]
	{
		bkgd->executeObjectBehaviour();
		//myBasicFillRect->executeObjectBehaviour();
		
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
