#include "SkyboundLayer.h"
#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vx/Core/Application.h>

int main(int argc, char** argv)
{
    LaunchOptions options;
    try
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--smoke-test")
                options.smoke = true;
            else if (arg == "--studio-smoke")
            {
                options.smoke = true;
                options.studio = true;
            }
            else if (arg == "--frames" && i + 1 < argc)
                options.frames = std::stoi(argv[++i]);
            else if (arg == "--screenshot" && i + 1 < argc)
                options.screenshot = argv[++i];
            else if (arg == "--help")
            {
                std::cout << "Skybound / VyNix AI Engine\nWASD/arrows move, Space fires, Enter "
                             "launches, P/Esc pauses, R restarts, M mutes.\nF1 Flight, F2 Scene "
                             "Studio, F3 engine guide.\n--smoke-test [--frames 360] [--screenshot "
                             "frame.ppm]\n--studio-smoke tests scene preview.\n";
                return 0;
            }
            else
                throw std::runtime_error("Unknown argument: " + arg);
        }
        if (options.frames < 120 || options.frames > 36000)
            throw std::runtime_error("Frame count must be 120..36000.");
        vx::WindowProps props;
        props.Title = "Skybound | VyNix AI Engine - C++ / OpenGL";
        props.Width = 1280;
        props.Height = 800;
        props.Visible = !options.smoke;
        vx::Application app(props);
        auto* game = new SkyboundLayer(app, options);
        app.PushLayer(game);
        app.Run(options.smoke ? options.frames : 0);
        return options.smoke && !game->SmokePassed() ? 2 : 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "VyNix startup error: " << e.what() << '\n';
        if (!options.smoke)
            MessageBoxA(nullptr, e.what(), "VyNix startup error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
