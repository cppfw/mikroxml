#pragma once

#include <vector>

#include <utki/span.hpp>
namespace mikroxml{
class malformed_xml : public std::logic_error{
public:
	malformed_xml(unsigned line_number, const std::string& message);
};
class parser{
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
		SKIP_UNKNOWN_EXCLAMATION_MARK_CONSTRUCT,
		cdata,
		cdata_terminator
	} state = State_e::IDLE;

	void parseIdle(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseTag(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseTagEmpty(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseTagSeekGt(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDeclaration(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDeclarationEnd(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseComment(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseCommentEnd(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseAttributes(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseAttributeName(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseAttributeSeekToEquals(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseAttributeSeekToValue(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseAttributeValue(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseContent(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseRefChar(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctype(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctypeBody(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctypeTag(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctypeSkipTag(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctypeEntityName(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctypeEntitySeekToValue(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseDoctypeEntityValue(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parseSkipUnknownExclamationMarkConstruct(utki::span<char>::const_iterator& i, utki::span<char>::const_iterator& e);
	void parse_cdata(utki::span<const char>::const_iterator& i, utki::span<const char>::const_iterator& e);
	void parse_cdata_terminator(utki::span<const char>::const_iterator& i, utki::span<const char>::const_iterator& e);
	
	void handleAttributeParsed();
	
	void processParsedTagName();
	
	void processParsedRefChar();
	
	std::vector<char> buf;
	std::vector<char> name; // general variable for storing name of something (attribute name, entity name, etc.)
	std::vector<char> refCharBuf;
	
	char attrValueQuoteChar;
	
	State_e stateAfterRefChar;
	
	unsigned lineNumber = 1;
	
	std::map<std::string, std::vector<char>> doctypeEntities;
	
public:
	parser();
	
	virtual void on_element_start(utki::span<const char> name) = 0;
	
	/**
	 * @brief Element end.
	 * @param name - name of the element which has ended. Name is empty if empty element has ended.
	 */
	virtual void on_element_end(utki::span<const char> name) = 0;
	
	/**
	 * @brief Attributes section end notification.
	 * This callback is called when all attributes of the last element have been parsed.
	 * @param is_empty_element - indicates weather the element is empty element or not.
	 */
	virtual void on_attributes_end(bool is_empty_element) = 0;
	
	/**
	 * @brief Attribute parsed notification.
	 * This callback may be called after 'on_element_start' notification. It can be called several times, once for each parsed attribute.
	 * @param name - name of the parsed attribute.
	 * @param value - value of the parsed attribute.
	 */
	virtual void on_attribute_parsed(utki::span<const char> name, utki::span<const char> value) = 0;
	
	/**
	 * @brief Content parsed notification.
	 * This callback may be called after 'onAttributesEnd' notification.
	 * @param str - parsed content.
	 */
	virtual void on_content_parsed(utki::span<const char> str) = 0;
	
	/**
	 * @brief feed UTF-8 data to parser.
	 * @param data - data to be fed to parser.
	 */
	void feed(utki::span<const char> data);
	
	/**
	 * @brief feed UTF-8 data to parser.
	 * @param data - data to be fed to parser.
	 */
	void feed(utki::span<const uint8_t> data){
		this->feed(utki::make_span(reinterpret_cast<const char*>(data.data()), data.size()));
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
	
	virtual ~parser()noexcept{}
};

}
