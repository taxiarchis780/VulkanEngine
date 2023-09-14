#ifndef __JSON_PARSER_CLASS__
#define __JSON_PARSER_CLASS__
#include <json.hpp>
#include <fstream>
#include <Model.h>
#include <string>
using json = nlohmann::json;


class JsonParser
{
public:	
	JsonParser(std::string JSON_PATH);
	std::string JSON_PATH;
	
	size_t sceneSize;
private:
	json data;

};


#endif