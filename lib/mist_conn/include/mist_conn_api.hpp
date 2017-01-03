/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#ifdef MIST_CONN_SHARED
#  if defined(_WIN32)||defined(_WIN64)||defined(__CYGWIN__)
#    ifdef __GNUC__
#      ifdef BUILDING_MIST_CONN
#        define MistConnApi __attribute__ ((dllexport))
#      else
#        define MistConnApi __attribute__ ((dllimport))
#      endif
#    else
#      ifdef BUILDING_MIST_CONN
#        define MistConnApi __declspec(dllexport)
#      else
#        define MistConnApi __declspec(dllimport)
#      endif
#    endif
#  else
#    define MistConnApi
#  endif
#else
#  define MistConnApi
#endif
