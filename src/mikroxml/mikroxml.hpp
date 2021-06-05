#pragma once

#include <vector>

#include <utki/span.hpp>
namespace mikroxml{
class malformed_xml : public std::logic_error{
public:
	malformed_xml(unsigned line_number, const std::string& message);
};
class parser{
	enum class state{
		idle,
		tag,
		tag_seek_gt,
		tag_empty,
		declaration,
		declaration_end,
		comment,
		comment_end,
		attributes,
		attribute_name,
		attribute_seek_to_equals,
		attribute_seek_to_value,
		attribute_value,
		content,
		ref_char,
		doctype,
		doctype_body,
		doctype_tag,
		doctype_entity_name,
		doctype_entity_seek_to_value,
		doctype_entity_value,
		doctype_skip_tag,
		skip_unknown_exclamation_mark_construct,
		cdata,
		cdata_terminator
	} cur_state = state::idle;

	void parse_idle(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_tag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_tag_empty(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_tag_seek_gt(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_declaration(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_declaration_end(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseComment(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseCommentEnd(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseAttributes(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseAttributeName(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseAttributeSeekToEquals(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseAttributeSeekToValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseAttributeValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseContent(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseRefChar(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctype(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctypeBody(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctypeTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctypeSkipTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctypeEntityName(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctypeEntitySeekToValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseDoctypeEntityValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parseSkipUnknownExclamationMarkConstruct(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_cdata(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_cdata_terminator(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	
	void handleAttributeParsed();
	
	void processParsedTagName();
	
	void processParsedRefChar();
	
	std::vector<char> buf;
	std::vector<char> name; // general variable for storing name of something (attribute name, entity name, etc.)
	std::vector<char> ref_char_buf;
	
	char attr_value_quote_char;
	
	state state_after_ref_char;
	
	unsigned line_number = 1;
	
	std::map<std::string, std::vector<char>> doctype_entities;
	
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
