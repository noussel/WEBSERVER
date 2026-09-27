#include "Response.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits.h>

std::string response::handleDelete()
{
    if (request.getMethod() != "DELETE")
        return buildErrorResponse(405);
    std::cout << "PATH ========= "<< std::endl;

    if (std::find(loc.allowed_methods.begin(),
                  loc.allowed_methods.end(),
                  "DELETE") == loc.allowed_methods.end())
        return buildErrorResponse(405);
    std::cout << "PATH ========= 2 "<< std::endl;

    std::string path = request.getPath();
    std::cout << "PATH = " << path << std::endl;
    // std::cout << "FULL = " << resolved << std::endl;
    // std::cout << "ROOT = " << loc.root << std::endl;
    if (path == "/" || path.empty())
        return buildErrorResponse(403);

    char rootResolved[PATH_MAX];
    if (!realpath(loc.root.c_str(), rootResolved))
        return buildErrorResponse(500);

    std::string rootPath(rootResolved);

    std::string fullPath = rootPath + path;

    char resolved[PATH_MAX];
    if (!realpath(fullPath.c_str(), resolved))
        return buildErrorResponse(404);

    std::string safePath(resolved);

    if (safePath.find(rootPath) != 0)
        return buildErrorResponse(403);

    struct stat st;

    if (stat(resolved, &st) == -1)
        return buildErrorResponse(404);

    if (access(resolved, W_OK) != 0)
        return buildErrorResponse(403);
    std::cout << "PATH = " << path << std::endl;
    std::cout << "FULL = " << resolved << std::endl;
    std::cout << "ROOT = " << loc.root << std::endl;

    if (S_ISREG(st.st_mode))
    {
        if (std::remove(resolved) != 0)
            return buildErrorResponse(500);

        std::string body = "<html><body><h1>File deleted successfully</h1></body></html>";
        return buildResponse(body, "text/html");
    }

    if (S_ISDIR(st.st_mode))
    {
        DIR* dir = opendir(resolved);
        if (!dir)
            return buildErrorResponse(500);

        struct dirent* entry;
        bool isEmpty = true;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;
            if (name != "." && name != "..")
            {
                isEmpty = false;
                break;
            }
        }
        closedir(dir);

        if (!isEmpty)
            return buildErrorResponse(403);

        if (rmdir(resolved) != 0)
            return buildErrorResponse(500);

        std::string body = "<html><body><h1>Directory deleted successfully</h1></body></html>";
        return buildResponse(body, "text/html");
    }

    return buildErrorResponse(400);
}
