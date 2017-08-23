#include "../../src/mikroxml/mikroxml.hpp"

#include <array>
#include <sstream>

#include <utki/debug.hpp>


class Parser : public mikroxml::Parser{
public:
	std::stringstream ss;
	
	void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) override{
		ss << " " << name << "='" << value << "'";
	}
	
	void onElementEnd(const utki::Buf<char> name) override{
		if(name.size() == 0){
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

	void onElementStart(const utki::Buf<char> name) override{
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
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attributeValue'>content</element>", " str = " << parser.ss.str())
	}
	{
		auto in = "<element attribute='attribute&amp;&lt;&gt;&quot;&apos;Value'>content&amp;&lt;&gt;&quot;&apos;</element>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attribute&<>\"'Value'>content&<>\"'</element>", " str = " << parser.ss.str())
	}
	{
		auto in = "<element attribute='attribute&#bf5;Value'>content&#41a;</element>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attribute௵Value'>contentК</element>", " str = " << parser.ss.str())
	}
	{
		auto in = "<element attribute='attribute&#bf5;Value'/>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attribute௵Value'/>", " str = " << parser.ss.str())
	}
}
