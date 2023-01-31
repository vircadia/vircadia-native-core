VERSION ?= 5387
BRANCH = branch-heads/$(VERSION)
DESTDIR ?= package
PKGNAME := webrtc
PKGDIR ?= $(DESTDIR)/$(PKGNAME)
PKGARCH := webrtc-$(VERSION)-linux.tar.xz
PKG := $(DESTDIR)/$(PKGARCH)

SRCDIR := ./src

RELDIR := out/Release
DBGDIR := out/Debug

LIBDIR := $(SRCDIR)/$(RELDIR)/obj
PKGLIBDIR := $(PKGDIR)/lib

DBGLIBDIR := $(SRCDIR)/$(DBGDIR)/obj
PKGDBGLIBDIR := $(PKGDIR)/debug/lib

LIB := $(LIBDIR)/libwebrtc.a
PKGLIB := $(LIB:$(LIBDIR)/%=$(PKGLIBDIR)/%)

DBGLIB := $(DBGLIBDIR)/libwebrtc.a
PKGDBGLIB := $(DBGLIB:$(DBGLIBDIR)/%=$(PKGDBGLIBDIR)/%)

PKGINCDIR := $(PKGDIR)/include/webrtc
ABSLSRCDIR := third_party/abseil-cpp
INC := $(shell find -wholename "$(SRCDIR)/api/*.h")
INC += $(shell find -wholename "$(SRCDIR)/rtc_base/*.h")
INC += $(shell find -wholename "$(SRCDIR)/modules/*.h")
INC += $(shell find -wholename "$(SRCDIR)/system_wrappers/*.h")
INC += $(shell find -wholename "$(SRCDIR)/common_video/*.h")
INC += $(shell find -wholename "$(SRCDIR)/common_audio/*.h")
INC += $(shell find -wholename "$(SRCDIR)/call/*.h")
INC += $(shell find -wholename "$(SRCDIR)/media/*.h")
INC += $(shell find -wholename "$(SRCDIR)/video/*.h")
INC += $(shell find -wholename "$(SRCDIR)/p2p/*.h")
INC += $(shell find -wholename "$(SRCDIR)/logging/*.h")
INC += $(shell find -wholename "$(SRCDIR)/$(ABSLSRCDIR)/*.h")

PKGINC := \
     $(patsubst $(SRCDIR)/%,               $(PKGINCDIR)/%, \
     $(patsubst $(SRCDIR)/$(ABSLSRCDIR)/%, $(PKGINCDIR)/%, \
     $(INC) \
     ))

PKGSHRDIR := $(PKGDIR)/share/webrtc
LICENSE := $(SRCDIR)/LICENSE
PKGLICENSE := $(PKGSHRDIR)/copyright

package: $(PKG)

$(PKG): $(PKGINC) $(PKGLIB) $(PKGDBGLIB) $(PKGLICENSE)
	-rm $@
	cd $(DESTDIR); \
	tar -cJf $(PKGARCH) $(PKGNAME); \
	cd -;

$(PKGINCDIR)/%.h: $(SRCDIR)/%.h
	mkdir -p `dirname $@`
	cp  $< $@

$(PKGINCDIR)/%.h: $(SRCDIR)/$(ABSLSRCDIR)/%.h
	mkdir -p `dirname $@`
	cp  $< $@

$(PKGLIB): $(LIB)
	mkdir -p `dirname $@`
	cp  $< $@

$(PKGDBGLIB): $(DBGLIB)
	mkdir -p `dirname $@`
	cp  $< $@

$(PKGLICENSE): $(LICENSE)
	mkdir -p `dirname $@`
	cp  $< $@

$(LIB): build-release
$(DBGLIB): build-debug

build-release: | $(SRCDIR)
	cd $(SRCDIR); \
	export PATH=$(PWD)/depot_tools:$$PATH; \
	export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/; \
	PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/ ninja -C $(RELDIR); \
	cd -

build-debug: | $(SRCDIR)
	cd $(SRCDIR); \
	export PATH=$(PWD)/depot_tools:$$PATH; \
	export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/; \
	PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/ ninja -C $(DBGDIR); \
	cd -

$(SRCDIR)/%: | $(SRCDIR)

$(SRCDIR):
	sudo add-apt-repository ppa:pipewire-debian/pipewire-upstream
	sudo apt update
	sudo apt install git libssl-dev pkg-config libglib2.0-dev \
	  libpipewire-0.3-dev libgbm-dev libegl*-dev libepoxy-dev libdrm-dev clang \
	  libasound2-dev libpulse-dev libxdamage-dev libxrandr-dev libxtst-dev \
	  libxcomposite-dev
	git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
	export PATH=$(PWD)/depot_tools:$(PATH); \
	fetch webrtc; \
	cd $(SRCDIR); \
	git checkout $(BRANCH); \
	gclient sync; \
	rm -rf third_party/boringssl; \
	cat ../patches/$(VERSION)/*.diff | patch -p0 || cat ../patches/default/*.diff | patch -p0; \
	export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig/; \
	gn gen $(RELDIR) --args='use_custom_libcxx=false target_sysroot="/" rtc_include_tests=false rtc_build_examples=false use_debug_fission=false is_debug=false use_rtti=true rtc_use_h264=true proprietary_codecs=true rtc_build_ssl=false rtc_ssl_root=" "'; \
	gn gen $(DBGDIR) --args='use_custom_libcxx=false target_sysroot="/" rtc_include_tests=false rtc_build_examples=false use_debug_fission=false is_debug=true rtc_use_h264=true proprietary_codecs=true rtc_build_ssl=false rtc_ssl_root=" "'; \
	cd -

getsumspace:
	-rm -rf $(SRCDIR)/.git
	-rm -rf $(SRCDIR)/third_party/.git

.PHONY: build-release build-debug getsumspace
