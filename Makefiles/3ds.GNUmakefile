# Rules for the 3ds, out_ctru and such.
ifeq ($(PLATFORM),3ds)

define shader-as
	$(eval CURBIN := $*.shbin)
	$(eval DEPSFILE := $*.shbin.d)
	echo "$(CURBIN).o: $< $1" > $(DEPSFILE)
	echo "extern const u8" `(echo $(notdir $(CURBIN)) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(notdir $(CURBIN)) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(notdir $(CURBIN)) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
	/opt/devkitpro/tools/bin/picasso -o $(CURBIN) $1
	/opt/devkitpro/tools/bin/bin2s $(CURBIN) | $(AS) -o $*.shbin.o
endef

%.shbin.o %_shbin.h : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o %_shbin.h : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o %_shbin.h : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)$(file)))
	@$(call shader-as,$<)

PICAFILES := src/modules/out_ctru.v.pica
OBJECTS += $(PICAFILES:.v.pica=.shbin.o)

endif
