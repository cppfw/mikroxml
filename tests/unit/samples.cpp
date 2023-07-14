#include <tst/set.hpp>
#include <tst/check.hpp>

#include <regex>

#include <utki/string.hpp>

#include <papki/fs_file.hpp>

#include "../../src/mikroxml/mikroxml.hpp"

namespace{
const std::string data_dir = "samples_data/";
}

namespace{
class parser : public mikroxml::parser{
public:
	std::vector<std::string> tagNameStack;
	
	std::stringstream ss;
	
	void on_attribute_parsed(utki::span<const char> name, utki::span<const char> value) override{
		this->ss << " " << name << "=\"" << value << "\"";
	}
	
	void on_element_end(utki::span<const char> name)override{
		// std::cout << "onElementEnd(): invoked" << std::endl;
        tst::check(!this->tagNameStack.empty(), SL);
		if(name.size() != 0){
			this->ss << "</" << name << ">";
			
			tst::check_eq(this->tagNameStack.back(), std::string(name.data(), name.size()), SL)
                    << "element start tag (" << this->tagNameStack.back() << ") does not match end tag (" << name << ")";
		}
		this->tagNameStack.pop_back();
	}

	void on_attributes_end(bool is_empty_element) override{
		// std::cout << "onAttributesEnd(): invoked" << std::endl;
		if(is_empty_element){
			this->ss << "/>";
		}else{
			this->ss << ">";
		}
	}

	void on_element_start(utki::span<const char> name) override{
		// std::cout << "onElementStart(): invoked" << std::endl;
		this->ss << '<' << name;
		this->tagNameStack.push_back(utki::make_string(name));
	}
	
	void on_content_parsed(utki::span<const char> str) override{
		// std::cout << "onContentParsed(): length = " << str.size() << " str = " << str << std::endl;
		this->ss << str;
	}
};
}

namespace{
const tst::set set("samples", [](tst::suite& suite){
    std::vector<std::string> files;

    {
		const std::regex suffix_regex("^.*\\.xml$");
		auto all_files = papki::fs_file(data_dir).list_dir();

		std::copy_if(
				all_files.begin(),
				all_files.end(),
				std::back_inserter(files),
				[&suffix_regex](auto& f){
					return std::regex_match(f, suffix_regex);
				}
			);
	}

    suite.add<std::string>(
        "sample",
        std::move(files),
        [](const auto& p){
            auto in_file_name = data_dir + p;

            parser parser;
	
            {
                papki::fs_file fi(in_file_name);
                papki::file::guard file_guard(fi);

				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
                std::array<uint8_t, std::numeric_limits<uint8_t>::max()> buf;

                while(true){
                    auto res = fi.read(buf);
                    tst::check_le(res, buf.size(), SL);
                    if(res == 0){
                        break;
                    }
                    parser.feed(utki::make_span(buf.data(), res));
                }
                parser.end();
                tst::check_eq(parser.tagNameStack.size(), size_t(0), SL);
            }

            auto out_string = parser.ss.str();

            auto out_data = to_uint8_t(utki::make_span(out_string));

            auto cmp_data = papki::fs_file(in_file_name + ".cmp").load();

            if(out_data != cmp_data){
                papki::fs_file failed_file(p + ".out");

                {
                    papki::file::guard file_guard(failed_file, papki::file::mode::create);
                    failed_file.write(out_data);
                }

                tst::check(false, SL) << "parsed file is not as expected: " << in_file_name;
            }
        }
    );
});
}
