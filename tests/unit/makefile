include prorab.mk
include prorab-test.mk

$(eval $(call prorab-try-simple-include, $(CONANBUILDINFO_DIR)conanbuildinfo.mak))

$(eval $(call prorab-config, ../../config))

this_name := tests

this_srcs += $(call prorab-src-dir, .)

this_ldlibs += -lmikroxml -lutki -lpapki -ltst $(addprefix -l,$(CONAN_LIBS))
this_ldflags += -L$(d)../../src/out/$(c)

this_cxxflags += $(addprefix -I,$(CONAN_INCLUDE_DIRS))
this_ldflags += $(addprefix -L,$(CONAN_LIB_DIRS))

this_no_install := true

$(eval $(prorab-build-app))

# add dependency on libmikroxml
$(prorab_this_name): $(abspath $(d)../../src/out/$(c)/libmikroxml$(dot_so))

this_test_cmd := $(prorab_this_name) --jobs=auto --junit-out=out/$(c)/junit.xml 
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c) $(CONAN_LIB_DIRS)
$(eval $(prorab-test))

$(eval $(call prorab-include, ../../src/makefile))
