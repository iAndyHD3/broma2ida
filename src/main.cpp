#include <ast.hpp>
#include <broma.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <cstdlib>
#include <format>


#if(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace fs = std::filesystem;

broma::Platform g_selectedPlatform = broma::Platform::None;

int error_with_message(std::string_view str)
{
    std::cout << '\n' << str;
    return -1;
}

ptrdiff_t getPlatformAddress(const broma::PlatformNumber& n)
{
    using enum broma::Platform;
    switch(g_selectedPlatform)
    {
    case Windows: return n.win;
    case MacIntel: case Mac: return n.imac;
    case MacArm: return n.m1;
    case iOS: return n.ios;
    default: return -1;
    }
}

void println(const auto& anything)
{
    std::cout << anything << '\n';
}

void getFunciton(const broma::Function& fun)
{
    auto address = getPlatformAddress(fun.binds);

    if(address <= 0) return;

    //0 address
    //1 function name
    constexpr auto FORMAT = "broma_rename(base + {:#x}, \"{}\");";

    std::cout << std::format(FORMAT, address, fun.prototype.name) << '\n';
}

std::string_view fixFunctionName(std::string_view f)
{
    if(f.find('~') != std::string_view::npos) return "destructor";
    return f;
}

void getMemberFunction(const broma::Field& field)
{
    auto func = field.get_as<broma::FunctionBindField>();
    if(!func) return;

    auto address = getPlatformAddress(func->binds);
    if(address <= 0) return;

    //0 address
    //1 class name
    //2 function name
    constexpr auto FORMAT = "broma_rename(base + {:#x}, \"{}::{}\");";

    std::cout << std::format(FORMAT, address, field.parent, fixFunctionName(func->prototype.name)) << '\n';
}

//static func rename(base, addr, name)
//{
//    if(set_name(addr, name, SN_NOWARN | SN_PUBLIC) != 1)
//    {
//        Message(form("Could not rename %s", name));
//    }
//}
//
//auto base = get_imagebase();


void generateFromRoot(const broma::Root& root)
{

std::string_view rawScript = R"(
#include <idc.idc>

static broma_rename(addr, name)
{
    if(set_name(addr, name, SN_NOWARN | SN_PUBLIC) != 1)
    {
        Message(form("Could not rename %s\n", name));
    }
}

static main()
{
auto base = get_imagebase();
)";


    std::cout << rawScript;

    for(const auto& cl : root.classes)
    {
        for(const auto& field : cl.fields)
        {
            getMemberFunction(field);
        }
    }



    for(const auto& freefuncs : root.functions)
    {
        getFunciton(freefuncs);
    }
    
    std::cout << "}";

}


void generateFromBroma(const std::string& filepath)
{
    auto root = broma::parse_file(filepath);
    generateFromRoot(root);
}

bool validBroFile(const fs::path& p)
{
    return fs::exists(p) && p.has_extension() && p.extension() == ".bro";
}


fs::path getLatestFromBindingsPath(fs::path bindingsPath)
{
    if(!fs::exists(bindingsPath)) return {};

    auto main_path = bindingsPath / "bindings";
    if(!fs::exists(main_path) || !fs::is_directory(main_path)) return {};

    fs::path ret;

    double latest = 0;
    for(const auto& bindingsdir : fs::recursive_directory_iterator(main_path))
    {
        if(!fs::is_directory(bindingsdir)) continue;

        try
        {
            if(auto newpath = bindingsdir.path(); std::stod(newpath.filename().string()) > latest)
            {
                ret = newpath;
            }
        }
        catch(std::invalid_argument)
        {
            continue;
        }
    }

    ret /= "GeometryDash.bro";

    return fs::exists(ret) ? ret : fs::path();
}

fs::path getBromaPath(int argc, char** argv)
{
    if(argc >= 2)
    {
        fs::path possible_path = argv[1];

        //full path
        if(validBroFile(possible_path))
        {
            return possible_path;
        }
        
        //relative path
        char* bindings_path_str = std::getenv("GEODE_BINDINGS_REPO_PATH");
        if(!bindings_path_str) return {};

        possible_path = fs::path(bindings_path_str) / possible_path;
        if(validBroFile(possible_path))
        {
            return possible_path;
        }
    }

    //no arguments, try to get latest GeometryDash.bro from PATH bindings
    char* bindings_path_str = std::getenv("GEODE_BINDINGS_REPO_PATH");
    if(!bindings_path_str) return {};

    return getLatestFromBindingsPath(bindings_path_str);
}

broma::Platform getPlatform(int argc, char** argv)
{
    using enum broma::Platform;
    if(argc <= 2) return Windows;

    std::string_view plat = argv[2];

    if(plat == "win") return Windows;
    if(plat == "mac") return Mac;
    if(plat == "imac") return MacIntel;
    if(plat == "m1") return MacArm;
    if(plat == "ios") return iOS;

    return None;
}

std::string_view getPlatformStr(broma::Platform p)
{
    using enum broma::Platform;
    switch(p)
    {
        case Windows: return "win";
        case Mac: return "mac";
        case MacArm: return "m1";
        case MacIntel: return "imac";
        case iOS: return "ios";
        default: return "";
    }
}
int main2(int argc, char** argv)
{
    auto path = getBromaPath(argc, argv);
    if(path.empty() || !fs::exists(path)) return error_with_message("Could not find a valid broma file path");

    auto platform = getPlatform(argc, argv);
    if(platform == broma::Platform::None) return error_with_message("Invalid platform\nvalid values: win, imac, m1, ios");

    g_selectedPlatform = platform;

    println(std::format("//{}\n//Platform: {}\n", path.string(), getPlatformStr(platform)));

    generateFromBroma(path.string());


    return 0;
}


int main(int argc, char** argv)
{
    try
    {
        return main2(argc, argv);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return -1;
}

