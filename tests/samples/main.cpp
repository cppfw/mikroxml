#include "../../src/mikroxml/mikroxml.hpp"

#include <array>

#include <utki/debug.hpp>
#include <papki/FSFile.hpp>

class Parser : public mikroxml::Parser{
public:
	void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) override{
		
	}
	
	void onElementEnd(const std::string& name) override{
		
	}
	
	void onAttributesEnd(bool isEmptyElement) override{
		
	}

	void onElementStart(const std::string& name) override{
		
	}
	
	void onContentParsed(const utki::Buf<char> str) override{
		
	}
};

int main(int argc, char** argv){
	papki::FSFile fi("test.xml");
	
	papki::File::Guard fileGuard(fi);
	
	std::array<std::uint8_t, 0xff> buf;
	
	Parser parser;
	
	while(true){
		auto res = fi.read(utki::wrapBuf(buf));
		ASSERT_ALWAYS(res <= buf.size())
		if(res == 0){
			break;
		}
		parser.feed(utki::wrapBuf(&*buf.begin(), res));
	}
	parser.end();
}
