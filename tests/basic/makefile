include prorab.mk

this_name := tests

this_cxxflags += -Wall
this_cxxflags += -Wno-format #no warnings about format
this_cxxflags += -Wno-format-security #no warnings about format
this_cxxflags += -DDEBUG
this_cxxflags += -fstrict-aliasing #strict aliasing!!!
this_cxxflags += -std=c++11
this_cxxflags += -O3
this_cxxflags += -g

this_srcs += main.cpp

this_ldlibs += -lmikroxml -lstdc++
this_ldflags += -L$(d)../../src

ifeq ($(os), linux)
else ifeq ($(os), macosx)
    this_cxxflags += -stdlib=libc++ #this is needed to be able to use c++11 std lib
    this_ldlibs += -lc++
else ifeq ($(os),windows)
endif

this_no_install := true

$(eval $(prorab-build-app))

this_dirs := $(subst /, ,$(d))
this_test := $(word $(words $(this_dirs)),$(this_dirs))


ifeq ($(os),windows)
    this_test_cmd := (cd $(d) && cmd /C 'set PATH=../../src;%PATH% && $$(notdir $$^)')
else ifeq ($(os),macosx)
    this_test_cmd := (cd $(d) && DYLD_LIBRARY_PATH=../../src ./$$(notdir $$^))
else ifeq ($(os),linux)
    this_test_cmd := (cd $(d) && LD_LIBRARY_PATH=../../src ./$$(notdir $$^))
else
    $(error "Unknown OS")
endif


define this_rules
test:: $(prorab_this_name)
	@myci-running-test.sh $(this_test)
	$(prorab_echo)$(this_test_cmd); \
		if [ $$$$? -ne 0 ]; then myci-error.sh "test failed"; exit 1; fi
	@myci-passed.sh
endef
$(eval $(this_rules))


#add dependency on libmikroxml
$(prorab_this_name): $(abspath $(d)../../src/libmikroxml$(soext))

$(eval $(call prorab-include,$(d)../../src/makefile))
