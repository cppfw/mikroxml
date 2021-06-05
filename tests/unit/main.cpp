#include <tst/set.hpp>
#include <tst/check.hpp>

#include "../../src/mikroxml/mikroxml.hpp"

#include <array>
#include <sstream>

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

tst::set set("basic", [](auto& suite){
	suite.template add<std::pair<std::string_view, std::string_view>>(
			"parse_snippet",
			{
				{"<element attribute=\"attributeValue\">content</element>",
				 		"<element attribute='attributeValue'>content</element>"},
				{"<element attribute='attribute&amp;&lt;&gt;&quot;&apos;Value'>content&amp;&lt;&gt;&quot;&apos;</element>",
						"<element attribute='attribute&<>\"'Value'>content&<>\"'</element>"},
				{"<element attribute='attribute&#xbf5;Value'>content&#1050;</element>",
						"<element attribute='attribute௵Value'>contentК</element>"},
				{"<element attribute='attribute&#xbf5;Value'/>",
						"<element attribute='attribute௵Value'/>"}
			},
			[](const auto& p){
				Parser parser;
		
				parser.feed(p.first);
				parser.end();

				auto str = parser.ss.str();

				tst::check_eq(std::string_view(str), p.second, SL);
			}
		);
});
