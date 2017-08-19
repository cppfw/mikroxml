#pragma once

#include <vector>

#include <utki/Buf.hpp>
#include <utki/Exc.hpp>

namespace mikroxml{
class Parser{
	enum class State_e{
		IDLE,
		TAG,
		TAG_SEEK_GT,
		TAG_EMPTY,
		DECLARATION,
		DECLARATION_END,
		COMMENT,
		COMMENT_END,
		ATTRIBUTES,
		ATTRIBUTE_NAME,
		ATTRIBUTE_SEEK_TO_EQUALS,
		ATTRIBUTE_SEEK_TO_VALUE,
		ATTRIBUTE_VALUE,
		CONTENT
	} state = State_e::IDLE;

	void parseIdle(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseTag(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseTagEmpty(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseTagSeekGt(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDeclaration(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDeclarationEnd(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseComment(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseCommentEnd(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseAttributes(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseAttributeName(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseAttributeSeekToEquals(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseAttributeSeekToValue(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseAttributeValue(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseContent(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	
	void handleAttributeParsed();
	
	void processParsedTagName();
	
	std::vector<std::string> elementNameStack;
	
	std::vector<char> buf;
	std::vector<char> attributeName;
	
	char attrValueQuoteChar;
	
	unsigned lineNumber = 1;
	
public:
	Parser(){
		this->buf.reserve(256);
	}
	
	class Exc : public utki::Exc{
	public:
		Exc(const std::string& message) : utki::Exc(message){}
	};
	
	class MalformedDocumentExc : public Exc{
	public:
		MalformedDocumentExc(unsigned lineNumber, const std::string& message);
	};
	
	virtual void onElementStart(const utki::Buf<char> name) = 0;
	
	virtual void onElementEnd() = 0;
	
	virtual void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) = 0;
	
	virtual void onContentParsed(const utki::Buf<char> str) = 0;
	
	/**
	 * @brief feed UTF-8 data to parser.
	 * @param data - data to be fed to parser.
	 */
	void feed(const utki::Buf<char> data);
	
	void feed(const utki::Buf<std::uint8_t> data){
		this->feed(utki::Buf<char>(reinterpret_cast<char*>(const_cast<std::uint8_t*>(&*data.begin())), data.size()));
	}
	
	/**
	 * @brief Finalize parsing after all data has been fed.
	 */
	void end();
	
	virtual ~Parser()noexcept{}
};
}
