#pragma once

#include <string>

#include <utki/Buf.hpp>

namespace mikroxml{
class Parser{
	
public:
	enum class Error_e{
		
	};
	
	virtual void onElementStart(const utki::Buf<char> name) = 0;
	
	virtual void onElementEnd() = 0;
	
	virtual void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) = 0;
	
	virtual void onStringParsed(const utki::Buf<char> str) = 0;
	
	virtual void onError(unsigned lineNumber, Error_e error) = 0;
	
	void feed(const utki::Buf<char> data);
	
	virtual ~Parser()noexcept{}
};
}
