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
	void writeToFile(Model model);
	void loadFile(std::vector<Model*> scene);
	size_t sceneSize;
private:
	json data;

};


#endif