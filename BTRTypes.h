#include <limits>
#include <type_traits>
#include <system_error>
#include <chrono>
#include <thread>
#include <vector>
#include <cuchar>
#include "ConvertUTF.h"
#include <SDL2/SDL.h>

extern bool demo;

namespace btr
{
    extern SDL_Renderer* context;
    class RenderWindow;

    template <class T>
    class Vector2
    {
    public:
        T x;
        T y;
        static_assert(std::is_integral_v<T> || std::numeric_limits<T>::is_iec559);
        Vector2();
        ~Vector2() = default;
        Vector2(T x, T y);
        template <typename U>
        explicit Vector2(const Vector2<U>& vector);
    };
    #include <SFML/System/Vector2.inl>

    typedef Vector2<int> Vector2i;
    typedef Vector2<unsigned int> Vector2u;
    typedef Vector2<float> Vector2f;
    typedef Vector2<double> Vector2d;
    
    class Color
    {
    public:
        uint8_t r,g,b;
        uint8_t a = 255;
        Color() = default;
        Color(uint32_t col)
        {
            r = (col & 0xFF000000) >> 24;
            g = (col & 0xFF0000) >> 16;
            b = (col & 0xFF00) >> 8;
            a = (col & 0xFF);
        }
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            this->r = r;
            this->g = g;
            this->b = b;
            this->a = a;
        }
        static const Color Black;
        static const Color White;
        static const Color Red;
        static const Color Green;
        static const Color Blue;
        static const Color Yellow;
        static const Color Magenta;
        static const Color Cyan;
        static const Color Transparent;
    };
    
    inline const Color Color::Black(0, 0, 0);
    inline const Color Color::White(255, 255, 255);
    inline const Color Color::Red(255, 0, 0);
    inline const Color Color::Green(0, 255, 0);
    inline const Color Color::Blue(0, 0, 255);
    inline const Color Color::Yellow(255, 255, 0);
    inline const Color Color::Magenta(255, 0, 255);
    inline const Color Color::Cyan(0, 255, 255);
    inline const Color Color::Transparent(0, 0, 0, 0);


    template<class T>
    class Rect
    {
        public:
        T left, top, width, height;
        inline Rect()
        {
            left = top = width = height = 0;
        }
        inline Rect(T left, T top, T width, T height)
        {
            this->left = left;
            this->top = top;
            this->width = width;
            this->height = height;
        }
        inline Rect(const Vector2<T>& pos, const Vector2<T>& size)
        {
            this->left = pos.x;
            this->top = pos.y;
            this->width = size.x;
            this->height = size.y;
        }
        template<class U>
        inline explicit Rect(Rect<U> otherRect)
        {
            this->left = otherRect.left;
            this->top = otherRect.top;
            this->width = otherRect.width;
            this->height = otherRect.height;
        }
        inline operator SDL_Rect ()
        {
            SDL_Rect rect;
            rect.x = this->left;
            rect.y = this->top;
            rect.w = this->width;
            rect.h = this->height;
            return rect;
        }
        inline bool contains(Vector2i pos)
        {
            auto sdlrect = (SDL_Rect)*this;
            SDL_Point point;
            point.x = pos.x;
            point.y = pos.y;
            return SDL_PointInRect(&point,&sdlrect);
        }
        inline const Vector2<T> getPosition() const
        {
            return Vector2<T>(left, top);
        }
        inline const Vector2<T> getSize() const
        {
            return Vector2<T>(width, height);
        }
    };
    typedef Rect<int> IntRect;
    typedef Rect<float> FloatRect;
    class Texture
    {
    private:
        SDL_Texture* tex = NULL;
        uint32_t width = 0, height = 0;
    public:
        inline Texture() = default;
        inline ~Texture()
        {
            if (tex != NULL) SDL_DestroyTexture(tex);
        }
        inline void create(uint32_t w, uint32_t h)
        {
            tex = SDL_CreateTexture(context,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,w,h);
            if (!tex)
            {
                fprintf(stderr,"Failed to create texture: %s\n",SDL_GetError());
            }
            width = w;
            height = h;
        }
        inline void update(uint8_t* pixels)
        {
            SDL_Rect rect{0,0,(int)width,(int)height};
            fprintf(stderr, "SDL_Rect rect{%d,%d,%d,%d};\n",rect.x,rect.y,rect.w,rect.h);
            SDL_Surface* texsurface;
            SDL_LockTextureToSurface(tex,&rect,&texsurface);
            SDL_FillRect(texsurface, NULL,0);
            uint32_t colkey = SDL_MapRGBA(texsurface->format,0,0,0,255);
            SDL_SetColorKey(texsurface,SDL_TRUE,colkey);
            memcpy(texsurface->pixels,pixels,width * height * 4);
            SDL_UnlockTexture(tex);
            SDL_SetTextureBlendMode(tex, SDL_BlendMode::SDL_BLENDMODE_BLEND);
        }
        inline void update(btr::RenderWindow& window);
        inline void setSmooth(bool smooth) { SDL_SetTextureScaleMode(tex, smooth ? SDL_ScaleModeLinear : SDL_ScaleModeNearest); }
        inline void setRepeated(bool) { }
        inline const SDL_Texture* getTextureHandle() const
        {
            return this->tex;
        }
        inline const Vector2u getSize() const
        {
            return Vector2u(this->width, this->height);
        }
    };
    class Sprite
    {
    private:
        Texture* texture = nullptr;
        Vector2f position = Vector2f(0,0);
        Vector2f origin = Vector2f(0,0);
        Color color = Color(255,255,255,255);
        IntRect texrect = IntRect(0,0,0,0);
        float xscale = 1.f,yscale = 1.f;
    public:
        inline Sprite() = default;
        inline void setTexture(Texture& texture, bool resetRect = true)
        {
            this->texture = std::addressof(texture);
            if (resetRect)
            {
                auto pos = Vector2i(0,0);
                auto size = (const Vector2i)this->texture->getSize();
                texrect = IntRect(Vector2i(0,0),(const Vector2i)this->texture->getSize());
            }
        }
        inline void setPosition(const Vector2f& pos)
        {
            this->position = pos;
        }
        inline void setPosition(float x, float y)
        {
            this->position.x = x;
            this->position.y = y;
        }
        inline void move(const Vector2f& pos)
        {
            this->position += pos;
        }
        inline void move(float x, float y)
        {
            this->position.x += x;
            this->position.y += y;
        }
        inline void setColor(const Color& color)
        {
            this->color = color;
        }
        inline void setOrigin(const Vector2f& origin)
        {
            this->origin = origin;
        }
        inline void setScale(float x, float y)
        {
            this->xscale = x;
            this->yscale = y;
        }
        inline Vector2f getScale() const
        {
            return Vector2f(this->xscale,this->yscale);
        }
        inline void setOrigin(float x, float y)
        {
            this->origin.x = x;
            this->origin.y = y;
        }
        inline const Vector2f& getOrigin() const 
        {
            return this->origin;
        }
        inline void setTextureRect(const IntRect& rect)
        {
            this->texrect = rect;
        }
        inline IntRect getTextureRect() const
        {
            return texrect;
        }
        inline const Texture* getTexture() const
        {
            return texture;
        }
        inline const Color& getColor() const { return this->color; }
        inline const Vector2f& getPosition() const { return position; };
    };
    class RectangleShape
    {
    private:
        Vector2f size, pos;
        Color fillcolor;
    public:
        RectangleShape()
        {
            fillcolor = Color::White;
            size = Vector2f(0,0);
            pos = Vector2f(0,0);
        }
        RectangleShape(Vector2f size)
        {
            this->size = size;
        }
        void setPosition(Vector2f pos)
        {
            this->pos = pos;
        }
        void setPosition(float X, float Y)
        {
            this->pos.x = X;
            this->pos.y = Y;
        }
        void setSize(Vector2f size)
        {
            this->size = size;
        }
        void setFillColor(Color color)
        {
            this->fillcolor = color;
        }
        const Color getFillColor() const
        {
            return this->fillcolor;
        }
        operator const btr::IntRect() const
        {
            return btr::IntRect((Vector2i)pos,(Vector2i)size);
        }
    };
    class Keyboard
    {
        public:
        enum Key
        {
            Unknown = SDL_SCANCODE_UNKNOWN,
            A = SDL_SCANCODE_A,
            B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,X,Y,Z,
            Num0 = SDL_SCANCODE_0,
            Num1 = SDL_SCANCODE_1,
            Num2,Num3,Num4,Num5,Num6,Num7,Num8,Num9,
            Escape = SDL_SCANCODE_ESCAPE,
            Pause = SDL_SCANCODE_PAUSE,
            Enter = SDL_SCANCODE_RETURN,
            LAlt = SDL_SCANCODE_LALT,
            RAlt = SDL_SCANCODE_RALT,
            Backspace = SDL_SCANCODE_BACKSPACE
        };
        static bool isKeyPressed(Key key)
        {
            return SDL_GetKeyboardState(NULL)[key];
        }
        static void setVirtualKeyboardVisible(bool visible)
        {
            if (SDL_HasScreenKeyboardSupport())
            {
                visible ? SDL_StartTextInput() : SDL_StopTextInput();
            }
        }
    };
    class RenderWindow;
    class Mouse
    {
    public:

        enum Button
        {
            Left = 1,Right = 3,Middle = 2,XButton1 = 4,XButton2 = 5,ButtonCount = 5
        };

        enum Wheel
        {
            VerticalWheel,HorizontalWheel
        };

        inline static Vector2i getPosition()
        {
            Vector2i mousePos;
            SDL_GetGlobalMouseState(&mousePos.x, &mousePos.y);
            return mousePos;
        }
        inline static Vector2i getPosition(RenderWindow& window);
        static void setPostion(const Vector2i& pos)
        {
            SDL_WarpMouseGlobal(pos.x,pos.y);
        }
        static bool isButtonPressed(Button button)
        {
            if (demo && button == Button::Left) return true;
            return SDL_GetGlobalMouseState(NULL, NULL) & SDL_BUTTON(button);
        }
        static void setPosition(const Vector2i& pos, const RenderWindow& relativeTo);
    };
    class Event
    {
    public:

        enum EventType
        {
            Closed,
            Resized,
            LostFocus,
            GainedFocus,
            TextEntered,
            KeyPressed,
            KeyReleased,
            MouseWheelMoved,    
            MouseWheelScrolled,
            MouseButtonPressed,
            MouseButtonReleased,
            MouseMoved,
            MouseEntered,
            MouseLeft,

            Count
        };

        struct SizeEvent
        {
            unsigned int width, height;
        };

        struct MouseMoveEvent
        {
            int x,y;
        };
        
        struct MouseButtonEvent
        {
            Mouse::Button button;
            int x,y;
        };

        struct MouseWheelEvent
        {
            Mouse::Wheel wheel;
            int delta,x,y;
        };

        struct KeyEvent
        {
            Keyboard::Key code;
            bool alt,control,shift,system;
        };
        
        struct TextEvent
        {
            uint32_t unicode;
        };
        EventType type;
        union
        {
            SizeEvent             size;       
            KeyEvent              key;        
            TextEvent             text;       
            MouseMoveEvent        mouseMove;  
            MouseButtonEvent      mouseButton;
            MouseWheelEvent       mouseWheelScroll; 
        };
        
    };
    namespace Style
    {
        enum
        {
            None = SDL_WINDOW_BORDERLESS,
            Resize = SDL_WINDOW_RESIZABLE,
            Fullscreen = SDL_WINDOW_FULLSCREEN,
            Default = Resize,
            Close = Default,
            Titlebar = Default
        };
    };
    class VideoMode
    {
        public:
        unsigned int width = 0,height = 0,bitsPerPixel = 0;
        VideoMode() = default;
        VideoMode(unsigned int width, unsigned int height, unsigned int bitsPerPixel = 32)
        {
            this->width = width;
            this->height = height;
            this->bitsPerPixel = bitsPerPixel;
        }
        static VideoMode getDesktopMode()
        {
            return VideoMode(640,480);
        }
    };
    class RenderWindow
    {
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        int fps;
        std::vector<uint32_t> pendingTextEvents;
    public:
        RenderWindow()
        {}
        RenderWindow(VideoMode mode, std::string str, uint32_t style)
        {
            fprintf(stderr, "SDL RENDERER IS WIP!!! DON'T REPORT ISSUES ON GITHUB!!!\n");
            SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
            //if (!(SDL_WasInit(SDL_INIT_VIDEO)))
            //{
            //    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
            //    atexit(SDL_Quit);
            //}
            if (!SDL_WasInit(SDL_INIT_VIDEO))
            {
                SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
                atexit(SDL_Quit);
            }
            window = SDL_CreateWindow(str.c_str(),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,mode.width, mode.height, style | SDL_WINDOW_OPENGL);
            if (!window) throw std::runtime_error("Failed to create SDL window.");
            
            renderer = SDL_CreateRenderer(window,-1,0);
            if (!renderer) throw std::runtime_error("Failed to create SDL renderer.");
            SDL_SetRenderDrawBlendMode(renderer, SDL_BlendMode::SDL_BLENDMODE_BLEND);
            context = renderer;
            SDL_RendererInfo info;
            if (SDL_GetRendererInfo(renderer,&info) == 0)
            {
                fprintf(stderr,"Using render driver %s", info.name);
            }
            else
            {
                fprintf(stderr,"Failed to get render info: %s\n",SDL_GetError());
            }
        }
        ~RenderWindow()
        {
            if (renderer) SDL_DestroyRenderer(renderer);
            if (window)
            {
                SDL_SetWindowFullscreen(window,0);
                SDL_DestroyWindow(window);
            }
            context = NULL;
        }
        Vector2i getPosition() const
        {
            Vector2i pos;
            SDL_GetWindowPosition(this->window,&pos.x,&pos.y);
            return pos;
        }
        void clear(Color color = Color(0,0,0,255))
        {
            SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer,255,255,255,255);
        }
        void setFramerateLimit(int fps)
        {
            this->fps = fps;
        }
        void draw(const Sprite& sprite)
        {
            //SDL_SetRenderDrawColor(renderer,sprite.getColor().r,sprite.getColor().g,sprite.getColor().b,sprite.getColor().a);
            SDL_SetTextureColorMod((SDL_Texture*)sprite.getTexture()->getTextureHandle(),sprite.getColor().r,sprite.getColor().g,sprite.getColor().b);
            SDL_SetTextureAlphaMod((SDL_Texture*)sprite.getTexture()->getTextureHandle(),sprite.getColor().a);
            SDL_Rect srcrect = sprite.getTextureRect();
            SDL_FRect dstrect{sprite.getPosition().x - sprite.getOrigin().x, sprite.getPosition().y - sprite.getOrigin().y, (float)srcrect.w * sprite.getScale().x, (float)srcrect.h * sprite.getScale().y};
            SDL_RenderCopyF(renderer,(SDL_Texture*)sprite.getTexture()->getTextureHandle(), &srcrect, &dstrect);
        }
        void draw(const RectangleShape& rectshape) const
        {
#if 1            
            IntRect rect = rectshape;
            SDL_Rect sdlrect = rect;
            uint8_t r,g,b,a;
            auto col = rectshape.getFillColor();
            SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
            SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);
            SDL_RenderFillRect(renderer, &sdlrect);
            SDL_SetRenderDrawColor(renderer,r,g,b,a);
#endif
        }
        SDL_Window* getSystemHandle() const { return window; };
        bool pollEvent(btr::Event& event)
        {
            static bool mouseInsideWindow = false;
            if (pendingTextEvents.size() != 0)
            {
                event.text.unicode = pendingTextEvents[0];
                event.type = btr::Event::TextEntered;
                pendingTextEvents.erase(pendingTextEvents.begin());
                return true;
            }
            if (IntRect(this->getPosition(),this->getSize()).contains(Mouse::getPosition()))
            {
                if (!mouseInsideWindow)
                {
                    mouseInsideWindow = true;
                    event.type = Event::MouseEntered;
                    return true;
                }
            }
            else if (mouseInsideWindow)
            {
                mouseInsideWindow = false;
                event.type = Event::MouseLeft;
                return true;
            }
            SDL_Event ev;
            int res = SDL_PollEvent(&ev);
            switch(ev.type)
            {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                event.type = Event::Closed;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                event.type = Event::Resized;
                {
                    event.size.width = ev.window.data1;
                    event.size.height = ev.window.data2;
                    break;
                }
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                event.type = Event::GainedFocus;
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                event.type = Event::LostFocus;
                break;
            case SDL_KEYDOWN:
                event.type = Event::KeyPressed;
                {
                    event.key.code = (btr::Keyboard::Key)ev.key.keysym.scancode;
                    event.key.alt = (btr::Keyboard::Key)ev.key.keysym.mod & KMOD_ALT;
                    event.key.control = (btr::Keyboard::Key)ev.key.keysym.mod & KMOD_CTRL;
                    event.key.shift = (btr::Keyboard::Key)ev.key.keysym.mod & KMOD_SHIFT;
                    if (ev.key.keysym.scancode == SDL_SCANCODE_BACKSPACE)
                    {
                        pendingTextEvents.push_back('\b');
                    }
                }
                break;
            case SDL_KEYUP:
                event.type = Event::KeyReleased;
                {
                    event.key.code = (btr::Keyboard::Key)ev.key.keysym.scancode;
                    event.key.alt = (btr::Keyboard::Key)ev.key.keysym.mod & KMOD_ALT;
                    event.key.control = (btr::Keyboard::Key)ev.key.keysym.mod & KMOD_CTRL;
                    event.key.shift = (btr::Keyboard::Key)ev.key.keysym.mod & KMOD_SHIFT;
                }
                break;
            case SDL_MOUSEMOTION:
                event.type = Event::MouseMoved;
                {
                    event.mouseMove.x = ev.motion.x;
                    event.mouseMove.y = ev.motion.y;
                }
                break;
            case SDL_MOUSEWHEEL:
                event.type = Event::MouseWheelScrolled;
                {
                    event.mouseWheelScroll.wheel = ev.wheel.x ? Mouse::Wheel::HorizontalWheel : Mouse::Wheel::VerticalWheel;
                    event.mouseWheelScroll.delta = Mouse::Wheel::HorizontalWheel ? ev.wheel.x : ev.wheel.y;
                    SDL_GetMouseState(&event.mouseWheelScroll.x, &event.mouseWheelScroll.y);
                    break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                event.type = ev.button.state ? Event::MouseButtonPressed : Event::MouseButtonReleased;
                {
                    event.mouseButton.button = (btr::Mouse::Button)ev.button.button;
                    event.mouseButton.x = ev.button.x;
                    event.mouseButton.y = ev.button.y;
                }
                break;
            case SDL_TEXTINPUT:
                {
                    uint32_t* utf32array = new uint32_t[SDL_TEXTINPUTEVENT_TEXT_SIZE];
                    uint32_t* origutf32array = utf32array;
                    unsigned char* utf8array = (unsigned char*)ev.text.text;
                    unsigned char* origutf8array = utf8array;
                    int lengthoftext = strlen((char*)utf8array);
#if 0
                    auto res = ConvertUTF8toUTF32((const unsigned char**)&utf8array, &utf8array[lengthoftext],&utf32array,utf32array + lengthoftext,strictConversion);
                    if (res != conversionOK)
                    {
                        const char* errorStr;
                        switch(res)
                        {
                            case sourceIllegal:
                                errorStr = "Illegal UTF-8 sequence.";
                                break;
                            case sourceExhausted:
                                errorStr = "String buffer underflow.";
                                break;
                            case targetExhausted:
                                errorStr = "Target buffer underflow.";
                                break;
                            default: errorStr = ""; break;
                        }
                        fprintf(stderr, "Could not decode UTF-8 SDL sequence. %s\n",errorStr);
                        delete[] origutf32array;
                        free(origutf8array);
                        break;
                    }
#endif
                    for (int i = 0; i < lengthoftext + 1; i++)
                    {
                        utf32array[i] = utf8array[i];
                        if (utf32array[i] != 0) pendingTextEvents.push_back(utf32array[i]);
                    }
                    event.text.unicode = pendingTextEvents[0];
                    event.type = btr::Event::TextEntered;
                    pendingTextEvents.erase(pendingTextEvents.begin());
                }
                break;
            }
            return !!res;
        }
        void display()
        {
            SDL_RenderPresent(renderer);
            if (fps != 0) std::this_thread::sleep_for(std::chrono::duration<double>(1. / fps));
        }
        Vector2i getSize() const
        {
            Vector2i size;
            SDL_GetWindowSize(window,&size.x,&size.y);
            return size;
        }
        void setFullscreen(bool fullscreen)
        {
            SDL_SetWindowFullscreen(window,fullscreen);
        }
        SDL_Renderer* getRenderer()
        {
            return renderer;
        }
        void requestFocus()
        {
            SDL_RaiseWindow(this->window);
            SDL_SetWindowInputFocus(this->window);
        }
        bool isOpen()
        {
            return !!(SDL_GetWindowFlags(window) & SDL_WINDOW_SHOWN);
        }
        void setKeyRepeatEnabled(bool) {}
        void setMouseCursorVisible(bool visible)
        {
            SDL_ShowCursor(visible);
        }
        void close()
        {
            // Hiding the window seems to be the only option here.
            SDL_HideWindow(window);
        }
    };
    void Texture::update(RenderWindow& window)
    {
        SDL_Rect rect{0,0,window.getSize().x,window.getSize().y};
        SDL_Surface* surface;
        SDL_LockTextureToSurface(this->tex,&rect,&surface);        
        SDL_RenderReadPixels(window.getRenderer(),&rect,SDL_PIXELFORMAT_RGBA32,surface->pixels,surface->pitch);
        SDL_UnlockTexture(tex);
    }
    Vector2i Mouse::getPosition(RenderWindow& window)
    {
        Vector2i pos;
        SDL_GetMouseState(&pos.x,&pos.y);
        return pos;
    }
    inline void Mouse::setPosition(const Vector2i& pos, const btr::RenderWindow& relativeTo)
    {
        SDL_WarpMouseInWindow(relativeTo.getSystemHandle(), pos.x, pos.y);
    }
}
