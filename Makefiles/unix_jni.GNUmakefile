ifeq ($(PLATFORM),unix_jni)

ifeq (,${JAVA_HOME})
  #$(error Please set JAVA_HOME)
  JAVA_HOME := /usr/lib/jvm/java-17-openjdk
endif

STATIC := 1

CFLAGS += -fPIC

CFLAGS += -I${JAVA_HOME}/include/
CFLAGS += -I${JAVA_HOME}/include/linux


src/os/sh_tty_sled_JniSled.h src/os/JniSled.class: src/os/JniSled.java
	javac -h src/os $^

sh/tty/sled/JniSled.class: src/os/JniSled.class
	mkdir -p $(shell dirname $@)
	cp $^ $@

JNI_PROJECT := sled

#OBJECTS += sh_tty_sled_JniSled.h

#PROJECT := $(JNI_PROJECT).jar

lib$(PROJECT).so: $(OBJECTS) $(ML_OBJECTS)
	$(CC) -shared $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) $(shell cat $(PLATFORM_LIBS) $(MODULES_STATIC_LIBS) 2>/dev/null || true)

$(PROJECT).jar: lib$(PROJECT).so sh/tty/sled/JniSled.class
	zip $@ $^
endif
