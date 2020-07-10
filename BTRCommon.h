#pragma once
#include <iostream>
//#include <direct.h>
#include "stb_image.h"
#include <SFML/Graphics.hpp>
#if __has_include("windows.h")
#include <windows.h>
#include <mmsystem.h>
#endif
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
#include <utility>
#include <functional>
#include <cmath>

/*#include "LuaBridge/LuaBridge.h"
#include "LuaBridge/Map.h"
#include "LuaBridge/UnorderedMap.h"
#include "LuaBridge/Vector.h"
#include "LuaBridge/List.h"*/
#include "SoundPlayback.h"
#include <cstring>
#include <SFML/Audio.hpp>
#include <SFML/OpenGL.hpp>
#include <bitset>
constexpr auto BTRWINDOWWIDTH = 640;
constexpr auto BTRWINDOWHEIGHT = 480;
constexpr auto pi = 3.14159265358979323846;
class BTRPaddle;
extern std::uniform_real_distribution<> dis;
extern std::random_device rd;
extern std::mt19937 gen;
extern int wallWidth;
extern int frameCnt;
extern int score;
extern int framesPassedlastPowerup;
extern int lives;
extern bool vertvelpowerup;
extern unsigned int devID;
std::string& GetCurPlayingFilename();
void PauseMidiPlayback();
void ContinueMidiPlayback();
void SelectMidiDevice(int selection);
void SelectMidiDevice();
inline sf::Color colors[2] = { sf::Color(252,128,0),sf::Color(255,255,0) };
// This section composes the engine part. Should be usable for "Adventures with Chickens" game.
inline void preprocess8bitpal(stbi_uc* pixels, int width, int height)
{
	int64_t totalPixels = (int64_t)width * height;
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
#if __cplusplus <= 201703L // Horrible hack.
namespace std {
	inline float lerp(float v0, float v1, float t) {
		return (1 - t) * v0 + t * v1;
	}
}
#endif
inline void preprocess8bitpalalpha(stbi_uc* pixels, int width, int height)
{
	int64_t totalPixels = width * (int64_t)height;
	for (int i = 0; i < totalPixels; i++)
	{
		if (pixels[i * 4] == pixels[i * 4 + 1]
			&& pixels[i * 4 + 1] == pixels[i * 4 + 2])
		{
			pixels[i * 4 + 3] = pixels[i * 4];
		}
	}
}
struct BTRsprite
{
	sf::Texture texture;
	sf::Sprite sprite;
	sf::IntRect intRect;
	int width, height;
	int realWidthPerTile, realHeightPerTile;
	int realnumofSprites;
	bool isVerticalFrame = false;
	int animFramePos = 0;
	int index = 0;
	int x = 0, y = 0; // positions in realnumOfFrames x realnumofSprites grids, starting at 0.
	int realnumOfFrames;
	BTRsprite(const char* filename, int numOfFrames, bool verticalFrame = false, int numofSprites = 2, bool alphaMap = false)
	{
		//std::cout << "Loading " << filename << "..." << std::endl;
		sprite = sf::Sprite();
		texture = sf::Texture();
		int n;
		int widthPerTile;
		auto retval = stbi_load(filename, &width, &height, &n, 4);
		if (!retval)
		{
			char* print = (char*)"Failed to load sprites from filename: ";
			throw std::runtime_error(strcat(print, filename));
		}
		int heightPerTile = height / numofSprites;
		texture.create(width, height);
		preprocess8bitpal(retval, width, height);
		if (alphaMap) preprocess8bitpalalpha(retval, width, height);
		texture.update(retval);
		free(retval);
		sprite.setTexture(texture);
		sprite.setPosition(sf::Vector2f(0, 0));
		widthPerTile = width / numOfFrames;
		sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(widthPerTile, heightPerTile)));
		realWidthPerTile = widthPerTile;
		realHeightPerTile = heightPerTile;
		isVerticalFrame = verticalFrame;
		realnumOfFrames = numOfFrames;
		realnumofSprites = numofSprites;
		texture.setRepeated(true);
	}
	inline void SetSpriteIndex(int index)
	{
		intRect = sf::IntRect(sf::Vector2i(realWidthPerTile * (animFramePos-1), realHeightPerTile * index), sf::Vector2i(realWidthPerTile, realHeightPerTile));
		y = index;
		return sprite.setTextureRect(intRect);
	}
	inline void SetTexRect(int x, int y)
	{
		intRect = sf::IntRect(sf::Vector2i(realWidthPerTile * x, realHeightPerTile * y), sf::Vector2i(realWidthPerTile, realHeightPerTile));
		this->x = x;
		this->y = y;
		sprite.setTextureRect(intRect);
	}
	operator sf::Sprite&()
	{
		return this->sprite;
	}
	operator sf::Sprite*()
	{
		return &this->sprite;
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
	enum class FontType
	{
		BTR_FONTSMALL,
		BTR_FONTLARGE,
		BTR_FONTLARGE2
	};
	char* bitmapChars = (char*)" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?.,''|-():;0123456789\0\0";
	FontType font = FontType::BTR_FONTSMALL;
	std::vector<BTRCharBitmap> bitmaps;
	std::unordered_map<unsigned char, BTRCharBitmap> charMaps;
	void RenderChars(std::string chars, sf::Vector2f pos, sf::RenderWindow* &window, sf::Color col = sf::Color(255,255,255,255))
	{
		sf::Sprite sprite;
		sprite.setTexture(fontImage, true);
		sprite.setPosition(pos);
		sprite.setColor(col);
		for (int i = 0; i < chars.size(); i++)
		{
			if (font == FontType::BTR_FONTLARGE && !std::isdigit(chars[i],std::locale(""))) continue;
			sf::Vector2i texPos;
			texPos.x = charMaps[chars[i]].x;
			texPos.y = charMaps[chars[i]].y;
			sf::Vector2i texWH;
			texWH.x = charMaps[chars[i]].width;
			texWH.y = charMaps[chars[i]].height;
			sf::IntRect rect = sf::IntRect(texPos,texWH);
			sprite.setTextureRect(rect);
			if (font == FontType::BTR_FONTLARGE2) fontImage.setSmooth(true);
			window->draw(sprite);
			sprite.move(sf::Vector2f(texWH.x, 0));
		}
		sprite.setColor(sf::Color(255, 255, 255, 255));
	}
	sf::Vector2f GetSizeOfText(std::string chars)
	{
		sf::Vector2f totalSize;
		for (int i = 0; i < chars.size(); i++)
		{
			if (font == FontType::BTR_FONTLARGE && !std::isdigit(chars[i], std::locale(""))) continue;
			sf::Vector2i texPos;
			texPos.x = charMaps[chars[i]].x;
			texPos.y = charMaps[chars[i]].y;
			sf::Vector2i texWH;
			texWH.x = charMaps[chars[i]].width;
			texWH.y = charMaps[chars[i]].height;
			totalSize.x += texWH.x;
		}
		totalSize.y = this->genCharHeight;
		return totalSize;
	}
	void RenderChars(std::string chars, float X, float Y, sf::RenderWindow* &window, sf::Color col = sf::Color(255, 255, 255, 255))
	{
		return RenderChars(chars, sf::Vector2f(X, Y), window, col);
	}
	BTRFont(const char* filename, FontType sFont = FontType::BTR_FONTSMALL)
	{
		font = sFont;
		char* bitmapCharsSmall = (char*)" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?.,''|-():;0123456789\0\0";
		char* bitmapCharsLarge = (char*)"0123456789";
		char* bitmapCharsLarge2 = (char*)" abcdefghijklmnopqrstuvwxyz";
		auto retval = stbi_load(filename, &width, &height, &n, 4);
		if (!retval)
		{
			char* print = (char*)"Failed to load bitmapped font from filename: ";
			throw std::runtime_error(strcat(print, filename));
		}
		preprocess8bitpal(retval, width, height);
		fontImage.create(width, height);
		fontImage.update(retval);
		free(retval);
		std::string defFilename = filename;
		defFilename.pop_back();
		defFilename.pop_back();
		defFilename.pop_back();
		defFilename += "txt";
		std::cout << "Font def file to open: " << defFilename << std::endl;
		std::ifstream file;
		file.open(defFilename);
		if (!file.is_open()) std::cout << "Font definition file not found" << std::endl;
		if (file.is_open())
		{
			std::string str;
			bool charMap = false;
			while (file >> str)
			{
//				file >> str;
				std::cout << "Read string: " << str << std::endl;
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
				if (font == FontType::BTR_FONTLARGE) std::cout << "character :" << *bitmapCharsLarge << std::endl;
				else if (font == FontType::BTR_FONTLARGE2) std::cout << "character :" << *bitmapCharsLarge2 << std::endl;
				if (font == FontType::BTR_FONTLARGE) bitmapDef.character = *bitmapCharsLarge++;
				else if (font == FontType::BTR_FONTLARGE2) bitmapDef.character = *bitmapCharsLarge2++;
				else bitmapDef.character = *bitmapCharsSmall++;
				charMaps.emplace(bitmapDef.character,bitmapDef);
				bitmaps.push_back(bitmapDef);
			}
			if (font == FontType::BTR_FONTSMALL) spacebetweenChars = charMaps['\''].width;
		}
		if (file.is_open()) file.close();
	}
};
// Gameplay related part.
struct BTRball;
struct BTRbrick;
struct BTRpowerup;
class BTRPaddle;
class BTRPlayArea;
template <typename T>
inline std::vector<sf::Vector2f> getNormals(T& obj)
{
	sf::Vector2f firstCorner = sf::Vector2f(obj.x, obj.y);
	sf::Vector2f secondCorner = sf::Vector2f(obj.x + obj.width, obj.y);
	sf::Vector2f thirdCorner = sf::Vector2f(obj.x, obj.y + obj.height);
	sf::Vector2f fourthCorner = sf::Vector2f(obj.x + obj.width, obj.y + obj.height);
	
	std::vector<sf::Vector2f> normals;
	std::vector<sf::Vector2f> cornerArrays = { firstCorner,secondCorner,thirdCorner,fourthCorner };
	for (int i = 0; i < cornerArrays.size(); i++)
	{
		auto& v = cornerArrays[i];
		auto next = i + 1;
		if (next >= cornerArrays.size()) next = 0;
		auto& nextVec = cornerArrays[next];
		auto edgeVec = nextVec - v;
		sf::Vector2f normVec = sf::Vector2f(edgeVec.x,edgeVec.y);
		std::cout << "Printing corners: x: "  << v.x << ", y: " << v.y << std::endl;
		normals.push_back(normVec);
	}
	return normals;
}

struct BTRObjectBase
{
	double x = 0, y = 0;
	double velX = 0, velY = 0;
	double oldx = x, oldy = y;
	double oldvelX = 0, oldvelY = 0;
	bool isFireball = false; // used for balls and bricks.
	int width = 0;
	uint32_t destroyed = false;
	int height = 0;
	double alpha = 1.0;
	double gravity = 0.25;
	virtual void Tick(BTRPlayArea& area)
	{

	}
	virtual ~BTRObjectBase() = default;
};
struct BTRSpark : BTRObjectBase
{
	int width = height = 3;
	sf::Color color = sf::Color(255, 255, 255, 255);
	sf::IntRect sparkRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(3, 3));
};
struct BTRExplodingBricks
{
	sf::Vector2f pos;
	sf::Sprite spr;
	int frameOffset = 0;
	int loop = 0;
};
extern std::vector<BTRExplodingBricks> explodingBricks;
extern std::vector<BTRSpark> sparks;
struct BTRpowerup;

class BTRPaddle
{
	public:
	double radiuses[5] = { 30,60,130,175,250 };
	double paddleRadius = 60;
	double drawPaddleRadius = 0;
	int curRadius = 1;
	int missilesLeft = 0;
	int tractorBeamPower = 240;
	enum PaddleStateFlags
	{
		PADDLE_MAGNET = 1 << 0,
		PADDLE_MISSILE = 1 << 1,
		PADDLE_TRACTOR = 1 << 2,
		PADDLE_BRICKFALL = 1 << 3
	};
	int stateFlags = 0;
	std::shared_ptr<BTRsprite> sprite;
	double lengthOfBall = 5;
	BTRPaddle()
	{
		sprite = std::shared_ptr<BTRsprite>(new BTRsprite("./ball/rockpaddle.png", 1, true, 32));
	}
};
struct BTRMissileObject;
struct BTRLevInfo
{
	unsigned char brickID;
	int x,y;
};
class BTRPlayArea
{
	public:
	enum BTRPlayAreaStates
	{
		AREA_NOGODOWN = 1 << 0,
		AREA_ENDOFGAME = 1 << 1,
	};
	std::vector<std::shared_ptr<BTRball>> balls;
	std::vector<BTRbrick> bricks;
	std::vector<sf::IntRect> brickTexRects;
	std::vector<BTRpowerup> powerups;
	std::vector<std::shared_ptr<BTRMissileObject>> missiles;
	sf::Texture brickTexture;
	std::string levelname;
	BTRPaddle paddle;
	int levnum;
	int framePassedLowBricks = 0;
	int rainBadPowerups = 0;
	int rainGoodPowerups = 0;
	int missileCooldown = 10;
	int levStateFlags = 0;
	int brickwidth,brickheight;
	bool randomPlay = false;
	// angle = 0;
	bool levelEnded = false;
	void Tick();
	void LostBall();
	void SpawnInitialBall();
	BTRPlayArea(std::string levfilename, sf::RenderWindow* window = nullptr);
	BTRPlayArea();
	void ExportBricks();
	void LoadBrickTex();
	void UpdateBrickGridPos();
};
struct BTRpowerup : BTRObjectBase
{
	int width = height = 32;
	int powerupID = 0;
	int aliveTick = 0;
	bool destroyed = false;
	BTRpowerup(sf::Vector2f pos, sf::Vector2f vel, int powerupID = 0)
	{
		this->x = pos.x;
		this->y = pos.y;
		this->velX = vel.x;
		this->velY = vel.y;
		this->powerupID = powerupID;
	}
	void Tick(BTRPlayArea& area) override;
	static void PowerupHandle(BTRPlayArea& area, int powerupID = 0);
};
struct BTRbrick : BTRObjectBase
{
	int width = 30;
	int height = 15;
	int hitsNeeded = 26;
	unsigned char brickID = 0;
	double hitvelX = 0, hitvelY = 0;
	bool explosionHit = false;
	bool explosionExpanded = false;
	bool goneThrough = false;
	int curXPos = 0,curYPos = 0;
	int hitTimes = 1;
	int collisionCooldown = 0;
	inline BTRbrick() = default;
	inline BTRbrick(const BTRLevInfo& levInfo) {x = levInfo.x; y = levInfo.y; brickID = levInfo.brickID;};
	bool BrickExistUnder(BTRPlayArea& area)
	{
		for (auto& curBrick : area.bricks)
		{
			if (curBrick.x == this->x)
			{
				if ((curBrick.curYPos - this->curYPos) == 1)
				{
					return true;
				}
			}
		}
		return false;
	}
	void ExplosiveBrickExpand(BTRPlayArea& area)
	{
		auto fireBrick = new BTRbrick();
		fireBrick->x = this->x - this->width;
		fireBrick->y = this->y;
		fireBrick->curYPos = this->curYPos;
		fireBrick->curXPos = this->curXPos - 1;
		fireBrick->brickID = 64;
		fireBrick->isFireball = 1;
		fireBrick->explosionExpanded = true;
		area.bricks.push_back(*fireBrick);
		fireBrick->x = this->x;
		fireBrick->y = this->y - this->height;
		fireBrick->curYPos = this->curYPos - 1;
		fireBrick->curXPos = this->curXPos;
		area.bricks.push_back(*fireBrick);
		fireBrick->y = this->y + this->height;
		fireBrick->curYPos = this->curYPos + 1;
		area.bricks.push_back(*fireBrick);
		fireBrick->y = this->y;
		fireBrick->curYPos = this->curYPos;
		fireBrick->x = this->x + this->width;
		fireBrick->curXPos = this->curXPos + 1;
		area.bricks.push_back(*fireBrick);
		delete fireBrick;
	}
	void Remove(BTRPlayArea& area)
	{
		if (framesPassedlastPowerup >= 40 * 5)
		{
			for (int iii = 0; iii < 30; iii++)
			{
				BTRSpark spark;
				spark.velX = dis(gen);
				spark.velY = dis(gen);
				spark.x = this->x + this->width * 0.5;
				spark.y = this->y + this->height * 0.5;
				spark.color = sf::Color(255, 255, 255);
				sparks.push_back(spark);
			}
			std::uniform_int_distribution<int> powerDist(0, 10);
			std::uniform_int_distribution<int> badpowerDist(16, 22);
			int pwrres = powerDist(gen);
			if (hitvelX == 0) hitvelX = dis(gen);
			if (!vertvelpowerup || hitvelY == 0) hitvelY = -5;
			auto liveres = gen() % 101;
			if (liveres <= 2)
			{
				pwrres = 15;
			}
			else if (liveres >= 55)
			{
				pwrres = badpowerDist(gen);
			}
			auto powerup = BTRpowerup(sf::Vector2f(this->x, this->y), sf::Vector2f(hitvelX, hitvelY), pwrres);
			BTRPlaySound("./ball/brickexplode.wav");
			area.powerups.push_back(powerup);
			framesPassedlastPowerup = 0;
		}
		if (this->brickID == 63)
		{
			for (int iii = 0; iii < 30; iii++)
			{
				BTRSpark spark;
				spark.velX = dis(gen);
				spark.velY = dis(gen);
				spark.x = this->x + this->width * 0.5;
				spark.y = this->y + this->height * 0.5;
				spark.color = sf::Color(255, 255, 255);
				sparks.push_back(spark);
			}
			BTRPlaySound("./ball/brickbreak.wav");
		}
		bool yelOrRed = 0;
		if (this->isFireball)
		{
			BTRPlaySound("./ball/explode.wav");
			for (int iii = 0; iii < 30; iii++)
			{
				BTRSpark spark;
				spark.velX = dis(gen);
				spark.velY = dis(gen);
				spark.x = this->x + this->width * 0.5;
				spark.y = this->y + this->height * 0.5;
				spark.color = colors[yelOrRed];
				yelOrRed ^= 1;
				sparks.push_back(spark);
				this->explosionHit = true;
			}
			for (int i = 0; i < area.bricks.size(); i++)
			{
				if (abs(area.bricks[i].x - this->x) <= 30
					&& abs(area.bricks[i].y - this->y) <= 15)
				{
					area.bricks[i].destroyed = true;
					area.bricks[i].explosionHit = true;
				}
			}
		}
	}
};
struct BTRball : BTRObjectBase
{
	int width = height = 9;
	bool split = false;
	bool goThrough = false;
	bool isFireball = false;
	bool ballHeld = false;
	bool invisibleSparkling = false;
	int offsetFromPaddle = 0;
	double angle = 0;
//	luabridge::LuaRef* ballRef;
	void Tick(BTRPlayArea& area) override;
	BTRball()
	{
//		ballRef = new luabridge::LuaRef(L, this);
	}
};

struct BTRExplosion : BTRExplodingBricks
{
	std::shared_ptr<BTRsprite> spr = std::shared_ptr<BTRsprite>(new BTRsprite("./ball/explosion.png",8,false,4));
	int framesPassed = 0;
	int spriteIndex = 0;
	bool destroyed = false;
	void Tick()
	{
		if (framesPassed > 6)
		{
			framesPassed = 0;
			spriteIndex++;
			if (spriteIndex > 3)
			{
				this->destroyed = true;
			}
		}
		framesPassed++;
	}
};
extern std::vector<BTRExplosion> explosions;
inline void vectorLerp(sf::Vector2f &pos, sf::Vector2f &toPos, double t)
{
	pos.x = std::lerp(pos.x, toPos.x, 0.5f);
	pos.y = std::lerp(pos.y, toPos.y, 0.5f);
}
template<typename T>
inline T spawnObject(sf::Vector2f pos, std::function<void(T&)> postSpawnFunc)
{
	T spawnObj = T();
	spawnObj.x = pos.x;
	spawnObj.y = pos.y;
	postSpawnFunc(spawnObj);
	return spawnObj;
}
template<typename T>
inline T* spawnObject(sf::Vector2f pos, std::function<void(T*)> postSpawnFunc)
{
	T* spawnObj = new T();
	spawnObj->x = pos.x;
	spawnObj->y = pos.y;
	postSpawnFunc(spawnObj);
	return spawnObj;
}
template<typename T>
inline std::shared_ptr<T> spawnObject(sf::Vector2f pos, std::function<void(std::shared_ptr<T>)> postSpawnFunc)
{
	std::shared_ptr<T> spawnObj = std::shared_ptr<T>(new T());
	spawnObj->x = pos.x;
	spawnObj->y = pos.y;
	postSpawnFunc(spawnObj);
	return spawnObj;
}

inline void spawnSpark(uint32_t count,sf::Color col,sf::Vector2f pos)
{
	for (int i = 0; i < count; i++)
	{
		BTRSpark spark;
		spark.velX = dis(gen);
		spark.velY = dis(gen);
		spark.x = pos.x;
		spark.y = pos.y;
		spark.color = col;
		sparks.push_back(spark);
	}
}

struct BTRMovingText
{
	sf::Vector2f pos;
	sf::Vector2f toPos;
	std::string movingText;
	void Tick()
	{
		vectorLerp(pos, toPos, 1 / 40.);
	}
};
struct BTRChompTeeth : BTRObjectBase
{
	bool flip = false;
	bool chompHard = false;
	void Tick(BTRPlayArea &area) override
	{
		x += 4;
		if (x + 32 > area.paddle.sprite->sprite.getPosition().x)
		{
			BTRPlaySound("./ball/chomp.wav");
			if (chompHard)
			{
				area.paddle.paddleRadius = area.paddle.drawPaddleRadius = area.paddle.radiuses[0];
				area.paddle.curRadius = 0;
			}
			else if (area.paddle.curRadius > 0) area.paddle.paddleRadius = area.paddle.drawPaddleRadius = area.paddle.radiuses[--area.paddle.curRadius];
			this->destroyed = true;
		}
	}
	virtual ~BTRChompTeeth() = default;
};
struct BTRMissileObject : BTRObjectBase
{
	int height = 32;
	int width = 20;
	void Tick(BTRPlayArea& area) override
	{
		velY -= gravity;
		y += velY;
		if (frameCnt % 5 == 0)
		{
			auto trailSpark = BTRSpark();
			trailSpark.color = sf::Color::Yellow;
			trailSpark.x = this->x + 8;
			trailSpark.y = this->y;
			sparks.push_back(trailSpark);
		}
		if (this->y + this->height < 0)
		{
			this->destroyed = true;
		}
		for (auto& curBrick : area.bricks)
		{
			if (this->x + this->width >= curBrick.x && this->x <= curBrick.x + curBrick.width
				&& this->y <= curBrick.y + curBrick.height && this->y + this->height / 2 >= curBrick.y)
			{
				this->destroyed = true;
				curBrick.hitvelX = 0;
				curBrick.hitvelY = this->velY;
				curBrick.destroyed = true;
				curBrick.isFireball = true;
				curBrick.explosionHit = true;
				auto expl = BTRExplosion();
				expl.pos.x = this->x - expl.spr->realWidthPerTile * 0.5;
				expl.pos.y = this->y - expl.spr->realHeightPerTile * 0.5;
				explosions.push_back(expl);
				gravity = 0;
				BTRPlaySound("./ball/missile.wav");
			}
		}
	}
};
extern BTRChompTeeth* chompteeth;
struct BTRButton
{
	sf::Vector2f pos;
	std::string str;
	bool wasHeld = false;
	bool smallButton = false;
	bool levEditOnly = false;
	std::function<void()> clickedFunc = [&,this]()
	{

	};
};
