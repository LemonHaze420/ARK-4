#include <cstdio>
#include <sstream>
#include <dirent.h>
#include "browser_entries.h"
#include "eboot.h"
#include "iso.h"


int fileTypeByExtension(string path){
    if (Entry::isTXT(path.c_str())){
        return FILE_TXT;
    }
    else if (Entry::isZip(path.c_str()) || Entry::isRar(path.c_str())){
        return FILE_ZIP;
    }
    else if (Entry::isPRX(path.c_str())){
        return FILE_PRX;
    }
    else if (Eboot::isEboot(path.c_str())){
        return FILE_PBP;
    }
    else if (Iso::isISO(path.c_str())){
        return FILE_ISO;
    }
    else if (Entry::isIMG(path.c_str())){
        return FILE_PICTURE;
    }
    else if (Entry::isMusic(path.c_str())){
        return FILE_MUSIC;
    }
    return FILE_BIN;
}


BrowserFile::BrowserFile(){
}

BrowserFile::BrowserFile(string path){
    this->path = path;
    size_t lastSlash = path.rfind("/", string::npos);
    this->name = path.substr(lastSlash+1, string::npos);
    this->selected = false;
    this->calcSize();
    this->filetype = fileTypeByExtension(path);
}

BrowserFile::BrowserFile(BrowserFile* orig){
    this->path = orig->path;
    this->selected = false;
    this->fileSize = orig->fileSize;
    this->filetype = orig->filetype;
}

BrowserFile::~BrowserFile(){
}

unsigned BrowserFile::getFileSize(){
    int fd = sceIoOpen(path.c_str(), PSP_O_RDONLY, 0777);
    unsigned size = sceIoLseek(fd, 0, SEEK_END);
    sceIoClose(fd);
    return size;
}

void BrowserFile::calcSize(){
    // Calculate the size (in Bytes, KB, MB or GB) of a BrowserFile, if it's a BrowserFolder, simply return its type
    unsigned size = this->getFileSize();

    ostringstream txt;

    if (size < 1024)
        txt<<size<<" Bytes";
    else if (1024 < size && size < 1048576)
        txt<<float(size)/1024.f<<" KB";
    else if (1048576 < size && size < 1073741824)
        txt<<float(size)/1048576.f<<" MB";
    else
        txt<<float(size)/1073741824.f<<" GB";
    this->fileSize = txt.str();
}

bool BrowserFile::isSelected(){
    return this->selected;
}

void BrowserFile::changeSelection(){
    this->selected = !this->selected;
}

string BrowserFile::getPath(){
    return this->path;
}

string BrowserFile::getName(){
    return this->name;
}

string BrowserFile::getSize(){
    return this->fileSize;
}

char* BrowserFile::getType(){
    return "File";
}

char* BrowserFile::getSubtype(){
    return getType();
}

void BrowserFile::loadIcon(){
}

void BrowserFile::getTempData1(){
}

void BrowserFile::getTempData2(){
}

void BrowserFile::doExecute(){
}

BrowserFolder::BrowserFolder(string path){
    this->path = path;
    size_t lastSlash = path.rfind("/", path.length()-2);
    this->name = path.substr(lastSlash+1, string::npos);
    this->selected = false;
    this->fileSize = "Folder";
    this->filetype = FOLDER;
}

BrowserFolder::BrowserFolder(BrowserFolder* orig){
    this->path = orig->path;
    this->name = orig->name;
    this->selected = false;
    this->fileSize = "Folder";
    this->filetype = FOLDER;
}

BrowserFolder::BrowserFolder(string parent, string name){
    this->path = parent;
    this->name = name;
    this->selected = false;
    this->fileSize = "Folder";
    this->filetype = FOLDER;
}

BrowserFolder::~BrowserFolder(){
}

char* BrowserFolder::getType(){
    return "FOLDER";
}

char* BrowserFolder::getSubtype(){
    return getType();
}
