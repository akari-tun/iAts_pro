COMPONENT_SRCDIRS := . config input io logo output platform protocols rx5808 sensors sensors/driver sensors/filter target tracker ui util wifi
# Must be a relative dir, so we can't use $PLATFORMS_DIR
COMPONENT_SRCDIRS += $(addprefix target/platforms/,$(PLATFORM_SOURCES))
COMPONENT_PRIV_INCLUDEDIRS := .