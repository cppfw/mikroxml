#include "../../src/mikroxml/mikroxml.hpp"

#include <array>
#include <sstream>

#include <utki/debug.hpp>


class Parser : public mikroxml::parser{
public:
	std::stringstream ss;
	
	void on_attribute_parsed(utki::span<const char> name, utki::span<const char> value) override{
		ss << " " << name << "='" << value << "'";
	}
	
	void on_element_end(utki::span<const char> name) override{
		if(name.size() == 0){
			ss << "/>";
		}else{
			ss << "</" << name << ">";
		}
	}

	void on_attributes_end(bool isEmptyElement) override{
		if(!isEmptyElement){
			ss << ">";
		}
	}

	void on_element_start(utki::span<const char> name) override{
		ss << '<' << name;
	}
	
	void on_content_parsed(utki::span<const char> str) override{
		ss << str;
	}
};

int main(int argc, char** argv){
	{
		std::string_view in = "<element attribute=\"attributeValue\">content</element>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attributeValue'>content</element>", " str = " << parser.ss.str())
	}
	{
		std::string_view in = "<element attribute='attribute&amp;&lt;&gt;&quot;&apos;Value'>content&amp;&lt;&gt;&quot;&apos;</element>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attribute&<>\"'Value'>content&<>\"'</element>", " str = " << parser.ss.str())
	}
	{
		std::string_view in = "<element attribute='attribute&#xbf5;Value'>content&#1050;</element>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attribute௵Value'>contentК</element>", " str = " << parser.ss.str())
	}
	{
		std::string_view in = "<element attribute='attribute&#xbf5;Value'/>";
		Parser parser;
		
		parser.feed(in);
		parser.end();
		
//		TRACE_ALWAYS(<< "out = " << parser.ss.str() << std::endl)
		ASSERT_INFO_ALWAYS(parser.ss.str() == "<element attribute='attribute௵Value'/>", " str = " << parser.ss.str())
	}
}
