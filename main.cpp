#include <iostream>
#include <cstdint>
#include <string>
#include "base64.h"
#include "md5.h"
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include "tea.h"
#include <thread>
#include <mutex>

#define MAX(x,y) (((x)>(y)) ? (x) : (y))
#define MIN(x,y) (((x)<(y)) ? (x) : (y))

std::mutex mu;

std::string getFileName(const std::string& s, std::string* pathwithoutname) {

    char sep = '/';

#ifdef _WIN32
    sep = '\\';
#endif

    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        pathwithoutname->clear();
        pathwithoutname->append(s.substr(0, i+1));
        return s.substr(i+1, s.length() - i);
    }

    return s;
}

void stringToUpper(std::string* s) {
    for (size_t i=0; i<s->length(); ++i) {
        char& c = (*s)[i];
        if (c>='a' && c<='z') {
            c = c-'a'+'A';
        }
    }
}

bool encodeFile(const std::string fpath, const std::string key) {
    //file reader
    struct stat results;
    if (stat(fpath.c_str(), &results) != 0) {
        mu.lock(); std::cout<< "[Thread: " << std::this_thread::get_id() << "][Error] File not found"<<std::endl; mu.unlock();
        return false;
    }
    std::ifstream ifile(fpath, std::ios::in | std::ios::binary);
    char* fbuffer = new char[results.st_size];
    ifile.read(fbuffer, results.st_size);
    ifile.close();
    unsigned char* fb64buffer = new unsigned char[results.st_size];
    memcpy(fb64buffer, fbuffer, (size_t)results.st_size);
    std::string fb64 = base64_encode(fb64buffer, results.st_size);
    delete[] fb64buffer;
    delete[] fbuffer;
    fbuffer = new char[fb64.size()];
    memcpy(fbuffer, fb64.c_str(), (size_t)fb64.size());
    mu.lock(); std::cout << "[Thread: " << std::this_thread::get_id() << "][Output] The file has been read" << std::endl; mu.unlock();

    //init variables
    unsigned int k[4];
    unsigned int kbuffer[4];
    memset(k, 0, sizeof(k));
    memset(kbuffer, 0, sizeof(kbuffer));

    memcpy(kbuffer, key.c_str(), MIN(key.length(), 16));
    for (int i=0; i<4; i++) k[i] = kbuffer[i];

    size_t vbuffer_size = (size_t)fb64.size();
    if (vbuffer_size%4 > 0) vbuffer_size += 4 - (vbuffer_size%4);
    unsigned char* vbuffer = new unsigned char [vbuffer_size];
    memset(vbuffer, 0, vbuffer_size);
    memcpy(vbuffer, fbuffer, (size_t)fb64.size());

    //преобразования
    size_t obuffer_size = vbuffer_size+4;
    char* obuffer = new char[obuffer_size];
    unsigned int v[2];
    memset(v, 0, sizeof(v));
    for (int i=0; i<fb64.size(); i+=4) {
        v[0] = *(unsigned int*)&vbuffer[i];
        encrypt(&v[0], &k[0]);
        memcpy(&obuffer[i], &v[0], 4);
        memcpy(vbuffer, fbuffer, (size_t)fb64.size());
    }
    memcpy(&obuffer[obuffer_size-4], &v[1], 4);

    //file writer
    std::string filefolder;
    std::string filename = key + ":" + getFileName(fpath, &filefolder);     //новое имя файла
    mu.lock(); std::cout << "[Thread: " << std::this_thread::get_id() << "][Output] File non-hased name: " << filename << std::endl; mu.unlock();

    filename = md5(filename);
    mu.lock(); std::cout << "[Thread: " << std::this_thread::get_id() << "][Output] File hased name: " << filename << std::endl; mu.unlock();
    stringToUpper(&filename);
    std::string fullpath = filefolder + filename;
    std::ofstream ofile(fullpath.c_str(), std::ios::out | std::ios::binary);
    if (ofile.good()) {
        ofile.clear();
    }
    ofile.write(obuffer, obuffer_size);
    ofile.close();
    mu.lock(); std::cout << "[Thread: " << std::this_thread::get_id() << "][Output] File has been encoded into: " << fullpath << std::endl; mu.unlock();

    delete[] fbuffer;
    delete[] vbuffer;
    delete[] obuffer;
    return true;
}

int main(const int argc, const char* argv[]) {
    //path
    std::string fpath;
    if (argc>1) {
        fpath.assign(argv[1]);
    } else {
        mu.lock(); std::cout << "[Output] You can open file(s) with this program" << std::endl; mu.unlock();
        mu.lock(); std::cout << "[Output] Or drag'n'drop on it" << std::endl; mu.unlock();
        mu.lock(); std::cout << "[Input] Enter filename(without spaces): "; mu.unlock();
        std::cin >> fpath;
    }

    //key reader
    std::string key;
    std::cout << "[Input] Enter key(max 16 symbols): ";
    std::cin >> key;
    std::cout << std::endl;

    std::thread** threads;
    if (argc>1) {
        threads = new std::thread*[argc-1];
        for (int i=0; i<argc-1; i++) {
            std::string tfpath(argv[i+1]);
            std::thread* t  = new std::thread(encodeFile, tfpath, key);
            threads[i] = t;
        }
        for (int i=0; i<argc-1; i++) {
            threads[i]->join();
        }
    } else {
        std::thread t(encodeFile, fpath, key);
        t.join();
    }

    for (size_t i=0; i<argc-1; i++) {
        delete threads[i];
    }
    delete[] threads;
    char _C; std::cin >> _C;            //breakpoint
    return 0;
}