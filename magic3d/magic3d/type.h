/******************************************************************************
 @Copyright    Copyright (C) 2008 - 2015 by MagicTech.

 @Platform     ANSI compatible
******************************************************************************/
/*
Magic3D Engine
Copyright (c) 2008-2015
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

#ifndef MAGIC3D_TYPE_H
#define MAGIC3D_TYPE_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <tinyxml2.h>

#include <btBulletDynamicsCommon.h>

#if defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#define OV_EXCLUDE_STATIC_CALLBACKS

#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>

#ifdef MAGIC3D_MEMORY
#include <mmgr.h>
#endif

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
typedef unsigned int   uint;
#if defined(MAGIC3D_OES1)
typedef unsigned short vindex;
#elif defined(MAGIC3D_OES2)
typedef unsigned short vindex;
#else
typedef unsigned int vindex;
#endif

#define M3D_MULTI_TEXTURE_COUNT 2
#define M3D_MULTI_SKIN_COUNT 4

#if defined(MAGIC3D_OES1) || defined(MAGIC3D_OES2) || defined(MAGIC3D_OES3)
#define MAGIC3D_OES
#endif

#define MAGIC3D_PASSWORD "magic3d"

#endif // MAGIC3D_TYPE_H
