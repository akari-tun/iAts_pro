COMPONENT_SRCDIRS := . servo logo platform ui util io gps config wifi
# Must be a relative dir, so we can't use $PLATFORMS_DIR
COMPONENT_SRCDIRS += $(addprefix target/platforms/,$(PLATFORM_SOURCES))
COMPONENT_PRIV_INCLUDEDIRS := .