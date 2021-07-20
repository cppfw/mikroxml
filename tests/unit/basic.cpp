#include <tst/set.hpp>
#include <tst/check.hpp>

#include "../../src/mikroxml/mikroxml.hpp"

#include <array>
#include <sstream>
#include <fstream>

namespace{
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
}

tst::set set("basic", [](tst::suite& suite){
	suite.add<std::pair<std::string_view, std::string_view>>(
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

				tst::check_eq(str, std::string(p.second), SL);
			}
		);
	
	suite.add(
		"read_from_istream",
		[](){
			Parser parser;

			static const size_t chunk_size = 0x1000; // 4kb

			std::ifstream s;
			s.open("samples_data/tiger.xml");

			while(!s.eof()){
				std::vector<char> buf;
				buf.reserve(chunk_size);
				for(size_t i = 0; i != chunk_size; ++i){
					char c;
					c = s.get();
					if(s.eof()){
						break;
					}
					buf.push_back(c);
				}
				parser.feed(utki::make_span(buf));
			}
			parser.end();

			tst::check_ne(parser.ss.str().size(), size_t(0), SL);
		}
	);
});
