#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#define SHELL_EXCEPTION_T std::string
#include "Strings.h"
#if defined(__WIN32__) || defined(_MSVC_LANG)
#include <windows.h>    //GetModuleFileNameA
#define stat _stat
#elif defined(__APPLE__)
#include <unistd.h>     //readlink
#include <libproc.h>    //??
#else
#include <unistd.h>     //readlink
#endif


class Shell {
public:

    /**
        Sends a command to cmd.exe, even if the application was run
        from another style of commandline (putty or cygwin)
        @param string cmi - The command to be sent to cmd
        @param bool writeToStdOut = false - If true, will print the cmi and results to stdout
        @param bool careAboutReturn = true - If I'm not using the result, don't bother taking the memory to store it
        @param bool sanitizeStdOut = false - Whether or not to make hidden characters visible
        @return string - The stdout and stderr of the result
    **/
#ifndef __APPLE__
    static std::string cmd(std::string cmi, bool writeToStdOut = false, bool careAboutReturn = true/*, bool sanitizeStdOut = false*/) {  //Does a string copy
#if defined(__WIN32__) || defined(_MSVC_LANG)
        std::string raw = exec("cmd.exe /c \"" + cmi + "\" 2>&1", writeToStdOut, careAboutReturn/*, sanitizeStdOut*/);
#else
        std::string realcmi = Strings::ReplaceAll(cmi, "\\", "\\\\");
        realcmi = Strings::ReplaceAll(realcmi, "\"", "\\\"");
        std::string raw = exec("cmd.exe /c \"" + realcmi + "\" 2>&1", writeToStdOut, careAboutReturn/*, sanitizeStdOut*/);
#endif
        //std::cout << cmi << std::endl;
        //std::cout << std::endl << "RAW: \"" << raw << "\"" << std::endl << std::endl;;
        raw = Strings::trim(raw);
        raw = Strings::ReplaceAll(raw, "\r\n", "\n");
        raw = Strings::ReplaceAll(raw, "\r", "\n");
        raw = Strings::ReplaceAll(raw, "\f", "\n");

        //std::cout << std::endl << "ST2: \"" << raw << "\"" << std::endl << std::endl;;
        return raw;
    }
#endif


    /**
        Deletes a file.  If the file does not exist, it will just exit.
        @param string filename - The file to delete
        @param uint_fast8_t retries = 10 - The number of times to try if the first time fails
        @throws SHELL_EXCEPTION_T
    **/
    static void deleteFile(const std::string& pfileName, uint_fast8_t retries = 10) {
        if (!fileExists(pfileName)) return;
        try {
            tryDelete(pfileName);
        }
        catch (SHELL_EXCEPTION_T const& e) {
            for (uint_fast8_t i = 0; i < retries; ++i) {
                wait(250);
                try {
                    tryDelete(pfileName);
                    return;
                }
                catch (SHELL_EXCEPTION_T const&) {}
            }
            throw e;
        }
    }


    /**
        Gets the hostname of the computer
        @return string
    **/
    inline static std::string hostname() {
        return cmd("hostname");
    }


    /**
        Gets the current working directory
    **/
    static std::string pwd() {
        std::string res = cmd("cd");
        if (Strings::EndsWith(res, "\\") || Strings::EndsWith(res, "/")) return res;
        return res + "/";
    }

    static std::string absolutePath(const std::string& pathname) {
        if (pathname.empty()) throw SHELL_EXCEPTION_T("Not a pathname, is empty");
        if (pathname.at(0) == '/') return pathname;
        if ((pathname.at(1) == ':') || (pathname.at(2) == '\\')) {
            return pathname;
        }
        return pwd() + pathname;
    }


    /**
        Checks if the program is being run in admin mode
        @return bool
    **/
    inline bool static isRunningAsAdmin() {
#if defined(__WIN32__) || defined(_MSVC_LANG)
        return !(Strings::Contains(cmd("net session"), "Access is denied"));
#else
        //Maybe is Super user?
        return false;
#endif
    }


    /**
        Creates a directory if it doesn't exist.
        @param string dirname - The name of the directory we want to exist
        @throws SHELL_EXCEPTION_T
    **/
    static void mkdir(const std::string& pDirectoryName) {
        if (dirExists(pDirectoryName)) return;
        try {
            if (!std::filesystem::create_directory(pDirectoryName)) {
                throw SHELL_EXCEPTION_T("Was not able to create " + pDirectoryName);
            }
        }
        catch (std::filesystem::filesystem_error const& e) {
            throw SHELL_EXCEPTION_T(e.what());
        }
    }


    /**
        Deletes a directory.  If the directory does not exist, it will just exit.
        @param string filename - The directory to delete
        @param uint_fast8_t retries = 10 - The number of times to try if the first time fails
        @throws SHELL_EXCEPTION_T
    **/
    static void deleteDirectory(const std::string& pDirectoryName, uint_fast8_t retries = 10) {
        if (!dirExists(pDirectoryName)) return;
        try {
            tryDeleteFilesInDir(pDirectoryName);
        }
        catch (SHELL_EXCEPTION_T const& e) {
            for (uint_fast8_t i = 0; i < retries; ++i) {
                wait(250);
                try {
                    tryDeleteFilesInDir(pDirectoryName);
                    return;
                }
                catch (SHELL_EXCEPTION_T const&) {}
            }
            throw e;
        }
    }


    /**
        This will force a directory to both exist and be completely empty.
        @param string dirname - The name of the directory we want to exist
        @param uint_fast8_t retries = 10 - The number of times to try if the first time fails
        @throws SHELL_EXCEPTION_T
    **/
    static void forceDirToEmpty(const std::string& pDirectoryName, uint_fast8_t retries = 10) {
        std::string winName = windowizePaths(pDirectoryName);
        deleteDirectory(winName, retries);
        mkdir(winName);
    }


    /**
        Moves a file from one location to another, also renames
        @param string sourceFile - The original file
        @param string destFile - The name of the file after move/rename
        @param bool fineIfSourceDoesntExists - Sometimes it's okay if the source isn't there
        @throws SHELL_EXCEPTION_T
    **/
    static void moveFile(const std::string& fromFileName, const std::string& toFilename, bool fineIfSourceDoesntExists = false) {
        if (!fileExists(fromFileName)) {
            if (!fineIfSourceDoesntExists) throw SHELL_EXCEPTION_T(fromFileName + " doesn't exist, can't move");
            return;
        }
        if (fileExists(toFilename)) deleteFile(toFilename);
        try {
            std::filesystem::rename(fromFileName.c_str(), toFilename.c_str());
        }
        catch (std::filesystem::filesystem_error const& e) {
            throw SHELL_EXCEPTION_T(e.what());
        }
        if (!fileExists(toFilename)) throw SHELL_EXCEPTION_T("Failed to move " + fromFileName + " to " + toFilename);
        if (fileExists(fromFileName)) throw SHELL_EXCEPTION_T("Failed to move " + fromFileName + " to " + toFilename);
    }


    /**
        Copies a file from one location to another, leaving the original
        @param string sourceFile - The original file
        @param string destFile - The name of the file after copy
        @param bool fineIfSourceDoesntExists - Sometimes it's okay if the source isn't there
        @throws SHELL_EXCEPTION_T
    **/
    static void copyFile(const std::string& fromFileName, const std::string& toFilename, bool fineIfSourceDoesntExists = false) {
        if (!fileExists(fromFileName)) {
            if (!fineIfSourceDoesntExists) throw SHELL_EXCEPTION_T(fromFileName + " doesn't exist, can't copy");
            return;
        }
        if (fileExists(toFilename)) deleteFile(toFilename);
        try {
            std::filesystem::copy(fromFileName, toFilename);
        }
        catch (std::filesystem::filesystem_error const& e) {
            throw SHELL_EXCEPTION_T(e.what());
        }
        if (!fileExists(fromFileName)) throw SHELL_EXCEPTION_T(fromFileName + " get removed during copy?");
        if (!fileExists(toFilename)) throw SHELL_EXCEPTION_T("Failed to copy " + fromFileName + " to " + toFilename);
    }

    /**
        Copies a directory from one location to another, leaving the original
        @param string sourceFile - The original file
        @param string destFile - The name of the file after copy
        @param bool fineIfSourceDoesntExists - Sometimes it's okay if the source isn't there
        @param uint_fast64_t newerThanOrEqualTimestamp = 0 - Only move files newer than a certain timestamp
        @param vector<string> extensions = {} - Which extnesions to allow, if left blank, all files are moved
        @param bool doRecursive = true
        @throws SHELL_EXCEPTION_T
    **/
    static void copyDir(const std::string& fromDirName, const std::string& toDirname, bool fineIfSourceDoesntExists = false, uint_fast64_t newerThanOrEqualTimestamp = 0, const std::vector<std::string>& extensions = {}, int doRecursive = 999) {
        if (doRecursive < 0) doRecursive = 0;
        if (!dirExists(fromDirName)) {
            if (!fineIfSourceDoesntExists) throw SHELL_EXCEPTION_T(fromDirName + " doesn't exist, can't copy");
            return;
        }
        if (dirExists(toDirname)) deleteDirectory(toDirname);
        try {
            std::vector<std::string> dirs = getDirsInDir(fromDirName, doRecursive - 1);
            mkdir(toDirname);

            //Sort them by length
            std::sort(dirs.begin(), dirs.end(), [](const std::string& first, const std::string& second) {
                return first.size() < second.size();
                });

            for (auto& s : dirs) {
                const std::string dirName = toDirname + s.substr(fromDirName.length());
                mkdir(dirName);
            }

            const std::vector<std::string> files = getFilesInDir(fromDirName, doRecursive);
            for (auto& s : files) {
                if (!extensions.empty()) {  //Filter out extensions if we want to
                    bool allowed = true;
                    for (auto& ex : extensions) {
                        if (!Strings::EndsWith(s, ex)) {
                            allowed = false;
                            break;
                        }
                    }
                    if (!allowed) continue;
                }

                if (Shell::filemtime(s) >= newerThanOrEqualTimestamp) {
                    const std::string fileName = toDirname + s.substr(fromDirName.length());
                    copyFile(s, fileName);
                }
            }
        }
        catch (std::filesystem::filesystem_error const& e) {
            throw SHELL_EXCEPTION_T(e.what());
        }
        if (!dirExists(fromDirName)) throw SHELL_EXCEPTION_T(fromDirName + " get removed during copy?");
        if (!dirExists(toDirname)) throw SHELL_EXCEPTION_T("Failed to copy " + fromDirName + " to " + toDirname);
    }


    /**
        Opens a file and then immediately close it.  Can be used to either be sure
        an empty file exists or to update it's modification time.
    **/
    static void touch(const std::string& filename) {
        std::fstream output_fstream;
        output_fstream.open(filename, std::ios_base::out);
        if (!output_fstream.is_open()) throw SHELL_EXCEPTION_T("Failed to touch " + filename);
    }

    /**
        This returns whether or not a file exists and is readable
        @param string filename - The filename to check
        @return bool
    **/
    inline static bool fileExists(const std::string& pFilename) {
        const std::string winName = windowizePaths(pFilename);
        return std::filesystem::exists(winName);
    }

    /**
        This returns whether or not a directory exists and is readable
        @param string filename - The filename to check
        @return bool
    **/
    inline static bool dirExists(const std::string& pFilename) {
        const std::string winName = windowizePaths(pFilename);
        return std::filesystem::exists(winName);
    }


    /**
        Parses a pathname or filename and returns the directory of it.  It
        is not like PHP's dirname where continual calls will result in one layer
        back, you must strip the final "/" to do that.
        @param string pathname - the pathname to find the directory of
        @return string - a properly formatted path ending in "/"
        @throw SHELL_EXCEPTION_T a non-path is given
    **/
    static std::string dirname(const std::string& pathname) {
        if (pathname.empty()) throw SHELL_EXCEPTION_T("Not a pathname, is empty");
        if (pathname.at(0) != '/') {
            if (pathname.length() <= 3) throw SHELL_EXCEPTION_T("Not a pathname, must start with / or X:\\ (" + pathname + ")");
            if (pathname.at(1) != ':') throw SHELL_EXCEPTION_T("Not a pathname, must start with / or X:\\ (" + pathname + ")");
            if (pathname.at(2) != '\\') throw SHELL_EXCEPTION_T("Not a pathname, must start with / or X:\\ (" + pathname + ")");
        }
        const size_t winSlash = pathname.rfind('\\');
        const size_t nixSlash = pathname.rfind('/');
        if (winSlash == std::string::npos) {
            if (nixSlash == std::string::npos) return pathname + "/";
            return pathname.substr(0, nixSlash) + "/";
        }
        else if (nixSlash == std::string::npos) {
            return pathname.substr(0, winSlash) + "/";
        }
        else if (winSlash > nixSlash) {
            return pathname.substr(0, winSlash) + "/";
        }
        return pathname.substr(0, nixSlash) + "/";
    }

    /**
        Steps one level up from the current directory
        @param string pathname - the properly formatted path ending in "/" to find the directory of
        @return string - a properly formatted path ending in "/"
        @throw SHELL_EXCEPTION_T a non-path is given
    **/
    static std::string dirdirname(const std::string& pathname) {
        if (pathname.empty()) throw SHELL_EXCEPTION_T("Not a pathname, is empty");
        if (pathname.at(pathname.length() - 1) != '/' && pathname.at(pathname.length() - 1) != '\\') throw SHELL_EXCEPTION_T("Not a pathname, must end with / or \\");
        return dirname(pathname.substr(0, pathname.length() - 1));
    }


    /**
        Parses a pathname and returns just the filename part, removing
        all path information.
        @param string pathname - the pathname to find the directory of
        @return string - just the filename
        @throw SHELL_EXCEPTION_T a non-path is given
    **/
    static std::string filename(const std::string& pathname) {
        if (pathname.empty()) throw SHELL_EXCEPTION_T("Not a filename, is empty");
        if (pathname.at(pathname.length() - 2) == '/') throw SHELL_EXCEPTION_T("No filename, just dir");
        if (pathname.at(pathname.length() - 2) == '\\') throw SHELL_EXCEPTION_T("No filename, just dir");
        const size_t winSlash = pathname.rfind('\\');
        const size_t nixSlash = pathname.rfind('/');
        if (winSlash == std::string::npos) {
            if (nixSlash == std::string::npos) return pathname;
            return pathname.substr(nixSlash + 1);  //These are safe because I asserted it above and threw
        }
        else if (nixSlash == std::string::npos) {
            return pathname.substr(winSlash + 1);
        }
        else if (winSlash > nixSlash) {
            return pathname.substr(winSlash + 1);
        }
        return pathname.substr(nixSlash + 1);
    }


    /**
        Parses a pathname and returns just the file extension
        @param string pathname - the pathname to find the directory of
        @return string - just the filename
        @throw SHELL_EXCEPTION_T a non-path is given
    **/
    static std::string fileExtension(const std::string& pathname) {
        if (pathname.empty()) throw SHELL_EXCEPTION_T("Not a filename, is empty");
        if (pathname.at(pathname.length() - 2) == '/') throw SHELL_EXCEPTION_T("No filename, just dir");
        if (pathname.at(pathname.length() - 2) == '\\') throw SHELL_EXCEPTION_T("No filename, just dir");
        const size_t dot = pathname.rfind('.');
        if (dot == std::string::npos) return "";
        return pathname.substr(dot + 1);
    }

    /**
        Pulls out just the last part of the name of the directory
        @param string pathname - the pathname to find the directory of
        @return string - just the last part
    **/
    static std::string dirnamename(const std::string& pathname) {
        const std::string result = dirname(pathname);
        return filename(result.substr(0, result.length() - 1));

    }


    /**
        Gets the filesize in bytes of a file
        @param string filename - The filename to check
        @return uint_fast64_t
        @throw SHELL_EXCEPTION_T a non-path is given
    **/
    static uint_fast64_t filesize(const std::string& filename) {
        const std::uintmax_t siz = std::filesystem::file_size(filename);
        if (siz == static_cast<std::uintmax_t>(-1)) throw SHELL_EXCEPTION_T("filesize failed");
        return static_cast<uint_fast64_t>(siz);
    }

    /**
        Returns an UNIX timestamp of the last time the file was modified
        @param string filename - The filename to check
        @return uint_fast64_t
    **/
    static uint_fast64_t filemtime(const std::string& filename) {
        struct stat result;
        if (stat(filename.c_str(), &result) == 0) return static_cast<uint_fast64_t>(result.st_mtime);
        return 0;
    }


    /**
        Returns the name of the executable that is currently running
        @return string
    **/
    static std::string getSelfExecutableName() {
#if defined(__WIN32__) || defined(_MSVC_LANG)
        char path[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string results = path;
#elif defined(__APPLE__)
        int ret;
        pid_t pid;
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = { 0 };
        pid = getpid();
        ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
        if (ret <= 0) {
            std::cerr << "PID " << pid << ": proc_pidpath error: " << strerror(errno) << "(" << errno << ")" << std::endl;
            return "";
        }
        return std::string(pathbuf);
#else
        char result[2048];
        const ssize_t count = readlink("/proc/self/exe", result, 2048);
        std::string results = std::string(result, (count > 0) ? static_cast<unsigned int>(count) : 0);
#endif
#ifndef __APPLE__
        if (!Strings::EndsWith(results, ".exe")) return results + ".exe";
        return results;
#endif
    }

    /**
        Returns the path of the executable that is currently running from
        @return string
    **/
    inline static std::string getSelfExecutableDir() {
        return dirname(getSelfExecutableName());
    }


    /**
        Gets all files in a directory and does so recursively
        @param string dir - The directory to scrape
        @param int recursive = 0 - Whether or not to recurse
        @return vector<string>
    **/
    static std::vector<std::string> getFilesInDir(const std::string& dir, int recursive = 0) {
        std::vector<std::string> result;
        _getFilesInDir(dir, result, recursive, false);
        return result;
    }

    /**
        Gets all subdirectories in a directory and does so recursively
        @param string dir - The directory to scrape
        @param int recursive = 0 - Whether or not to recurse
        @return vector<string>
    **/
    static std::vector<std::string> getDirsInDir(const std::string& dir, int recursive = 0) {
        std::vector<std::string> result;
        _getFilesInDir(dir, result, recursive, true);
        return result;
    }


    /**
        Pauses execution for a specified number of milliseconds
        @param uint_fast32_t milli - the number of milliseconds
        @param bool sleepForZeros = true - Whether or not to do a thread-sleep if milli is 0
    **/
    inline static void wait(uint_fast32_t milli, bool sleepForZeros = true) {
        if (!sleepForZeros && milli == 0) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(milli));
    }


#ifndef __APPLE__
    /**
        Checks to see if a particular process is running
        @param string processName - the name of the process (or partial) to find
        @return bool
    **/
    static bool isProcessRunning(const std::string& processName) {
        const std::string cmi = "tasklist /fi \"IMAGENAME eq " + processName + "\"";
        const std::string result = cmd(cmi);
        if (result.find(processName) == std::string::npos) return false;
        if (result.find("PID") == std::string::npos) return false;
        if (result.find("No tasks are running") != std::string::npos) return false;
        return true;
    }


    /**
        Unceremoniously kills a process and waits for it to no longer appear in
        the tasklist.  Noop if process is not running
        @param string processName - the name of the process (or partial) to find
        @param uint_fast16_t timeOutInSeconds - the number of seconds to wait for it to die before returning false
        @return bool - Will return true if the process is still running by the end of the call
    **/
    static bool forceKillProcess(const std::string& processName, uint_fast16_t timeOutInSeconds = 10) {
        if (!isProcessRunning(processName)) return true;
        cmd("taskkill /IM " + processName + " /F");
        return waitForProcessToEnd(processName, timeOutInSeconds);
    }

    /**
        Waits for process to no longer appear in the tasklist.
        @param string processName - the name of the process (or partial) to find
        @param uint_fast16_t timeOutInSeconds - the number of seconds to wait for it to die before returning false
        @return bool - Will return true if the process is still running by the end of the call
    **/
    static bool waitForProcessToEnd(const std::string& processName, uint_fast16_t timeOutInSeconds = 10) {
        const uint_fast16_t timeOutInSecondsTimesFour = timeOutInSeconds * 4;
        for (uint_fast16_t i = 0; i < timeOutInSecondsTimesFour; ++i) {  //Check every quarter second
            if (!isProcessRunning(processName)) {
                wait(100);  //Try making this longer?
                return true;
            }
            wait(250);
        }
        return false;
    }

    /**
        Waits for process to appear in the tasklist.
        @param string processName - the name of the process (or partial) to find
        @param uint_fast16_t timeOutInSeconds - the number of seconds to wait for it before returning false
        @return bool - Will return true if the process is not running by the end of the call
    **/
    static bool waitForProcessToStart(const std::string& processName, uint_fast16_t timeOutInSeconds = 10) {
        const uint_fast16_t timeOutInSecondsTimesFour = timeOutInSeconds * 4;
        for (uint_fast16_t i = 0; i < timeOutInSecondsTimesFour; ++i) {  //Check every quarter second
            if (isProcessRunning(processName)) {
                wait(100);  //Try making this longer?
                return true;
            }
            wait(250);
        }
        return false;
    }
#endif

    /**
        Forces a path to have the windows style: drive letter and \ slashes
        @param string - A windows or *nix path
        @return string - a Windows path
    **/
    static std::string windowizePaths(std::string str) {
        if ((str.length() > 10) && Strings::StartsWith(str, "/cygdrive/")) {
            char replacer = str.at(10);
            if (replacer > 95) replacer -= 32;
            str = std::string(1, replacer) + ":" + str.substr(11);
        }
        str = Strings::ReplaceAll(str, "/", "\\");
        str = Strings::ReplaceAll(str, "\\\\", "\\");
        return str;
    }

    /**
        Forces a path to have the unix style
        @param string - A windows or *nix path
        @return string - a Unix path
    **/
    static std::string unixizePaths(std::string str) {
        if ((str.length() > 2) && (str.at(1) == ':') && (str.at(2) == '\\')) {
            char replacer = str.at(0);
            if (replacer < 97) replacer += 32;
            str = std::string("/cygdrive/") + replacer + "/" + str.substr(3);
        }
        str = Strings::ReplaceAll(str, "\\", "/");
        str = Strings::ReplaceAll(str, "//", "/");
        return str;
    }


    //Allows you to see what's coming out as it's coming out, and also kill the process
    typedef bool (*exec_append_t)(std::string& result, const char* str);
    static bool AppendToString(std::string& result, const char* buffer) { result += buffer; return true; }
    static exec_append_t& Appender() {
        static exec_append_t appender = Shell::AppendToString;
        return appender;
    }

    //Sends a command to whatever shell is running the application
    static std::string exec(const std::string& cmi, bool writeToStdOut, bool careAboutReturn/*, bool sanitizeStdOut*/) {
        if (writeToStdOut) {
            //std::cout << std::endl << ">" << (sanitizeStdOut ? Strings::sanitize(cmi) : cmi) << std::endl;
            //std::cout << Strings::now(true) << std::endl;
        }
#if defined(__WIN32__) || defined(_MSVC_LANG)
        FILE* pipe = _popen(cmi.c_str(), "r");
#else
        FILE* pipe = popen(cmi.c_str(), "r");
#endif
        if (!pipe) return("popen() failed!");
        std::string result;
        try {
            char buffer[128];
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                //if (writeToStdOut) std::cout << (sanitizeStdOut ? Strings::sanitize(buffer) : buffer) << std::flush;
                if (careAboutReturn && !Appender()(result, buffer)) {
                    break;
                }
                if (feof(pipe)) break;
                if (ferror(pipe)) break;
            }
        }
        catch (...) {
            result += "ERROR";
        }
#if defined(__WIN32__) || defined(_MSVC_LANG)
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        if (writeToStdOut) std::cout << std::endl;
        return result;
    }

    static bool isValidFilename(const std::string& filename) {
        if (filename.empty()) return false;
        for (auto& ch : filename) {
            if (ch < ' ' || ch > '~') return false;
            if (ch == '/') return false;
#if defined(__WIN32__) || defined(_MSVC_LANG)
            switch (ch) {
            case '\\':
            case ':':
            case '*':
            case '?':
            case '"':
            case '<':
            case '>':
            case '|':
                return false;
            default:
                break;
            }
#elif defined(__APPLE__)
            if (ch == ':') return false;
#endif
        }
#if defined(__WIN32__) || defined(_MSVC_LANG)
        if (filename.length() == 3) {
            if (filename == "CON") return false;
            if (filename == "PRN") return false;
            if (filename == "AUX") return false;
            if (filename == "NUL") return false;
        }
        else if (filename.length() == 4) {
            if (filename == "COM1") return false;
            if (filename == "COM2") return false;
            if (filename == "COM3") return false;
            if (filename == "COM4") return false;
            if (filename == "COM5") return false;
            if (filename == "COM6") return false;
            if (filename == "COM7") return false;
            if (filename == "COM8") return false;
            if (filename == "COM9") return false;
            if (filename == "LPT1") return false;
            if (filename == "LPT2") return false;
            if (filename == "LPT3") return false;
            if (filename == "LPT4") return false;
            if (filename == "LPT5") return false;
            if (filename == "LPT6") return false;
            if (filename == "LPT7") return false;
            if (filename == "LPT8") return false;
            if (filename == "LPT9") return false;

        }
#endif
        return true;
    }


    static std::string getFileNameWithHostAndTimestamp() {
        time_t rawtime;
        char buffer[256];
        time(&rawtime);
#if defined(__WIN32__) || defined(_MSVC_LANG)
        struct tm timeinfo;
        localtime_s(&timeinfo, &rawtime);
        std::size_t bytes = strftime(buffer, 256, "%Y_%m_%d__%H_%M_%S", &timeinfo);
#else
        struct tm* timeinfo = localtime(&rawtime);
        std::size_t bytes = strftime(buffer, 256, "%Y_%m_%d__%H_%M_%S", timeinfo);
#endif

        return hostname() + "__" + std::string(buffer, bytes);
    }

    static std::string getTimestamp() {
        time_t rawtime;
        char buffer[256];
        time(&rawtime);
#if defined(__WIN32__) || defined(_MSVC_LANG)
        struct tm timeinfo;
        localtime_s(&timeinfo, &rawtime);
        std::size_t bytes = strftime(buffer, 256, "%m/%d %H:%M:%S", &timeinfo);
#else
        struct tm* timeinfo = localtime(&rawtime);
        std::size_t bytes = strftime(buffer, 256, "%m/%d %H:%M:%S", timeinfo);
#endif

        return std::string(buffer, bytes);
    }

private:
    //All functions are static, never allow construction
    Shell();
    Shell(const Shell&);
    Shell(Shell&&);
    Shell& operator=(const Shell&);
    Shell& operator=(Shell&&);


    static void _getFilesInDir(const std::filesystem::path& path, std::vector<std::string>& vec, int recursive, bool dirsOnly) {
        if (recursive < 0) recursive = 0;
        try {
            for (const auto& dirEntry : std::filesystem::directory_iterator(path)) {
                bool isDir = false;
                try {
                    isDir = std::filesystem::is_directory(dirEntry);
                }
                catch (...) {}
                if (isDir) {
                    if (recursive) _getFilesInDir(dirEntry, vec, recursive - 1, dirsOnly);
                    if (dirsOnly) vec.push_back(dirEntry.path().string() + "/");
                }
                else if (!dirsOnly) {
                    vec.push_back(dirEntry.path().string());
                }
            }
        }
        catch (...) {}
    }

    static void tryDelete(const std::string& pfileName) {
        try {
            if (!std::filesystem::remove(pfileName)) {
                throw SHELL_EXCEPTION_T("Failed to remove " + pfileName + " without throwing");
            }
        }
        catch (std::filesystem::filesystem_error const& e) {
            throw SHELL_EXCEPTION_T(e.what());
        }
    }

    static void tryDeleteFilesInDir(const std::string& pDirectoryName) {
        try {
            const std::uintmax_t res = std::filesystem::remove_all(pDirectoryName);
            if (res == static_cast<std::uintmax_t>(-1)) {
                throw SHELL_EXCEPTION_T("Failed to remove files from " + pDirectoryName);
            }
        }
        catch (std::filesystem::filesystem_error const& e) {
            std::cout << "Exception: " << e.what() << std::endl;
            throw SHELL_EXCEPTION_T(e.what());
        }
    }
};

#endif // SHELL_H
