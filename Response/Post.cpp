#include "Response.hpp"

// bool isValideNumber(const std::string &str){
//     for(size_t i = 0; i < str.size(); i++){
//         if(!isdigit(str[i]))
//             return false;
//     }
//     return true;
// }

std::string generatefile(){
    std::string chars = "abcdefghijklmnopABCDEFJHIGKLMNOP123456790";
    std::string filename;
    for(int i = 0; i < 5; i++){
        filename += chars[rand() % chars.size()];
    }
    return filename.c_str();
}

std::string response::saveToFile(HttpRequestParser &request, LocationConfig loc){
    std::string path;
    if(!loc.upload_path.empty())
        path = loc.upload_path + "/" + generatefile();
    else 
        path = loc.root + "/" + generatefile();
    std::cout <<  "\033[31m" << "path : " << path << "\033[0m" << std::endl;
    std::ofstream file(path.c_str(), std::ios::binary);
    if(!file.is_open())
        throw 500;
    file.write(request.getBody().c_str(), request.getBody().size());
    file.close();
    return "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
}

std::string response::errorhandling(){
    if (std::find(loc.allowed_methods.begin(),
                  loc.allowed_methods.end(),
                  request.getMethod()) == loc.allowed_methods.end())
        throw 405;
    if(request.getValue("Content-Length").empty())//Si le client ne met pas Content-Length → serveur ne sait pas quand s’arrêter → impossible de lire le body correctement → il renvoie 411 Length Required
        throw 411;
    // if(!isValideNumber(request.getValue("Content-Length"))) //Si le client met un Content-Length qui n’est pas un nombre → serveur ne peut pas lire le body correctement → il renvoie 400 Bad Request
    //     throw 400;
    if((size_t)atoi(request.getValue("Content-Length").c_str()) > serverConfig->client_max_body_size) //koukou // est ce quil est tjr dans config file ?   //Si le client met
        throw 413;
    // if(request.hasHeader("Content-Type") && !isValidContentType(request.getHeader("Content-Type"))) //Si le client met un Content-Type qui n’est pas supporté par le serveur → serveur ne peut pas traiter la requête → il renvoie 415 Unsupported Media Type
    //     throw 415;
    // if(request.getBody().size() != (size_t)atoi(request.getValue("Content-Length").c_str())) //incomplet body or body to long 
    // {
    //     std::cout << "---------------" <<  request.getBody().size() << "-------------- " << atoi(request.getValue("Content-Length").c_str()) << "\n";
    //     throw 400;
    // }
    return "";
}

std::string getExtension(const std::string path){
    size_t pos = path.find_last_of(".");
    if(pos == std::string::npos)
        return "";
    return path.substr(pos);
}

long getTimeMs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);//secondes depuis 1970
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


void checkTimeout(pid_t pid, double start){
    int result;
    int status;
    while((result = waitpid(pid, &status, WNOHANG)) == 0 && time(NULL) - start > 7){
        if(result < 0)
            throw 500;
        if(result > 0)
        {
            if(WIFEXITED(status) && WEXITSTATUS(status) != 0)
                throw 500;
            break;
        }
        if(time(NULL) - start > 7)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            throw 504;
        }
        usleep(10000);//10ms
    }
    std::cout << "\033[33m" << "ana hader " << "\033[0m" << "\n";

}

struct CGIsetUP{
    std::string fileEXtension;
    std::string cgiPath;
    std::string scriptPath;
    char **env ;
    char **args ;

};

struct EXECsetUP{
    int in_pipe[2];
    int out_pipe[2];
    pid_t pid;
    long start;
    std::string scriptOut;
};


void setCGI(HttpRequestParser request, CGIsetUP *CGIsetup, LocationConfig loc){
        CGIsetup->fileEXtension = getExtension(request.getPath());//return path from URL (GET /cgi-bin/test.py HTTP/1.1) => /cgi-bin/test.py
        if(loc.cgi_ext.find(CGIsetup->fileEXtension) == loc.cgi_ext.end())//415 Unsupported Media type
            throw 415;
        CGIsetup->scriptPath = loc.root + request.getPath();
        CGIsetup->cgiPath = loc.cgi_ext[CGIsetup->fileEXtension];

        //set environment variables
        std::string RM = "REQUEST_METHOD=" + request.getMethod();
        std::string CL = "CONTENT_LENGTH=" + request.getValue("Content_Length");//le script sait combien lire depuis stdin
        std::string CT = "CONTENT_TYPE=" + request.getValue("Content_Type");//le script sait comment parser le body
        std::string SN = "SCRIPT_NAME=" + request.getPath();//chemin du scrpt
        std::string PI = "PATH_INFO=" + request.getPathinfo();
        std::string SP = "SERVER_PROTOCOL=" + request.getVersion();
        std::string QS = "QUERY_STRING=" + request.getQuery();
        std::string GI = "GATEWAY_INTERFACE=CGI/1.1";

        char *env[] = {
            (char*)RM.c_str(),
            (char*)CL.c_str(),
            (char*)CT.c_str(),
            (char*)SN.c_str(),
            (char*)PI.c_str(),
            (char*)SP.c_str(),
            (char*)QS.c_str(),
            (char*)GI.c_str(),
            NULL
        };
        CGIsetup->env = new char *[9];
        int i = 0;
        for(; env[i]; i++){
            CGIsetup->env[i] = new char[strlen(env[i]) + 1];
            strcpy(CGIsetup->env[i], env[i]);
        }
        CGIsetup->env[8] = NULL;

        char *args[] = {
            (char*)CGIsetup->cgiPath.c_str(),
            (char*)CGIsetup->scriptPath.c_str(),
            NULL
        };
        CGIsetup->args = new char *[3];
        i = 0;
        for(; args[i]; i++){
            CGIsetup->args[i] = new char[strlen(args[i]) + 1];
            strcpy(CGIsetup->args[i], args[i]);
        }
        CGIsetup->args[2] = NULL;
}

void setEXEC(EXECsetUP *EXECsetup){
    if(pipe(EXECsetup->in_pipe) < 0 || pipe(EXECsetup->out_pipe) < 0)
        throw 500;
    EXECsetup->pid = fork();
    if(EXECsetup->pid < 0)
        throw 500;
    EXECsetup->start = time(NULL);
}

void execScript(EXECsetUP &EXECsetup, CGIsetUP &CGIsetup){

    close(EXECsetup.in_pipe[1]);
    dup2(EXECsetup.in_pipe[0], STDIN_FILENO);// seul dup2 peut remplace stdin out
    close(EXECsetup.in_pipe[0]);
    
    close(EXECsetup.out_pipe[0]);
    dup2(EXECsetup.out_pipe[1], STDOUT_FILENO);
    close(EXECsetup.out_pipe[1]);

    execve(CGIsetup.cgiPath.c_str(), CGIsetup.args, CGIsetup.env);//PATH name , args, env;
    perror("execve failed");
    exit(1);
}

void writeToScript(HttpRequestParser request, EXECsetUP &EXECsetup){

    close(EXECsetup.in_pipe[0]);
    close(EXECsetup.out_pipe[1]);

    std::string body = request.getBody();
    if(write(EXECsetup.in_pipe[1], body.c_str(), body.size()) < 0 )
        throw 500;
    
    close(EXECsetup.in_pipe[1]);
}


// void readFromScript(EXECsetUP &EXECsetup){
//         char buffer[4000];
//         int bytes_read = read(EXECsetup.out_pipe[0], buffer, sizeof(buffer));
//         if(bytes_read == -1)
//             throw 500;
//         EXECsetup.scriptOut.append(buffer, bytes_read);
//         if(bytes_read < content lentgh)
//             enum = build response;
            
//         close(EXECsetup.out_pipe[0]);
// }

void readFromScript(EXECsetUP &EXECsetup){
        char buffer[4000];
        int bytes;
        bytes = read(EXECsetup.out_pipe[0], buffer, sizeof(buffer));
        if(bytes < 0)
            throw 500;
        EXECsetup.scriptOut.append(buffer, bytes);
        // char buffer[1000];
        // int bytes_read= read(EXECsetup.out_pipe[0], buffer, sizeof(buffer));
        // if(bytes_read == -1)
        //     throw 500;
        // while(bytes_read> 0){
        //     EXECsetup.scriptOut.append(buffer, bytes_read);
        //     bytes_read= read(EXECsetup.out_pipe[0], buffer, sizeof(buffer));
        // }
        close(EXECsetup.out_pipe[0]);
}

// std::string scriptResponse(EXECsetUP &EXECsetup){
//     std::ostringstream response;
//     // std::string body = ;
//     if(EXECsetup.scriptOut.find("Content-Type") == std::string::npos 
//         && EXECsetup.scriptOut.find("Content-type") == std::string::npos){
//         response << "HTTP/1.1 200 OK\r\n"
//                 << "Content-Type: text/html\r\n"
//                 << "Content-Length: "
//                 << "Connection: close\r\n"
//                 << EXECsetup.scriptOut.size() 
//                 << "\r\n\r\n";
//             std::cout << "\033[34m" << EXECsetup.scriptOut << "\n" << "\033[0m";
//         return response.str() + EXECsetup.scriptOut + "\n";
//     }
//     response << "HTTP/1.1 200 OK\r\n"
//             << "Content-Length: "
//             << EXECsetup.scriptOut.size()
//             << "\r\n"
//             << EXECsetup.scriptOut + "\n";
//     return response.str();
// }

// -------------------------------------------------------------------

std::string scriptResponse(EXECsetUP &exec)
{
    std::string &out = exec.scriptOut;

    // 🔹 Trouver séparation headers / body CGI
    size_t pos = out.find("\r\n\r\n");
    size_t sep_len = 4;

    if (pos == std::string::npos)
    {
        pos = out.find("\n\n");
        sep_len = 2;
    }

    std::string cgi_headers;
    std::string body;

    if (pos != std::string::npos)
    {
        cgi_headers = out.substr(0, pos);
        body = out.substr(pos + sep_len);
    }
    else
    {
        // aucun header CGI → tout est body
        body = out;
    }

    // 🔹 Parser headers CGI
    std::istringstream iss(cgi_headers);
    std::string line;
    std::string content_type = "text/html";
    std::string status = "200 OK";

    while (std::getline(iss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("content-type:") == 0)
            content_type = line.substr(line.find(":") + 1);
        else if (lower.find("status:") == 0)
            status = line.substr(line.find(":") + 1);
    }

    // 🔹 Nettoyer espaces
    while (!content_type.empty() && content_type[0] == ' ')
        content_type.erase(0, 1);
    while (!status.empty() && status[0] == ' ')
        status.erase(0, 1);

    // 🔹 Construire réponse HTTP
    std::ostringstream response;

    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}
void deleteEnv(CGIsetUP &CGIsetup){
    for(int i = 0; CGIsetup.env[i]; i++){
        delete[] (CGIsetup.env[i]);
    }
    for(int i = 0; CGIsetup.args[i]; i++){
        delete[] (CGIsetup.args[i]);
    }
    delete[] CGIsetup.env;
    delete[] CGIsetup.args;
}

std::string response::executeCGI(){

    CGIsetUP CGIsetup;
    EXECsetUP EXECsetup;
    
    
    setCGI(request, &CGIsetup, loc);
    setEXEC(&EXECsetup);
    
    if(EXECsetup.pid == 0)
        execScript(EXECsetup, CGIsetup);
    
    writeToScript(request, EXECsetup);
    checkTimeout(EXECsetup.pid, EXECsetup.start);
    readFromScript(EXECsetup);
    deleteEnv(CGIsetup);

    return scriptResponse(EXECsetup);
}

std::string response::handlePOST(){ 
    try{
        loc = getLocation(); 
        // std::cout << "\033[34m" << "cgi_pass :" << loc.cgi_pass << "\033[0m" << "\n";
        // if (!loc.cgi_ext.empty())
        // {
        //     std::cout << "\033[34m"
        //         << "cgi_ext : " << loc.cgi_ext.begin()->first
        //         << " => " << loc.cgi_ext.begin()->second
        //         << "\033[0m" << std::endl;
        // }
        // else
        // {
        //     std::cout << "\033[31m"
        //             << "cgi_ext is empty"
        //             << "\033[0m" << std::endl;
        // }
        // std::cout << "\033[34m" << "path : " << loc.path << "\033[0m" << "\n";
        // std::cout << "\033[34m" << "root : " << loc.root << "\033[0m" << "\n";
        // std::cout << "\033[34m" << "upload path : " << loc.upload_path << "\033[0m" << "\n";
        // std::cout << "\033[34m" << "autoindex : " << loc.autoindex << "\033[0m" << "\n";
        // std::cout << "\033[34m" << "\033[0m" << "\n";

        // std::cout << "\033[31m" << "REQUEST :" << "\033[0m" << "\n";
        // request.print();

        errorhandling();
        std::string ext = getExtension(request.getPath());
        if(loc.cgi_ext.find(ext) != loc.cgi_ext.end())
            return executeCGI();
        // else if(!request.getPath().empty())
        //     throw 400;
        // else if(!ext.empty())
        //     throw 400;
        else if(!loc.upload_path.empty())
            return saveToFile(request, getLocation());
        else 
            throw 400;
    }
    catch(int code){
        return buildErrorResponse(code);
    }
}
