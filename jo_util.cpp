#include <string.h>
#include <math.h>
#include <iostream>
#include "FireLog.h"
#include "jansson.h"
#include "jo_util.hpp"

using namespace std;

#define START_DELIM "{{"
#define END_DELIM "}}"
#define DEFAULT_SEP "||"
#define SINGLE_SEP "|"

namespace gfilter {

ArgMap emptyMap;

json_t *json_float(float value) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%g", value);
  return json_string(buf);
}

string jo_parse(const char * pSource, const char * defaultValue, ArgMap &argMap) {
  string result(pSource);
  size_t startDelim = 0;
  int substitutions = 0; 
  while ((startDelim=result.find(START_DELIM,startDelim)) != string::npos) {
    size_t defaultSep = result.find(DEFAULT_SEP,startDelim);
    if (defaultSep == string::npos) {
      size_t singleSep = result.find(SINGLE_SEP,startDelim);
      if (singleSep != string::npos) {
        LOGWARN("jo_parse() expected " DEFAULT_SEP " before default parameter value");
        result.replace(singleSep, sizeof(SINGLE_SEP)-1, DEFAULT_SEP);
        defaultSep = singleSep;
      }
    }
    size_t endDelim = result.find(END_DELIM,startDelim);
    if (endDelim == string::npos) {
      LOGERROR1("jo_parse(): Invalid variable specification: '%s'", pSource);
      break;
    } 
    size_t varEnd = endDelim + sizeof(END_DELIM)-1;
    substitutions++;
    size_t nameStart = startDelim + sizeof(START_DELIM)-1;
    size_t nameEnd = defaultSep == string::npos ? endDelim : defaultSep;
    string name = result.substr(nameStart,nameEnd-nameStart);
    const char * pRep = argMap[name.c_str()];
    if (pRep) { 
      result.replace(startDelim, varEnd-startDelim, pRep);
    } else { // scan for template default
      if (defaultSep == string::npos) {
	result.replace(startDelim, varEnd-startDelim, defaultValue);
      } else {
	size_t defaultStart = defaultSep + sizeof(DEFAULT_SEP)-1;
	string rep = result.substr(defaultStart, endDelim-defaultStart);
	result.replace(startDelim, varEnd-startDelim, rep);
      }
    }
    startDelim = varEnd;
  }

  if (substitutions) {
    LOGTRACE2("jo_parse(%s) => %s",  pSource, result.c_str());
  }
  return result;
}

string jo_object_dump(json_t *pObj, ArgMap &argMap) {
  json_t *pValue;
  const char *key;
  string result;

  json_object_foreach(pObj, key, pValue) {
    if (!result.empty()) {
      result = result + " ";
    }
    if (key) {
      result = result + key + ":";
    } else {
      result = result + "null" + ":";
    }
    if (json_is_string(pValue)) {
      result = result + jo_parse(json_string_value(pValue), "", argMap);
    } else if (pValue) {
      char *valueStr = json_dumps(pValue, JSON_PRESERVE_ORDER|JSON_COMPACT);
      if (valueStr) {
	result = result + valueStr;
	free(valueStr);
      } else {
        result = result + "N/A";
      }
    }
  }

  return result;
}

json_t *jo_object(const json_t *pObj, const char *key, ArgMap &argMap) {
  json_t *pVal = json_object_get(pObj, key);

  return pVal;
}

bool jo_bool(const json_t *pObj, const char *key, bool defaultValue, ArgMap &argMap) {
  json_t *pValue = json_object_get(pObj, key);
  bool result = defaultValue;
  if (json_is_boolean(pValue)) {
    result = json_is_true(pValue);
  } else if (json_is_string(pValue)) {
    string valStr = jo_parse(json_string_value(pValue), "", argMap);
    if (!valStr.empty()) {
      result = valStr.compare("true") == 0;
    } 
  }
  LOGTRACE3("jo_bool(key:%s default:%d) -> %d", key, defaultValue, result);
  return result;
}

int jo_int(const json_t *pObj, const char *key, int defaultValue, ArgMap &argMap) {
  json_t *pValue = json_object_get(pObj, key);
  int result = defaultValue;
  if (pValue == NULL) {
    // default
  } else if (json_is_integer(pValue)) {
    result = (int)json_integer_value(pValue);
  } else if (json_is_number(pValue)) {
    result = (int)(0.5+json_number_value(pValue));
  } else if (json_is_string(pValue)) {
    string valStr = jo_parse(json_string_value(pValue), "", argMap);
    if (!valStr.empty()) {
      result = atoi(valStr.c_str());
    }
  } else {
    LOGERROR1("jo_int() expected integer value for %s", key);
  }
  LOGTRACE3("jo_int(key:%s default:%d) -> %d", key, defaultValue, result);
  return result;
}

double jo_double(const json_t *pObj, const char *key, double defaultValue, ArgMap &argMap) {
  json_t *pValue = json_object_get(pObj, key);
  double result = defaultValue;
  if (pValue == NULL) {
    // default
  } else if (json_is_number(pValue)) {
    result = json_number_value(pValue);
  } else if (json_is_string(pValue)) {
    string valStr = jo_parse(json_string_value(pValue), "", argMap);
    if (!valStr.empty()) {
      result = atof(valStr.c_str());
    }
  } else {
    LOGERROR1("jo_double() expected numeric value for %s", key);
  }

  LOGTRACE3("jo_double(key:%s default:%f) -> %f", key, defaultValue, result);
  return result;
}

string jo_string(const json_t *pObj, const char *key, const char *defaultValue, ArgMap &argMap) {
  json_t *pValue = json_object_get(pObj, key);
  const char * result = json_is_string(pValue) ? json_string_value(pValue) : defaultValue;
  LOGTRACE3("jo_string(key:%s default:%s) -> %s", key, defaultValue, result);

  return jo_parse(result, defaultValue, argMap);
}

template<typename T>
const vector<T> jo_vector(const json_t *pObj, const char *key, const vector<T> &defaultValue, ArgMap &argMap) {
  vector<T> result;
  json_t *pVector = json_object_get(pObj, key);
  json_t *pParsedObj = NULL;
  if (pVector) {
    if (json_is_string(pVector)) {
      string vectorStr = jo_parse(json_string_value(pVector), "", argMap);
      if (!vectorStr.empty()) {
	json_error_t jerr;
	pParsedObj = json_loads(vectorStr.c_str(), JSON_DECODE_ANY, &jerr);
	if (json_is_array(pParsedObj)) {
	  pVector = pParsedObj;
	} else if (json_is_number(pParsedObj)) {
	  double value = json_number_value(pParsedObj);
	  for (int i=0; i < defaultValue.size(); i++) {
	    result.push_back(value);
	  }
	} else {
	  LOGERROR1("Could not parse JSON string as vector: %s", vectorStr.c_str());
	}
      }
    } else if (json_is_number(pVector)) {
      result.push_back(json_number_value(pVector));
    } else if (json_is_array(pVector)) {
      // standard use case
    } else { 
      LOGERROR1("expected JSON array for %s", key);
    } 
    if (json_is_array(pVector)) {
      int index;
      json_t *pValue;
      json_array_foreach(pVector, index, pValue) {
        if (json_is_number(pValue)) {
	  result.push_back((T)json_number_value(pValue));
	} else {
	  LOGERROR("Expected vector of numbers");
	}
      }
    }
  }
  if (pParsedObj) {
    json_decref(pParsedObj);
  }
  if (result.size() == 0) {
    result = defaultValue;
  }
  if (logLevel >= FIRELOG_TRACE) {
    char buf[512];
    snprintf(buf, sizeof(buf), "jo_vector(key:%s default:[", key);
    for (int i = 0; i < defaultValue.size(); i++) {
      snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), i ? ",%g" : "%g",  (float) defaultValue[i]);
    }
    snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "]) -> [");
    for (int i = 0; i < result.size(); i++) {
      snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), i ? ",%g" : "%g",  (float) result[i]);
    }
    snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), "]");
    LOGTRACE(buf);
  }
  return result;
}

const vector<float> jo_vectorf(const json_t *pObj, const char *key, const vector<float> &defaultValue, ArgMap &argMap) {
  return jo_vector<float>(pObj, key, defaultValue, argMap);
}

const vector<int> jo_vectori(const json_t *pObj, const char *key, const vector<int> &defaultValue, ArgMap &argMap) {
  return jo_vector<int>(pObj, key, defaultValue, argMap);
}

} // namespace gfilter

