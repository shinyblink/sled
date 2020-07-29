ifeq ($(PLATFORM),ndless)

# we only support static linking
STATIC := 1

CC := $(NDLESS_SDK)/bin/nspire-gcc
AR := $(NDLESS_SDK)/bin/nspire-ar
LD := $(NDLESS_SDK)/bin/nspire-ld
GENZEHN := $(NDLESS_SDK)/bin/genzehn
MAKE_PRG := $(NDLESS_SDK)/bin/make-prg

CFLAGS += -I$(NDLESS_SDK)/include
LIBS += -L$(NDLESS_SDK)/lib

ZEHNFLAGS := --name "$(PROJECT)" --author "shinyblink" --uses-lcd-blit true

# these don't actually get invoked automatically, but! you can still use them manually \o/
$(PROJECT).tns: $(PROJECT)
	$(GENZEHN) --input $^ --output $@ $(ZEHNFLAGS)

$(PROJECT).prg.tns: $(PROJECT).tns
	$(MAKE_PRG) $^ $@

endif
