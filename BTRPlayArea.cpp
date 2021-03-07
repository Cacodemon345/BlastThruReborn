#include "BTRCommon.h"
#include "BTRPlayArea.h"
extern int score;
extern bool fadeOut;
extern bool ballLost;

void BTRPlayArea::SpawnInitialBall()
{
	auto newBall = new BTRball;
	newBall->x = paddle.sprite->sprite.getPosition().x + paddle.paddleRadius / 2;
	newBall->y = BTRWINDOWHEIGHT - 30ll -paddle.sprite->realHeightPerTile;
	newBall->velX = -5;
	newBall->velY = 5;
	newBall->ballHeld = true;
	newBall->offsetFromPaddle = paddle.paddleRadius / 2;
	this->balls.push_back(std::shared_ptr<BTRball>(newBall));
}
void BTRPlayArea::LoadBrickTex()
{
	if (texLoaded) return;
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
		free(pixels);
		for (int ii = 0; ii < height / 15; ii++)
			for (int i = 0; i < width / 30; i++)
			{
				sf::IntRect rect = sf::IntRect(sf::Vector2i(i * 30, ii * 15), sf::Vector2i(30, 15));
				brickTexRects.push_back(rect);
			}
		brickwidth = width;
		brickheight = height;
		texLoaded = true;
	}
}

// Blast Thru Reborn level file format
// Header is: BTRLEV (no null char)
// Endianness follows immediately after the BTRLEV string (at 0x6, which is of sizeof(char))
// 0: Little-endian
// 1: Big-endian
// 2: PDP-endian
// The rest of the file follows the BTRLevInfo struct (documented in BTRCommon.h).

void BTRPlayArea::ExportBricks()
{
	auto levFile = std::ofstream("./lev/cust.btrlev");
	if (levFile.is_open())
	{
		levFile.write("BTRLEV",6);
		unsigned char endianness = 0;
		
			constexpr auto littleorder = 0x41424344UL;
			constexpr auto bigorder = 0x44434241UL;
			constexpr auto pdporder = 0x42414443UL;
			constexpr unsigned long endorder = 'ABCD';
			switch(endorder)
			{
				case littleorder:
				endianness = 0;
				break;
				case bigorder:
				endianness = 1;
				break;
				case pdporder:
				endianness = 2;
				break;
			}
			levFile.write((char*)&endianness,sizeof(char));
		
		for (auto curBrick : this->bricks)
		{
			//BTRLevInfo brickInfo;
			//brickInfo.x = (int)curBrick.x;
			//brickInfo.y = (int)curBrick.y;
			//brickInfo.brickID = curBrick.brickID;
			//levFile.write((char*)&brickInfo,sizeof(brickInfo));
			int curX = curBrick.x;
			int curY = curBrick.y;
			int curBrickID = curBrick.brickID;
			levFile.write((char*)&curBrickID,sizeof(curBrickID));
			levFile.write((char*)&curX,sizeof(curX));
			levFile.write((char*)&curY,sizeof(curY));
			
		}
		levFile.close();
	}
}
BTRPlayArea::BTRPlayArea(std::string levfilename, sf::RenderWindow* window)
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
		auto header = new char[6];
		file.read (header,6);		
		if (strncmp(header,"BTRLEV",6) == 0)
		{
			unsigned char endianness = 0;
			file.read((char*)&endianness,1);
			while(!file.eof())
			{
				BTRbrick curBrick;
				file.read((char*)&curBrick.brickID,sizeof(int));
				int x,y;
				file.read((char*)&x,sizeof(x));
				file.read((char*)&y,sizeof(y));
				curBrick.x = x;
				curBrick.y = y;
				bricks.push_back(curBrick);
			}
			LoadBrickTex();
			if (window != nullptr) paddle.sprite->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window));
			SpawnInitialBall();
			file.close();
			return;
		}
		file.seekg(0,std::ios::beg);
		while (!file.eof() && yPos <= 31)
		{
			char* brickBytes = new char[20];
			file.read(brickBytes, 20);
			for (int i = 0; i < 20; i++)
			{
				if (brickBytes[i] != 0 && brickBytes[i] != (char)255)
				{
					auto newBrick = BTRbrick();
					newBrick.x = newBrick.width * (double)i + wallWidth / 2.;
					newBrick.y = (double)yPos * newBrick.height;
					newBrick.curYPos = yPos;
					newBrick.curXPos = i;
					newBrick.brickID = brickBytes[i];
					if (brickBytes[i] == 64)
					{
						newBrick.isFireball = true;
					}
					bricks.push_back(newBrick);
				}
			}
			yPos++;
			delete[] brickBytes;
		}
		LoadBrickTex();
		if (window != nullptr) paddle.sprite->sprite.setPosition((btr::Vector2f)sf::Mouse::getPosition(*window));
		SpawnInitialBall();
	}
	else
	{
		throw std::runtime_error("Failed to load level");
	}
}
BTRPlayArea::BTRPlayArea()
{
	LoadBrickTex();
}
void BTRPlayArea::UpdateBrickGridPos()
{
	for (auto& curBrick : bricks)
	{
		auto curX = curBrick.x - wallWidth / 2;
		curBrick.curXPos = (int)curX % curBrick.width;
		curBrick.curYPos = (int)curBrick.y % curBrick.height;
	}
}
extern bool endofgame;
extern int64_t randPlayedSet;
void BTRPlayArea::Tick()
{
	for (auto& curball : balls)
	{
		/*luabridge::LuaRef func = luabridge::getGlobal(L,"tickBallHandlers");
		func(curball->ballRef);*/
		curball->Tick(*this);
	}
	if (!ballLost) for (auto& curpowerup : powerups)
	{
		curpowerup.Tick(*this);
	}
	for (auto& curMissile : missiles)
	{
		curMissile->Tick(*this);
	}
	for (int mit = 0; mit < missiles.size(); mit++)
	{
		if (missiles[mit]->destroyed)
		{
			missiles.erase(missiles.begin() + mit);
			continue;
		}
	}
	if (chompteeth) chompteeth->Tick(*this);
	if (chompteeth && chompteeth->destroyed)
	{
		delete chompteeth;
		chompteeth = 0;
	}
	bool noMoreDestroyed = true;
	if (btr::Mouse::isButtonPressed(sf::Mouse::Button::Left)
		&& paddle.missilesLeft
		&& (paddle.stateFlags & paddle.PADDLE_MISSILE)
		&& missileCooldown > 10)
	{
		auto missile = new BTRMissileObject;
		missile->x = paddle.sprite->sprite.getPosition().x;
		missile->y = paddle.sprite->sprite.getPosition().y;
		missiles.push_back(std::shared_ptr<BTRMissileObject>(missile));
		auto missile2 = new BTRMissileObject;
		missile2->x = paddle.sprite->sprite.getPosition().x + paddle.paddleRadius;
		missile2->y = paddle.sprite->sprite.getPosition().y;
		missiles.push_back(std::shared_ptr<BTRMissileObject>(missile2));
		paddle.missilesLeft--;
		if (paddle.missilesLeft == 0)
		{
			paddle.stateFlags &= ~paddle.PADDLE_MISSILE;
		}
		missileCooldown = 0;
		BTRPlaySound("./ball/missilelaunch.wav");
	}
	if (btr::Mouse::isButtonPressed(sf::Mouse::Button::Left))
	{
		for (auto &curBall : this->balls)
        {
            curBall->ballHeld = false;
        }
	}
	missileCooldown++;
	if (paddle.stateFlags & paddle.PADDLE_TRACTOR && btr::Mouse::isButtonPressed(sf::Mouse::Button::Left))
	{
		auto centerPaddle = paddle.sprite->sprite.getPosition().x + paddle.paddleRadius / 2;
		auto lengthOfBall = 15;
		for (auto& curBall : balls)
		{
			curBall->angle += 1;
			if (curBall->angle >= 180) curBall->angle = -curBall->angle;
			auto angleToPaddle = atan2(paddle.sprite->sprite.getPosition().y - curBall->y ,curBall->x - centerPaddle);
			auto offsetFromCenter = cos(curBall->angle * 180 / pi) * lengthOfBall;
			curBall->x = std::lerp(curBall->x, centerPaddle, 0.25);
			//curBall->velX = lerp(curBall->x,centerPaddle,0.5);
			curBall->velX = offsetFromCenter;
		}
		BTRPlaySound("./ball/tractorbeam.wav",0,0,true,true);
		paddle.tractorBeamPower--;
		if (paddle.tractorBeamPower <= 0) paddle.stateFlags &= ~BTRPaddle::PADDLE_TRACTOR;
	}
	horzPosOfBricks.clear();
	for (int i = 0; i < bricks.size(); i++)
	{
		if (horzPosOfBricks.find(bricks[i].x) == horzPosOfBricks.end())
		{
			horzPosOfBricks.insert(bricks[i].x);
		}
		if (bricks[i].collisionCooldown > 0)
		{
			bricks[i].collisionCooldown--;
		}
		if (bricks[i].destroyed) while(bricks[i].destroyed)
		{
			if (!bricks[i].explosionHit)
			{
				if ((bricks[i].brickID == 58 || bricks[i].brickID == 59))
				{
					BTRPlaySound("./ball/brickdecrement.wav");
					bricks[i].brickID--;
					bricks[i].destroyed = false;
					break;
				}
				if (bricks[i].brickID == 63)
				{
					for (int iii = 0; iii < 30; iii++)
					{
						BTRSpark spark;
						spark.velX = dis(gen);
						spark.velY = dis(gen);
						spark.x = bricks[i].x + bricks[i].width * 0.5;
						spark.y = bricks[i].y + bricks[i].height * 0.5;
						spark.color = sf::Color(255, 255, 255);
						sparks.push_back(spark);
					}
					BTRPlaySound("./ball/brickbreak.wav");
					bricks[i].brickID--;
					bricks[i].destroyed--;
					break;
				}
				if (bricks[i].brickID == 61 && bricks[i].hitsNeeded > 0)
				{
					BTRPlaySound("./ball/bricknobreak.wav");
					bricks[i].hitsNeeded--;
					bricks[i].destroyed--;
					break;
				}
				if (bricks[i].brickID == 60)
				{
					BTRPlaySound("./ball/brickdecrement.wav");
					bricks[i].brickID++;
					bricks[i].destroyed--;
					score += 5;
					break;
				}
			}
			noMoreDestroyed = false;
			bricks[i].Remove(*this);
			BTRPlaySound("./ball/brick.wav");
			if (bricks[i].explosionHit && !bricks[i].goneThrough)
			{
				BTRExplodingBricks explbrick;
				explbrick.pos.x = bricks[i].x;
				explbrick.pos.y = bricks[i].y;
				explbrick.spr.setPosition(explbrick.pos);
				
				explodingBricks.push_back(explbrick);
			}
			paddle.lengthOfBall *= 1.005;
			score += 5;
			if (bricks[i].isFireball || bricks[i].explosionHit) score += 5;
			bricks.erase(bricks.begin() + i);
			break;
		}
	}
	int bricksToExclude = 0;
	if (this->rainBadPowerups && (frameCnt % 15) == 0)
	{
		std::uniform_int_distribution badPowerDist(16, 21);
		std::uniform_real_distribution randomXPos(wallWidth / 2., BTRWINDOWWIDTH - 32. - wallWidth / 2.);
		auto badPowerup = BTRpowerup(btr::Vector2f(randomXPos(rd), 0), btr::Vector2f(0, 5), badPowerDist(rd));
		powerups.push_back(badPowerup);
		BTRPlaySound("./ball/rainpowerup.wav");
		rainBadPowerups--;
	}
	if (this->rainGoodPowerups && (frameCnt % 15) == 0)
	{
		std::uniform_int_distribution goodPowerDist(0, 10);
		std::uniform_real_distribution randomXPos(wallWidth / 2., BTRWINDOWWIDTH - 32. - wallWidth / 2.);
		auto goodPowerup = BTRpowerup(btr::Vector2f(randomXPos(rd), 0), btr::Vector2f(0, 5), goodPowerDist(rd));
		powerups.push_back(goodPowerup);
		BTRPlaySound("./ball/rainpowerup.wav");
		rainGoodPowerups--;
	}
	for (auto& curBrick : bricks)
	{
		if (curBrick.brickID == 63 || curBrick.brickID == 61) bricksToExclude++;
	}
	if ((bricks.size() - bricksToExclude) <= 2) framePassedLowBricks++;
	if (framePassedLowBricks >= 40 * 5)
	{
		auto explodingPowerup = BTRpowerup(btr::Vector2f(paddle.sprite->sprite.getPosition().x,0), btr::Vector2f(0, 5), 14);
		powerups.push_back(explodingPowerup);
		BTRPlaySound("./ball/rainpowerup.wav");
		framePassedLowBricks = 0;
	}
	if ((bricks.size() - bricksToExclude) <= 0)
	{
		fadeOut = levelEnded = true;
		ballLost = false;
		if ((!randomPlay && levnum == 40) || (this->levStateFlags & AREA_ENDOFGAME) || (randomPlay && randPlayedSet == 0xFFFFFFFFFFll))
		{
			endofgame = true;			
		}
	}
	for (int i = 0; i < balls.size(); i++)
	{
		if (balls[i]->destroyed == true) balls.erase(balls.begin() + i);
	}
	if (balls.size() == 0 && !ballLost)
	{
		LostBall();
	}
}
unsigned int BTRPlayArea::getClosestBall()
{
	std::sort(balls.begin(),balls.end(),[](const std::shared_ptr<BTRball>& lhs,
										   const std::shared_ptr<BTRball>& rhs)
										   {
											   return std::greater<decltype(lhs->y)>()(lhs->y,rhs->y);
										   });
	return 0;
}
void BTRPlayArea::LostBall()
{
	ballLost = true;
	this->rainBadPowerups = this->rainGoodPowerups = false;
	paddle.stateFlags = 0;
	paddle.missilesLeft = 0;
	for (int iii = 0; iii < 30; iii++)
	{
		BTRSpark spark;
		spark.velX = dis(gen);
		spark.velY = dis(gen);
		spark.x = paddle.sprite->sprite.getPosition().x + paddle.paddleRadius * 0.5;
		spark.y = paddle.sprite->sprite.getPosition().y + paddle.sprite->realHeightPerTile * 0.5;
		spark.color = sf::Color(252, 128, 0);
		sparks.push_back(spark);
	}
}
void DownBricks(BTRPlayArea& area)
{
	for (auto it = area.bricks.rbegin(); it != area.bricks.rend(); ++it)
	{
		if (it->curYPos < 23 && !it->BrickExistUnder(area))
		{
			it->curYPos++;
			it->y += it->height;
		}
	}
}
bool TestAABBOverlap(BTRObjectBase& a, BTRObjectBase& b)
{
	sf::FloatRect aRect(btr::Vector2f(a.x,a.y),btr::Vector2f(a.width,a.height));
	sf::FloatRect bRect(btr::Vector2f(b.x,b.y),btr::Vector2f(b.width,b.height));
	if (bRect.intersects(aRect))
	{
		return true;
	}
	return false;
}
void BTRball::Tick(BTRPlayArea &area)
{
	if (!ballHeld)
	{
		this->x += this->velX;
		this->y += this->velY;
	}
	else
	{
		this->x = (double)area.paddle.sprite->sprite.getPosition().x + this->offsetFromPaddle;
		this->y = (double)BTRWINDOWHEIGHT - 30 - area.paddle.sprite->realHeightPerTile;
	}
	if (this->x >= (double)BTRWINDOWWIDTH - wallWidth / 2 - width
		|| this->x <= 0 + wallWidth / 2)
	{
		BTRPlaySound("./ball/wall.wav");
		this->velX = -this->velX;
	}
	if (this->y <= 0)
	{
		BTRPlaySound("./ball/wall.wav");
		this->velY = -this->velY;
		this->y = 1;
	}
	if (this->y >= BTRWINDOWHEIGHT)
	{
		if (area.levStateFlags & area.AREA_NOGODOWN)
		{
			BTRPlaySound("./ball/wall.wav");
			this->velY = -this->velY;
		}
		else
		{
			BTRPlaySound("./ball/balldie.wav");
			this->destroyed = true;
		}
	}
	if (frameCnt % 10 == 0 && this->isFireball)
	{
		auto fireSpark = BTRSpark();
		fireSpark.x = this->x + this->width * 0.5;
		fireSpark.y = this->y + this->height * 0.5;
		fireSpark.color = sf::Color(252, 128, 0);
		sparks.push_back(fireSpark);
	}
	if (frameCnt % 3 == 0 && this->invisibleSparkling)
	{
		auto fireSpark = BTRSpark();
		fireSpark.x = this->x + this->width * 0.5;
		fireSpark.y = this->y + this->height * 0.5;
		fireSpark.color = sf::Color(255, 255, 255);
		sparks.push_back(fireSpark);
	}
	this->x = std::clamp(this->x, wallWidth / 2., BTRWINDOWWIDTH - wallWidth / 2. - width);
	this->velX = std::clamp(this->velX, (double)-30, (double)30);
	this->velY = std::clamp(this->velY, (double)-15, (double)15);
	area.paddle.lengthOfBall = std::clamp(area.paddle.lengthOfBall, 5., 25.);
	bool velXReversed = false,velYReversed = false;
	for (int i = 0; i < area.bricks.size(); i++)
	{
		if (this->x + this->width >= area.bricks[i].x
			&& this->x <= area.bricks[i].x + area.bricks[i].width
			&& this->y + this->height >= area.bricks[i].y
			&& this->y <= area.bricks[i].y + area.bricks[i].height
			&& area.bricks[i].collisionCooldown <= 0)
		{
			area.bricks[i].hitvelX = this->velX;
			area.bricks[i].hitvelY = this->velY;
			if (!this->goThrough)
			{
				this->x -= this->velX;
				this->y -= this->velY;
				auto thishalfWidthX = this->x + this->width * 0.5;
				auto thishalfWidthY = this->y + this->height * 0.5;
				auto curbrickhalfWidthX = area.bricks[i].x + area.bricks[i].width * 0.5;
				auto curbrickhalfWidthY = area.bricks[i].y + area.bricks[i].height * 0.5;
				auto res = atan2(thishalfWidthY - curbrickhalfWidthY, thishalfWidthX - curbrickhalfWidthX) * 180 / pi;
				//std::cout << "Res in degress: " << res << std::endl;
				int cornerHit = 0;
				bool fallbackToAngle = 1;
				if (fallbackToAngle)
				{
					if (res >= -45 && res <= 45)
					{
						this->velX = -this->velX;
						cornerHit++;
						velXReversed = true;
					}
					if (res >= 45 && res <= 135)
					{
						this->velY = -this->velY;
						cornerHit++;
						velYReversed = true;
					}
					if (res >= 135 || res <= -135)
					{
						this->velX = -this->velX;
						cornerHit++;
						velXReversed = true;
					}
					if (res <= -45 && res >= -135)
					{
						this->velY = -this->velY;
						cornerHit++;
						velYReversed = true;
					}
					if (cornerHit >= 2)
					{
						this->x += velX;
						this->y += velY;
						//std::cout << "Corner hit" << std::endl;
					}
				}
			}
			if (this->goThrough)
			{
				area.bricks[i].explosionHit = true;
				if (!this->isFireball && !area.bricks[i].isFireball) area.bricks[i].goneThrough = true;
			}
			if (this->isFireball)
			{
				area.bricks[i].isFireball = true;
				area.bricks[i].explosionHit = true;
				auto expl = BTRExplosion();
				expl.pos.x = this->x - expl.spr->realWidthPerTile * 0.5;
				expl.pos.y = this->y - expl.spr->realHeightPerTile * 0.5;
				explosions.push_back(expl);
				int debrisCnt = 0;
				while (debrisCnt++ < 4)
				{
					auto addAngle = std::uniform_real_distribution<double>(0.,pi)(gen);
					auto debris = BTRDebris(btr::Vector2f(this->x,this->y),area.brickTexture,area.brickTexRects[area.bricks[i].brickID - 1], btr::Vector2f(this->velX * (velXReversed ? -1 : 1) * 0.75 + cos(addAngle),this->velY * (velYReversed ? -1 : 1) * 0.75 + sin(addAngle)));
					area.debrisObjects.push_back(debris);
				}
			}
			area.bricks[i].destroyed++;
			area.bricks[i].hitTimes++;
			area.bricks[i].collisionCooldown = 2;
			score += (area.paddle.lengthOfBall / 5 - 1) * 5;
		}
	}
	if (this->y + this->height >= area.paddle.sprite->sprite.getPosition().y
		&& this->x + this->width >= area.paddle.sprite->sprite.getPosition().x
		&& this->x <= area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius)
	{
		BTRPlaySound(area.paddle.stateFlags & BTRPaddle::PADDLE_MAGNET ? "./ball/ballhold.wav" : "./ball/paddle.wav");
		this->y = std::clamp(this->y,0.,BTRWINDOWWIDTH - 40.);
		long double lengthFactor = 1.0;
		auto length = sqrt(this->velX * this->velX + this->velY * this->velY);
		double angle = atan2((this->y + this->height * 0.5 - this->velY) - (area.paddle.sprite->sprite.getPosition().y + area.paddle.sprite->realHeightPerTile * 0.5),
							 (this->x + this->width * 0.5 - this->velX) - (area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius * 0.5)) * 180 / pi;
		if (this->x <= area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius / 2)
		{	
			if (this->x <= (double)area.paddle.sprite->sprite.getPosition().x + 8.)
			{
				angle = (-90 - 60) * pi / 180;
				lengthFactor = 0.75;
				BTRPlaySound("./ball/paddleedge.wav");
			}
			else if (this->x <= area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius / 2 - (area.paddle.paddleRadius * 0.25)) angle = (-90 - 40) * pi / 180;
			else angle = (-90 - 20) * pi / 180;
		}
		else if (this->x > area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius / 2)
		{
			if (this->x > area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius - 8)
			{
				angle = (-90 + 60) * pi / 180;
				lengthFactor = 0.75;
				BTRPlaySound("./ball/paddleedge.wav");
			}
			else if (this->x > area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius / 2 + (area.paddle.paddleRadius * 0.25)) angle = (-90 + 40) * pi / 180;
			else angle = (-90 + 20) * pi / 180;
		}
		this->velX = area.paddle.lengthOfBall * lengthFactor * cos(angle);
		this->velY = area.paddle.lengthOfBall * lengthFactor * sin(angle);
		if (area.paddle.lengthOfBall >= 15)
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
			BTRPlaySound("./ball/paddlefast.wav");
		}
		if (area.paddle.stateFlags & BTRPaddle::PADDLE_MAGNET) 
		{
			this->ballHeld = true;
			this->offsetFromPaddle = this->x - area.paddle.sprite->sprite.getPosition().x;
		}
		if (area.paddle.stateFlags & BTRPaddle::PADDLE_BRICKFALL)
		{
			DownBricks(area);
			BTRPlaySound("./ball/down.wav");
		}
	}
}
void BTRpowerup::PowerupHandle(BTRPlayArea& area, int powerupID)
{
	score += 85;
	std::string filename = "./ball/powerup.wav";
	if (powerupID > 15) filename = "./ball/powerupbad.wav";
	switch (powerupID)
	{
	default:
		std::cout << "Unknown Powerup ID " << powerupID << std::endl;
		break;
	case 0:
		if (area.paddle.curRadius > 3)
		{
			break;
		}
		area.paddle.paddleRadius = area.paddle.radiuses[++area.paddle.curRadius];
		filename = "./ball/grow.wav";
		break;
	case 1:
	{
		for (int i = 0; i < area.balls.size(); i++) if (!area.balls[i]->split)
		{
			auto newBall = new BTRball(*area.balls[i]);
			newBall->split = true;
			newBall->velX = -area.balls[i]->velX;
			newBall->angle = std::uniform_int_distribution(0, 360)(gen);
			area.balls.push_back(std::shared_ptr<BTRball>(newBall));
		}
		for (auto& curBall : area.balls)
		{
			curBall->split = false;
		}
		//BTRPlaySound("./ball/splitball.wav");
		filename = "./ball/splitball.wav";
		break;
	}
	case 2:
		{
			//area.UpdateBrickGridPos();
			std::vector<btr::Vector2f> explExpandedPositions;
			for (int brickidx = 0; brickidx < area.bricks.size(); brickidx++)
			{
				if (area.bricks[brickidx].brickID == 64 && !area.bricks[brickidx].explosionExpanded)
				{
					area.bricks[brickidx].ExplosiveBrickExpand(area);
				}
			}
			//area.UpdateBrickGridPos();
			for (auto& curBrick : area.bricks)
			{
				if (curBrick.explosionExpanded) explExpandedPositions.push_back(btr::Vector2f(curBrick.curXPos,curBrick.curYPos));

			}
			std::sort(area.bricks.begin(), area.bricks.end(), [](BTRbrick& curBrick, BTRbrick& curBrick2)
					  {
						  return std::less<int64_t>()(20ll * curBrick.curYPos + curBrick.curXPos, 20ll * curBrick2.curYPos + curBrick2.curXPos);
					  });
			for (int i = 0; i < area.bricks.size(); i++)
			{
				for (auto& curExplExpnPos : explExpandedPositions)
					if (!area.bricks[i].explosionExpanded //&& !area.bricks[i].isFireball
						&& area.bricks[i].curXPos == curExplExpnPos.x && area.bricks[i].curYPos == curExplExpnPos.y)
					{
						area.bricks.erase(area.bricks.begin() + i);
					}
			}
			for (auto& curBrick : area.bricks)
			{
				curBrick.explosionExpanded = false;
			}
			//area.UpdateBrickGridPos();
			/*std::sort(area.bricks.begin(), area.bricks.end(), [](BTRbrick& curBrick,BTRbrick& curBrick2)
					  {
						  return std::less<unsigned char>()(curBrick.brickID, curBrick2.brickID);
					  });*/
			break;
		}
	case 3:
		area.paddle.stateFlags |= BTRPaddle::PADDLE_TRACTOR;
		area.paddle.tractorBeamPower = 240;
		area.paddle.missilesLeft = 0;
		break;
	case 4:
		for (auto& curBall : area.balls)
		{
			curBall->goThrough = true;
		}
		break;
	case 5:
		for (auto& curBall : area.balls)
		{
			auto lengthOfBall = 5;
			auto angle = atan2(curBall->velY, curBall->velX);
			curBall->velX = lengthOfBall * cos(angle);
			curBall->velY = lengthOfBall * sin(angle);
		}
		area.paddle.lengthOfBall = 5;
		filename = "./ball/slower.wav";
		break;
	case 6:
		for (auto& curBall : area.balls)
		{
			curBall->isFireball = true;
			curBall->invisibleSparkling = false;
		}
		break;
	case 14:
		for (auto& curBrick : area.bricks)
		{
			curBrick.isFireball = true;
			auto expl = BTRExplosion();
			expl.pos.x = curBrick.x - expl.spr->realWidthPerTile * 0.5;
			expl.pos.y = curBrick.y - expl.spr->realHeightPerTile * 0.5;
			explosions.push_back(expl);
		}
	case 7:
		for (auto& curBrick : area.bricks) if (curBrick.isFireball)
		{
			curBrick.destroyed = true;
			curBrick.explosionHit = true;
		}
		break;
	case 8:
		PowerupHandle(area, 1);
		for (int i = 0; i < area.balls.size(); i++) if (!area.balls[i]->split)
		{
			auto newBall = new BTRball(*area.balls[i]);
			newBall->split = true;
			newBall->velX = -area.balls[i]->velX;
			newBall->velY = -area.balls[i]->velY;
			newBall->angle = std::uniform_int_distribution(0, 360)(gen);
			area.balls.push_back(std::shared_ptr<BTRball>(newBall));
		}
		for (auto& curBall : area.balls)
		{
			curBall->split = false;
		}
		//BTRPlaySound("./ball/splitball.wav");
		filename = "./ball/splitball.wav";
		break;
	case 9:
		area.paddle.stateFlags |= BTRPaddle::PADDLE_MAGNET;
		break;
	case 10:
		area.paddle.stateFlags |= BTRPaddle::PADDLE_MISSILE;
		area.paddle.missilesLeft = 20;
		area.paddle.tractorBeamPower = 0;
		break;
	case 12:
		area.rainBadPowerups = 16;
	case 11:
		area.rainGoodPowerups = 16;
		break;
	case 15:
		lives++;
		area.paddle.stateFlags = 0;
		filename = "./ball/halelujah.wav";
		break;
	case 17:
	case 16:
		chompteeth = new BTRChompTeeth;
		if (powerupID != 17) break;
		chompteeth->chompHard = true;
		break;
	case 18:
		area.balls.clear();
		area.LostBall();
		break;
	case 19:
		area.paddle.lengthOfBall *= 1.25;
		for (auto& curBall : area.balls)
		{
			auto lengthOfBall = sqrt(curBall->velX * curBall->velX + curBall->velY * curBall->velY) * 1.25;
			auto angle = atan2(curBall->velY, curBall->velX);
			curBall->velX = lengthOfBall * cos(angle);
			curBall->velY = lengthOfBall * sin(angle);
			filename = "./ball/faster.wav";
		}
		break;
	case 20:
		area.paddle.stateFlags |= area.paddle.PADDLE_BRICKFALL;
		break;
	case 21:
		for (auto& curBall : area.balls)
		{
			curBall->isFireball = false;
			curBall->invisibleSparkling = 1;
		}
		break;
	case 22:
		area.rainBadPowerups = 16;
		break;
	case 30:
		area.levStateFlags |= BTRPlayArea::AREA_NOGODOWN;
		filename = (char*)"./ball/powerup.wav";
		break;
	case 31:
		area.levStateFlags |= BTRPlayArea::AREA_ENDOFGAME;
		filename = (char*)"./ball/powerup.wav";
		break;
	}
	BTRPlaySound(filename);
}
void BTRpowerup::Tick(BTRPlayArea &area)
{
	aliveTick++;
	this->x += this->velX;
	this->y += this->velY;
	this->velY += this->gravity;
	if (this->x >= (double)BTRWINDOWWIDTH - wallWidth / 2. - width
		|| this->x <= 0 + wallWidth / 2)
	{
		BTRPlaySound("./ball/powerupbounce.wav");
		this->velX = -this->velX;
		this->x = std::clamp(this->x, wallWidth / 2., BTRWINDOWWIDTH - wallWidth / 2. - width);
	}
	if (this->y <= 0)
	{
		BTRPlaySound("./ball/powerupbounce.wav");
		this->velY = -this->velY;
		this->y = 1;
	}
	else if (this->y >= BTRWINDOWHEIGHT)
	{
		destroyed = true;
	}
	if (this->y + this->height >= area.paddle.sprite->sprite.getPosition().y
		&& this->x + this->width >= area.paddle.sprite->sprite.getPosition().x
		&& this->x <= area.paddle.sprite->sprite.getPosition().x + area.paddle.paddleRadius)
	{
		PowerupHandle(area, this->powerupID);
		sf::Color col = this->powerupID > 15 ? sf::Color(255, 0, 0) : sf::Color(0, 0, 255);
		spawnSpark(20, col, btr::Vector2f(this->x, this->y));
		destroyed = true;
	}
}
