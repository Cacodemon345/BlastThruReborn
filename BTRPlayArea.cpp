#include "BTRCommon.h"
extern int score;
extern bool fadeOut;
BTRPlayArea::BTRPlayArea(std::string levfilename)
{
	auto file = std::fstream(levfilename, std::ios::in | std::ios::binary);
	if (file.is_open())
	{
		int yPos = 0;
		levelname = levfilename;
		levelname.resize(levelname.size() - 4);
		char character = levelname[levelname.size() - 1];
		levnum = '0' - character;
		levnum += 1;
		while (!file.eof() && yPos <= 31)
		{
			char* brickBytes = new char[20];
			file.read(brickBytes, 20);
			for (int i = 0; i < 20; i++)
			{
				if (brickBytes[i] != 0 && brickBytes[i] != 255)
				{
					auto newBrick = BTRbrick();
					newBrick.x = newBrick.width * i + wallWidth / 2;
					newBrick.y = yPos * newBrick.height;
					newBrick.brickID = brickBytes[i];
					if (brickBytes[i] == 64)
					{
						newBrick.isFireball = true;
					}
					bricks.push_back(newBrick);
				}
			}
			yPos++;
		}
		int width, height, n;
		auto pixels = stbi_load("./ball/bricks.png", &width, &height, &n, 4);
		if (!pixels)
		{
			std::cerr << "Failed to load ./ball/bricks.png" << std::endl;
		}
		else
		{
			preprocess8bitpal(pixels, width, height);
			brickTexture.create(width, height);
			brickTexture.update(pixels);
			for (int ii = 0; ii < height / 15; ii++)
				for (int i = 0; i < width / 30; i++)
				{
					sf::IntRect rect = sf::IntRect(sf::Vector2i(i * 30, ii * 15), sf::Vector2i(30, 15));
					brickTexRects.push_back(rect);
				}
		}
		auto newBall = new BTRball;
		newBall->x = paddle.sprite->sprite.getPosition().x + paddle.sprite->width;
		newBall->y = BTRWINDOWHEIGHT - 30 - paddle.sprite->realHeightPerTile;
		newBall->velX = 5;
		newBall->velY = dis(gen);
		newBall->width = 9;
		newBall->height = 9;
		this->balls.push_back(newBall);
	}
	else
	{
		throw std::exception("Failed to load level");
	}
}

void BTRPlayArea::Tick()
{
	for (auto& curball : balls)
	{
		curball->Tick(*this);
	}
	bool noMoreDestroyed = true;
	for (int i = 0; i < bricks.size(); i++)
	{
		if (bricks[i].destroyed)
		{
			noMoreDestroyed = false;
			bricks[i].Remove(*this);
			BTRPlaySound("./ball/brick.wav");
			bricks.erase(bricks.begin() + i);
			score += 5;
		}
	}
	if (bricks.size() == 0 && levelEnded == false)
	{
		fadeOut = levelEnded = true;
	}
}
