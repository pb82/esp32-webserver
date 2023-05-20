#ifndef STORAGE_H
#define STORAGE_H

#include <stdio.h>

struct File
{
    ~File()
    {
        delete contents;
    }

    char *contents;
    long size;
};

class Storage
{
public:
    Storage();
    ~Storage();

    bool read_index(File &target);

private:
};

#endif