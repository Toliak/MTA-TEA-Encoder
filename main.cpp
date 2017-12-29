#include <iostream>
#include <cstdint>
#include <string>
#include "base64.h"
#include "md5.h"
#include <cstring>
#include <fstream>
#include <sys/stat.h>

#define MAX(x,y) (((x)>(y)) ? (x) : (y))
#define MIN(x,y) (((x)<(y)) ? (x) : (y))

void encrypt(unsigned int* v, const unsigned int* k) {
    unsigned int v0=v[0], v1=v[1], i, sum=0;
    unsigned int delta=0x9E3779B9;
    for(i=0; i<32; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum>>11) & 3]);
    }
    v[0]=v0; v[1]=v1;
}

void decrypt(unsigned int* v, const unsigned int* k) {
    unsigned int v0=v[0], v1=v[1], i, sum=0xC6EF3720;
    unsigned int delta=0x9E3779B9;
    for(i=0; i<32; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum>>11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
    v[0]=v0; v[1]=v1;
}

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

int main(int argc, char* argv[]) {
    std::string key;
    char _C;            //для прерывания в конце

    //путь к файлу
    std::string fpath;
    if (argc>1) {
        fpath = argv[1];
    } else {
        std::cout << "[Output] You can open file with this program" << std::endl;
        std::cout << "[Output] Or drag'n'drop on it" << std::endl;
        std::cout << "[Input] Enter filename(without spaces): ";
        std::cin >> fpath;
    }
    std::cout << "[Output] argc=="<<argc<<std::endl;
    //читаем файл
    struct stat results;
    if (stat(fpath.c_str(), &results) != 0) {
        std::cout<<"[Error] File not found"<<std::endl;
        std::cin >> _C;
        return 1;
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
    std::cout << "[Output] The file has been read" << std::endl;

    //читаем ключ
    std::cout << "[Input] Enter key(max 16 symbols): ";
    std::cin >> key;
    std::cout << std::endl;

    //подготовка
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
    std::string newvalue;
    size_t obuffer_size = vbuffer_size+4;
    char* obuffer = new char[obuffer_size];
    unsigned int v[2];
    memset(v, 0, sizeof(v));
    for (int i=0; i<fb64.size(); i+=4) {
        v[0] = *(unsigned int*)&vbuffer[i];
        encrypt(&v[0], &k[0]);
        newvalue.append((char*)&v[0], 4);
        memcpy(&obuffer[i], &v[0], 4);
        memcpy(vbuffer, fbuffer, (size_t)fb64.size());
    }
    newvalue.append((char*)&v[1], 4);
    memcpy(&obuffer[obuffer_size-4], &v[1], 4);
    std::string nnvalue(obuffer, obuffer_size);     //для сверки
    std::cout << "[Output] MD5(test): " << md5(newvalue) << std::endl;
    std::cout << "[Output] MD5: " << md5(nnvalue) << std::endl;

    unsigned char* b64_c = new unsigned char[newvalue.size()];
    memset(b64_c, 0, newvalue.size());
    memcpy(b64_c, newvalue.c_str(), newvalue.size());
    std::string b64 = base64_encode(b64_c, newvalue.size()); //было (не используется)
    delete[] b64_c;
    std::string b64_cut(b64, 0, MIN(b64.length(), 64));
    std::cout << "[Output] Base64 (first 64 characters): " << b64_cut << std::endl;

    //запись в файл
    std::string filefolder;
    std::string filename = key + ":" + getFileName(fpath, &filefolder);     //новое имя файла
    filename = md5(filename);
    stringToUpper(&filename);
    std::string fullpath = filefolder + filename;
    std::ofstream ofile(fullpath.c_str(), std::ios::out | std::ios::binary);
    if (ofile.good()) {
        ofile.clear();      //очистка файла, если он существует
    }
    ofile.write(obuffer, obuffer_size);
    ofile.close();
    std::cout << "[Output] File has been created: " << fullpath << std::endl;

    delete[] fbuffer;
    delete[] vbuffer;
    delete[] obuffer;
    std::cin >> _C;
    return 0;
}