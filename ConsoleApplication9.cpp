// ConsoleApplication9.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#if __cplusplus < 201703L
#error "C++17 or later is required to build the project."
#endif
#if !defined(SFML_STATIC)
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "BTRCommon.h"
#include "SoundPlayback.h"
#include <random>
#include <mutex>
#if __has_include(<sys/param.h>) && (defined(__unix__) || defined(__APPLE__))
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <float.h>
#endif
#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"
#include <signal.h>
//#include <immintrin.h>
#include <algorithm>
int lives = 2;
BTRsprite *ball = NULL;
//std::vector<BTRball*> balls;
std::vector<BTRSpark> sparks;
int wallWidth = 0;
int score = 0;
int frameCnt = 0;
int fireBrickFrame = 64;
unsigned char brickID = 1;
std::chrono::steady_clock clocktime;
std::chrono::steady_clock::time_point now;
int64_t sec = 0;
bool endofgame = false;
extern void ParseMidsFile(std::string filename);
extern void StartMidiPlayback();
#ifdef WIN32
extern void CALLBACK MidiCallBack(HMIDIOUT hmo, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
extern HMIDISTRM midiDev;
HMIDIOUT midiOutDev;
#endif // WIN32

extern bool eot;
extern bool oneshotplay;
extern bool gameSound;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(-5, 5);
bool fadeOut = false;
bool ballLost = false;
std::vector<BTRExplodingBricks> explodingBricks;
std::vector<BTRExplosion> explosions;
std::vector<BTRButton> btns;
std::vector<BTRButton> levEditBtns;
int framesPassedlastPowerup;
std::thread *thread = NULL;
std::vector<std::pair<std::string, int>> scoresAndNames;
BTRChompTeeth *chompteeth = NULL;
BTRPlayArea *playArea = NULL;
std::vector<unsigned char> randPlayedLevels;
int64_t randPlayedSet;
sf::Vector2i lastTouchPosition = sf::Vector2i(0,0);

static inline void AppendVerticesFromRect(sf::IntRect& rect, btr::Vector2f pos, sf::VertexArray& vertArray)
{
    vertArray.append(sf::Vertex(pos,btr::Vector2f(rect.left,rect.top)));
    vertArray.append(sf::Vertex(pos + btr::Vector2f(rect.width,0),btr::Vector2f(sf::Vector2i(rect.left,rect.top) + sf::Vector2i(rect.width,0))));
    vertArray.append(sf::Vertex(pos + btr::Vector2f(0,rect.height),btr::Vector2f(sf::Vector2i(rect.left,rect.top) + sf::Vector2i(0,rect.height))));
    vertArray.append(sf::Vertex(pos + btr::Vector2f(0,rect.height),btr::Vector2f(sf::Vector2i(rect.left,rect.top) + sf::Vector2i(0,rect.height))));
    vertArray.append(sf::Vertex(pos + btr::Vector2f(rect.width,rect.height),btr::Vector2f(sf::Vector2i(rect.left,rect.top) + sf::Vector2i(rect.width,rect.height))));
    vertArray.append(sf::Vertex(pos + btr::Vector2f(rect.width,0),btr::Vector2f(sf::Vector2i(rect.left,rect.top) + sf::Vector2i(rect.width,0))));
}

bool sortScoreList(const std::pair<std::string, int> t1, const std::pair<std::string, int> t2)
{
    if (std::get<1>(t1) > std::get<1>(t2))
        return true;
    return false;
}
extern void StopMidiPlayback();
extern std::string &GetCurPlayingFilename();
unsigned int devID = 0;
bool loadedEndMusic = false;
bool gameMusic = true;
void loadMusic(std::string mdsfilename, bool oneshot = false)
{
    if (!gameMusic)
        return;
    if (GetCurPlayingFilename() == mdsfilename)
    {
        return; // Let it continue;
    }
    if (thread)
    {
        eot = true;
        if (thread->joinable())
            thread->join();
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
    oneshotplay = oneshot;
    ParseMidsFile(mdsfilename);
    thread = new std::thread(StartMidiPlayback);
}
extern void DownBricks(BTRPlayArea &area);
//using namespace luabridge;
auto ActivatePowerup(int powerupID)
{
    return BTRpowerup::PowerupHandle(*playArea, powerupID);
}
std::vector<BTRMovingText> BuildTextFromString(std::string buildStr, BTRFont*& font, int baseYPos = 0)
{
    bool flipVal = 0;
    std::stringstream stream(buildStr);
    BTRMovingText moveText;
    
    std::vector<BTRMovingText> btrmoveTexts;
    while (!stream.eof())
    {
        std::getline(stream,moveText.movingText);
        flipVal ^= 1;
        moveText.toPos = btr::Vector2f(BTRWINDOWWIDTH / 2 - font->GetSizeOfText(moveText.movingText).x / 2, baseYPos * font->genCharHeight);
        moveText.pos = moveText.orgPos = btr::Vector2f(-font->GetSizeOfText(moveText.movingText).x + (BTRWINDOWWIDTH * flipVal), baseYPos++ * font->genCharHeight);
        btrmoveTexts.push_back(moveText);
    }
    return btrmoveTexts;
}
void DoFuncForDuration(unsigned long long sec, std::function<void()> func)
{
    auto time = std::chrono::duration_cast<std::chrono::seconds>(clocktime.now().time_since_epoch());
    while ((std::chrono::duration_cast<std::chrono::seconds>(clocktime.now().time_since_epoch()) - time).count() <= sec)
    {
        func();
    }   
}
const std::string gammaShaderCode =

    "#version 120"
    "\n"
    ""
    "uniform sampler2D texture;"
    "uniform float gamma; "
    "void main()"
    "{"
    " vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);"
    " gl_FragColor = pow(pixel, vec4(vec3(1.0f / gamma),1.0) );"
    "}";

const std::string colorShaderCode=
    "#version 120"
    "\n"
    "\n"
    "uniform sampler2D texture;"
    "uniform vec4 col;"
    "void main()"
    "{"
    " vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);"
    " float grayCol = (pixel.r + pixel.g + pixel.b) / 3.0;"
    " gl_FragColor = gl_Color * vec4(grayCol,grayCol,grayCol,1.0) * col;"
    "}";

bool firstRun = true;
bool vertvelpowerup = false;
bool backgrndcol = false;
bool isFullscreen = false;
bool demo = false;
bool simpleSparks = false;
float backgrndr = 1.0f;
float backgrndg = 1.0f;
float backgrndb = 1.0f;
char* homeprefix;
#if defined(__ANDROID__) || defined(ANDROID)
#include <android/log.h>
#endif
int main(int argc, char *argv[])
{
    int randNum = time(NULL);
    std::vector<int> randNums;
    for (int i = 0; i < lrint(2492.125); i++)
    {
        randNum = rd();
        randNums.push_back(randNum);
    }
    std::seed_seq* seq = new std::seed_seq(randNums.begin(), randNums.end());
    gen.seed(*seq);
    delete seq;

    CSimpleIniA ini;
    ini.SetUnicode(true);
    ini.Reset();
#if defined(__ANDROID__) || defined(ANDROID)
    int chdirErr = chdir("/storage/emulated/0/BlastThruRebornData");
    if (chdirErr == -1)
    {
        __android_log_print(ANDROID_LOG_INFO, "BlastThruReborn", "Failed to chdir into /storage/emulated/0/BlastThruRebornData");
        exit(-1);
    }
#endif
    SI_Error inierr = (SI_Error)ini.LoadFile("./bt.ini");
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
            else
                sectionName[sectionName.size() - 1]++;
        }
        std::sort(scoresAndNames.begin(), scoresAndNames.end(), &sortScoreList);
    }
    else
    {
        ini.SetValue("bt.ini", 0, 0);
        int cnt = 0;
        while (cnt < 20)
        {
            auto scorestr = std::string("score") + std::to_string(cnt);
            ini.SetValue("bt.ini", scorestr.c_str(), ";0");
            cnt++;
        }
        ini.SetValue("bt.ini", "scheck", "0");
        ini.SetValue("bt.ini", "gmididevselect", "off");
        ini.SetValue("bt.ini", "gmididevnum", "0");
        ini.SetValue("bt.ini", "ggamemusic", "on");
        ini.SetValue("bt.ini", "ggamesound", "on");
        ini.SetValue("bt.ini", "gvertvelpowerup", "off");
        ini.SetValue("bt.ini", "runcount", "0");
        ini.SetValue("bt.ini", "runsuccess", "0");
        ini.SetValue("bt.ini", "backgrndred", "0.0");
        ini.SetValue("bt.ini", "backgrndgreen", "0.0");
        ini.SetValue("bt.ini", "backgrndblue", "0.0");
        ini.SetBoolValue("bt.ini", "gfullscreen",0);
        ini.SetBoolValue("bt.ini", "backgrndcol", false);
	ini.SetValue("bt.ini","gamename", "Nameless Wonder");
#if defined(__ANDROID__) || defined(ANDROID)
        ini.SetBoolValue("bt.ini", "gmididevselect",1);
        ini.SetBoolValue("bt.ini", "gfullscreen",0);
        ini.SetLongValue("bt.ini", "gmididevnum", 0);
        devID = 0;
#endif
        ini.SaveFile("./bt.ini", 0);
    }
    std::string name;
    if (ini.GetValue("bt.ini", "gamename") == NULL)
    {
        std::cout << "Enter name to be saved in the Hall Of Fame: ";
        std::cin >> name;
        ini.SetValue("bt.ini", "gamename", name.c_str());
    }
    name = ini.GetValue("bt.ini", "gamename");
    gameMusic = ini.GetBoolValue("bt.ini", "ggamemusic");
    gameSound = ini.GetBoolValue("bt.ini", "ggamesound");
    simpleSparks = ini.GetBoolValue("bt.ini","gsimplesparks");
    backgrndcol = ini.GetBoolValue("bt.ini", "backgrndcol",false);
    backgrndr = ini.GetDoubleValue("bt.ini", "backgrndred",1);
    backgrndg = ini.GetDoubleValue("bt.ini", "backgrndgreen",1);
    backgrndb = ini.GetDoubleValue("bt.ini", "backgrndblue",1);
    ini.SetDoubleValue("bt.ini", "backgrndred", backgrndr);
    ini.SetDoubleValue("bt.ini", "backgrndgreen", backgrndg);
    ini.SetDoubleValue("bt.ini", "backgrndblue", backgrndb);
    ini.SetBoolValue("bt.ini", "backgrndcol", backgrndcol);
    ini.SetBoolValue("bt.ini","gsimplesparks",simpleSparks);
    bool mididevselect = ini.GetBoolValue("bt.ini", "gmididevselect");
    isFullscreen = ini.GetBoolValue("bt.ini","gfullscreen");
    if (!mididevselect)
    {
        SelectMidiDevice();
        ini.SetValue("bt.ini", "gmididevselect", "on");
        ini.SetLongValue("bt.ini", "gmididevnum", devID);
    }
    else
        SelectMidiDevice(ini.GetLongValue("bt.ini", "gmididevnum"));
    InitOpenAL();
    vertvelpowerup = ini.GetBoolValue("bt.ini", "gvertvelpowerup");
    for (int i = 0; i < argc; i++)
    {
        if (strncmp("vertvelpowerup", argv[i], strlen("vertvelpowerup")) == 0)
        {
            vertvelpowerup = true;
            std::cout << "vertvelpowerup = true" << std::endl;
        }
        if (strncmp("-vertvelpowerup", argv[i], strlen("-vertvelpowerup")) == 0)
        {
            vertvelpowerup = false;
            std::cout << "vertvelpowerup = false" << std::endl;
        }
        ini.SetBoolValue("bt.ini", "gvertvelpowerup", vertvelpowerup);
        if (strncmp("extractdata", argv[i], strlen("extractdata")) == 0)
        {
            int ret = system("./GloDecrypt.exe ./ball.glo ./balldecrypt.glo extract ./ball/");
            if (ret == -1)
                system("./GloDecrypt ./ball.glo ./balldecrypt.glo extract ./ball/");
        }
    }
    std::map<std::string, int> cheatKeys =
    {
        {"inc me", 14},
        {"let me hear you say fired up", 6},
        {"my velcro shoes", 3},
        {"no ska no swing", 9},
        {"not a small thing", 0},
        {"sidewinders", 10},
        {"detrimental abundancy", 22},
        {"mixed blessings", 12},
        {"beneficial abundancy", 11},
        {"through holy faith", 15},
        {"sweet", 8},
        {"funyons", 30},
        {"i never prosper", 31}
    };
    sf::Shader gammaShader;
    auto loaded = gammaShader.loadFromMemory(gammaShaderCode, sf::Shader::Fragment);
    if (!loaded)
    {
        std::cout << "Loading gamma shader failed" << std::endl;
    }
    sf::Shader colShader;
    loaded = colShader.loadFromMemory(colorShaderCode, sf::Shader::Fragment);
    if (!loaded)
    {
        std::cout << "Loading color shader failed" << std::endl;
    }
#if 0
        L = lua_open();
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
        )");
#endif
    std::vector<std::string> musics = {"./ball/coolin.mds", "./ball/samba.mds", "./ball/piano.mds", "./ball/rush.mds"};
    std::cout.setf(std::ios_base::boolalpha);
    //for (int i = 0; i < 19937 / 8 * 4; i++) gen.seed(rd());
    std::uniform_int_distribution<> dis(-5, 5);
    std::uniform_int_distribution<> fuzzrand(0, 2);
    std::uniform_int_distribution<> mdsrand(0, musics.size() - 1);

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
    wallSprite.setPosition(btr::Vector2f(wallWidth / -2, 0));
    wallSprite.setTexture(wallTex);
    wallSprite.setScale(1, 24);

    ball = new BTRsprite("./ball/ball.png", 32);
    auto loadSplashSprite = new BTRsprite("./art/romtech.png", 1, false, 1);
    std::stack<BTRMenuUIWindow> menuWindowStack;
    static const std::function<void()> closeMenuWindow = [&]()
    {
        menuWindowStack.pop();
    };
    sf::RenderWindow *window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn",
            isFullscreen ? sf::Style::Fullscreen : sf::Style::Titlebar | sf::Style::Close);
    sf::Texture windowTexture;
    windowTexture.create(window->getSize().x,window->getSize().y);
    sf::Sprite windowSprite;
    windowSprite.setTexture(windowTexture, true);
    window->setFramerateLimit(40);
    window->clear();
    window->draw(*loadSplashSprite);
    windowTexture.update(*window);
    window->display();
    auto fadeToColor = [&](sf::Color fadeColor) {
        uint32_t framerate = 60;
#ifdef WIN32
        framerate = GetDeviceCaps(GetDC(window->getSystemHandle()), VREFRESH);
#endif
        window->setFramerateLimit(framerate);
        double alpha = 1.0;
        int localFrameCnt = 0;
        while (localFrameCnt < framerate)
        {
            if (fadeColor == sf::Color(0, 0, 0, 255) && sf::Shader::isAvailable())
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
    sf::Event event;
    DoFuncForDuration(3, [&]()
                      {                          
                          window->pollEvent(event);
                          window->clear();
                          window->draw(*loadSplashSprite);
                          window->display();
                      });
    fadeToColor(sf::Color(0, 0, 0, 255));

    BTRPlaySound("./ball/grow.wav");
    window->setFramerateLimit(40);
    //loadMusic(musics[mdsrand(gen)]);

    now = clocktime.now();

    auto font = new BTRFont("./ball/fontcool.png");
    auto largeFont = new BTRFont("./ball/fontlarge.png", BTRFont::FontType::BTR_FONTLARGE);
    auto largeFont2 = new BTRFont("./ball/fontlarge2.png", BTRFont::FontType::BTR_FONTLARGE2);
    playArea = new BTRPlayArea("./lev/0.lev");
    //playArea->paddle.sprite = new BTRsprite("./ball/rockpaddle.png", 1, true, 32);
    long double fade = 1.0;
    sf::RectangleShape scrRect((btr::Vector2f)window->getSize());
    scrRect.setFillColor(sf::Color(0, 0, 0, 255));
    bool cursorVisible = true;
    auto cursor = new BTRsprite("./ball/cursor.png", 1, false, 1);
    auto sparkSprite = new BTRsprite("./ball/sparks.png", 15, false, 3);
    auto powerupSprite = new BTRsprite("./ball/powerup.png", 16, false, 2);
    auto pausedSprite = new BTRsprite("./ball/paused.png", 1, false, 1);
    auto magnetSprite = new BTRsprite("./ball/magnet.png", 16, false, 4);
    auto tractorSprite = new BTRsprite("./ball/power.png", 1, 0);
    auto explSprite = new BTRsprite("./ball/explosion.png", 8, false, 4);
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
    auto DrawBackground = [&]() {
        sf::Sprite sprite;
        sprite.setTexture(tex, true);

        window->clear();
        for (int posY = 0; posY < BTRWINDOWHEIGHT; posY += y)
            for (int posX = 0; posX < BTRWINDOWWIDTH; posX += x)
            {
                sprite.setPosition(btr::Vector2f(posX, posY));
                //gammaShader.setUniform("gamma", 1.f);
                //gammaShader.setUniform("texture", tex);
                colShader.setUniform("texture",tex);
                colShader.setUniform("col",sf::Glsl::Vec4(backgrndr,backgrndg,backgrndb,1));
                window->draw(sprite, backgrndcol ? &colShader : NULL /*,&gammaShader*/);
            }
        wallSprite.setPosition(BTRWINDOWWIDTH - wallWidth / 2, 0);
        window->draw(wallSprite);
        wallSprite.setPosition(btr::Vector2f(wallWidth / -2, 0));
        window->draw(wallSprite);
    };
    DrawBackground();
    windowTexture.update(*window);
    window->display();
    auto removeSparkObject = [](BTRSpark &spark) {
        if (spark.sparkRect.left >= 15 * 3 || spark.y > BTRWINDOWHEIGHT)
        {
            return true;
        }
        return false;
    };
    auto drawPaddle = [&](bool changeToCursor = true) {
        if (changeToCursor)
            playArea->paddle.sprite->sprite.setPosition(std::clamp(cursor->sprite.getPosition().x, wallWidth / 2.f, BTRWINDOWWIDTH - wallWidth / 2.f - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
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
    auto drawPaddleXY = [&](float x, float y, int width) {
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
    std::vector<BTRMovingText> moveTexts;
    moveTexts = BuildTextFromString(
                                    "Congratulations!\n"
                                    "You have entered the Hall of Scores!\n"
                                    "Enter your name in the box below:\n",
                                    font,3);
    int highScoreWidth, highScoreHeight, colchan;
    auto highScoreImage = stbi_load("./ball/halloffaith.png", &highScoreWidth, &highScoreHeight, &colchan, 4);
    window->setKeyRepeatEnabled(true);
    if (highScoreImage)
    {
        highScoreTexture.update(highScoreImage);
        free(highScoreImage);
    }
    auto winBoxImage = new BTRsprite("./ball/winbox.png", 1, 0, 1);
    auto winBoxImage2 = new BTRsprite("./ball/winbox2.png", 1, 0, 1);
    auto titleImage = new BTRsprite("./ball/bibleball.png", 1, false, 1);
    auto wincornerImage = new BTRsprite("./ball/wincorner.png", 14, 0, 1);
    auto winButtonImage = new BTRsprite("./ball/winbutton2.png", 1);
    auto winButtonSmallImage = new BTRsprite("./ball/winbutton.png", 1);
    wincornerImage->sprite.setOrigin(btr::Vector2f(wincornerImage->realWidthPerTile / 2, wincornerImage->height / 2));
    titleImage->SetTexRect(0, 0);
    titleImage->texture.setRepeated(true);
    bool isLevEdit = false;
    auto DrawFrame = [&](sf::RenderWindow *window, btr::Vector2f pos, btr::Vector2f size) {
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
        pos = btr::Vector2f(orgPosX, orgPosY);
        for (; pos.y < orgPosY + size.y; pos.y += wincornerImage->height)
        {
            wincornerImage->SetTexRect(3, 0);
            wincornerImage->sprite.setPosition(pos);
            window->draw(*wincornerImage);
            wincornerImage->SetTexRect(0, 0);
        }
        pos = btr::Vector2f(orgPosX + size.x, orgPosY);
        for (; pos.y < orgPosY + size.y; pos.y += wincornerImage->height)
        {
            wincornerImage->SetTexRect(3, 0);
            wincornerImage->sprite.setPosition(pos);
            window->draw(*wincornerImage);
            wincornerImage->SetTexRect(0, 0);
        }
        pos = btr::Vector2f(orgPosX, orgPosY);
        wincornerImage->SetTexRect(0, 0);
        wincornerImage->sprite.setPosition(pos);
        window->draw(*wincornerImage);
        wincornerImage->sprite.setPosition(pos + btr::Vector2f(size.x, 0));
        window->draw(*wincornerImage);
        wincornerImage->sprite.setPosition(pos + size);
        window->draw(*wincornerImage);
        wincornerImage->sprite.setPosition(pos + btr::Vector2f(0, size.y));
        window->draw(*wincornerImage);
    };
    auto editPlayArea = new BTRPlayArea();
    auto flipPaused = [&](sf::Event event, bool musRestart = true) {
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
            else
                menu = false;
        }
        else
        {
            BTRStopAllSounds();
            if (musRestart)
                ContinueMidiPlayback();
            menu = false;
        }
    };
    int newScoreAt = -1;
    auto updateScoreList = [&]()
    {
        {
            scoreTexts.clear();
            newScoreAt = -1;
            auto scorePair = std::make_pair("", score);
            scoresAndNames.push_back(scorePair);
            std::sort(scoresAndNames.begin(), scoresAndNames.end(), &sortScoreList);
            scoresAndNames.pop_back();
            bool scoreFound = false;
            int scoreY = 3;
            int scoreList = 1;
            bool flipVal = false;
            for (int i = 0; i < scoresAndNames.size(); i++)
            {
                auto &curScore = scoresAndNames[i];
                if (std::get<int>(curScore) == score)
                {
                    newScoreAt = i;
                    for (auto& curMovingText : moveTexts)
                    {
                        curMovingText.pos = curMovingText.orgPos;
                    }
                }
                std::stringstream istr;
                std::string str = std::to_string(scoreList) + ". " + std::get<std::string>(curScore);
                istr << str;
                istr.setf(std::ios::right, std::ios::adjustfield);
                str += "                           ";
                str.resize(str.size() - std::to_string(std::get<int>(curScore)).size());
                str += std::to_string(std::get<int>(curScore));
                istr << std::to_string(std::get<int>(curScore));
                auto moveText = BTRMovingText();
                flipVal ^= 1;
                moveText.toPos = btr::Vector2f(90, scoreY * font->genCharHeight);
                moveText.pos = btr::Vector2f(-font->GetSizeOfText(str).x + (BTRWINDOWWIDTH * flipVal), scoreY * font->genCharHeight);
                scoreY++;
                moveText.movingText = str;
                scoreTexts.push_back(moveText);
                scoreList++;
            }
        }
    };
    // Game Menu buttons.
    BTRButton retToGame;
    retToGame.clickedFunc = [&]() {
        if (firstRun)
        {
            return;
        }
        sf::Event event;
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        flipPaused(event);
        //BTRPlaySound("./ball/editselect.wav");
    };
    retToGame.str = "Return to Game";
    retToGame.pos = btr::Vector2f(640 / 2 - winButtonImage->width / 2, BTRWINDOWHEIGHT - 100);

    BTRButton quitGame;
    quitGame.clickedFunc = [&]() {
        BTRMenuUIWindow menuWindow;
        menuWindow.nameOfWindow = "Quit Game";
        //menuWindow.staticTexts.push(std::make_pair());
        window->display();
        windowTexture.update(*window); // Flip off the buffers.
        fadeToColor(sf::Color(0, 0, 0, 255));
        window->close();
    };
    quitGame.pos = btr::Vector2f(640 / 2 - winButtonImage->width / 2, BTRWINDOWHEIGHT - 80);
    quitGame.str = "Quit Game";

    BTRButton randomLevel;
    randomLevel.clickedFunc = [&]() {
        fadeOut = true;
        demo = false;
        score = 0;
        lives = 2;
        playArea->levnum = std::uniform_int_distribution<int>(1, 40)(rd) - 1;
        sf::Event event;
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        flipPaused(event, false);
        GetCurPlayingFilename() = "";
        //BTRPlaySound("./ball/editselect.wav");
        firstRun = false;
        fade = 1;
        playArea->randomPlay = true;
        isLevEdit = false;
    };
    randomLevel.str = "Random Play";
    randomLevel.pos = btr::Vector2f(640 / 2 - winButtonImage->width / 2, BTRWINDOWHEIGHT - 340);
    btns.push_back(randomLevel);

    BTRButton singlePlay;
    singlePlay.str = "Single Play";
    singlePlay.pos = btr::Vector2f(randomLevel.pos.x, randomLevel.pos.y - 20);
    singlePlay.clickedFunc = [&]() {
        btns[0].clickedFunc();
        playArea->levnum = 0;
        playArea->randomPlay = false;
    };
    btns.push_back(retToGame);
    btns.push_back(quitGame);
    btns.push_back(singlePlay);

    BTRButton levEdit;
    levEdit.clickedFunc = [&]() {
        isLevEdit = true;
        sf::Event event;
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        flipPaused(event, false);
    };
    levEdit.str = "Level Editor";
    levEdit.pos = btr::Vector2f(randomLevel.pos.x, randomLevel.pos.y + 20);
    btns.push_back(levEdit);

    BTRButton highScoreEnter;
    highScoreEnter.str = "High Scores";
    highScoreEnter.pos = btr::Vector2f(randomLevel.pos.x, randomLevel.pos.y + 40);
    highScoreEnter.clickedFunc = [&]() {
        sf::Event event;
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        flipPaused(event, false);
        BTRPlaySound("./ball/whoosh.wav");
        score = 0;
        updateScoreList();
        newScoreAt = -1;
        highScore = true;
        ballLost = false;
        fadeOut = false;
        loadMusic("./ball/hghscr.mds");
    };
    btns.push_back(highScoreEnter);
    // Level Editor stuff.
    BTRButton levEditMenu;
    levEditMenu.clickedFunc = [&]() {
        sf::Event event;
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        flipPaused(event);
        //BTRPlaySound("./ball/editselect.wav");
    };
    levEditMenu.pos = btr::Vector2f(BTRWINDOWWIDTH - winButtonSmallImage->width - 20 - wallWidth / 2, 20);
    levEditMenu.str = "Menu";
    levEditBtns.push_back(levEditMenu);

    BTRButton levEditPlay;
    levEditPlay.clickedFunc = [&]() {
        *playArea = *editPlayArea;
        singlePlay.clickedFunc();
        sf::Event event;
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        flipPaused(event, false);
        playArea->levnum = -1;
        playArea->SpawnInitialBall();
    };
    levEditPlay.smallButton = 1;
    levEditPlay.str = "Play";
    levEditPlay.pos = btr::Vector2f(levEditMenu.pos.x - winButtonSmallImage->width - 20, 20);
    levEditBtns.push_back(levEditPlay);

    BTRButton levEditLoad;
    levEditLoad.clickedFunc = [&]() {
        editPlayArea->bricks.clear();
        auto file = std::ifstream();
        file.open("./lev/cust.btrlev");
        if (!file.is_open()) return;
        char* header = new char[6];
        file.read (header,6);		
		if (strncmp(header,"BTRLEV",6) == 0)
		{
			unsigned char endianness = 0;
			file.read((char*)&endianness,1);
			while(!file.eof())
			{
                int curBrickID = 0;
				BTRbrick curBrick;
				file.read((char*)&curBrickID,sizeof(int));
				int x,y;
				file.read((char*)&x,sizeof(x));
				file.read((char*)&y,sizeof(y));
				curBrick.x = x;
				curBrick.y = y;
                curBrick.brickID = curBrickID;
                if (curBrick.brickID >= 64) curBrick.isFireball = true;
				editPlayArea->bricks.push_back(curBrick);
			}
        }
    };
    levEditLoad.str = "Load";
    levEditLoad.pos = btr::Vector2f(levEditPlay.pos.x - winButtonSmallImage->width - 20, 20);
    levEditBtns.push_back(levEditLoad);

    BTRButton levEditSave;
    levEditSave.clickedFunc = [editPlayArea]() {
        editPlayArea->ExportBricks();
    };
    levEditSave.smallButton = 1;
    levEditSave.str = "Save";
    levEditSave.pos = btr::Vector2f(levEditLoad.pos.x - winButtonSmallImage->width - 20, 20);
    levEditBtns.push_back(levEditSave);

    BTRButton levEditNew;
    levEditNew.clickedFunc = [&editPlayArea]() {
        editPlayArea->bricks.clear();
    };
    levEditNew.smallButton = 1;
    levEditNew.str = "New";
    levEditNew.pos = btr::Vector2f(levEditSave.pos.x - winButtonSmallImage->width - 20, 20);
    levEditBtns.push_back(levEditNew);

    // In-game menu stuff.
    BTRButton testMenuWindow;
    testMenuWindow.clickedFunc = [&]() {
        BTRMenuUIWindow menuWindow;
        BTRButton menuWindowQuitButton;
        menuWindowQuitButton.clickedFunc = closeMenuWindow;
        menuWindowQuitButton.str = "Close";
        menuWindowQuitButton.pos = btr::Vector2f(0,0);
        menuWindow.buttons.push_back(menuWindowQuitButton);
        menuWindow.position = btr::Vector2f(320,240);
        menuWindow.size = btr::Vector2f(100,100);
        menuWindow.nameOfWindow = "TestMenuWindow";
        menuWindowStack.push(menuWindow);
    };
    testMenuWindow.pos = btr::Vector2f(highScoreEnter.pos.x, highScoreEnter.pos.y + 40);
    testMenuWindow.str = "testMenuWindow";
    btns.push_back(testMenuWindow);

    sf::Sprite brickSprite;
    playArea->LoadBrickTex();
    brickSprite.setTexture(playArea->brickTexture, true);
    auto drawBricks = [&]() {
        sf::Sprite brickSprite;
        playArea->LoadBrickTex();
        brickSprite.setTexture(playArea->brickTexture, true);
        sf::VertexArray brickVertArray;
        brickVertArray.setPrimitiveType(sf::PrimitiveType::Triangles);
        for (auto &curBrick : playArea->bricks)
        {
            if (curBrick.brickID != 63 || (curBrick.brickID == 63 && isLevEdit))
            {
                auto& curBrickIntRect = playArea->brickTexRects[curBrick.isFireball ? fireBrickFrame : curBrick.brickID - 1];
                AppendVerticesFromRect(curBrickIntRect,btr::Vector2f(curBrick.x,curBrick.y),brickVertArray);
// Below commented out code is the more higher-level representation of the above.
#if 0
                if (curBrick.isFireball)
                {
                    brickSprite.setTextureRect(playArea->brickTexRects[fireBrickFrame]);
                }
                else brickSprite.setTextureRect(playArea->brickTexRects[curBrick.brickID - 1]);
                brickSprite.setPosition(btr::Vector2f(curBrick.x, curBrick.y));
                window->draw(brickSprite);
#endif
            }
        }


        for (auto &curExplBrick : explodingBricks)
        {
            if (curExplBrick.frameOffset >= 7)
            {
                curExplBrick.frameOffset = 0;
                curExplBrick.loop++;
            }
            AppendVerticesFromRect(playArea->brickTexRects[128ll + curExplBrick.frameOffset],curExplBrick.pos,brickVertArray);
            curExplBrick.frameOffset++;
        }
        sf::RenderStates vertexRenderstate(&playArea->brickTexture);
        window->draw(brickVertArray,vertexRenderstate);
#if __cplusplus <= 201703L    
        for (int i = 0; i < explodingBricks.size(); i++)
        {
            if (explodingBricks[i].loop > 4)
            {
                explodingBricks.erase(explodingBricks.begin() + i);
            }
        }
#else
        std::erase_if(explodingBricks, [](BTRExplodingBricks& explodingBrick){ return explodingBrick.loop > 4; });
#endif
        fireBrickFrame++;
        if (fireBrickFrame >= 128)
            fireBrickFrame = 64;
    };
    auto drawBricksFromArea = [&](BTRPlayArea *area) {
        auto orgArea = playArea;
        playArea = area;
        drawBricks();
        playArea = orgArea;
    };
    auto drawSparks = [&]() {
        sf::VertexArray simpleSparkVertArray;
        if (simpleSparks)
            simpleSparkVertArray.resize(5 * sparks.size());
        
        for (int i = 0; i < sparks.size(); i++)
        {
            auto curSpark = sparks[i];
            if (frameCnt % 3 == 0)
                sparks[i].sparkRect.left += 3;
            if (simpleSparks)
            {
                sf::Color sideCol = sf::Color(116 * double(1. - sparks[i].sparkRect.left / (15. * 3.)),
                                              116 * double(1. - sparks[i].sparkRect.left / (15. * 3.)),
                                              116 * double(1. - sparks[i].sparkRect.left / (15. * 3.)))
                                              * sparks[i].color;
                sf::Color mainCol = sf::Color(252 * double(1. - sparks[i].sparkRect.left / (15. * 3.)),
                                              252 * double(1. - sparks[i].sparkRect.left / (15. * 3.)),
                                              252 * double(1. - sparks[i].sparkRect.left / (15. * 3.)))
                                              * sparks[i].color;
                //sf::VertexArray vertArray = sf::VertexArray(sf::PrimitiveType::Points,5);
                simpleSparkVertArray[i * 5 + 0] = sf::Vertex(btr::Vector2f(sparks[i].x,sparks[i].y) + btr::Vector2f(-1,0),sideCol);
                simpleSparkVertArray[i * 5 + 1] = sf::Vertex(btr::Vector2f(sparks[i].x,sparks[i].y) + btr::Vector2f(1,0),sideCol);
                simpleSparkVertArray[i * 5 + 2] = sf::Vertex(btr::Vector2f(sparks[i].x,sparks[i].y) + btr::Vector2f(0,1),sideCol);
                simpleSparkVertArray[i * 5 + 3] = sf::Vertex(btr::Vector2f(sparks[i].x,sparks[i].y) + btr::Vector2f(0,-1),sideCol);
                simpleSparkVertArray[i * 5 + 4] = sf::Vertex(btr::Vector2f(sparks[i].x,sparks[i].y),mainCol);
                //window->draw(vertArray);
            }
            else
            {
                sparkSprite->sprite.setPosition(sparks[i].x, sparks[i].y);
                sparkSprite->sprite.setColor(sparks[i].color);
                sparkSprite->sprite.setTextureRect(curSpark.sparkRect);
            }
#if __cplusplus <= 201703L
            if (sparks[i].sparkRect.left < 15 * 3)
#endif
            {
                if (!simpleSparks) window->draw(sparkSprite->sprite);
            }
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
        if (simpleSparks) window->draw(simpleSparkVertArray);
    };
    char explBrickOffset = 0;
    bool explBrickReverse = false;
    auto brickDisplayPos = btr::Vector2f(BTRWINDOWWIDTH / 2 - editPlayArea->brickwidth / 2, BTRWINDOWHEIGHT - (editPlayArea->brickheight - 15 * 4.5) - wincornerImage->height / 4);
#if defined(__ANDROID__) || defined(ANDROID)
    cursorVisible = false;
    window->setMouseCursorVisible(false);
#endif
    if (isFullscreen)
    {
        cursorVisible = false;
        window->setMouseCursorVisible(false);
    }
    while (window->isOpen())
    {
        sf::Event event;
        while (window->pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                quitGame.clickedFunc();
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
                if (cheatText || highScore)
                {
                    if (event.text.unicode == 8 && !cheatstr.empty())
                        cheatstr.pop_back();
                    else if (event.text.unicode == 10 && highScore)
                    {
                        if (newScoreAt != -1)
                        {
                            std::get<std::string>(scoresAndNames[newScoreAt]) = cheatstr;  
                            scoreTexts[newScoreAt].movingText.replace(newScoreAt > 9 ? 4 : 3,cheatstr.size(),cheatstr);
                            newScoreAt = -1;
                            sf::Keyboard::setVirtualKeyboardVisible(false);
                            BTRPlaySound("./ball/whoosh.wav");
                        }                        
                    }
                    else
                        cheatstr += static_cast<char>(event.text.unicode);
                }
                break;
            case sf::Event::TouchEnded:
                {
                    int x = ((double)event.touch.x / sf::VideoMode::getDesktopMode().width) * (double)BTRWINDOWWIDTH;
                    int y = ((double)event.touch.y / sf::VideoMode::getDesktopMode().height) * (double)BTRWINDOWHEIGHT;
                    event.type = sf::Event::MouseButtonReleased;
                    event.mouseButton.x = x;
                    event.mouseButton.y = y;
                }
                
            case sf::Event::MouseButtonReleased:
                if (menu)
                {
                    if (!menuWindowStack.empty())
                    {
                        BTRMenuUIWindow& menuWindow = menuWindowStack.top();
                        for (auto &curBtn : menuWindow.buttons)
                        {
                            if (event.mouseButton.x >= curBtn.pos.x + menuWindow.position.x
                            && event.mouseButton.x <= curBtn.pos.x + winButtonSmallImage->width + menuWindow.position.x
                            && event.mouseButton.y <= curBtn.pos.y + winButtonSmallImage->height / 2 + menuWindow.position.y
                            && event.mouseButton.y >= curBtn.pos.y + menuWindow.position.y)
                            {
                                curBtn.clickedFunc();
                                BTRPlaySound("./ball/editselect.wav");
                                if (menuWindowStack.empty() || (!menuWindowStack.empty() && std::addressof(menuWindow) != std::addressof(menuWindowStack.top()))) break;
                            }
                            curBtn.wasHeld = false;
                        }
                    }
                    else for (auto &curBtn : btns)
                    {
                        if (event.mouseButton.x >= curBtn.pos.x && event.mouseButton.x <= curBtn.pos.x + winButtonImage->width && event.mouseButton.y <= curBtn.pos.y + winButtonImage->height / 2 && event.mouseButton.y >= curBtn.pos.y && curBtn.wasHeld)
                        {
                            curBtn.clickedFunc();
                            BTRPlaySound("./ball/editselect.wav");
                        }
                        curBtn.wasHeld = false;
                    }
                }
                if (isLevEdit && !menu)
                {
                    for (auto &curBtn : levEditBtns)
                    {
                        if (event.mouseButton.x >= curBtn.pos.x && event.mouseButton.x <= curBtn.pos.x + winButtonImage->width && event.mouseButton.y <= curBtn.pos.y + 20 && event.mouseButton.y >= curBtn.pos.y && curBtn.wasHeld)
                        {
                            curBtn.clickedFunc();
                            BTRPlaySound("./ball/editselect.wav");
                        }
                        curBtn.wasHeld = false;
                    }
                }
                break;
                case sf::Event::TouchBegan:
                {
                    int x = ((double)event.touch.x / sf::VideoMode::getDesktopMode().width) * (double)BTRWINDOWWIDTH;
                    int y = ((double)event.touch.y / sf::VideoMode::getDesktopMode().height) * (double)BTRWINDOWHEIGHT;
                    event.type = sf::Event::MouseButtonPressed;
                    event.mouseButton.x = x;
                    event.mouseButton.y = y;
                }
// Disabled until I implement proper resizing support.            
#if 0
            case sf::Event::Resized:
            {
                sf::View view = window->getView();
                view.setViewport(sf::FloatRect((event.size.width / 2 - BTRWINDOWWIDTH / 2) / (double)event.size.width,
                                 (event.size.height / 2 - BTRWINDOWHEIGHT / 2)  / (double)event.size.height,
                                BTRWINDOWWIDTH / (double)event.size.width,BTRWINDOWHEIGHT / (double)event.size.height));
                window->setView(view);
                windowTexture.create(event.size.width,event.size.height);
                break;
            }
#endif            
            
            case sf::Event::MouseButtonPressed:

                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    for (auto &curBall : playArea->balls)
                    {
                        curBall->ballHeld = false;
                    }
#if 0
                    auto curMissile = new BTRMissileObject;
                    curMissile->x = btr::Mouse::getPosition(*window).x;
                    curMissile->y = btr::Mouse::getPosition(*window).y;
                    playArea->missiles.push_back(std::shared_ptr<BTRMissileObject>(curMissile));
#endif
                    if (menu)
                    {
                        if (!menuWindowStack.empty())
                        {
                            BTRMenuUIWindow& menuWindow = menuWindowStack.top();
                            for (auto &curBtn : menuWindow.buttons)
                            {
                                if (event.mouseButton.x >= curBtn.pos.x + menuWindow.position.x
                                && event.mouseButton.x <= curBtn.pos.x + winButtonSmallImage->width + menuWindow.position.x
                                && event.mouseButton.y <= curBtn.pos.y + winButtonSmallImage->height / 2 + menuWindow.position.y
                                && event.mouseButton.y >= curBtn.pos.y + menuWindow.position.y)
                                {
                                    curBtn.wasHeld = true;
                                }
                            }
                        }
                        else for (auto &curBtn : btns)
                        {
                            if (event.mouseButton.x >= curBtn.pos.x && event.mouseButton.x <= curBtn.pos.x + winButtonImage->width && event.mouseButton.y <= curBtn.pos.y + 20 && event.mouseButton.y >= curBtn.pos.y)
                            {
                                curBtn.wasHeld = true;
                            }
                        }
                    }
                    if (isLevEdit && !menu)
                    {
                        for (auto &curBtn : levEditBtns)
                        {
                            if (event.mouseButton.x >= curBtn.pos.x && event.mouseButton.x <= curBtn.pos.x + winButtonSmallImage->width && event.mouseButton.y <= curBtn.pos.y + 20 && event.mouseButton.y >= curBtn.pos.y)
                            {
                                curBtn.wasHeld = true;
                            }
                        }
                    }
                }
                break;
            case sf::Event::MouseWheelScrolled:
            {
                if (!isLevEdit) break;
                if (event.mouseWheelScroll.delta < 0)
                {
                    brickID -= 1;
                    if (brickID <= 0)
                    {
                        brickID = 64;
                    }
                }
                else if (event.mouseWheelScroll.delta > 0)
                {
                    brickID = std::clamp(++brickID, (unsigned char)1, (unsigned char)64);
                }
                break;
            }
            case sf::Event::KeyPressed:
                if (endofgame)
                {
                    eot = true;
                    thread->detach();
                }
                if (event.key.code == sf::Keyboard::Pause || event.key.code == sf::Keyboard::Escape)
                {
                    if (!highScore)
                    {
                        flipPaused(event);
                    }
                    else
                    {
                        windowTexture.update(*window);
                        playArea->levnum = 0;
                        playArea->levelEnded = true;
                        highScore =
                            ballLost =
                                endofgame = false;
                        fadeOut = true;
                        scrRect.setFillColor(sf::Color(0, 0, 0, 255));
                        fade = 1;
                        lives = 0;
                        demo = true;
                        score = 0;
                        scoreTexts.clear();
                        randPlayedSet = 0;
                        //midiStreamPause(midiDev)
                    }
                }
                if (event.key.code == sf::Keyboard::Enter)
                {
                    if (event.key.alt)
                    {
                        delete window;
                        isFullscreen ^= 1;
                        if (isFullscreen)
                        {
                            window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn", sf::Style::Titlebar | sf::Style::Close | sf::Style::Fullscreen);
                            windowTexture.create(BTRWINDOWWIDTH,BTRWINDOWHEIGHT);
                        }
                        else
                            window = new sf::RenderWindow(sf::VideoMode(BTRWINDOWWIDTH, BTRWINDOWHEIGHT), "Blast Thru Reborn", sf::Style::Titlebar | sf::Style::Close);
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
                        catch (std::out_of_range)
                        {
                        } // Intentional.                    
                        cheatText = false;
                        cheatstr.clear();
                    }
                    else if (highScore)
                    {
                        if (newScoreAt != -1)
                        {
                            std::get<std::string>(scoresAndNames[newScoreAt]) = cheatstr;  
                            scoreTexts[newScoreAt].movingText.replace(newScoreAt > 9 ? 4 : 3,cheatstr.size(),cheatstr);
                            newScoreAt = -1;
                            sf::Keyboard::setVirtualKeyboardVisible(false);
                            BTRPlaySound("./ball/whoosh.wav");
                        }
                        else
                        {
                            windowTexture.update(*window);
                            playArea->levnum = 0;
                            playArea->levelEnded = true;
                            highScore =
                                ballLost =
                                    endofgame = false;
                            fadeOut = true;
                            scrRect.setFillColor(sf::Color(0, 0, 0, 255));
                            fade = 1;
                            lives = 0;
                            demo = true;
                            score = 0;
                            scoreTexts.clear();
                            randPlayedSet = 0;
                            //midiStreamPause(midiDev);
                        }
                    }
                }
                break;
            }
        }
        if (!window->isOpen())
            break;
        if (highScore)
        {
            window->clear();
            window->draw(highScoreSprite);
            if (newScoreAt != -1)
            {
                for (auto &curMovingText : moveTexts)
                {
                    curMovingText.Tick();
                    font->RenderChars(curMovingText.movingText, curMovingText.pos, window);
                }
                winBoxImage2->sprite.setPosition(btr::Vector2f(BTRWINDOWWIDTH,BTRWINDOWHEIGHT) / 2.f - btr::Vector2f(winBoxImage2->width / 2,0));
                window->draw(*winBoxImage2);
                font->RenderChars(cheatstr + '|',winBoxImage2->sprite.getPosition(),window);
                window->display();
                continue;
            }
            for (auto &curMovingText : scoreTexts)
            {
                curMovingText.Tick();
                font->RenderChars(curMovingText.movingText, curMovingText.pos, window);
            }
            font->RenderChars("Hall of Scores", btr::Vector2f(BTRWINDOWWIDTH / 2, font->genCharHeight * 2) - btr::Vector2f(font->GetSizeOfText("Hall of Fame").x / 2, 0), window);
            window->display();
            continue;
        }
        if (endofgame)
        {
            if (!loadedEndMusic)
            {
                loadedEndMusic = true;
                loadMusic("./ball/exmil.mds", true);
            }
            if (thread && !thread->joinable())
            {
                loadedEndMusic = false;
                endofgame = false;
                lives = 0;
                fadeOut = true;
                ballLost = true;
                playArea->balls.clear();
                continue;
            }
            window->clear();
            windowSprite.setColor(sf::Color(255, 255, 255, 255 * 0.5));
            window->draw(windowSprite);
            largeFont2->RenderChars("excellent", btr::Vector2f(BTRWINDOWWIDTH / 2 - font->GetSizeOfText("excellent").x / 2, 0), window, sf::Color(255, 255, 0));
            font->RenderChars("You completed the game!", btr::Vector2f(BTRWINDOWWIDTH / 2, largeFont2->GetSizeOfText("excellent").y) - btr::Vector2f(font->GetSizeOfText("You completed the game!").x / 2, 0), window);
            window->display();
            continue;
        }
        if (cheatText)
        {
            window->clear();
            windowSprite.setColor(sf::Color(255, 255, 255, 255 * 0.5));
            window->draw(windowSprite);
            winBoxImage->sprite.setPosition(btr::Vector2f(BTRWINDOWWIDTH / 2 - winBoxImage->width / 2, BTRWINDOWHEIGHT / 2));
            window->draw(*winBoxImage);
            font->RenderChars("Cheaters never prosper...", winBoxImage->sprite.getPosition() - btr::Vector2f(0, font->genCharHeight), window);
            font->RenderChars(cheatstr + '|', winBoxImage->sprite.getPosition(), window);
            window->display();
            continue;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) && sf::Keyboard::isKeyPressed(sf::Keyboard::H))
        {
            cheatText = true;
            cheatstr.clear();
        }
        if (paused)
        {
            window->clear();
            windowSprite.setColor(sf::Color(255, 255, 255, 255 * 0.5));
            window->draw(windowSprite);
            if (!menu)
            {
                pausedSprite->sprite.setPosition(btr::Vector2f(BTRWINDOWWIDTH / 2, BTRWINDOWHEIGHT / 2) - btr::Vector2f(pausedSprite->width / 2, pausedSprite->height / 2));
                window->draw(pausedSprite->sprite);
            }
            if (menu)
            {
                titleImage->sprite.setPosition(btr::Vector2f(BTRWINDOWWIDTH / 2 - titleImage->width / 2, 0));
                window->draw(*titleImage);
                btr::Vector2f pos = titleImage->sprite.getPosition() + btr::Vector2f(0, titleImage->height);
                if (!menuWindowStack.empty())
                {
                    auto& topMenu = menuWindowStack.top();
                    DrawFrame(window,topMenu.position,topMenu.size);
                    font->RenderChars(topMenu.nameOfWindow,topMenu.position,window);
                    for (auto &curBtn : topMenu.buttons)
                    {
                        winButtonSmallImage->sprite.setPosition(curBtn.pos + topMenu.position);
                        winButtonSmallImage->SetSpriteIndex(curBtn.wasHeld);
                        window->draw(*winButtonSmallImage);
                        auto curBtnPos = curBtn.pos  + topMenu.position;
                        curBtnPos.x += winButtonSmallImage->width / 2;
                        curBtnPos -= btr::Vector2f(font->GetSizeOfText(curBtn.str).x / 2, 0);
                        //curBtnPos += btr::Vector2f(0, curBtn.wasHeld ? 5 : 0);
                        font->RenderChars(curBtn.str, curBtnPos, window);            
                    }
                }
                else
                {
                    DrawFrame(window, pos, btr::Vector2f(300, 370));
                    font->RenderChars("Game Menu", btr::Vector2f(640 / 2 - font->GetSizeOfText("Game Menu").x / 2, font->genCharHeight + pos.y), window);
                    for (auto &curBtn : btns)
                    {
                        winButtonImage->sprite.setPosition(curBtn.pos);
                        winButtonImage->SetSpriteIndex(curBtn.wasHeld);
                        window->draw(*winButtonImage);
                        auto curBtnPos = curBtn.pos;
                        curBtnPos.x += winButtonImage->width / 2;
                        curBtnPos -= btr::Vector2f(font->GetSizeOfText(curBtn.str).x / 2, 0);
                        //curBtnPos += btr::Vector2f(0, curBtn.wasHeld ? 5 : 0);
                        font->RenderChars(curBtn.str, curBtnPos, window);
                    }
                }
                if (!cursorVisible)
                {
                    ball->SetSpriteIndex(1);
                    ball->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window) - btr::Vector2f(ball->realWidthPerTile / 2, ball->realHeightPerTile / 2));
                    cursor->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window) - btr::Vector2f(cursor->width / 2, cursor->height / 2));
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
            frameCnt = 0;
            if (ballLost && lives <= 0)
            {
                if (demo) score = 0;
                BTRPlaySound("./ball/scream.wav");
                fadeToColor(sf::Color(255, 255, 255, 255));
                endofgame = false;
                updateScoreList();
                if (demo) newScoreAt = -1;
                if (newScoreAt != -1) sf::Keyboard::setVirtualKeyboardVisible(true);
                fadeOut = false;
                ballLost = false;
                highScore = true;
                BTRPlaySound("./ball/whoosh.wav");
                loadMusic("./ball/hghscr.mds");
                continue;
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
                playArea->missiles.clear();
                delete chompteeth;
                chompteeth = 0;
                playArea->paddle.stateFlags = 0;
                playArea->paddle.sprite->sprite.setPosition(cursor->sprite.getPosition());
                if (!ballLost)
                {
                    if (playArea->levnum != -1 || playArea->levelEnded)
                    {
                        std::string str = "./lev/";
                        bool isRandom = playArea->randomPlay || demo;
                        long long newLev = 0;
                        playArea->levelEnded = 0;
                        while (1)
                        {
                            if (!playArea->randomPlay)
                                break;
                            randPlayedSet |= 1ll << (long long)(playArea->levnum - 1ll);
                            newLev = std::uniform_int_distribution<long long>(1ll, 40ll)(rd) - 1ll;
                            if (randPlayedSet == 0xFFFFFFFFFFll)
                            {
                                std::cout << "All bits set" << std::endl;
                                break;
                            }
                            if (randPlayedSet & (1ll << newLev))
                                continue;
                            std::cout << "Rand bits set: " << std::bitset<40>(randPlayedSet) << std::endl;
                            break;
                        }
                        newLev = isRandom ? newLev : playArea->levnum++;
                        newLev = std::clamp(newLev, 0ll, 39ll);
                        str += std::to_string(newLev) + ".lev";
                        int oldLevnum = playArea->levnum;
                        delete playArea;
                        playArea = 0;
                        playArea = new BTRPlayArea(str, window);
                        playArea->levelEnded = false;
                        playArea->levnum = isRandom ? ++newLev : oldLevnum++;
                        playArea->randomPlay = isRandom;
                        explodingBricks.clear();
                    }                    
                    loadMusic(demo ? "./ball/org.mds" : musics[mdsrand(gen)]);
                    BTRPlaySound("./ball/grow.wav");
                    if (demo)
                    {
                        for (auto &curBall : playArea->balls)
                        {
                            curBall->ballHeld = false;
                        }
                        lives = 0;
                    }
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

        auto curTime = clocktime.now().time_since_epoch();
        DrawBackground();
        ball->Animate();
        if (isLevEdit)
        {
            window->setFramerateLimit(40);
            drawBricksFromArea(editPlayArea);
            DrawFrame(window, btr::Vector2f(wallWidth / 2, wincornerImage->height / 2), btr::Vector2f(BTRWINDOWWIDTH - wallWidth, 50));
            DrawFrame(window, btr::Vector2f(wallWidth / 2, BTRWINDOWHEIGHT - (editPlayArea->brickheight - 15 * 4)), btr::Vector2f(BTRWINDOWWIDTH - wallWidth, editPlayArea->brickheight - 15 * 4 - wincornerImage->height / 2));
            for (auto &curBtn : levEditBtns)
            {
                winButtonSmallImage->sprite.setPosition(curBtn.pos);
                winButtonSmallImage->SetSpriteIndex(curBtn.wasHeld);
                window->draw(*winButtonSmallImage);
                auto curBtnPos = curBtn.pos;
                curBtnPos.x += winButtonSmallImage->width / 2;
                curBtnPos -= btr::Vector2f(font->GetSizeOfText(curBtn.str).x / 2, 0);
                //curBtnPos += btr::Vector2f(0, curBtn.wasHeld ? 5 : 0);
                font->RenderChars(curBtn.str, curBtnPos, window);
            }
            auto brickRect = sf::IntRect(sf::Vector2i(), sf::Vector2i(editPlayArea->brickwidth, editPlayArea->brickheight - 15 * 5));
            auto orgBrickRect = brickSprite.getTextureRect();
            brickSprite.setTexture(editPlayArea->brickTexture);
            brickSprite.setTextureRect(brickRect);
            brickSprite.setPosition(brickDisplayPos);
            window->draw(brickSprite);
            auto brickTexPos = brickSprite.getPosition();
            sf::RectangleShape outlineRect(btr::Vector2f(30, 15));
            sf::IntRect rect = editPlayArea->brickTexRects[brickID - 1];
            outlineRect.setPosition(brickTexPos + btr::Vector2f(rect.left, rect.top));
            outlineRect.setOutlineColor(sf::Color(255, 0, 0, 255));
            outlineRect.setOutlineThickness(-1);
            outlineRect.setFillColor(sf::Color::Transparent);
            window->draw(outlineRect);
            if (explBrickReverse)
                explBrickOffset--;
            else
                explBrickOffset++;
            if (explBrickOffset >= 6 || explBrickOffset < 0)
            {
                explBrickReverse ^= 1;
                if (explBrickOffset < 0)
                    explBrickOffset = 0;
            }
            brickSprite.setPosition(brickTexPos + btr::Vector2f(rect.left, rect.top));
            brickSprite.setTextureRect(editPlayArea->brickTexRects[128ll + explBrickOffset]);
            window->draw(brickSprite);
            if (!cursorVisible)
            {
                ball->SetSpriteIndex(1);
                ball->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window) - btr::Vector2f(ball->realWidthPerTile / 2, ball->realHeightPerTile / 2));
                cursor->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window) - btr::Vector2f(cursor->width / 2, cursor->height / 2));
                window->draw(cursor->sprite);
                window->draw(ball->sprite);
                //ball->Animate();
                ball->SetSpriteIndex(0);
            }
            {
                auto mousePos = btr::Mouse::getPosition(*window);
                if (btr::Mouse::isButtonPressed(sf::Mouse::Left) && mousePos.x >= brickDisplayPos.x && mousePos.y >= brickDisplayPos.y && mousePos.x <= brickDisplayPos.x + brickRect.width && mousePos.y <= brickDisplayPos.y + brickRect.height)
                {
                    auto mousePosInsideBrickDisp = mousePos - (sf::Vector2i)brickDisplayPos;
                    for (int index = 0; index < editPlayArea->brickTexRects.size(); index++)
                    {
                        auto &curBrickRect = editPlayArea->brickTexRects[index];
                        if (curBrickRect.contains(mousePosInsideBrickDisp))
                        {
                            auto prevBrickID = brickID;
                            brickID = std::clamp(index + 1,1,64);
                            if (brickID != prevBrickID) BTRPlaySound("./ball/editselect.wav");
                            break;
                        }
                    }
                }
            }
            if (isLevEdit && !paused && btr::Mouse::getPosition(*window).y >= 60 && btr::Mouse::getPosition(*window).y <= (BTRWINDOWHEIGHT - 15 * 5) && (btr::Mouse::isButtonPressed(sf::Mouse::Left) || btr::Mouse::isButtonPressed(sf::Mouse::Right)))
            {
                auto mousePos = btr::Mouse::getPosition(*window);
                bool brickIDsame = false;
                for (int i = 0; i < editPlayArea->bricks.size(); i++)
                {
                    auto &curBrick = editPlayArea->bricks[i];
                    if (mousePos.x >= curBrick.x && mousePos.y >= curBrick.y && mousePos.x <= curBrick.x + curBrick.width && mousePos.y <= curBrick.y + curBrick.height)
                    {
                        if ((btr::Mouse::isButtonPressed(sf::Mouse::Left) && curBrick.brickID != brickID) || (btr::Mouse::isButtonPressed(sf::Mouse::Right)))
                            editPlayArea->bricks.erase(editPlayArea->bricks.begin() + i);
                        else
                            brickIDsame = true;
                        if (btr::Mouse::isButtonPressed(sf::Mouse::Right))
                            BTRPlaySound("./ball/explode.wav");
                    }
                }

                if (btr::Mouse::isButtonPressed(sf::Mouse::Left) && !brickIDsame)
                {
                    BTRPlaySound("./ball/editbrick.wav");
                    auto curPos = btr::Mouse::getPosition(*window);
                    curPos.x -= wallWidth / 2;
                    auto gridAlignX = curPos.x % 30;
                    auto gridAlignY = curPos.y % 15;
                    spawnSpark(30, sf::Color(255, 255, 255), ((btr::Vector2f)curPos - btr::Vector2f(gridAlignX, gridAlignY)) + btr::Vector2f(wallWidth / 2, 0));
                    spawnObject(
                        btr::Vector2f((curPos.x - gridAlignX) + wallWidth / 2, curPos.y - gridAlignY),
                        std::function<void(BTRbrick &)>() = [editPlayArea](BTRbrick &brick) {
                            brick.brickID = brickID;
                            if (brick.brickID > 64)
                                brick.brickID = 64;
                            if (brick.brickID == 64)
                                brick.isFireball = true;
                            editPlayArea->bricks.push_back(brick);
                        });
                }
            }
            drawSparks();
            windowTexture.update(*window);
            window->display();
            frameCnt++;
            continue;
        }
        playArea->Tick();
        playArea->paddle.sprite->Animate();
        playArea->paddle.sprite->sprite.setTextureRect(sf::IntRect(sf::Vector2i(playArea->paddle.sprite->width / 2 - playArea->paddle.drawPaddleRadius / 2, playArea->paddle.sprite->realHeightPerTile * (playArea->paddle.sprite->animFramePos - 1)), sf::Vector2i(playArea->paddle.drawPaddleRadius, playArea->paddle.sprite->realHeightPerTile)));
        playArea->paddle.sprite->sprite.setOrigin(-playArea->paddle.paddleRadius / 2 + playArea->paddle.drawPaddleRadius / 2, 0);

        if (drawCornerText)
            font->RenderChars(textToDraw, btr::Vector2f(wallWidth / 2, BTRWINDOWHEIGHT - font->genCharHeight), window);
        if (frameCnt % 10 == 0 && cornerTextBlinkTime < 7)
        {
            drawCornerText ^= 1;
            cornerTextBlinkTime++;
        }

        drawBricks();
        for (int i = 0; i < playArea->powerups.size(); i++)
        {
            if (vertvelpowerup)
                playArea->powerups[i].aliveTick++;
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
            if (playArea->powerups[i].destroyed == true)
                playArea->powerups.erase(playArea->powerups.begin() + i);
        }

        for (auto &curball : playArea->balls)
        {
            if (curball->isFireball)
                ball->SetSpriteIndex(1);
            else
                ball->SetSpriteIndex(0);
            ball->sprite.setPosition(btr::Vector2f(curball->x, curball->y));
            if (!curball->invisibleSparkling)
                window->draw(ball->sprite);
            if (curball->ballHeld)
                magnetHeldBall++;
        }
        for (auto &curMissile : playArea->missiles)
        {
            powerupSprite->sprite.setPosition(curMissile->x, curMissile->y);
            powerupSprite->SetTexRect(15, 1);
            //powerupSprite->sprite.setOrigin(btr::Vector2f(8, 0));
            window->draw(powerupSprite->sprite);
            //powerupSprite->sprite.setOrigin(btr::Vector2f(0, 0));
        }
        drawSparks();
        largeFont->RenderChars(std::to_string(score), btr::Vector2f(wallWidth / 2, 0), window);
        font->RenderChars(playArea->levnum == -1 ? "custom level" : "level " + std::to_string(playArea->levnum), btr::Vector2f(wallWidth / 2, largeFont->genCharHeight), window);
        if (!cursorVisible)
        {
            ball->SetSpriteIndex(1);
            ball->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window) - btr::Vector2f(ball->realWidthPerTile / 2, ball->realHeightPerTile / 2));
            cursor->sprite.setPosition((btr::Vector2f)btr::Mouse::getPosition(*window) - btr::Vector2f(cursor->width / 2, cursor->height / 2));
        }
        for (auto &curExpl : explosions)
        {
            curExpl.Tick();
            if (curExpl.destroyed)
            {
                continue;
            }
            explSprite->SetTexRect(curExpl.framesPassed, curExpl.spriteIndex);
            explSprite->sprite.setPosition(curExpl.pos);
            window->draw(explSprite->sprite);
        }
        for (int i = 0; i < explosions.size(); i++)
        {
            if (explosions[i].destroyed)
            {
                explosions.erase(explosions.begin() + i);
                continue;
            }
        }
        for (auto &curDebris : playArea->debrisObjects)
        {
            auto pos = curDebris.shape.getPosition();
            curDebris.velY += curDebris.gravity;
            pos.x += curDebris.velX;
            pos.y += curDebris.velY;
            curDebris.shape.setPosition(pos);
            curDebris.shape.setRotation(curDebris.shape.getRotation() + 9.);
            window->draw(curDebris.shape);
            curDebris.shape.setFillColor(curDebris.shape.getFillColor() - sf::Color(0,0,0,255 * (1/40.)));
            if (curDebris.shape.getFillColor().a <= 0) curDebris.destroyed = true;
        }
        for (int i = 0; i < playArea->debrisObjects.size(); i++)
        {
            if (playArea->debrisObjects[i].destroyed)
            {
                playArea->debrisObjects.erase(playArea->debrisObjects.begin() + i);
            }
        }
        if (!ballLost)
        {
            if (demo)
            { 
                // Demo logic is here.
                double posXOfPaddle = playArea->paddle.sprite->sprite.getPosition().x;
                if (playArea->balls[playArea->getClosestBall()]->y <= 400)
                {
                    if (playArea->powerups.size() >= 1)
                    {                        
                        playArea->paddle.sprite->sprite.setPosition(std::clamp((double)std::lerp(posXOfPaddle,(float)playArea->powerups[0].x - (double)playArea->paddle.paddleRadius / 2.,0.50), wallWidth / 2., BTRWINDOWWIDTH - wallWidth / 2. - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
                    }
                    if (playArea->paddle.stateFlags & BTRPaddle::PADDLE_MISSILE)
                    {
                        static double brickXPos = wallWidth / 2;
                        std::uniform_int_distribution<int> randDist(0,playArea->horzPosOfBricks.size()-1);
                        if ((frameCnt % 80) == 0)
                        {
                            int itNum = randDist(gen);
                            for (auto& curVal : playArea->horzPosOfBricks)
                            {
                                itNum--;
                                if (itNum <= 0)
                                {
                                    brickXPos = curVal;
                                    break;
                                }
                            }
                        }
                        playArea->paddle.sprite->sprite.setPosition(std::clamp((double)std::lerp(posXOfPaddle,(float)brickXPos - (double)playArea->paddle.paddleRadius / 2.,0.50), wallWidth / 2., BTRWINDOWWIDTH - wallWidth / 2. - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
                    }
                    //else playArea->paddle.sprite->sprite.setPosition(std::clamp((float)playArea->balls[playArea->getClosestBall()]->x - (float)playArea->paddle.paddleRadius / 2, wallWidth / 2.f, BTRWINDOWWIDTH - wallWidth / 2.f - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
                    // Demo logic ends here.
                }
                else playArea->paddle.sprite->sprite.setPosition(std::clamp((double)std::lerp(posXOfPaddle,playArea->balls[playArea->getClosestBall()]->x - (float)playArea->paddle.paddleRadius / 2, 0.50), wallWidth / 2., BTRWINDOWWIDTH - wallWidth / 2. - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
            }
            else playArea->paddle.sprite->sprite.setPosition(std::clamp(cursor->sprite.getPosition().x, wallWidth / 2.f, BTRWINDOWWIDTH - wallWidth / 2.f - (float)playArea->paddle.paddleRadius), BTRWINDOWHEIGHT - 30);
        }
        drawPaddleXY(playArea->paddle.sprite->sprite.getPosition().x, playArea->paddle.sprite->sprite.getPosition().y, playArea->paddle.drawPaddleRadius);
        auto orgpos = playArea->paddle.sprite->sprite.getPosition();
        auto orgOrig = playArea->paddle.sprite->sprite.getOrigin();
        playArea->paddle.sprite->sprite.setOrigin(0, 0);
        for (int i = 0; i < lives; i++)
        {
            drawPaddleXY(BTRWINDOWWIDTH - playArea->paddle.radiuses[0] * ((long long)i + 1) - wallWidth / 2 - (9ll * i), 0, playArea->paddle.radiuses[0]);
        }
        playArea->paddle.sprite->sprite.setOrigin(orgOrig);
        for (int i = 0; i < playArea->paddle.missilesLeft; i++)
            if (playArea->paddle.stateFlags & BTRPaddle::PADDLE_MISSILE)
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
            DrawFrame(window, btr::Vector2f(BTRWINDOWWIDTH / 2 - 120, 0), btr::Vector2f(240, 20));
            tractorSprite->sprite.setPosition(btr::Vector2f(BTRWINDOWWIDTH / 2 - 120, 0));
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
        if (frameCnt >= 60 * 40 && demo)
        {
            fadeOut = true;
            ballLost = true;
            lives = 0;
        }
        if (!cursorVisible)
        {
            //window->draw(ball->sprite);
            //window->draw(cursor->sprite);
            ball->sprite.setOrigin(0, 0);
        }
        ball->SetSpriteIndex(0);
        scrRect.setSize((btr::Vector2f)window->getSize());
        scrRect.setFillColor(sf::Color(0, 0, 0, 255 * fade));
        if (fade > 0)
            fade -= 1 / (long double)40;
        else
            fade = 0;
        window->draw(scrRect);
        /*std::chrono::duration<double, std::milli> nowCurTime = clocktime.now().time_since_epoch() - curTime;
        auto frameRateStr = std::string("Render time: ") + std::to_string(nowCurTime.count());
        font->RenderChars(frameRateStr, 0, 0, window);*/
#if defined(__ANDROID__) || defined(ANDROID)
        windowTexture.create(window->getSize().x,window->getSize().y);
        windowSprite.setTexture(windowTexture,true);
#endif
        windowTexture.update(*window);
        window->display();

        frameCnt++;
        framesPassedlastPowerup++;
    }
    eot = true;
#ifdef _WIN32
    midiStreamStop(midiDev);
    midiOutReset((HMIDIOUT)midiDev);
#endif
    delete window;
    delete magnetSprite;
    delete winBoxImage;
    deInitOpenAL();
    unsigned int scheck = 0;
    if (!(inierr < 0))
        for (int i = 0; i < scoresAndNames.size(); i++)
        {
            scheck += static_cast<unsigned int>(std::get<int>(scoresAndNames[i]) & 0xFF);

            ini.SetValue("bt.ini",
                         ("score" + std::to_string(i)).c_str(),
                         (std::get<std::string>(scoresAndNames[i]) + ';' + std::to_string(std::get<int>(scoresAndNames[i]))).c_str());
            std::cout << "Saved score: " << ini.GetValue("bt.ini", ("score" + std::to_string(i)).c_str()) << std::endl;
        }
    ini.SetLongValue("bt.ini", "scheck", scheck);
    ini.SetBoolValue("bt.ini", "gfullscreen",isFullscreen);
    ini.SaveFile("./bt.ini", false);
}
