#include <limits>
#include <type_traits>
#include <system_error>
#include <chrono>
#include <thread>
#include <vector>
#include "ConvertUTF.h"
#include <SDL2/SDL.h>

extern bool demo;
inline std::chrono::steady_clock waitClock;

namespace btr
{
    extern SDL_Renderer* context;
    class RenderWindow;

    template <class T>
    class Vector2
    {
    public:
        T x = 0;
        T y = 0;
        static_assert(std::is_integral_v<T> || std::numeric_limits<T>::is_iec559);
        Vector2() = default;
        ~Vector2() = default;
        Vector2(T x, T y)
        {
            this->x = x;
            this->y = y;
        }
        template <typename U>
        explicit Vector2(const Vector2<U>& vector)
        {
            this->x = vector.x;
            this->y = vector.y;
        }
    };
    
    template<typename T>
    inline Vector2<T>& operator -(const Vector2<T>& right)
    {
        return Vector2<T>(-right.x, -right.y);
    }

    template<typename T>
    inline Vector2<T>& operator +=(Vector2<T>& left, const Vector2<T>& right)
    {
        left.x += right.x;
        left.y += right.y;
        return left;
    }

    template<typename T>
    inline Vector2<T>& operator -=(Vector2<T>& left, const Vector2<T>& right)
    {
        left.x -= right.x;
        left.y -= right.y;
        return left;
    }

    template<typename T>
    inline Vector2<T>& operator *=(Vector2<T>& left, const Vector2<T>& right)
    {
        left.x *= right.x;
        left.y *= right.y;
        return left;    
    }

    template<typename T>
    inline Vector2<T>& operator /=(Vector2<T>& left, const Vector2<T>& right)
    {
        left.x /= right.x;
        left.y /= right.y;
        return left;
    }

    template<typename T>
    inline Vector2<T> operator +(const Vector2<T>& left, const Vector2<T>& right)
    {
        return Vector2<T>(left.x + right.x, left.y + right.y);
    }

    template<typename T>
    inline Vector2<T> operator +(const Vector2<T>& left, T right)
    {
        return Vector2<T>(left.x + right, left.y + right);
    }
    
    template<typename T>
    inline Vector2<T> operator -(const Vector2<T>& left, T right)
    {
        return Vector2<T>(left.x - right, left.y - right);
    }

    template<typename T>
    inline Vector2<T> operator *(const Vector2<T>& left, T right)
    {
        return Vector2<T>(left.x * right, left.y * right);
    }

    template<typename T>
    inline Vector2<T> operator /(const Vector2<T>& left, T right)
    {
        return Vector2<T>(left.x / right, left.y / right);
    }

    template<typename T>
    inline Vector2<T> operator -(const Vector2<T>& left, const Vector2<T>& right)
    {
        return Vector2<T>(left.x - right.x, left.y - right.y);
    }

    template<typename T>
    inline Vector2<T> operator *(const Vector2<T>& left, const Vector2<T>& right)
    {
        return Vector2<T>(left.x * right.x, left.y * right.y);
    }

    template<typename T>
    inline Vector2<T> operator /(const Vector2<T>& left, const Vector2<T>& right)
    {
        return Vector2<T>(left.x / right.x, left.y / right.y);
    }
    
    template<typename T>
    inline bool operator ==(const Vector2<T>& left, const Vector2<T>& right)
    {
        return left.x == right.x && left.y == right.y;
    }

    template<typename T>
    inline bool operator !=(const Vector2<T>& left, const Vector2<T>& right)
    {
        return !(left == right);
    }

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
        operator SDL_Color() const
        {
            SDL_Color color;
            color.a = this->a;
            color.r = this->r;
            color.g = this->g;
            color.b = this->b;
            return color;
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

    inline bool operator == (const Color& left, const Color& right)
    {
        return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
    }
    inline bool operator != (const Color& left, const Color& right)
    {
        return !(left == right);
    }
#define CLAMP(a, min, max) (a < min ? min : (a > max ? max : a))
    inline Color operator + (const Color& left, const Color& right)
    {
        Color retcol;
        uint32_t a = CLAMP((uint32_t)(left.a) + (uint32_t)(right.a), 0u, 255u);
        uint32_t r = CLAMP((uint32_t)(left.r) + (uint32_t)(right.r), 0u, 255u);
        uint32_t g = CLAMP((uint32_t)(left.g) + (uint32_t)(right.g), 0u, 255u);
        uint32_t b = CLAMP((uint32_t)(left.b) + (uint32_t)(right.b), 0u, 255u);
        return Color(uint8_t(r), uint8_t(g), uint8_t(b), uint8_t(a));
    }
#undef CLAMP
    inline Color operator * (const Color& left, const Color& right)
    {
        uint32_t a = (uint32_t)(left.a) * (uint32_t)(right.a);
        uint32_t r = (uint32_t)(left.r) * (uint32_t)(right.r);
        uint32_t g = (uint32_t)(left.g) * (uint32_t)(right.g);
        uint32_t b = (uint32_t)(left.b) * (uint32_t)(right.b);
        return Color(r / 255u, g / 255u, b / 255u, a / 255u);
    }

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
        inline operator SDL_FRect()
        {
            SDL_FRect rect;
            rect.x = (float)this->left;
            rect.y = (float)this->top;
            rect.w = (float)this->width;
            rect.h = (float)this->height;
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
        Texture& operator =(const Texture& right)
        {
            if (this->tex) SDL_DestroyTexture(tex);
            this->tex = SDL_CreateTexture(context, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, right.width, right.height);
            auto pixels = new uint32_t[right.width * right.height];
            SDL_SetRenderTarget(context, right.tex);
            SDL_Rect rect{0, 0, (int)right.width, (int)right.height};
            SDL_RenderReadPixels(context, &rect, SDL_PIXELFORMAT_RGBA32, pixels, (int)right.width * 4);
            SDL_UpdateTexture(this->tex, &rect, pixels, right.width * 4);
            SDL_SetRenderTarget(context, nullptr);

            this->width = right.width;
            this->height = right.height;
            return *this;
        }
        inline void create(uint32_t w, uint32_t h)
        {
            if (tex) { SDL_DestroyTexture(tex); tex = NULL; }
            tex = SDL_CreateTexture(context, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!tex)
            {
                fprintf(stderr,"Failed to create texture: %s\n",SDL_GetError());
            }
            SDL_SetTextureBlendMode(tex, SDL_BlendMode::SDL_BLENDMODE_BLEND);
            width = w;
            height = h;
        }
        inline void update(uint8_t* pixels)
        {
            SDL_Rect rect{0,0,(int)width,(int)height};
//            fprintf(stderr, "SDL_Rect rect{%f,%f,%f,%f};\n",rect.x,rect.y,rect.w,rect.h);
            SDL_UpdateTexture(tex, &rect, pixels, width * 4);
        }
        inline void update(btr::RenderWindow& window);
        inline void setSmooth(bool smooth)
        {
            SDL_SetTextureScaleMode(tex, smooth ? SDL_ScaleMode::SDL_ScaleModeLinear : SDL_ScaleMode::SDL_ScaleModeNearest);
            //GPU_SetImageFilter(tex, smooth ? GPU_FilterEnum::GPU_FILTER_LINEAR : GPU_FilterEnum::GPU_FILTER_NEAREST);
        }
        inline void setRepeated(bool repeated)
        {
        }
        inline const SDL_Texture* getTextureHandle() const
        {
            return this->tex;
        }
        inline const Vector2u getSize() const
        {
            return Vector2u(this->width, this->height);
        }
    };
    enum PrimitiveType
    {
        Points,
        Lines,
        Triangles
    };
    class Vertex
    {
    public:
        Vector2f vertPos = Vector2f(0,0);
        Color color = Color(0,0,0,255);

        Vertex() = default;
        Vertex(const Vector2f& pos) { vertPos = pos; }
        Vertex(const Vector2f& pos, const Color& col) { vertPos = pos; color = col; }
        Vertex(const Vector2f& pos, const Vector2f& texPos) { vertPos = pos; }
        Vertex(const Vector2f& pos, const Color& col, const Vector2f texPos) { vertPos = pos; color = col; }
    };
    class VertexArray : public std::vector<Vertex>
    {
    private:
        PrimitiveType type;
    public:
        void append(const Vertex& vert) { this->push_back(vert); }
        VertexArray() : std::vector<Vertex>() {}
        explicit VertexArray(PrimitiveType type, std::size_t count) : std::vector<Vertex>(count) { this->type = type; }
        std::size_t getVertexCount() { return this->size(); }
        void setPrimitiveType(PrimitiveType newType) { type = newType; }
        PrimitiveType getPrimitiveType() const { return type; }
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
            Default = 0,
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
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;
        int fps = 0;
        bool closed = false;
        std::vector<uint32_t> pendingTextEvents;
    public:
        RenderWindow() = default;
        RenderWindow(VideoMode mode, std::string str, uint32_t style)
        {
            fprintf(stderr, "SDL RENDERER IS WIP!!! DON'T REPORT ISSUES ON GITHUB!!!\n");
            SDL_InitSubSystem(SDL_INIT_TIMER | SDL_INIT_VIDEO);
            window = SDL_CreateWindow(str.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mode.width, mode.height, style);
            if (!window) throw std::runtime_error("Failed to create SDL window.");

            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
            context = renderer;

            if (!renderer) throw std::runtime_error("Failed to create SDL renderer.");
            // Force displaying of window on Wayland.
            clear();
            display();
            clear();
            display();
            SDL_GL_SetSwapInterval(0);
            SDL_SetWindowResizable(window, (SDL_bool)!!(style & SDL_WINDOW_RESIZABLE));
            SDL_SetRenderDrawBlendMode(renderer, SDL_BlendMode::SDL_BLENDMODE_BLEND);
        }
        ~RenderWindow()
        {
            if (window)
            {
                SDL_DestroyRenderer(context);
                context = nullptr;
                SDL_DestroyWindow(window);
                SDL_QuitSubSystem(SDL_INIT_TIMER);
            }
            context = NULL;
        }
        Vector2i getPosition() const
        {
            Vector2i pos;
            SDL_GetWindowPosition(this->window,&pos.x,&pos.y);
            //printf("Window position reported: %dx%d\n",pos.x,pos.y);
            return pos;
        }
        void clear(Color color = Color(0,0,0,255))
        {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        void setFramerateLimit(int fps)
        {
            this->fps = fps;
        }
        void draw(const Sprite& sprite)
        {
            SDL_SetRenderDrawColor(renderer,sprite.getColor().r,sprite.getColor().g,sprite.getColor().b,sprite.getColor().a);
            SDL_SetTextureColorMod((SDL_Texture*)sprite.getTexture()->getTextureHandle(),sprite.getColor().r,sprite.getColor().g,sprite.getColor().b);
            SDL_SetTextureAlphaMod((SDL_Texture*)sprite.getTexture()->getTextureHandle(),sprite.getColor().a);
//            GPU_UnsetTargetColor(renderer);
//            GPU_SetTargetRGBA(renderer, sprite.getColor().r,sprite.getColor().g,sprite.getColor().b, sprite.getColor().a);
            SDL_Rect srcrect = sprite.getTextureRect();
            SDL_FRect dstrect{sprite.getPosition().x - sprite.getOrigin().x, sprite.getPosition().y - sprite.getOrigin().y, (float)srcrect.w * sprite.getScale().x, (float)srcrect.h * sprite.getScale().y};
            SDL_RenderCopyF(renderer,(SDL_Texture*)sprite.getTexture()->getTextureHandle(), &srcrect, &dstrect);

            //GPU_BlitRect((GPU_Image*)sprite.getTexture()->getTextureHandle(), &srcrect, renderer, &dstrect);
        }
        void draw(const RectangleShape& rectshape) const
        {          
            IntRect rect = rectshape;
            SDL_Rect sdlrect = rect;
            uint8_t r,g,b,a;
            auto col = rectshape.getFillColor();
            SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
            SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);
            SDL_RenderFillRect(renderer, &sdlrect);
            SDL_SetRenderDrawColor(renderer,r,g,b,a);
        }
        void draw(const VertexArray& vertArray)
        {
            auto primitiveType = vertArray.getPrimitiveType();
            auto vertData = vertArray.data();

            switch (primitiveType)
            {
                case PrimitiveType::Lines: {SDL_RenderGeometryRaw(renderer, NULL, (float*)vertData, sizeof(Vertex), (SDL_Color*)vertData + sizeof(Vector2f), sizeof(Vertex), nullptr, 0, vertArray.size(), nullptr, 0, 0); break;}
                case PrimitiveType::Points:
                {
                    for (int i = 0; i < vertArray.size(); i++)
                    {
                        SDL_SetRenderDrawColor(getRenderer(), vertArray[i].color.r, vertArray[i].color.g, vertArray[i].color.b, vertArray[i].color.a);
                        SDL_RenderDrawPointF(getRenderer(), vertArray[i].vertPos.x, vertArray[i].vertPos.y);
                    }
                    SDL_SetRenderDrawColor(getRenderer(), 255, 255, 255, 255);
                    break;
                }
            }

            //GPU_FlushBlitBuffer();
            //GPU_PrimitiveBatchV(NULL,renderer, targetPrimitive, (unsigned short)vertArray.size(), (void*)vertData,0,NULL,GPU_BATCH_XY_RGBA8);
        }
        SDL_Window* getSystemHandle() const { return window; };
        bool pollEvent(btr::Event& event)
        {
            if (pendingTextEvents.size() != 0)
            {
                event.text.unicode = pendingTextEvents[0];
                event.type = btr::Event::TextEntered;
                pendingTextEvents.erase(pendingTextEvents.begin());
                return true;
            }
            SDL_Event ev;
            int res = SDL_PollEvent(&ev);
            if (res)
            switch(ev.type)
            {
            case SDL_QUIT:
                event.type = Event::Closed;
                break;
            case SDL_WINDOWEVENT:
            {
                switch (ev.window.event)
                {
                    case SDL_WINDOWEVENT_ENTER:
                        event.type = Event::MouseEntered;
                        break;
                    case SDL_WINDOWEVENT_LEAVE:
                        event.type = Event::MouseLeft;
                        break;

                    case SDL_WINDOWEVENT_RESIZED:
                        event.type = Event::Resized;
                        {
                            event.size.width = ev.window.data1;
                            event.size.height = ev.window.data2;
                            SDL_RenderSetLogicalSize(renderer, 640, 480);
                            break;
                        }
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        event.type = Event::GainedFocus;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        event.type = Event::LostFocus;
                        break;
                
                    default:
                        break;
                }
                break;
            }
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
                    uint32_t len = 0;
                    uint32_t reschar = 0;
                    for (auto it = ev.text.text; it < &ev.text.text[32]; it++)
                    {
                        if (*it == 0)
                        {
                            if (reschar != 0) pendingTextEvents.push_back(reschar);
                            break;
                        }
                        if (*it & 0x10000000)
                        {
                            reschar |= (*it & 0x3F) << (6 * ++len);
                            continue;
                        }
                        if (reschar != 0) pendingTextEvents.push_back(reschar);
                        reschar = *it;
                        
                    }
                    event.text.unicode = pendingTextEvents[0];
                    event.type = btr::Event::TextEntered;
                    pendingTextEvents.erase(pendingTextEvents.begin());
                }
                break;
            }
            return res;
        }
        void display()
        {
            SDL_RenderPresent(renderer);
            if (fps != 0)
            {
                auto time = waitClock.now().time_since_epoch().count();
                while ((waitClock.now().time_since_epoch().count() - time) <= (1000 * 1000 * 1000) / fps)
                {

                }
            }
        }
        Vector2i getSize() const
        {
            Vector2i size;
            SDL_GetWindowSize(window,&size.x,&size.y);
            //printf("Window size reported: %dx%d\n",size.x,size.y);
            return size;
        }
        void setFullscreen(bool fullscreen)
        {
            //GPU_SetFullscreen(fullscreen, false);
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
            return !closed;
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
            closed = true;
        }
    };
    void Texture::update(RenderWindow& window)
    {
        SDL_Rect rect{0,0,window.getSize().x,window.getSize().y};
        uint8_t pixels[640 * 480 * 4];

        SDL_RenderReadPixels(window.getRenderer(), &rect, SDL_PIXELFORMAT_RGBA32, &pixels, 640 * 4);
        SDL_UpdateTexture(this->tex, &rect, pixels, 640 * 4);
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
