#include "../../src/mikroxml/mikroxml.hpp"

#include <array>

#include <utki/debug.hpp>
#include <papki/FSFile.hpp>

class Parser : public mikroxml::Parser{
	unsigned indent = 0;
	
	void outIndent(){
		for(unsigned i = 0; i != this->indent; ++i){
			this->ss << "\t";
		}
	}
	
public:
	std::vector<std::string> tagNameStack;
	
	std::stringstream ss;
	
	void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) override{
		this->ss << " " << name << "='" << value << "'";
	}
	
	void onElementEnd(const utki::Buf<char> name) override{
//		TRACE(<< "onElementEnd(): invoked" << std::endl)
		if(name.size() != 0){
			--this->indent;
			this->outIndent();
			this->ss << "</" << name << ">" << std::endl;
			
			ASSERT_INFO_ALWAYS(this->tagNameStack.back() == std::string(&*name.begin(), name.size()), "element start tag (" << this->tagNameStack.back() << ") does not match end tag (" << name << ")")
		}
		this->tagNameStack.pop_back();
	}

	void onAttributesEnd(bool isEmptyElement) override{
//		TRACE(<< "onAttributesEnd(): invoked" << std::endl)
		if(isEmptyElement){
			this->ss << "/>" << std::endl;
		}else{
			this->ss << ">" << std::endl;
			++this->indent;
		}
	}

	void onElementStart(const utki::Buf<char> name) override{
//		TRACE(<< "onElementStart(): invoked" << std::endl)
		this->outIndent();
		this->ss << '<' << name;
		this->tagNameStack.push_back(utki::toString(name));
	}
	
	void onContentParsed(const utki::Buf<char> str) override{
//		TRACE(<< "onContentParsed(): length = " << str.size() << " str = " << str << std::endl)
		this->outIndent();
		this->ss << str << std::endl;
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
		papki::FSFile fi(filename);
		papki::File::Guard fileGuard(fi);

		std::array<std::uint8_t, 0xff> buf;

		while(true){
			auto res = fi.read(utki::wrapBuf(buf));
			ASSERT_ALWAYS(res <= buf.size())
			if(res == 0){
				break;
			}
			parser.feed(utki::wrapBuf(&*buf.begin(), res));
		}
		parser.end();
		ASSERT_ALWAYS(parser.tagNameStack.size() == 0)
	}
	
	{
		papki::FSFile fi("out.xml");
		papki::File::Guard fileguard(fi, papki::File::E_Mode::CREATE);
		
		fi.write(utki::wrapBuf(reinterpret_cast<const std::uint8_t*>(parser.ss.str().c_str()), parser.ss.str().length()));
	}
}
