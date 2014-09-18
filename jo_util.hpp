#ifndef JO_UTIL_HPP
#define JO_UTIL_HPP

#include <vector>
#include <errno.h>
#include "jansson.h"
#include "gfilter.hpp"
#include "winjunk.hpp"

using namespace std;

namespace gfilter {
  CLASS_DECLSPEC typedef map<string,const char *> ArgMap;
  CLASS_DECLSPEC extern ArgMap emptyMap;

  CLASS_DECLSPEC const vector<float> jo_vectorf(const json_t *pObj, const char *key, const vector<float> &defaultValue, ArgMap &argMap) ;
  CLASS_DECLSPEC const vector<int> jo_vectori(const json_t *pObj, const char *key, const vector<int> &defaultValue, ArgMap &argMap) ;
  CLASS_DECLSPEC bool jo_bool(const json_t *pObj, const char *key, bool defaultValue=0, ArgMap &argMap=emptyMap) ;

  CLASS_DECLSPEC int jo_int(const json_t *pObj, const char *key, int defaultValue=0, ArgMap &argMap=emptyMap) ;

  CLASS_DECLSPEC double jo_double(const json_t *pObj, const char *key, double defaultValue=0, ArgMap &argMap=emptyMap) ;

  CLASS_DECLSPEC inline float jo_float(const json_t *pObj, const char *key, double defaultValue=0, ArgMap &argMap=emptyMap) {
      return (float) jo_double(pObj, key, defaultValue, argMap);
  }

  CLASS_DECLSPEC string jo_string(const json_t *pObj, const char *key, const char *defaultValue = "", ArgMap &argMap=emptyMap) ;

  CLASS_DECLSPEC string jo_parse(const char * pSource, const char * defaultValue = "", ArgMap &argMap=emptyMap);

  CLASS_DECLSPEC json_t *jo_object(const json_t *pStage, const char *key, ArgMap &argMap=emptyMap) ;
  
  CLASS_DECLSPEC string jo_object_dump(json_t *pObj, ArgMap &argMap) ; 

  CLASS_DECLSPEC json_t *json_float(float value);

} // namespace firesight

#endif
