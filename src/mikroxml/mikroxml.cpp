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

#include "mikroxml.hpp"

#include <sstream>

#include <utki/string.hpp>
#include <utki/unicode.hpp>

using namespace mikroxml;

namespace {
const std::string comment_tag_word = "!--";
const std::string doctype_tag_word = "!DOCTYPE";
const std::string doctype_element_tag_word = "!ELEMENT";
const std::string doctype_attlist_tag_word = "!ATTLIST";
const std::string doctype_entity_tag_word = "!ENTITY";
const std::string cdata_tag_word = "![CDATA[";
constexpr auto buffer_reserve_size = 0x100; // 4kb
constexpr auto ref_char_buffer_reserve_size = 10;
} // namespace

parser::parser()
{
	this->buf.reserve(buffer_reserve_size);
	this->name.reserve(buffer_reserve_size);
	this->ref_char_buf.reserve(ref_char_buffer_reserve_size);
}

malformed_xml::malformed_xml(unsigned line_number, const std::string& message) :
	std::logic_error([line_number, &message]() {
		std::stringstream ss;
		ss << message << " line: " << line_number;
		return ss.str();
	}())
{}

void parser::feed(utki::span<const char> data)
{
	for (auto i = data.begin(), e = data.end(); i != e; ++i) {
		switch (this->cur_state) {
			case state::idle:
				this->parse_idle(i, e);
				break;
			case state::tag:
				this->parse_tag(i, e);
				break;
			case state::tag_empty:
				this->parse_tag_empty(i, e);
				break;
			case state::tag_seek_gt:
				this->parse_tag_seek_gt(i, e);
				break;
			case state::declaration:
				this->parse_declaration(i, e);
				break;
			case state::declaration_end:
				this->parse_declaration_end(i, e);
				break;
			case state::comment:
				this->parse_comment(i, e);
				break;
			case state::comment_end:
				this->parse_comment_end(i, e);
				break;
			case state::attributes:
				this->parse_attributes(i, e);
				break;
			case state::attribute_name:
				this->parse_attribute_name(i, e);
				break;
			case state::attribute_seek_to_equals:
				this->parse_attribute_seek_to_equals(i, e);
				break;
			case state::attribute_seek_to_value:
				this->parse_attribute_seek_to_value(i, e);
				break;
			case state::attribute_value:
				this->parse_attribute_value(i, e);
				break;
			case state::content:
				this->parse_content(i, e);
				break;
			case state::ref_char:
				this->parse_ref_char(i, e);
				break;
			case state::doctype:
				this->parse_doctype(i, e);
				break;
			case state::doctype_body:
				this->parse_doctype_body(i, e);
				break;
			case state::doctype_tag:
				this->parse_doctype_tag(i, e);
				break;
			case state::doctype_skip_tag:
				this->parse_doctype_skip_tag(i, e);
				break;
			case state::doctype_entity_name:
				this->parse_doctype_entity_name(i, e);
				break;
			case state::doctype_entity_seek_to_value:
				this->parse_doctype_entity_seek_to_value(i, e);
				break;
			case state::doctype_entity_value:
				this->parse_doctype_entity_value(i, e);
				break;
			case state::skip_unknown_exclamation_mark_construct:
				this->parse_skip_unknown_exclamation_mark_construct(i, e);
				break;
			case state::cdata:
				this->parse_cdata(i, e);
				break;
			case state::cdata_terminator:
				this->parse_cdata_terminator(i, e);
				break;
		}
		if (i == e) {
			return;
		}
	}
}

void parser::process_parsed_ref_char()
{
	this->cur_state = this->state_after_ref_char;

	if (this->ref_char_buf.size() == 0) {
		return;
	}

	if (this->ref_char_buf[0] == '#') { // numeric character reference
		this->ref_char_buf.push_back(0); // null-terminate

		char* end_ptr = nullptr;
		char* start_ptr = &*(++this->ref_char_buf.begin());
		auto base = [&start_ptr]() {
			if (*start_ptr == 'x') { // hexadecimal format
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				++start_ptr;
				return utki::integer_base::hex;
			} else { // decimal format
				return utki::integer_base::dec;
			}
		}();

		uint32_t unicode = std::strtoul(start_ptr, &end_ptr, utki::to_int(base));
		if (end_ptr != &*this->ref_char_buf.rbegin()) {
			std::stringstream ss;
			ss << "unknown numeric character reference encountered: " << &*(++this->ref_char_buf.begin());
			throw malformed_xml(this->line_number, ss.str());
		}
		auto utf8 = utki::to_utf8(char32_t(unicode));
		for (auto i = utf8.begin(), e = utf8.end(); *i != '\0' && i != e; ++i) {
			this->buf.push_back(*i);
		}
	} else { // character name reference
		std::string ref_char_string(this->ref_char_buf.data(), this->ref_char_buf.size());

		auto i = this->doctype_entities.find(ref_char_string);
		if (i != this->doctype_entities.end()) {
			this->buf.insert(std::end(this->buf), std::begin(i->second), std::end(i->second));
		} else if (ref_char_string == "amp") {
			this->buf.push_back('&');
		} else if (ref_char_string == "lt") {
			this->buf.push_back('<');
		} else if (ref_char_string == "gt") {
			this->buf.push_back('>');
		} else if (ref_char_string == "quot") {
			this->buf.push_back('"');
		} else if (ref_char_string == "apos") {
			this->buf.push_back('\'');
		} else {
			std::stringstream ss;
			ss << "unknown name character reference encountered: "
			   << std::string(this->ref_char_buf.data(), this->ref_char_buf.size());
			throw malformed_xml(this->line_number, ss.str());
		}
	}

	this->ref_char_buf.clear();
}

void parser::parse_ref_char(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case ';':
				this->process_parsed_ref_char();
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				this->ref_char_buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_tag_empty(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	ASSERT(this->buf.empty())
	ASSERT(this->name.empty())
	for (; i != e; ++i) {
		switch (*i) {
			case '>':
				this->on_attributes_end(true);
				this->on_element_end(utki::make_span<char>(nullptr, 0));
				this->cur_state = state::idle;
				return;
			default:
				throw malformed_xml(this->line_number, "unexpected '/' character in attribute list encountered.");
		}
	}
}

void parser::parse_content(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '<':
				this->on_content_parsed(utki::make_span(this->buf));
				this->buf.clear();
				this->cur_state = state::tag;
				return;
			case '&':
				ASSERT(this->ref_char_buf.empty())
				this->state_after_ref_char = this->cur_state;
				this->cur_state = state::ref_char;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::handle_attribute_parsed()
{
	this->on_attribute_parsed(utki::make_span(this->name), utki::make_span(this->buf));
	this->name.clear();
	this->buf.clear();
	this->cur_state = state::attributes;
}

void parser::parse_attribute_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	ASSERT(!this->name.empty())
	for (; i != e; ++i) {
		switch (*i) {
			case '\'':
				if (this->attr_value_quote_char == '\'') {
					this->handle_attribute_parsed();
					return;
				}
				this->buf.push_back(*i);
				break;
			case '"':
				if (this->attr_value_quote_char == '"') {
					this->handle_attribute_parsed();
					return;
				}
				this->buf.push_back(*i);
				break;
			case '&':
				ASSERT(this->ref_char_buf.empty())
				this->state_after_ref_char = this->cur_state;
				this->cur_state = state::ref_char;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_attribute_seek_to_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				break;
			case '\'':
				this->attr_value_quote_char = '\'';
				this->cur_state = state::attribute_value;
				return;
			case '"':
				this->attr_value_quote_char = '"';
				this->cur_state = state::attribute_value;
				return;
			default:
				throw malformed_xml(this->line_number, R"(unexpected character encountered, expected "'" or '"'.)");
		}
	}
}

void parser::parse_attribute_seek_to_equals(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				break;
			case '=':
				ASSERT(!this->name.empty())
				ASSERT(this->buf.empty())
				this->cur_state = state::attribute_seek_to_value;
				return;
			default:
				{
					std::stringstream ss;
					ss << "unexpected character encountered (0x" << std::hex << unsigned(*i) << "), expected '='";
					throw malformed_xml(this->line_number, ss.str());
				}
		}
	}
}

void parser::parse_attribute_name(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				ASSERT(!this->name.empty())
				this->cur_state = state::attribute_seek_to_equals;
				return;
			case '=':
				ASSERT(this->buf.empty())
				this->cur_state = state::attribute_seek_to_value;
				return;
			default:
				this->name.push_back(*i);
				break;
		}
	}
}

void parser::parse_attributes(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	ASSERT(this->buf.empty())
	ASSERT(this->name.empty())
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				break;
			case '/':
				this->cur_state = state::tag_empty;
				return;
			case '>':
				this->on_attributes_end(false);
				this->cur_state = state::idle;
				return;
			case '=':
				throw malformed_xml(this->line_number, "unexpected '=' encountered");
			default:
				this->name.push_back(*i);
				this->cur_state = state::attribute_name;
				return;
		}
	}
}

void parser::parse_comment(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '-':
				this->cur_state = state::comment_end;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				break;
		}
	}
}

void parser::parse_comment_end(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				this->buf.clear();
				this->cur_state = state::comment;
				return;
			case '-':
				if (this->buf.size() == 1) {
					this->buf.clear();
					this->cur_state = state::comment;
					return;
				}
				ASSERT(this->buf.empty())
				this->buf.push_back('-');
				break;
			case '>':
				if (this->buf.size() == 1) {
					this->buf.clear();
					this->cur_state = state::idle;
					return;
				}
				ASSERT(this->buf.empty())
				this->buf.clear();
				this->cur_state = state::comment;
				return;
		}
	}
}

void parser::end()
{
	if (this->cur_state != state::idle) {
		std::array<char, 1> new_line = {{'\n'}};
		this->feed(utki::make_span(new_line));
	}
}

namespace {
bool starts_with(const std::vector<char>& vec, const std::string& str)
{
	if (vec.size() < str.size()) {
		return false;
	}

	for (size_t i = 0; i != str.size(); ++i) {
		if (str[i] != vec[i]) {
			return false;
		}
	}
	return true;
}
} // namespace

void parser::process_parsed_tag_name()
{
	if (this->buf.empty()) {
		throw malformed_xml(this->line_number, "tag name cannot be empty");
	}

	switch (this->buf[0]) {
		case '?':
			// some declaration, we just skip it.
			this->buf.clear();
			this->cur_state = state::declaration;
			return;
		case '!':
			//			TRACE(<< "this->buf = " << std::string(&*this->buf.begin(), this->buf.size()) << std::endl)
			if (starts_with(this->buf, doctype_tag_word)) {
				this->cur_state = state::doctype;
			} else {
				this->cur_state = state::skip_unknown_exclamation_mark_construct;
			}
			this->buf.clear();
			return;
		case '/':
			if (this->buf.size() <= 1) {
				throw malformed_xml(this->line_number, "end tag cannot be empty");
			}
			this->on_element_end(utki::make_span(this->buf).subspan(1));
			this->buf.clear();
			this->cur_state = state::tag_seek_gt;
			return;
		default:
			this->on_element_start(utki::make_span(this->buf));
			this->buf.clear();
			this->cur_state = state::attributes;
			return;
	}
}

void parser::parse_tag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				this->process_parsed_tag_name();
				return;
			case '>':
				this->process_parsed_tag_name();
				switch (this->cur_state) {
					case state::attributes:
						this->on_attributes_end(false);
						[[fallthrough]];
					default:
						this->cur_state = state::idle;
						break;
				}
				return;
			case '[':
				this->buf.push_back(*i);
				if (this->buf.size() == cdata_tag_word.size() && starts_with(this->buf, cdata_tag_word)) {
					this->buf.clear();
					this->cur_state = state::cdata;
					return;
				}
				break;
			case '-':
				this->buf.push_back(*i);
				if (this->buf.size() == comment_tag_word.size() && starts_with(this->buf, comment_tag_word)) {
					this->cur_state = state::comment;
					this->buf.clear();
					return;
				}
				break;
			case '/':
				if (!this->buf.empty()) {
					this->process_parsed_tag_name();

					// After parsing usual tag we expect attributes, but since we got '/' the tag has no any attributes,
					// so it is empty. In other cases, like '!DOCTYPE' tag the cur_state should remain.
					if (this->cur_state == state::attributes) {
						this->cur_state = state::tag_empty;
					}
					return;
				}
				[[fallthrough]];
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_doctype(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '>':
				ASSERT(this->buf.empty())
				this->cur_state = state::idle;
				return;
			case '[':
				this->cur_state = state::doctype_body;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				break;
		}
	}
}

void parser::parse_doctype_body(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case ']':
				ASSERT(this->buf.empty())
				this->cur_state = state::doctype;
				return;
			case '<':
				this->cur_state = state::doctype_tag;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				break;
		}
	}
}

void parser::parse_doctype_tag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				if (this->buf.size() == 0) {
					throw malformed_xml(this->line_number, "empty DOCTYPE tag name encountered");
				}

				if (starts_with(this->buf, doctype_element_tag_word)
					|| starts_with(this->buf, doctype_attlist_tag_word))
				{
					this->cur_state = state::doctype_skip_tag;
				} else if (starts_with(this->buf, doctype_entity_tag_word)) {
					this->cur_state = state::doctype_entity_name;
				} else {
					throw malformed_xml(this->line_number, "unknown DOCTYPE tag encountered");
				}
				this->buf.clear();
				return;
			case '>':
				throw malformed_xml(this->line_number, "unexpected > character while parsing DOCTYPE tag");
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_doctype_skip_tag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '>':
				ASSERT(this->buf.empty())
				this->cur_state = state::doctype_body;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				break;
		}
	}
}

void parser::parse_doctype_entity_name(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				if (this->buf.size() == 0) {
					break;
				}

				this->name = std::move(this->buf);
				ASSERT(this->buf.empty())

				this->cur_state = state::doctype_entity_seek_to_value;
				return;
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_doctype_entity_seek_to_value(
	utki::span<const char>::iterator& i,
	utki::span<const char>::iterator& e
)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				break;
			case '"':
				this->cur_state = state::doctype_entity_value;
				return;
			default:
				throw malformed_xml(
					this->line_number,
					"unexpected character encountered while seeking to DOCTYPE entity value, expected '\"'."
				);
		}
	}
}

void parser::parse_doctype_entity_value(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '"':
				this->doctype_entities.insert(std::make_pair(utki::make_string(this->name), std::move(this->buf)));

				this->name.clear();

				ASSERT(this->buf.empty())

				this->cur_state = state::doctype_skip_tag;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_skip_unknown_exclamation_mark_construct(
	utki::span<const char>::iterator& i,
	utki::span<const char>::iterator& e
)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '>':
				ASSERT(this->buf.empty())
				this->cur_state = state::idle;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				break;
		}
	}
}

void parser::parse_tag_seek_gt(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '\n':
				++this->line_number;
				[[fallthrough]];
			case ' ':
			case '\t':
			case '\r':
				break;
			case '>':
				this->cur_state = state::idle;
				return;
			default:
				{
					std::stringstream ss;
					ss << "unexpected character encountered (" << *i << "), expected '>'.";
					throw malformed_xml(this->line_number, ss.str());
				}
		}
	}
}

void parser::parse_declaration(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '?':
				this->cur_state = state::declaration_end;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				break;
		}
	}
}

void parser::parse_declaration_end(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '>':
				this->cur_state = state::idle;
				return;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				this->cur_state = state::declaration;
				return;
		}
	}
}

void parser::parse_idle(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case '<':
				this->cur_state = state::tag;
				return;
			case '&':
				this->state_after_ref_char = state::content;
				this->cur_state = state::ref_char;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->line_number;
				[[fallthrough]];
			default:
				ASSERT(this->buf.empty())
				this->buf.push_back(*i);
				this->cur_state = state::content;
				return;
		}
	}
}

void parser::feed(const std::string& str)
{
	this->feed(utki::make_span(str.c_str(), str.length()));
}

void parser::parse_cdata(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case ']':
				this->buf.push_back(*i);
				this->cur_state = state::cdata_terminator;
				return;
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parse_cdata_terminator(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e)
{
	for (; i != e; ++i) {
		switch (*i) {
			case ']':
				ASSERT(!this->buf.empty())
				ASSERT(this->buf.back() == ']')
				this->buf.push_back(']');
				break;
			case '>':
				ASSERT(!this->buf.empty())
				ASSERT(this->buf.back() == ']')
				if (this->buf.size() < 2 || this->buf[this->buf.size() - 2] != ']') {
					this->buf.push_back('>');
					this->cur_state = state::cdata;
				} else { // CDATA block ended
					this->on_content_parsed(utki::make_span(this->buf.data(), this->buf.size() - 2));
					this->buf.clear();
					this->cur_state = state::idle;
				}
				return;
			default:
				this->buf.push_back(*i);
				this->cur_state = state::cdata;
				return;
		}
	}
}
