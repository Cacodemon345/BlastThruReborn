#pragma once
#include <iostream>
#include <direct.h>
#include "stb_image.h"
#include <SFML/Graphics.hpp>
#include <windows.h>
#include <mmsystem.h>
#include <sndfile.hh>
#include <AL/al.h>
#include <AL/alc.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <numeric>
#include <cstdlib>
#include <random>
constexpr auto BTRWINDOWWIDTH = 640;
constexpr auto BTRWINDOWHEIGHT = 480;
constexpr auto pi = 3.14159265358979323846;
class BTRPaddle;
extern std::uniform_int_distribution<> dis;
extern std::random_device rd;
extern std::mt19937 gen;
extern int wallWidth;
extern int frameCnt;
// This section composes the engine part. Should be usable for "Adventures with Chickens" game.
extern void BTRPlaySound(const char* filename);
extern BTRPaddle paddle;
extern int score;

inline void preprocess8bitpal(stbi_uc* pixels, int width, int height)
{
	int64_t totalPixels = width * height;
	for (int i = 0; i < totalPixels; i++)
	{
		if (pixels[i * 4] == 0
			&& pixels[i * 4 + 1] == 0
			&& pixels[i * 4 + 2] == 0)
		{
			pixels[i * 4 + 3] = 0;
		}
	}
}
struct BTRsprite
{
	sf::Texture texture;
	sf::Sprite sprite;
	int width, height;
	int realWidthPerTile, realHeightPerTile;
	int realnumofSprites;
	bool isVerticalFrame = false;
	int animFramePos = 0;
	int index = 0;
	int realnumOfFrames;
	BTRsprite(const char* filename, int numOfFrames, bool verticalFrame = false, int numofSprites = 2)
	{
		sprite = sf::Sprite();
		texture = sf::Texture();
		int n;
		int widthPerTile;
		auto retval = stbi_load(filename, &width, &height, &n, 4);
		if (!retval)
		{
			char* print = (char*)"Failed to load sprites from filename: ";
			throw std::exception(strcat(print, filename));
		}
		int heightPerTile = height / numofSprites;
		texture.create(width, height);
		preprocess8bitpal(retval, width, height);
		texture.update(retval);
		sprite.setTexture(texture);
		sprite.setPosition(sf::Vector2f(0, 0));
		widthPerTile = width / numOfFrames;
		sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(widthPerTile, heightPerTile)));
		realWidthPerTile = widthPerTile;
		realHeightPerTile = heightPerTile;
		isVerticalFrame = verticalFrame;
		realnumOfFrames = numOfFrames;
		realnumofSprites = numofSprites;
	}
	inline void SetSpriteIndex(int index)
	{
		auto intRect = sf::IntRect(sf::Vector2i(realWidthPerTile * (animFramePos-1), realHeightPerTile * index), sf::Vector2i(realWidthPerTile, realHeightPerTile));
		return sprite.setTextureRect(intRect);
	}
	void Animate()
	{
		if (isVerticalFrame)
		{
			if (animFramePos >= realnumofSprites)
			{
				animFramePos = 0;
			}
		}
		else if (animFramePos >= realnumOfFrames)
		{
			animFramePos = 0;
		}
		sf::IntRect intRect;
		if (!isVerticalFrame)
			intRect = sf::IntRect(sf::Vector2i(realWidthPerTile * animFramePos, 0), sf::Vector2i(realWidthPerTile, realHeightPerTile));
		else
		{
			intRect = sf::IntRect(sf::Vector2i(0, realHeightPerTile * animFramePos), sf::Vector2i(realWidthPerTile, realHeightPerTile));
		}
		sprite.setTextureRect(intRect);
		animFramePos++;
	}
};


struct BTRCharBitmap
{
	unsigned char character;
	int x, y, width, height;
};
struct BTRFont
{
	sf::Texture fontImage;
	int width, height, n;
	int genCharHeight = 0;
	int spacebetweenChars = 0;
	enum FontType
	{
		BTR_FONTSMALL,
		BTR_FONTLARGE,
	};
	char* bitmapChars;
	FontType font = BTR_FONTSMALL;
	std::vector<BTRCharBitmap> bitmaps;
	std::unordered_map<unsigned char, BTRCharBitmap> charMaps;
	void RenderChars(std::string chars, sf::Vector2f pos, sf::RenderWindow* &window)
	{
		sf::Sprite sprite;
		sprite.setTexture(fontImage, true);
		sprite.setPosition(pos);
		for (int i = 0; i < chars.size(); i++)
		{
			if (font == BTR_FONTLARGE && !std::isdigit(chars[i],std::locale(""))) continue;
			sf::Vector2i texPos;
			texPos.x = charMaps[chars[i]].x;
			texPos.y = charMaps[chars[i]].y;
			sf::Vector2i texWH;
			texWH.x = charMaps[chars[i]].width;
			texWH.y = charMaps[chars[i]].height;
			sf::IntRect rect = sf::IntRect(texPos,texWH);
			sprite.setTextureRect(rect);
			window->draw(sprite);
			sprite.move(sf::Vector2f(texWH.x, 0));
		}
	}
	void RenderChars(std::string chars, float X, float Y, sf::RenderWindow* &window)
	{
		return RenderChars(chars, sf::Vector2f(X, Y), window);
	}
	BTRFont(const char* filename, FontType sFont = BTR_FONTSMALL)
	{
		font = sFont;
		char* bitmapCharsSmall = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?.,''|-():;0123456789\0\0";
		char* bitmapCharsLarge = "0123456789";
		auto retval = stbi_load(filename, &width, &height, &n, 4);
		if (!retval)
		{
			char* print = (char*)"Failed to load bitmapped font from filename: ";
			throw std::exception(strcat(print, filename));
		}
		preprocess8bitpal(retval, width, height);
		fontImage.create(width, height);
		fontImage.update(retval);
		std::string defFilename = filename;
		defFilename.pop_back();
		defFilename.pop_back();
		defFilename.pop_back();
		defFilename += "txt";
		std::fstream file;
		file.open(defFilename, std::ios::in | std::ios::out | std::fstream::app);
		if (!file.is_open()) std::cout << "Font definition file not found" << std::endl;
		if (file.is_open())
		{
			std::string str;
			bool charMap = false;
			while (!file.eof())
			{
				file >> str;
				if (str == "Character_Mapping")
				{
					file >> genCharHeight;
					std::cout << "General Character Height: " << genCharHeight << std::endl;
					int devnull;
					file >> devnull;
					charMap ^= 1;
					break;
				}
			}
			while (!file.eof() && charMap)
			{
				auto bitmapDef = BTRCharBitmap();
				std::cout << "Creating BTRCharBitmap with: ";
				file >> bitmapDef.x;
				std::cout << "x :" << bitmapDef.x << ", ";
				file >> bitmapDef.y;
				std::cout << "y :" << bitmapDef.y << ", ";
				file >> bitmapDef.width;
				std::cout << "width :" << bitmapDef.width << ", ";
				bitmapDef.height = genCharHeight;
				if (font == BTR_FONTLARGE) std::cout << "character :" << *bitmapCharsLarge << std::endl;
				else std::cout << "character :" << *bitmapCharsSmall << std::endl;
				if (font == BTR_FONTLARGE) bitmapDef.character = *bitmapCharsLarge++;
				else bitmapDef.character = *bitmapCharsSmall++;
				charMaps.emplace(bitmapDef.character,bitmapDef);
				bitmaps.push_back(bitmapDef);
			}
			if (font == BTR_FONTSMALL) spacebetweenChars = charMaps['\''].width;
		}
	}
};
// Gameplay related part.
struct BTRball;
struct BTRbrick;

class BTRPlayArea
{
	public:
	std::vector<BTRball*> balls;
	std::vector<BTRbrick> bricks;
	std::vector<std::vector<BTRbrick>> newbricks;
	std::vector<sf::IntRect> brickTexRects;
	sf::Texture brickTexture;
	std::string levelname;
	int levnum;
	bool levelEnded = false;
	void Tick();
	BTRPlayArea(std::string levfilename);
};
struct BTRObjectBase
{
	double x = 0, y = 0;
	double velX = 0, velY = 0;
	double oldx = x, oldy = y;
	double oldvelX = 0, oldvelY = 0;
	bool isFireball; // used for balls and bricks.
	int width = 0;
	bool destroyed = false;
	int height = 0;
	double alpha = 1.0;
	double gravity = 0.25;
	virtual void Tick(BTRPlayArea& area)
	{

	}
};
struct BTRSpark : BTRObjectBase
{
	int width = height = 3;
	sf::Color color = sf::Color(255, 255, 255, 255);
	sf::IntRect sparkRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(3, 3));
};
extern std::vector<BTRSpark> sparks;
struct BTRbrick : BTRObjectBase
{
	int width = 30;
	int height = 15;
	unsigned char brickID = 0;
	void Remove(BTRPlayArea& area)
	{
		if (this->isFireball)
		{
			for (int iii = 0; iii < 30; iii++)
			{
				BTRSpark spark;
				spark.velX = dis(gen);
				spark.velY = dis(gen);
				spark.x = this->x + this->width * 0.5;
				spark.y = this->y + this->height * 0.5;
				spark.color = sf::Color(252, 128, 0);
				sparks.push_back(spark);
			}
			for (int i = 0; i < area.bricks.size(); i++)
			{
				if (abs(area.bricks[i].x - this->x) <= 30
					&& abs(area.bricks[i].y - this->y) <= 15)
				{
					area.bricks[i].destroyed = true;
				}
			}
		}
	}
};
struct BTRpowerup : BTRObjectBase
{
	int width = height = 32;
	void Tick(BTRPlayArea& area) override
	{

	}
};
class BTRPaddle
{
	public:
	double radiuses[5] = { 30,60,130,175,250 };
	double paddleRadius = 30;
	enum PaddleStateFlags
	{
		PADDLE_MAGNET = 1 << 0,
		PADDLE_MISSILE = 1 << 1,
		PADDLE_TRACTOR = 1 << 2
	};
	int stateFlags = 0;
	BTRsprite* sprite;
};
struct BTRball : BTRObjectBase
{
	void Tick(BTRPlayArea& area) override
	{
		this->x += this->velX;
		this->y += this->velY;
		if (this->x >= BTRWINDOWWIDTH - wallWidth / 2 - width
			|| this->x <= 0 + wallWidth / 2)
		{
			BTRPlaySound("./ball/wall.wav");
			this->velX = -this->velX;
		}
		if (this->y >= BTRWINDOWHEIGHT
			|| this->y <= 0)
		{
			BTRPlaySound("./ball/wall.wav");
			this->velY = -this->velY;
		}
		this->x = std::clamp(this->x, wallWidth / 2., BTRWINDOWWIDTH - wallWidth / 2. - width);
		for (int i = 0; i < area.bricks.size(); i++)
		{
			if (this->x + this->width >= area.bricks[i].x
				&& this->x <= area.bricks[i].x + area.bricks[i].width
				&& this->y + this->height >= area.bricks[i].y
				&& this->y <= area.bricks[i].y + area.bricks[i].height)
			{
				this->x -= this->velX;
				this->y -= this->velY;
				auto thishalfWidthX = this->x + this->width * 0.5;
				auto thishalfWidthY = this->y + this->height * 0.5;
				auto curbrickhalfWidthX = area.bricks[i].x + area.bricks[i].width * 0.5;
				auto curbrickhalfWidthY = area.bricks[i].y + area.bricks[i].height * 0.5;
				auto res = atan2(thishalfWidthY - curbrickhalfWidthY, thishalfWidthX - curbrickhalfWidthX) * 180 / pi;
				//std::cout << "Res in degress: " << res << std::endl;
				if (res >= -45 && res <= 45)
				{
					this->velX = -this->velX;
				}
				if (res >= 45 && res <= 135)
				{
					this->velY = -this->velY;
				}
				if (res >= 135 || res <= -135)
				{
					this->velX = -this->velX;
				}
				if (res <= -45 && res >= -135)
				{
					this->velY = -this->velY;
				}
				area.bricks[i].destroyed = true;
			}
		}
		if (this->y + this->height >= paddle.sprite->sprite.getPosition().y
			&& this->x + this->width >= paddle.sprite->sprite.getPosition().x
			&& this->x <= paddle.sprite->sprite.getPosition().x + paddle.sprite->width)
		{
			BTRPlaySound("./ball/paddle.wav");
			auto length = sqrt(this->velX * this->velX + this->velY * this->velY);
			double angle;
			if (this->x <= paddle.sprite->sprite.getPosition().x + paddle.sprite->width / 2) angle = (-90 - 30) * pi / 180;
			else if (this->x > paddle.sprite->sprite.getPosition().x + paddle.sprite->width / 2) angle = (-90 + 30) * pi / 180;
			this->velX = length * cos(angle);
			this->velY = length * sin(angle);
		}
	}
};



