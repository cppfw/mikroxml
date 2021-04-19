#include <mikroxml/mikroxml.hpp>

#include <iostream>

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

int main(int argc, const char** argv){
	auto in = "<element attribute=\"attributeValue\">content</element>";
	Parser parser;
	
	parser.feed(in);
	parser.end();
	
	std::cout << "Hello mikroxml!" << '\n';
}
