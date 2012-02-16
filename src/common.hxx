/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.16
 * @brief Common header file to include system libraries, etc
 */
#ifndef WIN32
#include "abpch.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#else
#error common.hxx not yet Windows-ready
#endif
