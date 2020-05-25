// ConsoleApplication9.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#if !defined(SFML_STATIC)
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "BTRCommon.h"
#include "SoundPlayback.h"
#include <random>
#include "SimpleIni.h"
int lives = 2;
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
extern void StartMidiPlayback();
#ifdef WIN32
extern void CALLBACK MidiCallBack(HMIDIOUT hmo, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
extern HMIDISTRM midiDev;
HMIDIOUT midiOutDev;
#endif // WIN32

extern bool eot;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(-5, 5);
bool fadeOut = false;
bool ballLost = false;
std::vector<BTRExplodingBricks> explodingBricks;
std::vector<BTRExplosion> explosions;
std::vector<BTRButton> btns;
int framesPassedlastPowerup;
std::thread* thread = NULL;
std::vector<std::pair<std::string, int>> scoresAndNames;
BTRChompTeeth* chompteeth = NULL;
BTRPlayArea* playArea = NULL;
std::vector<unsigned char> randPlayedLevels;

bool sortScoreList(const std::pair<std::string, int> t1, const std::pair<std::string, int> t2)
{
    if (std::get<1>(t1) > std::get<1>(t2)) return true;
    return false;
}
extern void StopMidiPlayback();
extern std::string& GetCurPlayingFilename();
unsigned int devID = 0;
void loadMusic(std::string mdsfilename)
{
    if (GetCurPlayingFilename() == mdsfilename)
    {
        return; // Let it continue;
    }
    if (thread)
    {
        eot = true;
        if (thread->joinable()) thread->join();
        eot = false;
        delete thread;
    }
#if defined(WIN32)
    // Completely close the stream and then reopen to avoid weird bugs.
    midiStreamStop(midiDev);
    midiOutReset((HMIDIOUT)midiDev);
    midiStreamClose(midiDev);
    //unsigned int devID = 0;
    midiStreamOpen(&midiDev, &devID, 1, 0, 0, 0);
#endif
    ParseMidsFile(mdsfilename);
    thread = new std::thread(StartMidiPlayback);
}
extern void DownBricks(BTRPlayArea& area);
//using namespace luabridge;
auto ActivatePowerup(int powerupID)
{
    return BTRpowerup::PowerupHandle(*playArea, powerupID);
}
const std::string gammaShaderCode =

"#version 120"\
"\n"\
""\
"uniform sampler2D texture;" \
"uniform float gamma; " \
"void main()"\
"{" \
" vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);" \
" gl_FragColor = pow(pixel, vec4(vec3(1.0f / gamma),1.0) );" \
"}";
bool firstRun = true;
int main()
{
    SelectMidiDevice();
    InitOpenAL();
    std::map<std::string, int> cheatKeys =
    {
        {"inc me",14},
        {"let me hear you say fired up",6},
        {"my velcro shoes",3},
        {"no ska no swing",9},
        {"not a small thing",0},
        {"sidewinders",10},
        {"detrimental abundancy",22},
        {"mixed blessings",12},
        {"beneficial abundancy",11},
        {"through holy faith",15},
        {"sweet",8},
        {"funyons",30}
    };
    sf::Shader gammaShader;
    auto loaded = gammaShader.loadFromMemory(gammaShaderCode, sf::Shader::Fragment);
    if (!loaded)
    {
        std::cout << "Loading gamma shader failed" << std::endl;
    }
#if !defined(_HAS_CXX20)
    //static_assert(false, "C++20 support not available");
#endif
    /*    L = lua_open();
        luaL_openlibs(L);
        getGlobalNamespace(L)
            .beginNamespace("registeredBallTicks")
            .endNamespace()
            .beginClass<BTRball>("BTRball")
                .addProperty("x", &BTRball::x)
                .addProperty("y", &BTRball::y)
                .addProperty("velX",&BTRball::velX)
                .addProperty("velY",&BTRball::velY)
                .addProperty("isFireball",&BTRball::isFireball)
                .addProperty("invisibleSparkling",&BTRball::invisibleSparkling)
            .endClass()
            .beginClass<BTRPaddle>("BTRpaddle")
            .endClass()
            .beginNamespace("audio")
                .addFunction("PlaySound",&BTRPlaySound)
            .endNamespace();

        luaL_dostring(L, R"(
        function registerBallTickHandler(func)
            table.insert(registeredBallTicks,func);
        end
        function tickBallHandlers(ball)
            for i,v in ipairs(registeredBallTicks) do
                v(ball);
            end
        end
        )");*/

    std::vector<std::string> musics = { "./ball/coolin.mds","./ball/samba.mds","./ball/piano.mds","./ball/rush.mds" };
    std::cout.setf(std::ios_base::boolalpha);
    //for (int i = 0; i < 19937 / 8 * 4; i++) gen.seed(rd());
    std::uniform_int_distribution<> dis(-5, 5);
    std::uniform_int_distribution<> fuzzrand(0, 2);
    std::uniform_int_distribution<> mdsrand(0, musics.size() - 1);
    CSimpleIniA ini;
    ini.SetUnicode(true);
    auto inierr = ini.LoadFile("./bt.ini");
    if (!(inierr < 0))
    {
        auto section = ini.GetSection("bt.ini");
        std::string scorestr;
        std::string sectionName = "score0";
        while (sectionName != "score1:")
        {
            scorestr = ini.GetValue("bt.ini", sectionName.c_str());
            std::string name;
            auto res = scorestr.find_first_of(';');
            if (res == std::string::npos)
            {
                std::cout << "Invalid score string encountered!" << std::endl;
            }
            else
            {
                name = scorestr.substr(0, res);
                auto scoreint = std::stoi(scorestr.substr(res + 1, scorestr.size() - res));
                std::cout << sectionName << ": " << name << ", " << scoreint << std::endl;
                scoresAndNames.push_back(std::make_pair(name, scoreint));
            };
            if (sectionName == "score9")
            {
                sectionName.pop_back();
                sectionName += "10";
            }
            else sectionName[sectionName.size() - 1]++;
        }
        std::sort(scoresAndNames.begin(), scoresAndNames.end(), &sortScoreList);
    }

    unsigned int devID = 0;
#if defined(WIN32)
    //midiStreamOpen(&midiDev, &devID, 1, 0, 0, 0);
#endif
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
    free(retval);
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
    wallTex.setSmooth(1);
    free(wallPixels);
    sf::Sprite wallSprite;
    wallSprite.setPosition(sf::Vector2f(wallWidth / -2, 0));
    wallSprite.setTexture(wallTex);
    wallSprite.setScale(1, 24);

    ball = new BTRsprite("./ball/ball.png", 32);
    auto loadSplashSprite = new BTRsprite("./art/romtech.png",1,false,1);
    sf::Texture windowTexture;
    windowTexture.create(BTRWINDOWWIDTH, BTRWINDOWHEIGHT);
    sf::Sprite windowSprite;
    windowSprite.setTexture(windowTexture, true);
    sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn", sf::Style::Titlebar | sf::Style::Close);
    window->setFramerateLimit(40);
    window->clear();
    window->draw(*loadSplashSprite);
    windowTexture.update(*window);
    window->display();        
    auto fadeToColor = [&](sf::Color fadeColor)
    {
        uint32_t framerate = 60;
#ifdef WIN32
        framerate = GetDeviceCaps(GetDC(window->getSystemHandle()), VREFRESH);
#endif
        window->setFramerateLimit(framerate);
        double alpha = 1.0;
        int localFrameCnt = 0;

        while (localFrameCnt < framerate)
        {
            if (fadeColor == sf::Color(0, 0, 0, 255))
            {
                gammaShader.setUniform("texture", windowTexture);
                gammaShader.setUniform("gamma", (float)alpha);
                window->clear();
                window->draw(windowSprite, &gammaShader);
                window->display();
                alpha -= 1.f / (float)framerate;
                localFrameCnt++;
                continue;
            }
            windowSprite.setColor(sf::Color(255, 255, 255, 255 * alpha));
            window->clear(fadeColor);
            window->draw(windowSprite);
            window->display();
            alpha -= 1. / framerate;
            localFrameCnt++;
        }
        window->setFramerateLimit(40);
    };
    BTRPlaySound("./sound/intro.wav");
    window->requestFocus();
    window->pollEvent(sf::Event());
    std::this_thread::sleep_for(std::chrono::seconds(3));
    fadeToColor(sf::Color(0, 0, 0, 254));
    
    BTRPlaySound("./ball/grow.wav");
    window->setFramerateLimit(40);
    //loadMusic(musics[mdsrand(gen)]);


    now = clocktime.now();
    
    auto font = new BTRFont("./ball/fontcool.png");
    auto largeFont = new BTRFont("./ball/fontlarge.png", BTRFont::BTR_FONTLARGE);
    playArea = new BTRPlayArea("./lev/0.lev");
    //playArea->paddle.sprite = new BTRsprite("./ball/rockpaddle.png", 1, true, 32);
    double fade = 1.0;
    sf::RectangleShape scrRect(sf::Vector2f(BTRWINDOWWIDTH, BTRWINDOWHEIGHT));
    scrRect.setFillColor(sf::Color(0, 0, 0, 255));
    bool cursorVisible = true;
    auto cursor = new BTRsprite("./ball/cursor.png", 1, false, 1);
    auto sparkSprite = new BTRsprite("./ball/sparks.png", 15, false, 3);
    auto powerupSprite = new BTRsprite("./ball/powerup.png", 16, false, 2);
    auto pausedSprite = new BTRsprite("./ball/paused.png", 1, false, 1);
    auto magnetSprite = new BTRsprite("./ball/magnet.png", 16, false, 4);
    auto tractorSprite = new BTRsprite("./ball/power.png", 1, 0);
    tractorSprite->SetSpriteIndex(1);
    pausedSprite->SetTexRect(0, 0);
    powerupSprite->Animate();
    framesPassedlastPowerup = 40 * 5;
    bool paused = false;
    bool highScore = false;
    bool cheatText = false;
    bool exitingFromHighScore = false;
    bool menu = false;
    int magnetHeldBall = false;
    bool drawCornerText = 1;
    int cornerTextBlinkTime = 0;
    std::string textToDraw = "Welcome to Blast Thru Reborn!";
    std::string cheatstr = "";

    menu = true;
    paused = true;
    PauseMidiPlayback();
    BTRPlaySound("./sound/menu.wav", 1, 0, 1);
    auto DrawBackground = [&]()
    {
        sf::Sprite sprite;
        sprite.setTexture(tex, true);

        window->clear();
        for (int posY = 0; posY < window->getSize().y; posY += y)
            for (int posX = 0; posX < window->getSize().x; posX += x)
            {
                sprite.setPosition(sf::Vector2f(posX, posY));
                //gammaShader.setUniform("gamma", 1.f);
                //gammaShader.setUniform("texture", tex);
                window->draw(sprite/*,&gammaShader*/);
            }
        wallSprite.setPosition(window->getSize().x - wallWidth / 2, 0);
        window->draw(wallSprite);
        wallSprite.setPosition(sf::Vector2f(wallWidth / -2, 0));
        window->draw(wallSprite);
    };
    DrawBackground();
    windowTexture.update(*window);
    window->display();
    auto removeSparkObject = [](BTRSpark& spark)
    {
        if (spark.sparkRect.left >= 15 * 3 || spark.y > BTRWINDOWHEIGHT)
        {
            return true;
        }
        return false;
    };
    auto drawPaddle = [&](bool changeToCursor = true)
    {
        if (changeToCursor) playArea->paddle.sprite->sprite.setPosition(std::clamp(cursor->sprite.getPosition().x, wallWidth / 2.f, BTRWINDOWWIDTH - wallWidth / 2.f - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
        //paddle.sprite->Animate();
        window->draw(playArea->paddle.sprite->sprite);
        playArea->paddle.sprite->sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, playArea->paddle.sprite->realHeightPerTile * (playArea->paddle.sprite->animFramePos - 1)), sf::Vector2i(3, playArea->paddle.sprite->realHeightPerTile)));
        auto orgpos = playArea->paddle.sprite->sprite.getPosition();
        playArea->paddle.sprite->sprite.setPosition(orgpos.x - 3, orgpos.y);
        window->draw(playArea->paddle.sprite->sprite);
        playArea->paddle.sprite->sprite.setTextureRect(sf::IntRect(sf::Vector2i(playArea->paddle.sprite->width - 3, playArea->paddle.sprite->realHeightPerTile * (playArea->paddle.sprite->animFramePos - 1)), sf::Vector2i(3, playArea->paddle.sprite->realHeightPerTile)));
        playArea->paddle.sprite->sprite.setPosition(orgpos.x + playArea->paddle.paddleRadius, orgpos.y);
        window->draw(playArea->paddle.sprite->sprite);
        playArea->paddle.sprite->sprite.setPosition(orgpos);
    };
    auto drawPaddleXY = [&](float x, float y, int width)
    {
        auto orgRad = playArea->paddle.paddleRadius;
        auto orgPos = playArea->paddle.sprite->sprite.getPosition();
        playArea->paddle.sprite->sprite.setPosition(x, y);
        playArea->paddle.paddleRadius = width;
        playArea->paddle.sprite->sprite.setTextureRect(sf::IntRect(sf::Vector2i(playArea->paddle.sprite->width / 2 - playArea->paddle.paddleRadius / 2, playArea->paddle.sprite->realHeightPerTile * (playArea->paddle.sprite->animFramePos - 1)), sf::Vector2i(playArea->paddle.paddleRadius, playArea->paddle.sprite->realHeightPerTile)));
        drawPaddle(false);
        playArea->paddle.paddleRadius = orgRad;
        playArea->paddle.sprite->sprite.setPosition(orgPos);
    };

    sf::Texture highScoreTexture;
    highScoreTexture.create(BTRWINDOWWIDTH, BTRWINDOWHEIGHT);
    sf::Sprite highScoreSprite;
    highScoreSprite.setTexture(highScoreTexture, true);

    std::vector<BTRMovingText> scoreTexts;
    int highScoreWidth, highScoreHeight, colchan;
    auto highScoreImage = stbi_load("./ball/halloffaith.png", &highScoreWidth, &highScoreHeight, &colchan, 4);
    window->setKeyRepeatEnabled(true);
    if (highScoreImage)
    {
        highScoreTexture.update(highScoreImage);
        free(highScoreImage);
    }
    auto winBoxImage = new BTRsprite("./ball/winbox.png", 1, 0, 1);
    auto titleImage = new BTRsprite("./ball/bibleball.png", 1, false, 1);
    auto wincornerImage = new BTRsprite("./ball/wincorner.png", 14, 0, 1);
    auto winButtonImage = new BTRsprite("./ball/winbutton2.png", 1);
    wincornerImage->sprite.setOrigin(sf::Vector2f(wincornerImage->realWidthPerTile / 2, wincornerImage->height / 2));
    titleImage->SetTexRect(0, 0);
    titleImage->texture.setRepeated(true);
    auto DrawFrame = [&](sf::RenderWindow* window, sf::Vector2f pos, sf::Vector2f size)
    {
        sf::RectangleShape rect;
        rect.setFillColor(sf::Color(16, 24, 32));
        rect.setPosition(pos);
        rect.setSize(size);
        window->draw(rect);
        auto orgPosX = pos.x;
        auto orgPosY = pos.y;
        for (; pos.x < orgPosX + size.x; pos.x += wincornerImage->realWidthPerTile)
        {
            wincornerImage->SetTexRect(1, 0);
            wincornerImage->sprite.setPosition(pos);
            window->draw(*wincornerImage);
        }
        pos.x = orgPosX;
        pos.y += size.y;
        for (; pos.x < orgPosX + size.x; pos.x += wincornerImage->realWidthPerTile)
        {
            wincornerImage->SetTexRect(1, 0);
            wincornerImage->sprite.setPosition(pos);
            window->draw(*wincornerImage);
        }
        pos = sf::Vector2f(orgPosX, orgPosY);
        for (; pos.y < orgPosY + size.y; pos.y += wincornerImage->height)
        {
            wincornerImage->SetTexRect(3, 0);
            wincornerImage->sprite.setPosition(pos);
            window->draw(*wincornerImage);
            wincornerImage->SetTexRect(0, 0);
        }
        pos = sf::Vector2f(orgPosX + size.x, orgPosY);
        for (; pos.y < orgPosY + size.y; pos.y += wincornerImage->height)
        {
            wincornerImage->SetTexRect(3, 0);
            wincornerImage->sprite.setPosition(pos);
            window->draw(*wincornerImage);
            wincornerImage->SetTexRect(0, 0);
        }
        pos = sf::Vector2f(orgPosX, orgPosY);
        wincornerImage->SetTexRect(0, 0);
        wincornerImage->sprite.setPosition(pos);
        window->draw(*wincornerImage);
        wincornerImage->sprite.setPosition(pos + sf::Vector2f(size.x, 0));
        window->draw(*wincornerImage);
        wincornerImage->sprite.setPosition(pos + size);
        window->draw(*wincornerImage);
        wincornerImage->sprite.setPosition(pos + sf::Vector2f(0, size.y));
        window->draw(*wincornerImage);
    };

    auto flipPaused = [&](sf::Event event,bool musRestart = true)
    {
        paused ^= 1;
        BTRStopAllSounds();
        PauseMidiPlayback();
        if (paused)
        {
            BTRPlaySound("./sound/menu.wav", 1, 0, 1);
            if (event.key.code == sf::Keyboard::Escape)
            {
                menu = true;
            }
            else menu = false;
        }
        else
        {
            BTRStopAllSounds();
            if (musRestart) ContinueMidiPlayback();
            menu = false;
        }
    };
	BTRButton retToGame;
	retToGame.clickedFunc = [&]()
	{
        if (firstRun)
        {
            return;
        }
		sf::Event event;
		event.type = sf::Event::KeyPressed;
		event.key.code = sf::Keyboard::Escape;
		flipPaused(event);
		BTRPlaySound("./ball/editselect.wav");
	};
	retToGame.str = "Return to Game";
	retToGame.pos = sf::Vector2f(640 / 2 - winButtonImage->width / 2, BTRWINDOWHEIGHT - 100);
	BTRButton quitGame;
	quitGame.clickedFunc = [&]()
	{
		window->display();
		windowTexture.update(*window); // Flip off the buffers.
		fadeToColor(sf::Color(0, 0, 0, 255));
		window->close();
	};
	quitGame.pos = sf::Vector2f(640 / 2 - winButtonImage->width / 2, BTRWINDOWHEIGHT - 80);
	quitGame.str = "Quit Game";
	BTRButton randomLevel;
	randomLevel.clickedFunc = [&]()
	{
		fadeOut = true;
		score = 0;
		lives = 2;
		playArea->levnum = std::uniform_int_distribution<int>(1, 40)(rd) - 1;
		sf::Event event;
		event.type = sf::Event::KeyPressed;
		event.key.code = sf::Keyboard::Escape;
		flipPaused(event, false);
		GetCurPlayingFilename() = "";
		BTRPlaySound("./ball/editselect.wav");
        firstRun = false;
		fade = 1;
        playArea->randomPlay = true;
	};
	randomLevel.str = "Random Play";
	randomLevel.pos = sf::Vector2f(640 / 2 - winButtonImage->width / 2, BTRWINDOWHEIGHT - 340);
	btns.push_back(randomLevel);
	BTRButton singlePlay;
	singlePlay.str = "Single Play";
	singlePlay.pos = sf::Vector2f(randomLevel.pos.x, randomLevel.pos.y - 20);
	singlePlay.clickedFunc = [&]()
	{
        btns[0].clickedFunc();
		playArea->levnum = 0;
        playArea->randomPlay = false;
	};
	btns.push_back(retToGame);
	btns.push_back(quitGame);
	btns.push_back(singlePlay);
    bool isFullscreen = false;
    while (window->isOpen())
    {
        sf::Event event;
        while (window->pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window->display();// Flip off the buffers.
                windowTexture.update(*window);
                fadeToColor(sf::Color(0, 0, 0, 255));
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
            case sf::Event::TextEntered:
                /*if (std::isdigit(static_cast<unsigned char>(event.text.unicode), std::locale("")))
                {
                    std::string str;
                    str += static_cast<unsigned char>(event.text.unicode);
                    BTRpowerup::PowerupHandle(*playArea, std::stoi(str));
                }*/
                if (cheatText)
                {
                    if (event.text.unicode == 127) cheatstr.pop_back();
                    else cheatstr += static_cast<char>(event.text.unicode);
                }
                break;
            case sf::Event::MouseButtonReleased:
                if (menu)
                {
                    for (auto& curBtn : btns)
                    {
                        if (event.mouseButton.x >= curBtn.pos.x
                            && event.mouseButton.x <= curBtn.pos.x + winButtonImage->width
                            && event.mouseButton.y <= curBtn.pos.y + 20
                            && event.mouseButton.y >= curBtn.pos.y
                            && curBtn.wasHeld)
                        {
                            BTRPlaySound("./ball/editselect.wav");
                            curBtn.clickedFunc();
                        }
                        curBtn.wasHeld = false;
                    }
                }
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    for (auto& curBall : playArea->balls)
                    {
                        curBall->ballHeld = false;
                    }
#if 0
                    auto curMissile = new BTRMissileObject;
                    curMissile->x = sf::Mouse::getPosition(*window).x;
                    curMissile->y = sf::Mouse::getPosition(*window).y;
                    playArea->missiles.push_back(std::shared_ptr<BTRMissileObject>(curMissile));
#endif
                    if (menu)
                    {
                        for (auto& curBtn : btns)
                        {
                            if (event.mouseButton.x >= curBtn.pos.x
                                && event.mouseButton.x <= curBtn.pos.x + winButtonImage->width
                                && event.mouseButton.y <= curBtn.pos.y + 20
                                && event.mouseButton.y >= curBtn.pos.y)
                            {
                                curBtn.wasHeld = true;
                            }
                        }
                    }
                }
                break;
            case sf::Event::KeyPressed:
                if (event.key.code == sf::Keyboard::Pause || event.key.code == sf::Keyboard::Escape) if (!highScore)
                {
                    flipPaused(event);
                }
                if (event.key.code == sf::Keyboard::Enter)
                {
                    if (event.key.alt)
                    {
                        delete window;
                        isFullscreen ^= 1;
                        if (isFullscreen) window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn", sf::Style::Titlebar | sf::Style::Close | sf::Style::Fullscreen);
                        else window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn", sf::Style::Titlebar | sf::Style::Close);
                        window->setFramerateLimit(40);
                        //break;
                    }
                    if (cheatText)
                    {
                        try
                        {
                            if (cheatstr.substr(0, 5) == "goto:")
                            {
                                auto str = cheatstr.substr(5);
                                auto num = std::stoi(str);
                                BTRpowerup::PowerupHandle(*playArea, 14);
                                playArea->levnum = num - 1;
                                cheatText = false;
                                cheatstr.clear();
                                break;
                            }
                            BTRpowerup::PowerupHandle(*playArea, cheatKeys.at(cheatstr));
                            BTRPlaySound("./ball/cheat.wav");
                            textToDraw = "You cheater you!";
                            cornerTextBlinkTime = 1;
                        }
                        catch (std::out_of_range) {} // Intentional.
                        cheatText = false;
                        cheatstr.clear();
                    }
                    else if (highScore)
                    {
                        windowTexture.update(*window);
                        playArea->levnum = 0;
                        highScore = false;
                        fadeOut = true;
                        ballLost = false;
                        scrRect.setFillColor(sf::Color(0, 0, 0, 255));
                        fade = 1;
                        lives = 2;
                        score = 0;
                        scoreTexts.clear();
                        //midiStreamPause(midiDev);
                    }
                }
                break;
            }
        }
        if (!window->isOpen()) break;
        if (highScore)
        {
            window->clear();
            window->draw(highScoreSprite);
            for (auto& curMovingText : scoreTexts)
            {
                curMovingText.Tick();
                font->RenderChars(curMovingText.movingText, curMovingText.pos, window);
            }
            font->RenderChars("Hall of Fame", sf::Vector2f(BTRWINDOWWIDTH / 2, font->genCharHeight * 2) - sf::Vector2f(font->GetSizeOfText("Hall of Fame").x / 2, 0), window);
            window->display();
            continue;
        }
        if (cheatText)
        {
            window->clear();
            windowSprite.setColor(sf::Color(255, 255, 255, 255 * 0.5));
            window->draw(windowSprite);
            winBoxImage->sprite.setPosition(sf::Vector2f(BTRWINDOWWIDTH / 2 - winBoxImage->width / 2, BTRWINDOWHEIGHT / 2));
            window->draw(*winBoxImage);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::BackSpace) && cheatstr.size() > 0) cheatstr.pop_back();
            font->RenderChars("Cheaters never prosper...", winBoxImage->sprite.getPosition() - sf::Vector2f(0, font->genCharHeight), window);
            font->RenderChars(cheatstr + '|', winBoxImage->sprite.getPosition(), window);
            window->display();
            continue;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) && sf::Keyboard::isKeyPressed(sf::Keyboard::H))
        {
            cheatText = true;
        }
        if (paused)
        {
            window->clear();
            windowSprite.setColor(sf::Color(255, 255, 255, 255 * 0.5));
            window->draw(windowSprite);
            pausedSprite->sprite.setPosition(sf::Vector2f(BTRWINDOWWIDTH / 2, BTRWINDOWHEIGHT / 2) - sf::Vector2f(pausedSprite->width / 2, pausedSprite->height / 2));
            window->draw(pausedSprite->sprite);
            if (menu)
            {
                titleImage->sprite.setPosition(sf::Vector2f(BTRWINDOWWIDTH / 2 - titleImage->width / 2, 0));
                window->draw(*titleImage);
                sf::Vector2f pos = titleImage->sprite.getPosition() + sf::Vector2f(0, titleImage->height);

                DrawFrame(window, pos, sf::Vector2f(300, 370));
                glEnable(GL_SCISSOR_TEST);
                glScissor(pos.x, pos.y, 300, 370);
                font->RenderChars("Game Menu", sf::Vector2f(640 / 2 - font->GetSizeOfText("Game Menu").x / 2, font->genCharHeight + pos.y), window);
                for (auto& curBtn : btns)
                {
                    winButtonImage->sprite.setPosition(curBtn.pos);
                    winButtonImage->SetSpriteIndex(curBtn.wasHeld);
                    window->draw(*winButtonImage);
                    auto curBtnPos = curBtn.pos;
                    curBtnPos.x += winButtonImage->width / 2;
                    curBtnPos -= sf::Vector2f(font->GetSizeOfText(curBtn.str).x / 2, 0);
                    //curBtnPos += sf::Vector2f(0, curBtn.wasHeld ? 5 : 0);
                    font->RenderChars(curBtn.str, curBtnPos, window);
                }
                glScissor(0, 0, BTRWINDOWWIDTH, BTRWINDOWHEIGHT);
                glDisable(GL_SCISSOR_TEST);
                if (!cursorVisible)
                {
                    ball->SetSpriteIndex(1);
                    ball->sprite.setPosition((sf::Vector2f)sf::Mouse::getPosition(*window) - sf::Vector2f(ball->realWidthPerTile / 2, ball->realHeightPerTile / 2));
                    cursor->sprite.setPosition((sf::Vector2f)sf::Mouse::getPosition(*window) - sf::Vector2f(cursor->width / 2, cursor->height / 2));
                    window->draw(cursor->sprite);
                    window->draw(ball->sprite);
                    ball->Animate();
                    ball->SetSpriteIndex(0);
                }
            }
            window->display();
            continue;
        }
        if (fadeOut)
        {
            if (ballLost && lives <= 0)
            {
                BTRPlaySound("./ball/scream.wav");
                fadeToColor(sf::Color(255, 255, 255, 255));
                auto scorePair = std::make_pair("", score);
                scoresAndNames.push_back(scorePair);
                std::sort(scoresAndNames.begin(), scoresAndNames.end(), &sortScoreList);
                scoresAndNames.pop_back();
                bool scoreFound = false;
                int scoreY = 3;
                int scoreList = 1;
                bool flipVal = false;
                for (auto& curScore : scoresAndNames)
                {
                    if (std::get<int>(curScore) == score)
                    {
                        scoreFound = true;
                    }
                    std::istringstream istr;
                    istr.setf(std::ios::right, std::ios::adjustfield);
                    std::string str = std::to_string(scoreList) + ". " + std::get<std::string>(curScore);
                    str += "                           ";
                    str.resize(str.size() - std::to_string(std::get<int>(curScore)).size());
                    str += std::to_string(std::get<int>(curScore));
                    auto moveText = BTRMovingText();
                    flipVal ^= 1;
                    moveText.toPos = sf::Vector2f(90, scoreY * font->genCharHeight);
                    moveText.pos = sf::Vector2f(-font->GetSizeOfText(str).x + (BTRWINDOWWIDTH * flipVal), scoreY * font->genCharHeight);
                    scoreY++;
                    moveText.movingText = str;
                    scoreTexts.push_back(moveText);
                    scoreList++;
                }
                fadeOut = false;
                ballLost = false;
                highScore = true;
                BTRPlaySound("./ball/whoosh.wav");
                loadMusic("./ball/hghscr.mds");
            }
            else
            {
                fadeToColor(sf::Color(0, 0, 0, 255));
                fadeOut = false;
                framesPassedlastPowerup = 40 * 5;
                sparks.clear();
                explosions.clear();
                explodingBricks.clear();
                playArea->powerups.clear();
                delete chompteeth;
                chompteeth = 0;
                playArea->paddle.stateFlags = 0;
                playArea->paddle.sprite->sprite.setPosition(cursor->sprite.getPosition());
                if (!ballLost)
                {
                    std::string str = "./lev/";
                    bool isRandom = playArea->randomPlay;
                    int newLev = 0;
                    int totalSum = 0;
                    while (1)
                    {
                        if (!playArea->randomPlay) break;
                        newLev = std::uniform_int_distribution(1, 40)(rd) - 1;
                        bool alreadyPlayed = 0;
                        for (auto& curVal : randPlayedLevels)
                        {
                            totalSum += curVal;
                            if (newLev + 1 == curVal)
                            {
                                alreadyPlayed = true;
                            }
                        }
                        if (!alreadyPlayed) randPlayedLevels.push_back(playArea->levnum + 1);
                        if (totalSum >= 820)
                        {

                        }
                    }
                    newLev = isRandom ? std::uniform_int_distribution(1, 40)(rd) - 1 : playArea->levnum++;
                    str += std::to_string(newLev) + ".lev";
                    int oldLevnum = playArea->levnum;                   
                    delete playArea;
                    playArea = 0;                  
                    playArea = new BTRPlayArea(str, window);
                    playArea->levelEnded = false;
                    playArea->levnum = isRandom ? ++newLev : oldLevnum++;
                    playArea->randomPlay = isRandom;
                    explodingBricks.clear();
                    loadMusic(musics[mdsrand(gen)]);
                    BTRPlaySound("./ball/grow.wav");
                }
                else
                {
                    lives--;
                    playArea->SpawnInitialBall();
                    playArea->paddle.lengthOfBall = 5;
                    ballLost = fadeOut = false;
                    playArea->paddle.paddleRadius = playArea->paddle.radiuses[1];
                    playArea->paddle.curRadius = 1;
                }
                if (exitingFromHighScore)
                {
                    //midiStreamRestart(midiDev);
                    exitingFromHighScore = false;
                }

            }
        }
        sf::Sprite sprite;
        sprite.setTexture(tex, true);

        DrawBackground();
        ball->Animate();
        playArea->Tick();
        playArea->paddle.sprite->Animate();
        playArea->paddle.sprite->sprite.setTextureRect(sf::IntRect(sf::Vector2i(playArea->paddle.sprite->width / 2 - playArea->paddle.drawPaddleRadius / 2, playArea->paddle.sprite->realHeightPerTile * (playArea->paddle.sprite->animFramePos - 1)), sf::Vector2i(playArea->paddle.drawPaddleRadius, playArea->paddle.sprite->realHeightPerTile)));
        playArea->paddle.sprite->sprite.setOrigin(-playArea->paddle.paddleRadius / 2 + playArea->paddle.drawPaddleRadius / 2, 0);
        if (fireBrickFrame >= 128) fireBrickFrame = 64;
        if (drawCornerText) font->RenderChars(textToDraw, sf::Vector2f(wallWidth / 2, BTRWINDOWHEIGHT - font->genCharHeight), window);
        if (frameCnt % 10 == 0 && cornerTextBlinkTime < 7)
        {
            drawCornerText ^= 1;
            cornerTextBlinkTime++;
        }
        sf::Sprite brickSprite;
        brickSprite.setTexture(playArea->brickTexture, true);
        for (auto& curBrick : playArea->bricks) if (curBrick.brickID != 63)
        {
            brickSprite.setTextureRect(playArea->brickTexRects[curBrick.brickID - 1]);
            if (curBrick.isFireball)
            {
                brickSprite.setTextureRect(playArea->brickTexRects[fireBrickFrame]);
            }
            brickSprite.setPosition(sf::Vector2f(curBrick.x, curBrick.y));
            window->draw(brickSprite);
        }
        for (auto& curExplBrick : explodingBricks)
        {
            if (curExplBrick.frameOffset >= 7)
            {
                curExplBrick.frameOffset = 0;
                curExplBrick.loop++;
            }
            brickSprite.setTextureRect(playArea->brickTexRects[128ll + curExplBrick.frameOffset]);
            brickSprite.setPosition(curExplBrick.pos);
            window->draw(brickSprite);
            curExplBrick.frameOffset++;
        }
        for (int i = 0; i < explodingBricks.size(); i++)
        {
            if (explodingBricks[i].loop > 4)
            {
                explodingBricks.erase(explodingBricks.begin() + i);
            }
        }
        for (int i = 0; i < playArea->powerups.size(); i++)
        {
            if (playArea->powerups[i].aliveTick < 30)
            {
                powerupSprite->SetTexRect(10 - fuzzrand(gen), 1);
                powerupSprite->sprite.setPosition(playArea->powerups[i].x, playArea->powerups[i].y);
                window->draw(powerupSprite->sprite);
            }
            else
            {
                powerupSprite->SetTexRect(11 + (playArea->powerups[i].powerupID > 15), 1);
                powerupSprite->sprite.setPosition(playArea->powerups[i].x, playArea->powerups[i].y);
                window->draw(powerupSprite->sprite);
                powerupSprite->SetTexRect(playArea->powerups[i].powerupID - (playArea->powerups[i].powerupID > 15 ? 16 : 0), playArea->powerups[i].powerupID > 15);
                window->draw(powerupSprite->sprite);
            }
            if (playArea->powerups[i].destroyed == true) playArea->powerups.erase(playArea->powerups.begin() + i);
        }
        fireBrickFrame++;
        for (auto& curball : playArea->balls)
        {
            if (curball->isFireball) ball->SetSpriteIndex(1);
            else ball->SetSpriteIndex(0);
            ball->sprite.setPosition(sf::Vector2f(curball->x, curball->y));
            if (!curball->invisibleSparkling) window->draw(ball->sprite);
            if (curball->ballHeld) magnetHeldBall++;
        }
        for (auto& curMissile : playArea->missiles)
        {
            powerupSprite->sprite.setPosition(curMissile->x, curMissile->y);
            powerupSprite->SetTexRect(15, 1);
            //powerupSprite->sprite.setOrigin(sf::Vector2f(8, 0));
            window->draw(powerupSprite->sprite);
            //powerupSprite->sprite.setOrigin(sf::Vector2f(0, 0));
        }
        for (int i = 0; i < sparks.size(); i++)
        {
            auto curSpark = sparks[i];
            sparkSprite->sprite.setPosition(sparks[i].x, sparks[i].y);
            sparkSprite->sprite.setColor(sparks[i].color);
            if (frameCnt % 3 == 0) sparks[i].sparkRect.left += 3;
            sparkSprite->sprite.setTextureRect(curSpark.sparkRect);
#if __cplusplus <= 201703L
            if (sparks[i].sparkRect.left < 15 * 3)
#endif
            window->draw(sparkSprite->sprite);
#if __cplusplus <= 201703L
            else
            {
                sparks.erase(sparks.begin() + i);
                continue;
            }
#endif
            sparks[i].velY += sparks[i].gravity;
            sparks[i].y += sparks[i].velY;
            sparks[i].x += sparks[i].velX;
        }
#if __cplusplus > 201703L
        std::erase_if(sparks, removeSparkObject);
#endif
        largeFont->RenderChars(std::to_string(score), sf::Vector2f(wallWidth / 2, 0), window);
        font->RenderChars("level " + std::to_string(playArea->levnum), sf::Vector2f(wallWidth / 2, largeFont->genCharHeight), window);
        if (!cursorVisible)
        {
            ball->SetSpriteIndex(1);
            ball->sprite.setPosition((sf::Vector2f)sf::Mouse::getPosition(*window) - sf::Vector2f(ball->realWidthPerTile / 2, ball->realHeightPerTile / 2));
            cursor->sprite.setPosition((sf::Vector2f)sf::Mouse::getPosition(*window) - sf::Vector2f(cursor->width / 2, cursor->height / 2));
        }
        for (auto& curExpl : explosions)
        {
            curExpl.Tick();
            if (curExpl.destroyed)
            {
                continue;
            }
            curExpl.spr->SetTexRect(curExpl.framesPassed, curExpl.spriteIndex);
            curExpl.spr->sprite.setPosition(curExpl.pos);
            window->draw(curExpl.spr->sprite);
        }
        for (int i = 0; i < explosions.size(); i++)
        {
            if (explosions[i].destroyed)
            {
                explosions.erase(explosions.begin() + i);
                continue;
            }
        }
        if (!ballLost) playArea->paddle.sprite->sprite.setPosition(std::clamp(cursor->sprite.getPosition().x, wallWidth / 2.f, BTRWINDOWWIDTH - wallWidth / 2.f - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
        drawPaddleXY(playArea->paddle.sprite->sprite.getPosition().x, playArea->paddle.sprite->sprite.getPosition().y, playArea->paddle.drawPaddleRadius);
        auto orgpos = playArea->paddle.sprite->sprite.getPosition();
        auto orgOrig = playArea->paddle.sprite->sprite.getOrigin();
        playArea->paddle.sprite->sprite.setOrigin(0, 0);
        for (int i = 0; i < lives; i++)
        {
            drawPaddleXY(BTRWINDOWWIDTH - playArea->paddle.radiuses[0] * ((long long)i + 1) - wallWidth / 2 - (9ll * i), 0, playArea->paddle.radiuses[0]);
        }
        playArea->paddle.sprite->sprite.setOrigin(orgOrig);
        for (int i = 0; i < playArea->paddle.missilesLeft; i++) if (playArea->paddle.stateFlags & BTRPaddle::PADDLE_MISSILE)
        {
            powerupSprite->SetTexRect(15, 1);
            int totalWidth = 16 * playArea->paddle.missilesLeft;
            powerupSprite->sprite.setPosition((BTRWINDOWWIDTH / 2 - totalWidth / 2) + i * 16, 0);
            window->draw(powerupSprite->sprite);
        }
        playArea->paddle.sprite->sprite.setTextureRect(sf::IntRect(sf::Vector2i(playArea->paddle.sprite->width / 2 - playArea->paddle.drawPaddleRadius / 2, playArea->paddle.sprite->realHeightPerTile * (playArea->paddle.sprite->animFramePos - 1)), sf::Vector2i(playArea->paddle.drawPaddleRadius, playArea->paddle.sprite->realHeightPerTile)));
        playArea->paddle.drawPaddleRadius = std::lerp(playArea->paddle.drawPaddleRadius, playArea->paddle.paddleRadius, 0.25);
        if (playArea->paddle.stateFlags & playArea->paddle.PADDLE_TRACTOR)
        {
            DrawFrame(window, sf::Vector2f(BTRWINDOWWIDTH / 2 - 120, 0), sf::Vector2f(240, 20));
            tractorSprite->sprite.setPosition(sf::Vector2f(BTRWINDOWWIDTH / 2 - 120, 0));
            tractorSprite->intRect.width = playArea->paddle.tractorBeamPower;
            tractorSprite->sprite.setTextureRect(tractorSprite->intRect);
            window->draw(*tractorSprite);
        }
        if (magnetHeldBall)
        {
            magnetSprite->SetTexRect(magnetSprite->x, magnetSprite->y);
            magnetSprite->x++;
            if (magnetSprite->x > magnetSprite->realnumOfFrames - 1)
            {
                magnetSprite->x = 0;
                magnetSprite->y ^= 1;
            }
            magnetSprite->sprite.setPosition(orgpos.x + playArea->paddle.drawPaddleRadius, orgpos.y - magnetSprite->realHeightPerTile * 0.5);
            window->draw(magnetSprite->sprite);
            int oldY = magnetSprite->y;
            magnetSprite->SetTexRect(magnetSprite->x, magnetSprite->y + 2);
            magnetSprite->sprite.setPosition((orgpos.x - magnetSprite->realWidthPerTile) - playArea->paddle.sprite->sprite.getOrigin().x, orgpos.y - magnetSprite->realHeightPerTile * 0.5);
            window->draw(magnetSprite->sprite);
            magnetSprite->SetTexRect(magnetSprite->x, oldY);
            magnetHeldBall = 0;
        }
        if (chompteeth)
        {
            chompteeth->flip ^= ((frameCnt % 10) == 0);
            powerupSprite->SetTexRect(13 + chompteeth->flip, 1);
            powerupSprite->sprite.setPosition(chompteeth->x, orgpos.y);
            window->draw(powerupSprite->sprite);
        }
        if (ballLost)
        {
            playArea->paddle.sprite->sprite.move(playArea->paddle.paddleRadius / 4, 0);
            playArea->paddle.paddleRadius = playArea->paddle.drawPaddleRadius = std::lerp(playArea->paddle.paddleRadius, 0, 0.5);
            if (playArea->paddle.paddleRadius <= 0.5)
            {
                fadeOut = true;
                playArea->paddle.paddleRadius = playArea->paddle.radiuses[1];
            }
        }
        if (!cursorVisible)
        {
            window->draw(ball->sprite);
            window->draw(cursor->sprite);
            ball->sprite.setOrigin(0, 0);
        }
        ball->SetSpriteIndex(0);
        scrRect.setFillColor(sf::Color(0, 0, 0, 255 * fade));
        if (fade > 0) fade -= 1 / (double)40;
        else fade = 0;
        window->draw(scrRect);
        windowTexture.update(*window);
        window->display();
        frameCnt++;
        framesPassedlastPowerup++;
    }
    eot = true;
#if defined(WIN32)
    midiStreamStop(midiDev);
    //if (thread->joinable()) thread->join();
    midiOutReset((HMIDIOUT)midiDev);
#endif
    delete window;
    delete magnetSprite;
    delete winBoxImage;
    deInitOpenAL();
    if (!(inierr < 0))
    for (int i = 0; i < scoresAndNames.size(); i++)
    {
        ini.SetValue("bt.ini",
                     ("score" + std::to_string(i)).c_str(),
                     (std::get<std::string>(scoresAndNames[i]) + ';' + std::to_string(std::get<int>(scoresAndNames[i]))).c_str());
        std::cout << "Saved score: " << ini.GetValue("bt.ini", ("score" + std::to_string(i)).c_str()) << std::endl;
    }
    ini.SaveFile("./bt.ini", false);
}
