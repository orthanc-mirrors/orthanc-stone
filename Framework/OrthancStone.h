#pragma once

/**
 * Besides the "pragma once" above that only protects this file,
 * define a macro to prevent including different versions of
 * "OrthancStone.h"
 **/
#ifndef __ORTHANC_STONE_H
#define __ORTHANC_STONE_H

#include <OrthancFramework.h>

#if ORTHANC_ENABLE_OPENGL == 1
#  define GL_GLEXT_PROTOTYPES 1
#endif


#endif /* __ORTHANC_STONE_H */
