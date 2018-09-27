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
		CONTENT,
		REF_CHAR,
		DOCTYPE,
		DOCTYPE_BODY,
		DOCTYPE_TAG,
		DOCTYPE_ENTITY_NAME,
		DOCTYPE_ENTITY_SEEK_TO_VALUE,
		DOCTYPE_ENTITY_VALUE,
		DOCTYPE_SKIP_TAG,
		SKIP_UNKNOWN_EXCLAMATION_MARK_CONSTRUCT
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
	void parseRefChar(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctype(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctypeBody(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctypeTag(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctypeSkipTag(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctypeEntityName(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctypeEntitySeekToValue(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseDoctypeEntityValue(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	void parseSkipUnknownExclamationMarkConstruct(utki::Buf<char>::const_iterator& i, utki::Buf<char>::const_iterator& e);
	
	void handleAttributeParsed();
	
	void processParsedTagName();
	
	void processParsedRefChar();
	
	std::vector<char> buf;
	std::vector<char> name; //general variable for storing name of something (attribute name, entity name, etc.)
	std::vector<char> refCharBuf;
	
	char attrValueQuoteChar;
	
	State_e stateAfterRefChar;
	
	unsigned lineNumber = 1;
	
	std::map<std::string, std::vector<char>> doctypeEntities;
	
public:
	Parser();
	
	class Exc : public utki::Exc{
	public:
		Exc(const std::string& message) : utki::Exc(message){}
	};
	
	class MalformedDocumentExc : public Exc{
	public:
		MalformedDocumentExc(unsigned lineNumber, const std::string& message);
	};
	
	virtual void onElementStart(const utki::Buf<char> name) = 0;
	
	/**
	 * @brief Element end.
	 * @param name - name of the element which has ended. Name is empty if empty element has ended.
	 */
	virtual void onElementEnd(const utki::Buf<char> name) = 0;
	
	/**
	 * @brief Attributes section end notification.
	 * This callback is called when all attributes of the last element have been parsed.
	 * @param isEmptyElement - indicates weather the element is empty element or not.
	 */
	virtual void onAttributesEnd(bool isEmptyElement) = 0;
	
	/**
	 * @brief Attribute parsed notification.
	 * This callback may be called after 'onElementStart' notification. It can be called several times, once for each parsed attribute.
	 * @param name - name of the parsed attribute.
	 * @param value - value of the parsed attribute.
	 */
	virtual void onAttributeParsed(const utki::Buf<char> name, const utki::Buf<char> value) = 0;
	
	/**
	 * @brief Content parsed notification.
	 * This callback may be called after 'onAttributesEnd' notification.
	 * @param str - parsed content.
	 */
	virtual void onContentParsed(const utki::Buf<char> str) = 0;
	
	/**
	 * @brief feed UTF-8 data to parser.
	 * @param data - data to be fed to parser.
	 */
	void feed(const utki::Buf<char> data);
	
	/**
	 * @brief feed UTF-8 data to parser.
	 * @param data - data to be fed to parser.
	 */
	void feed(const utki::Buf<std::uint8_t> data){
		this->feed(utki::wrapBuf(reinterpret_cast<const char*>(&*data.begin()), data.size()));
	}
	
	/**
	 * @brief Parse in string.
	 * @param str - string to parse.
	 */
	void feed(const std::string& str);
	
	/**
	 * @brief Finalize parsing after all data has been fed.
	 */
	void end();
	
	virtual ~Parser()noexcept{}
};
}
