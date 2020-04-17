// ConsoleApplication9.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "BTRCommon.h"
#include "SoundPlayback.h"
#include <random>


BTRsprite* ball = NULL;
//std::vector<BTRball*> balls;
std::vector<BTRSpark> sparks;
int wallWidth = 0;
int score = 0;
int frameCnt = 0;
int fireBrickFrame = 64;
std::chrono::steady_clock clocktime;
std::chrono::steady_clock::time_point now;
int64_t sec = 0;
extern void ParseMidsFile(std::string filename);
extern void CALLBACK MidiCallBack(HMIDIOUT hmo, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
extern void StartMidiPlayback();
extern bool eot;
HMIDISTRM midiDev;
HMIDIOUT midiOutDev;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(-5, 5);
BTRPaddle paddle;
bool fadeOut = false;
int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    InitOpenAL();
    atexit(&deInitOpenAL);
    ParseMidsFile("./ball/coolin.mds");
    std::thread thread = std::thread(StartMidiPlayback);
    std::cout.setf(std::ios_base::boolalpha);
    std::uniform_int_distribution<> dis(-5, 5);
    int x, y, n;
    auto retval = stbi_load("./ball/back.png", &x, &y, &n, 4);
    if (retval == NULL)
    {
        std::cerr << "Failed to load image: " << std::endl;
        std::cerr << stbi_failure_reason() << std::endl;
        return -1;
    }
    sf::Texture tex;
    tex.create(x, y);
    tex.update(retval);
    int wallHeight = 0;
    auto wallPixels = stbi_load("./ball/walls.png", &wallWidth, &wallHeight, &n, 4);
    if (wallPixels == NULL)
    {
        std::cerr << "Failed to load image: " << std::endl;
        std::cerr << stbi_failure_reason() << std::endl;
        return -1;
    }
    sf::Texture wallTex;
    wallTex.create(wallWidth, wallHeight);
    wallTex.update(wallPixels);
    sf::Sprite wallSprite;
    wallSprite.setPosition(sf::Vector2f(wallWidth / -2, 0));
    wallSprite.setTexture(wallTex);
    wallSprite.setScale(1, 24); 
    paddle.sprite = new BTRsprite("./ball/rockpaddle.png", 1, true, 32);
    ball = new BTRsprite("./ball/ball.png",32);
    sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn",sf::Style::Titlebar | sf::Style::Close);
    window->setFramerateLimit(40);
    now = clocktime.now();
    auto font = new BTRFont("./ball/fontcool.png");
    auto largeFont = new BTRFont("./ball/fontlarge.png", BTRFont::BTR_FONTLARGE);
    auto playArea = new BTRPlayArea("./lev/0.lev");
    double fade = 1.0;
    sf::RectangleShape scrRect(sf::Vector2f(BTRWINDOWWIDTH,BTRWINDOWHEIGHT));
    scrRect.setFillColor(sf::Color(0, 0, 0, 255));
    bool cursorVisible = true;
    auto cursor = new BTRsprite("./ball/cursor.png", 1, false, 1);
    auto sparkSprite = new BTRsprite("./ball/sparks.png", 15, false, 3);

    auto removeSparkObject = [](BTRSpark& spark)
    {
        if (spark.alpha <= 0)
        {
            return true;
        }
        return false;
    };
    sf::Texture windowTexture;
    windowTexture.create(BTRWINDOWWIDTH, BTRWINDOWHEIGHT);
    while (window->isOpen())
    {
        sf::Event event;
        while (window->pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window->close();
                break;
            case sf::Event::MouseEntered:
                cursorVisible = false;
                window->setMouseCursorVisible(false);
                break;
            case sf::Event::MouseLeft:
                cursorVisible = true;
                window->setMouseCursorVisible(true);
                break;
            }
        }
        if (fadeOut)
        {
            window->setFramerateLimit(60);
            double alpha = 1.0;
            int localFrameCnt = 0;
            sf::Sprite windowSprite;
            windowSprite.setTexture(windowTexture, true);
            while (localFrameCnt < 60)
            {
                windowSprite.setColor(sf::Color(255, 255, 255, 255 * alpha));
                window->clear();
                window->draw(windowSprite);
                window->display();
                alpha -= 1 / 60.;
                localFrameCnt++;
            }
            window->setFramerateLimit(40);
            fadeOut = false;
            alpha = 0;
            std::string str = "./lev/";
            str += std::to_string(playArea->levnum) + ".lev";
            std::cout << "Entering: " << str << std::endl;
            int oldLevnum = playArea->levnum;
            playArea = new BTRPlayArea(str);
            playArea->levelEnded = false;
            playArea->levnum = oldLevnum++;
        }
        sf::Sprite sprite;
        sprite.setTexture(tex, true);
        windowTexture.update(*window);
        window->clear();
        for (int posY = 0; posY < window->getSize().y; posY += y)
        for (int posX = 0; posX < window->getSize().x; posX += x)
        {
            sprite.setPosition(sf::Vector2f(posX, posY));
            window->draw(sprite);
        }
        wallSprite.setPosition(window->getSize().x - wallWidth / 2, 0);
        window->draw(wallSprite);
        wallSprite.setPosition(sf::Vector2f(wallWidth / -2, 0));
        window->draw(wallSprite);
        ball->Animate();
        playArea->Tick();
        if (fireBrickFrame >= 128) fireBrickFrame = 64;
        for (auto& curBrick : playArea->bricks)
        {
            sf::Sprite brickSprite;
            brickSprite.setTexture(playArea->brickTexture, true);
            brickSprite.setTextureRect(playArea->brickTexRects[curBrick.brickID - 1]);
            if (curBrick.isFireball)
            {
                brickSprite.setTextureRect(playArea->brickTexRects[fireBrickFrame]);
            }
            brickSprite.setPosition(sf::Vector2f(curBrick.x, curBrick.y));
            window->draw(brickSprite);
        }
        fireBrickFrame++;
        for (auto& curball : playArea->balls)
        {
            ball->sprite.setPosition(sf::Vector2f(curball->x, curball->y));
            window->draw(ball->sprite);
        }
        for (int i = 0; i < sparks.size(); i++)
        {
            if (sparks[i].sparkRect.left >= 15 * 3)
            {
                sparks.erase(sparks.begin() + i);
                continue;
            }
            auto curSpark = sparks[i];
            sparkSprite->sprite.setPosition(sparks[i].x, sparks[i].y);
            sparkSprite->sprite.setColor(sparks[i].color);
            if (frameCnt % 3 == 0) sparks[i].sparkRect.left += 3;
            sparkSprite->sprite.setTextureRect(curSpark.sparkRect);
            window->draw(sparkSprite->sprite);
            sparks[i].velY += sparks[i].gravity;
            sparks[i].y += sparks[i].velY;
            sparks[i].x += sparks[i].velX;
        }
        largeFont->RenderChars(std::to_string(score), sf::Vector2f(wallWidth / 2, 0), window);
        font->RenderChars("level " + std::to_string(playArea->levnum), sf::Vector2f(wallWidth /2, largeFont->genCharHeight), window);
        if (!cursorVisible)
        {
            ball->SetSpriteIndex(1);
            ball->sprite.setPosition((sf::Vector2f)sf::Mouse::getPosition(*window) - sf::Vector2f(ball->realWidthPerTile / 2, ball->realHeightPerTile / 2));
            cursor->sprite.setPosition((sf::Vector2f)sf::Mouse::getPosition(*window) - sf::Vector2f(cursor->width / 2, cursor->height / 2));
            window->draw(ball->sprite);
            window->draw(cursor->sprite);
            ball->SetSpriteIndex(0);
        }
        paddle.sprite->sprite.setPosition(std::clamp(cursor->sprite.getPosition().x,wallWidth / 2.f,BTRWINDOWWIDTH - wallWidth / 2.f - paddle.sprite->width),BTRWINDOWHEIGHT - 30);
        paddle.sprite->Animate();
        window->draw(paddle.sprite->sprite);
        scrRect.setFillColor(sf::Color(0, 0, 0, 255 * fade));
        if (fade > 0) fade -= 1 / (double)40;
        else fade = 0;
        window->draw(scrRect);
        window->display();
        frameCnt++;
    }
    eot = true;
    thread.join();
    midiOutReset(midiOutDev);
}
