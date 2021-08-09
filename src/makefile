include prorab.mk
include prorab-license.mk

$(eval $(call prorab-try-simple-include, $(CONANBUILDINFO_DIR)conanbuildinfo.mak))

$(eval $(call prorab-config, ../config))

this_name := mikroxml

this_soname := $(shell cat $(d)soname.txt)

this_srcs := $(call prorab-src-dir,.)

this_cxxflags += $(addprefix -I,$(CONAN_INCLUDE_DIRS))
this_cxxflags_test += $(addprefix -I,$(CONAN_INCLUDE_DIRS))
this_ldflags += $(addprefix -L,$(CONAN_LIB_DIRS))

this_ldlibs += -lutki

$(eval $(prorab-build-lib))

this_src_dir := 
this_license_file := ../LICENSE
$(eval $(prorab-license))
