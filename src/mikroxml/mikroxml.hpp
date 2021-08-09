/*
MIT License

Copyright (c) 2017-2021 Ivan Gagis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* ================ LICENSE END ================ */

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
	void parse_comment(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_comment_end(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_attributes(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_attribute_name(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_attribute_seek_to_equals(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_attribute_seek_to_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_attribute_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_content(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_ref_char(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype_body(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype_tag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype_skip_tag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype_entity_name(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype_entity_seek_to_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_doctype_entity_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_skip_unknown_exclamation_mark_construct(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_cdata(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	void parse_cdata_terminator(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e);
	
	void handle_attribute_parsed();
	
	void process_parsed_tag_name();
	
	void process_parsed_ref_char();
	
	std::vector<char> buf;
	std::vector<char> name; // general variable for storing name of something (attribute name, entity name, etc.)
	std::vector<char> ref_char_buf;
	
	char attr_value_quote_char;
	
	state state_after_ref_char;
	
	unsigned line_number = 1;
	
	std::map<std::string, std::vector<char>> doctype_entities;
	
public:
	parser();
	
	/**
	 * @brief Element start.
	 * @param name - name of the element which has started.
	 */
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
