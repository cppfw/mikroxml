#include "mikroxml.hpp"

#include <sstream>

#include <utki/unicode.hpp>

#include <utki/string.hpp>

using namespace mikroxml;

namespace{
const std::string commentTag_c = "!--";
const std::string doctypeTag_c = "!DOCTYPE";
const std::string doctypeElementTag_c = "!ELEMENT";
const std::string doctypeAttlistTag_c = "!ATTLIST";
const std::string doctypeEntityTag_c = "!ENTITY";
const std::string cdata_tag = "![CDATA[";
}

parser::parser(){
	this->buf.reserve(256);
	this->name.reserve(256);
	this->ref_char_buf.reserve(10);
}

malformed_xml::malformed_xml(unsigned line_number, const std::string& message) :
		std::logic_error([line_number, &message](){
			std::stringstream ss;
			ss << message << " line: " << line_number;
			return ss.str();
		}())
{}

void parser::feed(utki::span<const char> data){
	for(auto i = data.begin(), e = data.end(); i != e; ++i){
		switch(this->cur_state){
			case state::idle:
				this->parseIdle(i, e);
				break;
			case state::tag:
				this->parseTag(i, e);
				break;
			case state::tag_empty:
				this->parseTagEmpty(i, e);
				break;
			case state::tag_seek_gt:
				this->parseTagSeekGt(i, e);
				break;
			case state::declaration:
				this->parseDeclaration(i, e);
				break;
			case state::declaration_end:
				this->parseDeclarationEnd(i, e);
				break;
			case state::comment:
				this->parseComment(i, e);
				break;
			case state::comment_end:
				this->parseCommentEnd(i, e);
				break;
			case state::attributes:
				this->parseAttributes(i, e);
				break;
			case state::attribute_name:
				this->parseAttributeName(i, e);
				break;
			case state::attribute_seek_to_equals:
				this->parseAttributeSeekToEquals(i, e);
				break;
			case state::attribute_seek_to_value:
				this->parseAttributeSeekToValue(i, e);
				break;
			case state::attribute_value:
				this->parseAttributeValue(i, e);
				break;
			case state::content:
				this->parseContent(i, e);
				break;
			case state::ref_char:
				this->parseRefChar(i, e);
				break;
			case state::doctype:
				this->parseDoctype(i, e);
				break;
			case state::doctype_body:
				this->parseDoctypeBody(i, e);
				break;
			case state::doctype_tag:
				this->parseDoctypeTag(i, e);
				break;
			case state::doctype_skip_tag:
				this->parseDoctypeSkipTag(i, e);
				break;
			case state::doctype_entity_name:
				this->parseDoctypeEntityName(i, e);
				break;
			case state::doctype_entity_seek_to_value:
				this->parseDoctypeEntitySeekToValue(i, e);
				break;
			case state::doctype_entity_value:
				this->parseDoctypeEntityValue(i, e);
				break;
			case state::skip_unknown_exclamation_mark_construct:
				this->parseSkipUnknownExclamationMarkConstruct(i, e);
				break;
			case state::cdata:
				this->parse_cdata(i, e);
				break;
			case state::cdata_terminator:
				this->parse_cdata_terminator(i, e);
				break;
		}
		if(i == e){
			return;
		}
	}
}

void parser::processParsedRefChar(){
	this->cur_state = this->state_after_ref_char;
	
	if(this->ref_char_buf.size() == 0){
		return;
	}
	
	if(this->ref_char_buf[0] == '#'){ // numeric character reference
		this->ref_char_buf.push_back(0); // null-terminate

		char* endPtr;
		char* startPtr = &*(++this->ref_char_buf.begin());
		int base;
		if(*startPtr == 'x'){ // hexadecimal format
			base = 16;
			++startPtr;
		}else{ // decimal format
			base = 10;
		}

		std::uint32_t unicode = std::strtoul(startPtr, &endPtr, base);
		if(endPtr != &*this->ref_char_buf.rbegin()){
			std::stringstream ss;
			ss << "Unknown numeric character reference encountered: " << &*(++this->ref_char_buf.begin());
			throw malformed_xml(this->line_number, ss.str());
		}
		auto utf8 = utki::to_utf8(char32_t(unicode));
		for(auto i = utf8.begin(), e = utf8.end(); *i != '\0' && i != e; ++i){
			this->buf.push_back(*i);
		}
	}else{ // character name reference
		std::string refCharString(this->ref_char_buf.data(), this->ref_char_buf.size());
		
		auto i = this->doctype_entities.find(refCharString);
		if(i != this->doctype_entities.end()){
			this->buf.insert(std::end(this->buf), std::begin(i->second), std::end(i->second));
		}else if(std::string("amp") == refCharString){
			this->buf.push_back('&');
		}else if(std::string("lt") == refCharString){
			this->buf.push_back('<');
		}else if(std::string("gt") == refCharString){
			this->buf.push_back('>');
		}else if(std::string("quot") == refCharString){
			this->buf.push_back('"');
		}else if(std::string("apos") == refCharString){
			this->buf.push_back('\'');
		}else{
			std::stringstream ss;
			ss << "Unknown name character reference encountered: " << this->ref_char_buf.data();
			throw malformed_xml(this->line_number, ss.str());
		}
	}
	
	this->ref_char_buf.clear();
}

void parser::parseRefChar(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case ';':
				this->processParsedRefChar();
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				this->ref_char_buf.push_back(*i);
				break;
		}
	}
}

void parser::parseTagEmpty(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	ASSERT(this->buf.size() == 0)
	ASSERT(this->name.size() == 0)
	for(; i != e; ++i){
		switch(*i){
			case '>':
				this->on_attributes_end(true);
				this->on_element_end(utki::make_span<char>(nullptr, 0));
				this->cur_state = state::idle;
				return;
			default:
				throw malformed_xml(this->line_number, "Unexpected '/' character in attribute list encountered.");
		}
	}
}

void parser::parseContent(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '<':
				this->on_content_parsed(utki::make_span(this->buf));
				this->buf.clear();
				this->cur_state = state::tag;
				return;
			case '&':
				ASSERT(this->ref_char_buf.size() == 0)
				this->state_after_ref_char = this->cur_state;
				this->cur_state = state::ref_char;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::handleAttributeParsed(){
	this->on_attribute_parsed(utki::make_span(this->name), utki::make_span(this->buf));
	this->name.clear();
	this->buf.clear();
	this->cur_state = state::attributes;
}

void parser::parseAttributeValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	ASSERT(this->name.size() != 0)
	for(; i != e; ++i){
		switch(*i){
			case '\'':
				if(this->attr_value_quote_char == '\''){
					this->handleAttributeParsed();
					return;
				}
				this->buf.push_back(*i);
				break;
			case '"':
				if(this->attr_value_quote_char == '"'){
					this->handleAttributeParsed();
					return;
				}
				this->buf.push_back(*i);
				break;
			case '&':
				ASSERT(this->ref_char_buf.size() == 0)
				this->state_after_ref_char = this->cur_state;
				this->cur_state = state::ref_char;
				return;
			case '\r':
				// ignore
				break;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseAttributeSeekToValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
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
				throw malformed_xml(this->line_number, "Unexpected character encountered, expected \"'\" or '\"'.");
		}
	}
}

void parser::parseAttributeSeekToEquals(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '=':
				ASSERT(this->name.size() != 0)
				ASSERT(this->buf.size() == 0)
				this->cur_state = state::attribute_seek_to_value;
				return;
			default:
				{
					std::stringstream ss;
					ss << "Unexpected character encountered (0x" << std::hex << unsigned(*i) << "), expected '='";
					throw malformed_xml(this->line_number, ss.str());
				}
		}
	}
}

void parser::parseAttributeName(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				ASSERT(this->name.size() != 0)
				this->cur_state = state::attribute_seek_to_equals;
				return;
			case '=':
				ASSERT(this->buf.size() == 0)
				this->cur_state = state::attribute_seek_to_value;
				return;
			default:
				this->name.push_back(*i);
				break;
		}
	}
}

void parser::parseAttributes(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	ASSERT(this->buf.size() == 0)
	ASSERT(this->name.size() == 0)
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
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

void parser::parseComment(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '-':
				this->cur_state = state::comment_end;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseCommentEnd(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			default:
				this->buf.clear();
				this->cur_state = state::comment;
				return;
			case '-':
				if(this->buf.size() == 1){
					this->buf.clear();
					this->cur_state = state::comment;
					return;
				}
				ASSERT(this->buf.size() == 0)
				this->buf.push_back('-');
				break;
			case '>':
				if(this->buf.size() == 1){
					this->buf.clear();
					this->cur_state = state::idle;
					return;
				}
				ASSERT(this->buf.size() == 0)
				this->buf.clear();
				this->cur_state = state::comment;
				return;
		}
	}
}

void parser::end(){
	if(this->cur_state != state::idle){
		std::array<char, 1> newLine = {{'\n'}};
		this->feed(utki::make_span(newLine));
	}
}

namespace{
bool startsWith(const std::vector<char>& vec, const std::string& str){
	if(vec.size() < str.size()){
		return false;
	}
	
	for(unsigned i = 0; i != str.size(); ++i){
		if(str[i] != vec[i]){
			return false;
		}
	}
	return true;
}
}

void parser::processParsedTagName(){
	if(this->buf.size() == 0){
		throw malformed_xml(this->line_number, "tag name cannot be empty");
	}
	
	switch(this->buf[0]){
		case '?':
			// some declaration, we just skip it.
			this->buf.clear();
			this->cur_state = state::declaration;
			return;
		case '!':
//			TRACE(<< "this->buf = " << std::string(&*this->buf.begin(), this->buf.size()) << std::endl)
			if(startsWith(this->buf, doctypeTag_c)){
				this->cur_state = state::doctype;
			}else{
				this->cur_state = state::skip_unknown_exclamation_mark_construct;
			}
			this->buf.clear();
			return;
		case '/':
			if(this->buf.size() <= 1){
				throw malformed_xml(this->line_number, "end tag cannot be empty");
			}
			this->on_element_end(utki::make_span(&*(++this->buf.begin()), this->buf.size() - 1));
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

void parser::parseTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				this->processParsedTagName();
				return;
			case '>':
				this->processParsedTagName();
				switch(this->cur_state){
					case state::attributes:
						this->on_attributes_end(false);
						// fall-through
					default:
						this->cur_state = state::idle;
						break;
				}
				return;
			case '[':
				this->buf.push_back(*i);
				if(this->buf.size() == cdata_tag.size() && startsWith(this->buf, cdata_tag)){
					this->buf.clear();
					this->cur_state = state::cdata;
					return;
				}
				break;
			case '-':
				this->buf.push_back(*i);
				if(this->buf.size() == commentTag_c.size() && startsWith(this->buf, commentTag_c)){
					this->cur_state = state::comment;
					this->buf.clear();
					return;
				}
				break;
			case '/':
				if(this->buf.size() != 0){
					this->processParsedTagName();

					// After parsing usual tag we expect attributes, but since we got '/' the tag has no any attributes, so it is empty.
					// In other cases, like '!DOCTYPE' tag the cur_state should remain.
					if(this->cur_state == state::attributes){
						this->cur_state = state::tag_empty;
					}
					return;
				}
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseDoctype(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->cur_state = state::idle;
				return;
			case '[':
				this->cur_state = state::doctype_body;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDoctypeBody(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case ']':
				ASSERT(this->buf.size() == 0)
				this->cur_state = state::doctype;
				return;
			case '<':
				this->cur_state = state::doctype_tag;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDoctypeTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				if(this->buf.size() == 0){
					throw malformed_xml(this->line_number, "Empty DOCTYPE tag name encountered");
				}
				
				if(
						startsWith(this->buf, doctypeElementTag_c) ||
						startsWith(this->buf, doctypeAttlistTag_c)
				){
					this->cur_state = state::doctype_skip_tag;
				}else if(startsWith(this->buf, doctypeEntityTag_c)){
					this->cur_state = state::doctype_entity_name;
				}else{
					throw malformed_xml(this->line_number, "Unknown DOCTYPE tag encountered");
				}
				this->buf.clear();
				return;
			case '>':
				throw malformed_xml(this->line_number, "Unexpected > character while parsing DOCTYPE tag");
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseDoctypeSkipTag(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->cur_state = state::doctype_body;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDoctypeEntityName(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				if(this->buf.size() == 0){
					break;
				}
				
				this->name = std::move(this->buf);
				ASSERT(this->buf.size() == 0)
				
				this->cur_state = state::doctype_entity_seek_to_value;
				return;
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseDoctypeEntitySeekToValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
			case ' ':
			case '\t':
			case '\r':
				break;
			case '"':
				this->cur_state = state::doctype_entity_value;
				return;
			default:
				throw malformed_xml(this->line_number, "Unexpected character encountered while seeking to DOCTYPE entity value, expected '\"'.");
		}
	}
}

void parser::parseDoctypeEntityValue(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '"':
				this->doctype_entities.insert(std::make_pair(utki::make_string(this->name), std::move(this->buf)));
				
				this->name.clear();
				
				ASSERT(this->buf.size() == 0)
				
				this->cur_state = state::doctype_skip_tag;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				this->buf.push_back(*i);
				break;
		}
	}
}

void parser::parseSkipUnknownExclamationMarkConstruct(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '>':
				ASSERT(this->buf.size() == 0)
				this->cur_state = state::idle;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseTagSeekGt(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '\n':
				++this->line_number;
				// fall-through
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
					ss << "Unexpected character encountered (" << *i << "), expected '>'.";
					throw malformed_xml(this->line_number, ss.str());
				}
		}
	}
}

void parser::parseDeclaration(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '?':
				this->cur_state = state::declaration_end;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				break;
		}
	}
}

void parser::parseDeclarationEnd(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case '>':
				this->cur_state = state::idle;
				return;
			case '\n':
				++this->line_number;
				// fall-through
			default:
				this->cur_state = state::declaration;
				return;
		}
	}
}

void parser::parseIdle(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
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
				// fall-through
			default:
				ASSERT(this->buf.size() == 0)
				this->buf.push_back(*i);
				this->cur_state = state::content;
				return;
		}
	}
}

void parser::feed(const std::string& str){
	this->feed(utki::make_span(str.c_str(), str.length()));
}

void parser::parse_cdata(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
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

void parser::parse_cdata_terminator(utki::span<const char>::iterator& i, utki::span<const char>::iterator& e){
	for(; i != e; ++i){
		switch(*i){
			case ']':
				ASSERT(!this->buf.empty())
				ASSERT(this->buf.back() == ']')
				this->buf.push_back(']');
				break;
			case '>':
				ASSERT(!this->buf.empty())
				ASSERT(this->buf.back() == ']')
				if(this->buf.size() < 2 || this->buf[this->buf.size() - 2] != ']'){
					this->buf.push_back('>');
					this->cur_state = state::cdata;
				}else{ // CDATA block ended
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
