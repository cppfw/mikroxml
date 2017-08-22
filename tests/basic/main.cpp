#include "../../src/mikroxml/mikroxml.hpp"

#include <array>
#include <sstream>

#include <utki/debug.hpp>
#include <papki/FSFile.hpp>

class Parser : public mikroxml::Parser{
public:
	std::stringstream ss;
	
	void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) override{
		ss << " " << name << "=\"" << value << "\"";
	}
	
	void onElementEnd(const std::string& name) override{
		if(name.length() == 0){
			ss << "/>";
		}else{
			ss << "</" << name << ">";
		}
	}

	void onAttributesEnd(bool isEmptyElement) override{
		if(!isEmptyElement){
			ss << ">";
		}
	}

	void onElementStart(const std::string& name) override{
		ss << '<' << name;
	}
	
	void onContentParsed(const utki::Buf<char> str) override{
		ss << str;
	}
};

int main(int argc, char** argv){
	{
		auto in = "<element attribute=\"attributeValue\">content</element>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
	}
}
