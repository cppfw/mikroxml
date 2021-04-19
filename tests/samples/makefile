include prorab.mk
include prorab-test.mk
include myci.mk

$(eval $(call myci-try-include, $(CONANBUILDINFO_DIR)conanbuildinfo.mak))

$(eval $(call prorab-config, ../../config))

this_name := parsexml

this_srcs += $(call prorab-src-dir, .)

this_ldlibs += -lpapki -lmikroxml -lutki
this_ldflags += -L$(d)../../src/out/$(c)

this_cxxflags += $(addprefix -I,$(CONAN_INCLUDE_DIRS))
this_ldflags += $(addprefix -L,$(CONAN_LIB_DIRS))

this_no_install := true

$(eval $(prorab-build-app))

# add dependency on libmikroxml
$(prorab_this_name): $(abspath $(d)../../src/out/$(c)/libmikroxml$(dot_so))

this_test_cmd := for f in $(patsubst $(d)%,%,$(shell ls $(d)testdata/*.xml)); do \
		echo "checking file $$$$f" && \
		(cd $(d) && LD_LIBRARY_PATH=../../src/out/$(c) DYLD_LIBRARY_PATH=$$$$LD_LIBRARY_PATH out/$(c)/parsexml $$$$f ) && \
		cmp $(d)out.xml $(d)$$$$f.cmp ; \
		if [ $$$$? -ne 0 ]; then myci-error.sh "parsed XML did not match etalon sample"; exit 1; fi; \
		rm -f $(d)out.xml; \
	done
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c) $(CONAN_LIB_DIRS)
$(eval $(prorab-test))

$(eval $(call prorab-include, ../../src/makefile))
