#ifndef STRINGS_H
#define STRINGS_H

#include <chrono>
#include <ctime>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>	//setprecision
#include <cstdlib>
#include <fstream>
#include <sstream>
#define hint_likely(x) x

class TimeOffset {
public:
    inline TimeOffset() {
        std::memset(&value, 0, sizeof value);
    }
    inline TimeOffset(struct tm * val) {
        std::memcpy(&value, val, sizeof value);
    }
    TimeOffset(int seconds, int minutes = 0, int hours = 0, int days = 0, int months = 0, int years = 0){
        value.tm_sec = seconds;
        value.tm_min = minutes;
        value.tm_hour = hours;
        value.tm_mday = days;
        value.tm_mon = months;
        value.tm_year = years;
    }

    inline void applyTo(TimeOffset& other) const {
        applyTo(other.value);
    }
    void applyTo(struct tm & other) const {
        other.tm_sec += value.tm_sec;
        while(other.tm_sec < 0) {
            other.tm_sec += 60;
            --other.tm_min;
        } 
        while(other.tm_sec > 59) {
            other.tm_sec -= 60;
            ++other.tm_min;
        }

        other.tm_min += value.tm_min;
        while (other.tm_min < 0) {
            other.tm_min += 60;
            --other.tm_hour;
        }
        while (other.tm_min > 59) {
            other.tm_min -= 60;
            ++other.tm_hour;
        }

        other.tm_hour += value.tm_hour;
        while (other.tm_hour < 0) {
            other.tm_hour += 24;
            --other.tm_mday;
        }
        while (other.tm_hour > 23) {
            other.tm_hour -= 24;
            ++other.tm_mday;
        }

        other.tm_mday += value.tm_mday;
        while (other.tm_mday < 0) {
            other.tm_mday += 30;
            --other.tm_mon;
        }
        while (other.tm_mday > 29) {
            other.tm_mday -= 30;
            ++other.tm_mon;
        }

        other.tm_mon += value.tm_mon;
        while (other.tm_mon < 0) {
            other.tm_mon += 12;
            --other.tm_year;
        }
        while (other.tm_mon > 11) {
            other.tm_mon -= 12;
            ++other.tm_year;
        }

        other.tm_year += value.tm_year;
        mktime(&other);
    }
    std::string toString() const {
        std::stringstream s;
        if (value.tm_mday || value.tm_mon || value.tm_year) {
            s << value.tm_mday << "/";
            s << value.tm_mon << "/";
            s << value.tm_year << " ";
        }
        s << value.tm_hour << ":";
        if (value.tm_min < 10 && value.tm_min > -10) s << "0";
        s << value.tm_min << ":";
        if (value.tm_sec < 10 && value.tm_sec > -10) s << "0";
        s << value.tm_sec;
        return s.str();
    }
    struct tm value;
};

class Strings {
public:


    /**
        This method counts the number of times a character appears
        in a string
        @param string - a string to be checked
        @param char - the char to be counted
        @return uint_fast64_t
    **/
    static uint_fast64_t countCharacter(const std::string & s, char c){
        uint_fast64_t result = 0;
        for(auto & cc : s) if (cc == c) ++result;
        return result;
    }


    /**
        This will take a numeric type and convert it to a string
        it's a template function that works with anything other than 8
        bit types
        @param <T> - the number to turn into a string
        @return string
    **/
    template <typename I> inline static std::string ToString(I f){
        std::stringstream s;
        s << f;
        return s.str();
    }

    /**
        This method is special for an 8 bit unsigned integer.  This is
        because it can be mistaken for a char.
        @param uint8_t - the number to stringify
        @return string
    **/
    inline static std::string ToString8(uint8_t f){
        return ToString<uint16_t>(static_cast<uint16_t>(f));
    }

    /**
        This method is special for an 8 bit integer.  This is because it
        can be mistaken for a char.
        @param uint8_t - the number to stringify
        @return string
    **/
    inline static std::string ToString8(int8_t f){
        return ToString<int16_t>(static_cast<int16_t>(f));
    }

    /**
        This version of ToString is used to turn floating point values into
        strings with a specific level of precision
        @param <F> - the number to stringify
        @param uint_fast8_t precision - the number of digits to print
        @return string
    **/
    template <typename I> static std::string ToString(I f, uint_fast8_t precision){
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss.setf(std::ios::fixed);
        ss << std::setprecision(static_cast<int>(precision)) << f;
        return ss.str();
    }

    /**
        Replaces all instances of a substring
        @param string - the string that we are replacing values in
        @param string - the substring to look for
        @param string - the substring to put in
        @return string
    **/

    static std::string ReplaceAll(std::string str, const std::string & from, const std::string & to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }


    /**
        This removes all whitespace and invisible from the front and back of
        a string
        @param string - the string to trim
        @return string
    **/
    static std::string trim(const std::string & str){
        if (hint_likely(str.length() == 0)) return str;
        const char * iterator1 = str.data();
        const char * iterator2 = str.data() + str.length();
        while(iterator1 != iterator2){
            const char tmp = *iterator1;
            if (hint_likely((tmp > ' ') && (tmp <= '~'))) goto findOtherEnd;  //yeah yeah yeah... c++ includes goto, I'm using it
            ++iterator1;
        }
        return "";

        findOtherEnd:
        while(iterator1 != iterator2){
            const char tmp = *iterator2;
            if (hint_likely((tmp > ' ') && (tmp <= '~'))) break;
            --iterator2;
        }
        return std::string(iterator1, static_cast<size_t>(iterator2 - iterator1 + 1));
    }


    /**
        This removes all whitespace and invisible from the front and back of
        a string
        @param string - the string to trim
        @return string
    **/
    static void trim_inPlace(std::string & str){
        if (hint_likely(str.length() == 0)) return;
        const char * iterator1 = str.data();
        const char * iterator2 = str.data() + str.length();
        const char * iterator1Orig = iterator1;
        while(iterator1 != iterator2){
            const char tmp = *iterator1;
            if (hint_likely((tmp > ' ') && (tmp <= '~'))) goto findOtherEnd;  //yeah yeah yeah... c++ includes goto, I'm using it
            ++iterator1;
        }
        str.clear();
        return;

        findOtherEnd:
        while(iterator1 != iterator2){
            const char tmp = *iterator2;
            if (hint_likely((tmp > ' ') && (tmp <= '~'))) break;
            --iterator2;
        }

        const size_t totalBytes = static_cast<size_t>(iterator2 - iterator1 + 1);
        if (totalBytes == str.length()) return;
        if (iterator1 != iterator1Orig) memmove(const_cast<char *>(str.data()), iterator1, totalBytes);
        str.erase(totalBytes);
    }




    /**
        This checks to see if a string starts with another substring
        @param string haystack - the string to search for
        @param string needle - the substring we want to check if it starts it
        @return bool
    **/
    static bool StartsWith(const std::string & haystack, const std::string & needle) {
        if (haystack.length() < needle.length()) return false;
        const char * it1 = haystack.data();
        const char * it2 = needle.data();
        const char * it3 = it2 + needle.length();
        while(it2 != it3){
            if (*it1 != *it2) return false;
            ++it1;
            ++it2;
        }
        return true;
    }


    /**
        This checks to see if a string ends with another substring
        @param string haystack - the string to search for
        @param string needle - the substring we want to check if it starts it
        @return bool
    **/
    static bool EndsWith(const std::string & haystack, const std::string & needle) {
        if (haystack.length() < needle.length()) return false;
        const char * it1 = haystack.data() + haystack.length() - 1;
        const char * it2 = needle.data() + needle.length() - 1;
        const char * it3 = needle.data() - 1;
        while(it2 != it3){
            if (*it1 != *it2) return false;
            --it1;
            --it2;
        }
        return true;
    }

    /**
        Checks to see if a string contains another substring
        @param string haystack - the string we are searching
        @param string/char needle - the substring we are searching for
        @return bool
    **/
    inline static bool Contains(const std::string & haystack, const std::string & needle){
        return haystack.find(needle) != std::string::npos;
    }
    inline static bool Contains(const std::string & haystack, char needle){
        return haystack.find(needle) != std::string::npos;
    }

    /**
        Creates a string of a specific length with random characters.
        @param uint_fast16_t len - the length of the string to create
        @return string - a string of len length
    **/
    static std::string RandomString(uint_fast16_t len){
        static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::string tmp_s;
        tmp_s.reserve(len);
        for (uint_fast16_t i = 0; i < len; ++i) {
            tmp_s += alphanum[static_cast<unsigned int>(rand()) % (sizeof(alphanum) - 1)];
        }
        return tmp_s;
    }

    /**
        Creates a string of a specific length with random lowercase characters.
        @param uint_fast16_t len - the length of the string to create
        @return string - a string of len length
    **/
    static std::string RandomStringLower(uint_fast16_t len){
        static const char alphanum[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string tmp_s;
        tmp_s.reserve(len);
        for (uint_fast16_t i = 0; i < len; ++i) {
            tmp_s += alphanum[static_cast<unsigned int>(rand()) % (sizeof(alphanum) - 1)];
        }
        return tmp_s;
    }

    /**
        Returns the now datetime in string format
        @param const std::string & format - The format to output the time as
        @return string
    **/
    static std::string CurrentYear(){
        time_t rawtime;
        char buffer[5];
        time (&rawtime);
        #if defined(__WIN32__) || defined(_MSVC_LANG)
            struct tm timeinfo;
            localtime_s(&timeinfo, &rawtime);
            strftime (buffer, 5, "%Y", &timeinfo);
        #else
            struct tm * timeinfo = localtime(&rawtime);
            strftime (buffer, 5, "%Y", timeinfo);
        #endif
        return std::string(buffer, 4);
    }

    static std::string now(bool includeDate, const TimeOffset& tOffset = TimeOffset()) {
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
        auto t = std::time(0);
        #if defined(__WIN32__) || defined(_MSVC_LANG)
            struct tm timeinfo;
            localtime_s(&timeinfo, &t);
            struct tm * now = &timeinfo;
        #else
            struct tm * now = std::localtime(&t);
        #endif
        tOffset.applyTo(*now);
        int bytes = 0;
        char buffer[sizeof "9999-12-31 29:59:59.9999"];
        if (includeDate){
            #if defined(__WIN32__) || defined(_MSVC_LANG)
                bytes = sprintf_s(
                    buffer,
                    sizeof(buffer),
            #else
                bytes = sprintf(
                    buffer,
            #endif
                "%04d-%02d-%02d %02d:%02d:%02d.%04d",
                now->tm_year + 1900,
                now->tm_mon + 1,
                now->tm_mday,
                now->tm_hour,
                now->tm_min,
                now->tm_sec,
                static_cast<int>(millis));
        } else {
            #if defined(__WIN32__) || defined(_MSVC_LANG)
                bytes = sprintf_s(
                    buffer,
                    sizeof(buffer),
            #else
                bytes = sprintf(
                    buffer,
            #endif
                "%02d:%02d:%02d.%04d",
                now->tm_hour,
                now->tm_min,
                now->tm_sec,
                static_cast<int>(millis));
        }

        return std::string(buffer, static_cast<std::size_t>(bytes));
    }

    static std::string now_noms(const TimeOffset& tOffset = TimeOffset(), const char * format = "%Y-%m-%d %H:%M:%S") {
        auto t = std::time(0);
        #if defined(__WIN32__) || defined(_MSVC_LANG)
            struct tm timeinfo;
            localtime_s(&timeinfo, &t);
            struct tm * now = &timeinfo;
        #else
            struct tm * now = std::localtime(&t);
        #endif
        tOffset.applyTo(*now);
        char buffer[256];
        size_t bytes = strftime(buffer, sizeof buffer, format, now);
        return std::string(buffer, bytes);
    }

    /**
        Splits a string up whenever it encounters a specific string
        @param const std::string & val - the string to pull apart
        @param const std::string & token - the substring that indicates a split
        @return std::vector<std::string>
    **/
    static std::vector<std::string> tokenize(const std::string & val, const std::string & token){
        std::vector<std::string> res;
        std::size_t start = 0;
        std::size_t pos = val.find(token);
        while(pos != std::string::npos){
            res.push_back(val.substr(start, pos - start));
            start = pos + token.length();
            pos = val.find(token, start);
        }
        if (start <= val.length()) res.push_back(val.substr(start));
        return res;
    }

    /**
        Splits a string up whenever it encounters a specific charactr
        @param const std::string & val - the string to pull apart
        @param char token - the substring that indicates a split
        @return std::vector<std::string>
    **/
    static std::vector<std::string> tokenize(const std::string & val, char token){
        std::vector<std::string> res;
        std::size_t start = 0;
        std::size_t pos = val.find(token);
        while(pos != std::string::npos){
            res.push_back(val.substr(start, pos - start));
            start = pos + 1;
            pos = val.find(token, start);
        }
        if (start <= val.length()) res.push_back(val.substr(start));
        return res;
    }


    /**
        @param std::string tim - A string containing y, d, h, m, s each indicating a duration, they must be in this order
        @return uint_fast64_t
    **/
    static uint_fast64_t strToSeconds(std::string tim){
        uint_fast64_t seconds = 0;
        std::size_t pos = tim.find('y');
        if (pos != std::string::npos){
            seconds = static_cast<uint_fast64_t>(atoi(trim(tim.substr(0, pos)).c_str()) * 31557600);
            tim.erase(0, pos + 1);
        }

        pos = tim.find('d');
        if (pos != std::string::npos){
            seconds += static_cast<uint_fast64_t>(atoi(trim(tim.substr(0, pos)).c_str()) * 86400);
            tim.erase(0, pos + 1);
        }

        pos = tim.find('h');
        if (pos != std::string::npos){
            seconds += static_cast<uint_fast64_t>(atoi(trim(tim.substr(0, pos)).c_str()) * 3600);
            tim.erase(0, pos + 1);
        }

        pos = tim.find('m');
        if (pos != std::string::npos){
            seconds += static_cast<uint_fast64_t>(atoi(trim(tim.substr(0, pos)).c_str()) * 60);
            tim.erase(0, pos + 1);
        }
        pos = tim.find('s');
        if (pos != std::string::npos){
            seconds += static_cast<uint_fast64_t>(atoi(trim(tim.substr(0, pos)).c_str()));
        }
        return seconds;
    }

    static std::string c_escape(const std::string & s){
        std::string result;
        for(char c : s){
            switch(c){
                case '\"':
                    result += "\\\"";
                    break;
                default:
                    if(isprint(static_cast<unsigned char>(c))){
                        result += c;
                    } else {
                        std::stringstream stream;
                        stream << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(c));
                        const std::string code = stream.str();
                        result += std::string("\\x")+(code.size()<2?"0":"")+code;
                    }
                    break;
            }
        }
        return result;
    }

    static void file_put_contents(const std::string & input, const std::string & filename){
        std::ofstream out(filename);
        out << input;
    }

    static std::string file_get_contents(const std::string& filename) {
        std::ifstream out(filename);
        std::stringstream buffer;
        buffer << out.rdbuf();
        return buffer.str();
    }
private:
    Strings();
    Strings(const Strings&);
    Strings(Strings&&);
    Strings& operator=(const Strings&);
    Strings& operator=(Strings&&);
};

#endif // STRINGS_H
