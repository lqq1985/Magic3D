/******************************************************************************
 @Copyright    Copyright (C) 2008 - 2016 by MagicTech.

 @Platform     ANSI compatible
******************************************************************************/
/*
Magic3D Engine
Copyright (c) 2008-2016
Thiago C. Moraes
http://www.magictech.com.br

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
   If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef MAGIC3D_LOG_H
#define MAGIC3D_LOG_H

#include <stdio.h>
#include <stdarg.h>

namespace Magic3D
{

enum LOG
{
    eLOG_SUCCESS = 0,
    eLOG_FAILURE,
    eLOG_PLAINTEXT,
    eLOG_RENDERER
};

class LogCallBack
{
private:
    bool simple;
public:
    LogCallBack(bool simple)
    {
        this->simple = simple;
    }
    virtual ~LogCallBack()
    {

    }
    bool isSimple()
    {
        return simple;
    }

    virtual void log(LOG type, const char* text) = 0;
};

class Log {
private:
    static char* logFile;
    static Log* instance;

    LogCallBack* callback;

    Log();
    ~Log();

    void write(LOG type, const char* text);

    const char* getColor(LOG type);

public:
    static void start();
    static void finish();
    static Log* getInstance();
    static void setLogFile(const char* fileName);

    static bool clear();
    static void logFormat(LOG type, const char* text, ...);
    static void log(LOG type, const char* text);
    static void debug(const char* file, int line);

    static void logMatrix(LOG type, float* matrix);

    static void setCallBack(LogCallBack* callback);
};

}
#endif
