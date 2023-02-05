#include <string>
#include <iomanip>
#include <SDL.h>
#include <CL/opencl.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <fstream>

struct HexCharStruct
{
    unsigned char c;
    HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs)
{
    return (o << std::hex << (int)hs.c << ' ');
}

inline HexCharStruct hex(unsigned char _c)
{
    return HexCharStruct(_c);
}


const int width = 800;
const int height = 800;

const float highres_factor = 1;

int Frame_Count = 0;

void ShowDebug(double x, double y, double zoom, int res) {
    std::cout << "\x1B[2J\x1B[H" << "Pos x: " << x << "\nPos y: " << y << "\nZoom level: " << log2(1 / zoom) << "x\nIterations: " << res << "\n";
}

void Render(cl::Context& context, cl::Kernel& kernel, cl::CommandQueue& command_queue, SDL_Texture*& texture, SDL_Renderer*& renderer, double zoom, double offset_x, double offset_y, int resolution) {
    cl::Buffer output_cl(context, CL_MEM_WRITE_ONLY, width * height * sizeof(unsigned char));

    kernel.setArg(0, output_cl);
    kernel.setArg(1, width);
    kernel.setArg(2, height);
    kernel.setArg(3, zoom);
    kernel.setArg(4, offset_x);
    kernel.setArg(5, offset_y);
    kernel.setArg(6, resolution);

    cl::NDRange global_size(width, height);
    command_queue.enqueueNDRangeKernel(kernel, cl::NullRange, global_size);

    unsigned char* output = new unsigned char[width * height];
    command_queue.enqueueReadBuffer(output_cl, CL_TRUE, 0, width * height * sizeof(unsigned char), output);

    
    SDL_UpdateTexture(texture, NULL, output, width);
    SDL_RenderClear(renderer);
    SDL_Rect dst = { 0, 0, width, height };
    SDL_RenderCopy(renderer, texture, NULL, &dst);

    SDL_RenderPresent(renderer);
    delete[] output;
    ShowDebug(offset_x, offset_y, zoom, resolution);
}

int main(int argc, char* args[]){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Mandelbrot Set", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Context context(CL_DEVICE_TYPE_GPU);
    std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
    cl::CommandQueue command_queue(context, devices[0]);

    std::ifstream t("Mandelbrot.kernel");
    std::stringstream buffer;
    buffer << t.rdbuf();

    std::string source = buffer.str();
       
    cl::Program program(context, source);
    program.build(devices);
    cl::Kernel kernel(program, "mandelbrot");

    
    double zoom = 4;
    double offset_x = 0;
    double offset_y = 0;
    int resolution = 256;

    bool quit = false;
    bool right = false;
    bool left = false;
    bool down = false;
    bool up = false;
    bool plus = false;
    bool minus = false;
    bool plus_res = false;
    bool minus_res = false;


    while (!quit) {
        Render(context, kernel, command_queue, texture, renderer, zoom, offset_x, offset_y, resolution);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            switch (event.type) {
            case SDL_KEYDOWN:
                std::cout << event.key.keysym.sym;
                switch (event.key.keysym.sym) {
                case 1073741903:
                    if (!right) offset_x+= zoom;
                    right = true;
                    break;
                case 1073741904:
                    if (!left) offset_x-= zoom;
                    left = true;
                    break;
                case 1073741905:
                    if (!down) offset_y+= zoom;
                    down = true;
                    break;
                case 1073741906:
                    if (!up) offset_y-= zoom;
                    up = true;
                    break;
                case 43:
                    if (!plus) zoom /= 2l;
                    plus = true;
                    break;
                case 45:
                    if (!minus) zoom *= 2l;
                    minus = true;
                    break;
                case 44:
                    if (!plus_res) resolution /= 2; if (resolution < 1) resolution = 1;
                    plus_res = true;
                    break;
                case 46:
                    if (!minus_res) resolution *= 2;
                    minus_res = true;
                    break;
                }

                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case 1073741903:
                    right = false;
                    break;
                case 1073741904:
                    left = false;
                    break;
                case 1073741905:
                    down = false;
                    break;
                case 1073741906:
                    up = false;
                    break;
                case 43:
                    plus = false;
                    break;
                case 45:
                    minus = false;
                    break;
                case 44:
                    plus_res = false;
                    break;
                case 46:
                    minus_res = false;
                    break;
                }
                break;

            default:
                break;
            }
        }
    }

    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
