#include "../../src/mikroxml/mikroxml.hpp"

#include <array>

#include <utki/debug.hpp>
#include <utki/string.hpp>
#include <papki/fs_file.hpp>

class Parser : public mikroxml::parser{
public:
	std::vector<std::string> tagNameStack;
	
	std::stringstream ss;
	
	void on_attribute_parsed(utki::span<const char> name, utki::span<const char> value) override{
		this->ss << " " << name << "=\"" << value << "\"";
	}
	
	void on_element_end(utki::span<const char> name) override{
//		TRACE(<< "onElementEnd(): invoked" << std::endl)
		if(name.size() != 0){
			this->ss << "</" << name << ">";
			
			ASSERT_INFO_ALWAYS(this->tagNameStack.back() == std::string(name.data(), name.size()), "element start tag (" << this->tagNameStack.back() << ") does not match end tag (" << name << ")")
		}
		this->tagNameStack.pop_back();
	}

	void on_attributes_end(bool isEmptyElement) override{
//		TRACE(<< "onAttributesEnd(): invoked" << std::endl)
		if(isEmptyElement){
			this->ss << "/>";
		}else{
			this->ss << ">";
		}
	}

	void on_element_start(utki::span<const char> name) override{
//		TRACE(<< "onElementStart(): invoked" << std::endl)
		this->ss << '<' << name;
		this->tagNameStack.push_back(utki::make_string(name));
	}
	
	void on_content_parsed(utki::span<const char> str) override{
//		TRACE(<< "onContentParsed(): length = " << str.size() << " str = " << str << std::endl)
		this->ss << str;
	}
};

int main(int argc, char** argv){
	std::string filename;
	switch(argc){
		case 0:
		case 1:
			std::cout << "Warning: expected 1 argument: <xml-file>" << std::endl;
			std::cout << "\tGot 0 arguments, assuming <xml-file>=test.xml" << std::endl;
			filename = "test.xml";
			break;
		default:
		case 2:
			filename = argv[1];
			break;
	}
	
	Parser parser;
	
	{
		papki::fs_file fi(filename);
		papki::file::guard fileGuard(fi);

		std::array<std::uint8_t, 0xff> buf;

		while(true){
			auto res = fi.read(utki::make_span(buf));
			ASSERT_ALWAYS(res <= buf.size())
			if(res == 0){
				break;
			}
			parser.feed(utki::make_span(buf.data(), res));
		}
		parser.end();
		ASSERT_ALWAYS(parser.tagNameStack.size() == 0)
	}
	
	{
		papki::fs_file fi("out.xml");
		papki::file::guard file_guard(fi, papki::file::mode::create);
		
		fi.write(utki::make_span(reinterpret_cast<const std::uint8_t*>(parser.ss.str().c_str()), parser.ss.str().length()));
	}
}
