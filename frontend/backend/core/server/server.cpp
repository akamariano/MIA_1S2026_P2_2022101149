#include "server.h"
#include "../../include/httplib.h"
#include "../mount/mount_manager.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <filesystem>

using namespace std;
using namespace httplib;

#include "../commands/mkdisk.h"
#include "../commands/fdisk.h"
#include "../commands/rmdisk.h"
#include "../commands/mount.h"
#include "../commands/mkfs.h"
#include "../commands/mkgrp.h"
#include "../commands/rmgrp.h"
#include "../commands/mkusr.h"
#include "../commands/rmusr.h"
#include "../commands/chgrp.h"
#include "../commands/mkdir_cmd.h"
#include "../commands/mkfile_cmd.h"
#include "../commands/cat_cmd.h"
#include "../commands/rep_cmd.h"
#include "../filesystem/login.h"
#include "../filesystem/logout.h"
#include "../filesystem/session_manager.h"
#include "../commands/remove_cmd.h"
#include "../commands/rename_cmd.h"
#include "../commands/copy_cmd.h"
#include "../commands/move_cmd.h"
#include "../commands/find_cmd.h"
#include "../commands/chown_cmd.h"
#include "../commands/chmod_cmd.h"
#include "../commands/journaling_cmd.h"
#include "../commands/loss_cmd.h"
#include "../filesystem/EXT2Utils.h"
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"
#include <algorithm>

string captureOutput(function<void()> fn) {
    streambuf* oldBuf = cout.rdbuf();
    ostringstream ss;
    cout.rdbuf(ss.rdbuf());
    fn();
    cout.rdbuf(oldBuf);
    return ss.str();
}

// Limpia comillas al inicio y fin de un string
static string stripQuotes(const string& s) {
    string r = s;
    if (!r.empty() && r.front() == '"') r.erase(0, 1);
    if (!r.empty() && r.back()  == '"') r.pop_back();
    return r;
}

// Divide respetando comillas: -path="/home/mi carpeta" se trata como un token
vector<string> splitArgs(const string& input) {
    vector<string> tokens;
    string current;
    bool inQuotes = false;

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == '"') {
            // Si estamos fuera de comillas y el siguiente char es espacio o fin -> comilla suelta, ignorar
            if (!inQuotes && (i+1 >= input.size() || input[i+1] == ' ')) {
                // comilla suelta al final de token, ignorar
                continue;
            }
            inQuotes = !inQuotes;
            // No agregar la comilla al token
        } else if (c == ' ' && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

string processCommand(const string& rawInput) {
    if (rawInput.empty()) return "";

    // Limpiar comentarios inline (# fuera de comillas)
    string input;
    bool inQ = false;
    for (size_t i = 0; i < rawInput.size(); i++) {
        if (rawInput[i] == '"') inQ = !inQ;
        if (rawInput[i] == '#' && !inQ) break;
        input += rawInput[i];
    }
    // Trim
    while (!input.empty() && input.back() == ' ') input.pop_back();
    if (input.empty()) return "";

    vector<string> args = splitArgs(input);
    if (args.empty()) return "";

    string command = args[0];
    transform(command.begin(), command.end(), command.begin(), ::tolower);

    return captureOutput([&]() {

        // ================== MKDISK ==================
        if (command == "mkdisk") {
            int size = -1; char unit = 'M';
            string fit = "FF", path = "";
            bool hasUnknown = false;
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if      (lower.find("-size=") == 0) size = stoi(p.substr(6));
                else if (lower.find("-unit=") == 0) unit = toupper(p.substr(6)[0]);
                else if (lower.find("-fit=")  == 0) { fit = p.substr(5); transform(fit.begin(), fit.end(), fit.begin(), ::toupper); }
                else if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
                else hasUnknown = true;
            }
            if (hasUnknown) { cout << "ERROR: Parámetros inválidos\n"; return; }
            if (size <= 0 || path.empty()) { cout << "ERROR: Parámetros inválidos\n"; return; }
            MkDisk mk; mk.execute(size, unit, fit, path);
        }

        // ================== RMDISK ==================
        else if (command == "rmdisk") {
            string path = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string lower = args[i];
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-path=") == 0) path = stripQuotes(args[i].substr(6));
            }
            if (path.empty()) { cout << "ERROR: -path es obligatorio\n"; return; }
            RmDisk rm; rm.execute(path);
        }

        // ================== FDISK ==================
                else if (command == "fdisk") {
            int size = -1; char unit = 'K'; char type = 'P';
            string fit = "WF", path = "", name = "";
            string deleteType = "", addStr = "";
            bool hasAdd = false;
            int addVal = 0;
 
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if      (lower.find("-size=")   == 0) size = stoi(p.substr(6));
                else if (lower.find("-unit=")   == 0) unit = toupper(p.substr(6)[0]);
                else if (lower.find("-path=")   == 0) path = stripQuotes(p.substr(6));
                else if (lower.find("-type=")   == 0) type = toupper(p.substr(6)[0]);
                else if (lower.find("-fit=")    == 0) { fit = p.substr(5); transform(fit.begin(), fit.end(), fit.begin(), ::toupper); }
                else if (lower.find("-name=")   == 0) name = stripQuotes(p.substr(6));
                else if (lower.find("-delete=") == 0) deleteType = lower.substr(8);
                else if (lower.find("-add=")    == 0) { addVal = stoi(p.substr(5)); hasAdd = true; }
            }
 
            if (!deleteType.empty()) {
                // FDISK DELETE
                FDisk fd; fd.executeDelete(deleteType, name, path);
            } else if (hasAdd) {
                // FDISK ADD
                FDisk fd; fd.executeAdd(addVal, unit, name, path);
            } else {
                // FDISK CREATE (original)
                if (size <= 0 || path.empty() || name.empty()) {
                    cout << "ERROR: Parámetros inválidos\n"; return;
                }
                FDisk fd; fd.execute(size, unit, path, type, fit, name);
            }
        }

        // ================== MOUNT / MOUNTED ==================
        else if (command == "mount" || command == "mounted") {
            if (command == "mounted" || args.size() == 1) {
                MountManager::showMounted(); return;
            }
            string path = "", name = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
                else if (lower.find("-name=") == 0) name = stripQuotes(p.substr(6));
            }
            if (path.empty() || name.empty()) { cout << "ERROR: -path y -name son obligatorios\n"; return; }
            Mount m; m.execute(path, name);
        }
                // ================== UNMOUNT ==================
        else if (command == "unmount") {
            string id = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-id=") == 0) id = p.substr(4);
            }
            if (id.empty()) { cout << "ERROR: -id es obligatorio\n"; return; }
            MountedPartition* part = MountManager::getMountedById(id);
            if (!part) {
                cout << "ERROR: ID '" << id << "' no está montado\n"; return;
            }
            MountManager::unmountById(id);
            cout << "OK: Partición '" << id << "' desmontada correctamente\n";
        }
        // ================== MKFS ==================
               else if (command == "mkfs") {
            string id = "", fs = "2fs";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if      (lower.find("-id=")   == 0) id = p.substr(4);
                else if (lower.find("-fs=")   == 0) fs = lower.substr(4); // -fs=2fs o -fs=3fs
                else if (lower.find("-type=") == 0) {
                    // compatibilidad con -type=full (ignorar, solo usar -fs)
                }
            }
            if (id.empty()) { cout << "ERROR: -id es obligatorio\n"; return; }
            if (fs != "2fs" && fs != "3fs") {
                cout << "ERROR: -fs debe ser '2fs' (EXT2) o '3fs' (EXT3)\n"; return;
            }
            Mkfs mkfs; mkfs.execute(id, fs);
        }
 
        // ================== LOGIN ==================
        else if (command == "login") {
            string user = "", pass = "", id = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if      (lower.find("-user=") == 0) user = stripQuotes(p.substr(6));
                else if (lower.find("-pass=") == 0) pass = stripQuotes(p.substr(6));
                else if (lower.find("-id=")   == 0) id   = p.substr(4);
            }
            if (user.empty() || pass.empty() || id.empty()) { cout << "ERROR: Parámetros inválidos\n"; return; }
            Login l; l.execute(user, pass, id);
        }

        // ================== LOGOUT ==================
        else if (command == "logout") {
            Logout lo; lo.execute();
        }

        // ================== MKGRP ==================
        else if (command == "mkgrp") {
            string name = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string lower = args[i];
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-name=") == 0) name = stripQuotes(args[i].substr(6));
            }
            if (name.empty()) { cout << "ERROR: -name es obligatorio\n"; return; }
            MkGrp mg; mg.execute(name);
        }

        // ================== RMGRP ==================
        else if (command == "rmgrp") {
            string name = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string lower = args[i];
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-name=") == 0) name = stripQuotes(args[i].substr(6));
            }
            if (name.empty()) { cout << "ERROR: -name es obligatorio\n"; return; }
            RmGrp rg; rg.execute(name);
        }

        // ================== MKUSR ==================
        else if (command == "mkusr") {
            string user = "", pass = "", grp = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if      (lower.find("-user=") == 0) user = stripQuotes(p.substr(6));
                else if (lower.find("-pass=") == 0) pass = stripQuotes(p.substr(6));
                else if (lower.find("-grp=")  == 0) grp  = stripQuotes(p.substr(5));
            }
            if (user.empty() || pass.empty() || grp.empty()) {
                cout << "ERROR: -user, -pass y -grp son obligatorios\n"; return;
            }
            MkUsr mu; mu.execute(user, pass, grp);
        }

        // ================== RMUSR ==================
        else if (command == "rmusr") {
            string user = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string lower = args[i];
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-user=") == 0) user = stripQuotes(args[i].substr(6));
            }
            if (user.empty()) { cout << "ERROR: -user es obligatorio\n"; return; }
            RmUsr ru; ru.execute(user);
        }

        // ================== CHGRP ==================
        else if (command == "chgrp") {
            string user = "", grp = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if      (lower.find("-user=") == 0) user = stripQuotes(p.substr(6));
                else if (lower.find("-grp=")  == 0) grp  = stripQuotes(p.substr(5));
            }
            if (user.empty() || grp.empty()) { cout << "ERROR: Parámetros inválidos\n"; return; }
            ChGrp cg; cg.execute(user, grp);
        }

        // ================== MKDIR ==================
        else if (command == "mkdir") {
            string path = ""; bool createParents = false;
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower == "-p") createParents = true;
                else if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
            }
            if (path.empty()) { cout << "ERROR: -path es obligatorio\n"; return; }
            // Validar que el path sea válido (no solo backslash)
            if (path == "\\" || path == "/") {
                MkdirCmd md; md.execute(path, createParents);
            } else {
                MkdirCmd md; md.execute(path, createParents);
            }
        }

        // ================== MKFILE ==================
        else if (command == "mkfile") {
            string path = "", cont = ""; int size = 0; bool r = false;
            bool sizeNeg = false;
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower == "-r") r = true;
                else if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
                else if (lower.find("-size=") == 0) {
                    int s = stoi(p.substr(6));
                    if (s < 0) { sizeNeg = true; }
                    else size = s;
                }
                else if (lower.find("-cont=") == 0) cont = stripQuotes(p.substr(6));
            }
            if (sizeNeg) { cout << "ERROR: -size no puede ser negativo\n"; return; }
            if (path.empty()) { cout << "ERROR: -path es obligatorio\n"; return; }
            MkfileCmd mf; mf.execute(path, r, size, cont);
        }

        // ================== CAT ==================
        else if (command == "cat") {
            vector<string> files;
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-file") == 0) {
                    size_t eq = p.find('=');
                    if (eq != string::npos) {
                        string fp = stripQuotes(p.substr(eq + 1));
                        files.push_back(fp);
                    }
                }
            }
            if (files.empty()) { cout << "ERROR: Debe especificar al menos -file1\n"; return; }
            CatCmd cat; cat.execute(files);
        }

      // ================== REP ==================
        else if (command == "rep") {
            string name = "", path = "", id = "", pathFileLs = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                // Limpiar comillas sueltas en cualquier posición
                string val = p;
                // Remover comillas de apertura o cierre sueltas
                while (!val.empty() && val.back() == '"') val.pop_back();
                while (!val.empty() && val.front() == '"') val.erase(0,1);
                string lval = lower;
                while (!lval.empty() && lval.back() == '"') lval.pop_back();
                while (!lval.empty() && lval.front() == '"') lval.erase(0,1);

                if      (lval.find("-name=") == 0)         name       = lval.substr(6);
                else if (lval.find("-path_file_ls=") == 0) pathFileLs = val.substr(14);
                else if (lval.find("-path=") == 0)         path       = val.substr(6);
                else if (lval.find("-id=") == 0)           id         = val.substr(4);
            }
            if (name.empty() || path.empty() || id.empty()) {
                cout << "ERROR: -name, -path e -id son obligatorios\n"; return;
            }
            RepCmd rep; rep.execute(name, path, id, pathFileLs);
        }
                // ================== REMOVE ==================
        else if (command == "remove") {
            string path = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
            }
            if (path.empty()) { cout << "ERROR: -path es obligatorio\n"; return; }
            RemoveCmd rm; rm.execute(path);
        }
        // RENAME
else if (command == "rename") {
    string path = "", name = "";
    for (int i = 1; i < (int)args.size(); i++) {
        string p = args[i], lower = p;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
        else if (lower.find("-name=") == 0) name = stripQuotes(p.substr(6));
    }
    if (path.empty() || name.empty()) {
        cout << "ERROR: -path y -name son obligatorios\n"; return;
    }
    RenameCmd rc; rc.execute(path, name);
}
        //COPY
        else if (command == "copy") {
            string path = "", destino = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-path=") == 0)    path    = stripQuotes(p.substr(6));
                else if (lower.find("-destino=") == 0) destino = stripQuotes(p.substr(9));
            }
            if (path.empty() || destino.empty()) {
                cout << "ERROR: -path y -destino son obligatorios\n"; return;
            }
            CopyCmd cc; cc.execute(path, destino);
        }
        //MOVE
                else if (command == "move") {
            string path = "", destino = "";
            for (int i = 1; i < (int)args.size(); i++) {
                string p = args[i], lower = p;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower.find("-path=") == 0)         path    = stripQuotes(p.substr(6));
                else if (lower.find("-destino=") == 0) destino = stripQuotes(p.substr(9));
            }
            if (path.empty() || destino.empty()) {
                cout << "ERROR: -path y -destino son obligatorios\n"; return;
            }
            MoveCmd mc; mc.execute(path, destino);
        }
        //FIND
        else if (command == "find") {
    string path = "", name = "";
    for (int i = 1; i < (int)args.size(); i++) {
        string p = args[i], lower = p;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
        else if (lower.find("-name=") == 0) name = stripQuotes(p.substr(6));
    }
    if (path.empty() || name.empty()) {
        cout << "ERROR: -path y -name son obligatorios\n"; return;
    }
    FindCmd fc; fc.execute(path, name);
}
// CHOWN
else if (command == "chown") {
    string path = "", usuario = ""; bool r = false;
    for (int i = 1; i < (int)args.size(); i++) {
        string p = args[i], lower = p;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "-r") r = true;
        else if (lower.find("-path=") == 0)    path    = stripQuotes(p.substr(6));
        else if (lower.find("-usuario=") == 0) usuario = stripQuotes(p.substr(9));
    }
    if (path.empty() || usuario.empty()) {
        cout << "ERROR: -path y -usuario son obligatorios\n"; return;
    }
    ChownCmd cc; cc.execute(path, usuario, r);
}

// CHMOD
else if (command == "chmod") {
    string path = "", ugo = ""; bool r = false;
    for (int i = 1; i < (int)args.size(); i++) {
        string p = args[i], lower = p;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "-r") r = true;
        else if (lower.find("-path=") == 0) path = stripQuotes(p.substr(6));
        else if (lower.find("-ugo=") == 0)  ugo  = p.substr(5);
    }
    if (path.empty() || ugo.empty()) {
        cout << "ERROR: -path y -ugo son obligatorios\n"; return;
    }
    ChmodCmd cm; cm.execute(path, ugo, r);
}
// LOSS
else if (command == "loss") {
    string id = "";
    for (int i = 1; i < (int)args.size(); i++) {
        string p = args[i], lower = p;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("-id=") == 0) id = p.substr(4);
    }
    if (id.empty()) { cout << "ERROR: -id es obligatorio\n"; return; }
    LossCmd lc; lc.execute(id);
}

// JOURNALING
else if (command == "journaling") {
    string id = "";
    for (int i = 1; i < (int)args.size(); i++) {
        string p = args[i], lower = p;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("-id=") == 0) id = p.substr(4);
    }
    if (id.empty()) { cout << "ERROR: -id es obligatorio\n"; return; }
    JournalingCmd jc; jc.execute(id);
}
        else {
            cout << "ERROR: Comando '" << command << "' no reconocido\n";
        }
    });
}

void startServer(int port) {
    Server svr;

    svr.set_pre_routing_handler([](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return Server::HandlerResponse::Handled;
        }
        return Server::HandlerResponse::Unhandled;
    });

    svr.Get("/status", [](const Request&, Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    svr.Post("/command", [](const Request& req, Response& res) {
        string body = req.body;
        string command = "";
        // Extraer valor de "command" respetando \" escapadas
        
        size_t pos = body.find("\"command\"");
        if (pos != string::npos) {
            size_t start = body.find("\"", pos + 9);
            if (start != string::npos) {
                start++; // saltar la comilla de apertura
                string raw;
                bool escaped = false;
                for (size_t i = start; i < body.size(); i++) {
                    if (escaped) {
                        raw += body[i];
                        escaped = false;
                    } else if (body[i] == '\\') {
                        raw += body[i];
                        escaped = true;
                    } else if (body[i] == '"') {
                        break; // comilla de cierre real
                    } else {
                        raw += body[i];
                    }
                }
                command = raw;
            }
        }
        if (command.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"Comando vacío\"}", "application/json");
            return;
        }

       // Desencode escapes del JSON: \n -> newline, \" -> ", \\ -> backslash
        string decoded;
        for (size_t i = 0; i < command.size(); i++) {
            if (command[i] == '\\' && i+1 < command.size()) {
                char next = command[i+1];
                if      (next == 'n')  { decoded += '\n'; i++; }
                else if (next == '"')  { decoded += '"';  i++; }
                else if (next == '\\') { decoded += '\\'; i++; }
                else if (next == 't')  { decoded += '\t'; i++; }
                else if (next == 'r')  { decoded += '\r'; i++; }
                else { decoded += command[i]; }
            } else {
                decoded += command[i];
            }
        }

        string fullOutput = "";
        istringstream ss(decoded);
        string line;
        while (getline(ss, line)) {
            // Trim
            while (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            string out = processCommand(line);
            if (!out.empty()) fullOutput += out;
        }

        string escaped = "";
        for (char c : fullOutput) {
            if      (c == '"')  escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else escaped += c;
        }

        res.set_content("{\"output\":\"" + escaped + "\"}", "application/json");
    });

    svr.Get("/report", [](const Request& req, Response& res) {
        if (!req.has_param("path")) {
            res.status = 400;
            res.set_content("{\"error\":\"path requerido\"}", "application/json");
            return;
        }
        string filePath = req.get_param_value("path");
        ifstream file(filePath, ios::binary);
        if (!file.is_open()) {
            res.status = 404;
            res.set_content("{\"error\":\"Archivo no encontrado\"}", "application/json");
            return;
        }
        string ext = filesystem::path(filePath).extension().string();
        string contentType = "application/octet-stream";
        if (ext == ".jpg" || ext == ".jpeg") contentType = "image/jpeg";
        else if (ext == ".png") contentType = "image/png";
        else if (ext == ".txt") contentType = "text/plain";
        else if (ext == ".pdf") contentType = "application/pdf";
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        res.set_content(content, contentType.c_str());
    });
    // ===== GET /disks =====
    svr.Get("/disks", [](const Request&, Response& res) {
        ostringstream json;
        json << "[";
        bool first = true;
        for (auto& m : MountManager::getAll()) {
            if (!first) json << ",";
            json << "{"
                 << "\"id\":\"" << m.id << "\","
                 << "\"path\":\"" << m.path << "\","
                 << "\"name\":\"" << m.name << "\","
                 << "\"start\":" << m.start << ","
                 << "\"size\":" << m.size
                 << "}";
            first = false;
        }
        json << "]";
        res.set_content(json.str(), "application/json");
    });

    // ===== GET /browse?id=491A&path=/ =====
    svr.Get("/browse", [](const Request& req, Response& res) {
        if (!req.has_param("id") || !req.has_param("path")) {
            res.status = 400;
            res.set_content("{\"error\":\"id y path requeridos\"}", "application/json");
            return;
        }
        string id   = req.get_param_value("id");
        string path = req.get_param_value("path");

        MountedPartition* part = MountManager::getMountedById(id);
        if (!part) {
            res.status = 404;
            res.set_content("{\"error\":\"ID no montado\"}", "application/json");
            return;
        }

        FILE* disk = fopen(part->path.c_str(), "rb");
        if (!disk) {
            res.status = 500;
            res.set_content("{\"error\":\"No se pudo abrir disco\"}", "application/json");
            return;
        }

        SuperBlock sb;
        readSuperBlock(disk, part->start, sb);

        // Validar magic number — rechaza particiones sin formato
        if (sb.s_magic != 0xEF53) {
            fclose(disk);
            res.status = 400;
            res.set_content("{\"error\":\"Particion sin formato EXT2/EXT3\"}", "application/json");
            return;
        }

        int inodeNum = resolvePath(disk, sb, path);
        if (inodeNum == -1) {
            fclose(disk);
            res.status = 404;
            res.set_content("{\"error\":\"Ruta no encontrada\"}", "application/json");
            return;
        }

        // Validar que el inodo del directorio está activo en el bitmap
        if (readBitmapInode(disk, sb, inodeNum) != 1) {
            fclose(disk);
            res.status = 404;
            res.set_content("{\"error\":\"Directorio no disponible\"}", "application/json");
            return;
        }

        Inode dirInode;
        readInode(disk, sb, inodeNum, dirInode);

        if (dirInode.i_type != '0') {
            fclose(disk);
            res.status = 400;
            res.set_content("{\"error\":\"La ruta no es un directorio\"}", "application/json");
            return;
        }

        ostringstream json;
        json << "{\"path\":\"" << path << "\",\"entries\":[";
        bool first = true;

        for (int b = 0; b < 12; b++) {
            if (dirInode.i_block[b] == -1) break;

            // Validar que el bloque está activo
            if (readBitmapBlock(disk, sb, dirInode.i_block[b]) != 1) continue;

            DirectoryBlock db;
            memset(&db, 0, sizeof(DirectoryBlock));
            readBlock(disk, sb, dirInode.i_block[b], &db);

            for (int e = 0; e < 4; e++) {
                if (db.b_content[e].b_inodo == -1) continue;

                string ename(db.b_content[e].b_name,
                             strnlen(db.b_content[e].b_name, 12));

                // Saltar entradas especiales y nombres vacíos
                if (ename.empty() || ename == "." || ename == "..") continue;

                int childInodeNum = db.b_content[e].b_inodo;

                // Validar rango del inodo
                if (childInodeNum < 0 || childInodeNum >= sb.s_inodes_count) continue;

                // Validar que el inodo hijo está activo en el bitmap
                if (readBitmapInode(disk, sb, childInodeNum) != 1) continue;

                Inode child;
                readInode(disk, sb, childInodeNum, child);

                // Validar tipo válido
                if (child.i_type != '0' && child.i_type != '1') continue;

                // Formatear permisos UGO
                string typePrefix = (child.i_type == '0') ? "d" : "-";
                int p = child.i_perm;
                int u = (p / 100) % 10, g = (p / 10) % 10, o = p % 10;
                auto bits = [](int x) -> string {
                    string r;
                    r += (x & 4) ? "r" : "-";
                    r += (x & 2) ? "w" : "-";
                    r += (x & 1) ? "x" : "-";
                    return r;
                };
                string perms = typePrefix + bits(u) + bits(g) + bits(o);

                // Formatear fecha — validar timestamp razonable (> año 2000)
                char dateBuf[32] = "fecha invalida";
                if (child.i_ctime > 946684800) {
                    struct tm* ti = localtime(&child.i_ctime);
                    if (ti) strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %H:%M", ti);
                }

                // Escapar nombre para JSON
                string safeName;
                for (char c : ename) {
                    if      (c == '"')  safeName += "\\\"";
                    else if (c == '\\') safeName += "\\\\";
                    else safeName += c;
                }

                if (!first) json << ",";
                json << "{"
                     << "\"name\":\""  << safeName << "\","
                     << "\"type\":\""  << (child.i_type == '0' ? "folder" : "file") << "\","
                     << "\"size\":"    << child.i_size << ","
                     << "\"perm\":\""  << perms << "\","
                     << "\"uid\":"     << child.i_uid << ","
                     << "\"gid\":"     << child.i_gid << ","
                     << "\"date\":\""  << dateBuf << "\""
                     << "}";
                first = false;
            }
        }

        json << "]}";
        fclose(disk);
        res.set_content(json.str(), "application/json");
    });

    // ===== GET /file?id=491A&path=/home/a.txt =====
    svr.Get("/file", [](const Request& req, Response& res) {
        if (!req.has_param("id") || !req.has_param("path")) {
            res.status = 400;
            res.set_content("{\"error\":\"id y path requeridos\"}", "application/json");
            return;
        }
        string id   = req.get_param_value("id");
        string path = req.get_param_value("path");

        MountedPartition* part = MountManager::getMountedById(id);
        if (!part) {
            res.status = 404;
            res.set_content("{\"error\":\"ID no montado\"}", "application/json");
            return;
        }

        FILE* disk = fopen(part->path.c_str(), "rb");
        if (!disk) {
            res.status = 500;
            res.set_content("{\"error\":\"No se pudo abrir disco\"}", "application/json");
            return;
        }

        SuperBlock sb;
        readSuperBlock(disk, part->start, sb);

        int inodeNum = resolvePath(disk, sb, path);
        if (inodeNum == -1) {
            fclose(disk);
            res.status = 404;
            res.set_content("{\"error\":\"Archivo no encontrado\"}", "application/json");
            return;
        }

        // Validar bitmap
        if (readBitmapInode(disk, sb, inodeNum) != 1) {
            fclose(disk);
            res.status = 404;
            res.set_content("{\"error\":\"Archivo no disponible\"}", "application/json");
            return;
        }

        Inode inode;
        readInode(disk, sb, inodeNum, inode);

        if (inode.i_type != '1') {
            fclose(disk);
            res.status = 400;
            res.set_content("{\"error\":\"No es un archivo\"}", "application/json");
            return;
        }

        string content;
        for (int b = 0; b < 12; b++) {
            if (inode.i_block[b] == -1) break;
            FileBlock fb;
            memset(&fb, 0, sizeof(FileBlock));
            readBlock(disk, sb, inode.i_block[b], &fb);
            int toRead = min((int)sizeof(fb.b_content),
                             inode.i_size - (int)content.size());
            if (toRead <= 0) break;
            content.append(fb.b_content, toRead);
        }
        // Bloque indirecto
        if (inode.i_block[12] != -1) {
            PointerBlock pb;
            readBlock(disk, sb, inode.i_block[12], &pb);
            for (int i = 0; i < 16; i++) {
                if (pb.b_pointers[i] == -1) break;
                FileBlock fb;
                memset(&fb, 0, sizeof(FileBlock));
                readBlock(disk, sb, pb.b_pointers[i], &fb);
                int toRead = min((int)sizeof(fb.b_content),
                                 inode.i_size - (int)content.size());
                if (toRead <= 0) break;
                content.append(fb.b_content, toRead);
            }
        }
        fclose(disk);

        // Escapar para JSON
        string escaped;
        for (char c : content) {
            if      (c == '"')  escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else escaped += c;
        }

        res.set_content("{\"path\":\"" + path + "\",\"content\":\"" + escaped + "\"}",
                        "application/json");
    });

    // ===== GET /session =====
    svr.Get("/session", [](const Request&, Response& res) {
        const Session& s = SessionManager::get();
        string json;
        if (s.active) {
            json = "{\"active\":true,\"username\":\"" + s.username +
                   "\",\"groupname\":\"" + s.groupname +
                   "\",\"partitionId\":\"" + s.partitionId + "\"}";
        } else {
            json = "{\"active\":false,\"username\":\"\",\"groupname\":\"\",\"partitionId\":\"\"}";
        }
        res.set_content(json, "application/json");
    });

    // ===== GET /journaling?id=491B =====
    svr.Get("/journaling", [](const Request& req, Response& res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("{\"error\":\"id requerido\"}", "application/json");
            return;
        }
        string id = req.get_param_value("id");
        string output = processCommand("journaling -id=" + id);

        string escaped;
        for (char c : output) {
            if      (c == '"')  escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else escaped += c;
        }
        res.set_content("{\"output\":\"" + escaped + "\"}", "application/json");
    });
    cout << "==============================\n";
    cout << " EXTREAMFS SERVER\n";
    cout << " Puerto: " << port << "\n";
    cout << " http://localhost:" << port << "\n";
    cout << "==============================\n";

    svr.listen("0.0.0.0", port);
}