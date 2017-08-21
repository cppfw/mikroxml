#include "../../src/mikroxml/mikroxml.hpp"

#include <array>
#include <sstream>

#include <utki/debug.hpp>
#include <papki/FSFile.hpp>

class Parser : public mikroxml::Parser{
public:
	std::stringstream ss;
	
	void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) override{
		
	}
	
	void onElementEnd() override{
		ss << '>';
	}

	void onElementStart(const utki::Buf<char> name) override{
		ss << '<' << name;
	}
	
	void onContentParsed(const utki::Buf<char> str) override{
		
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
