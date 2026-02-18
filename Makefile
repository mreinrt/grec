# Compiler
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Wno-unused-result

# Package config flags
GTKFLAGS = `pkg-config --cflags --libs gtk+-3.0`
X11FLAGS = `pkg-config --cflags --libs x11`

# Targets
TARGETS = grec grec_prefs

# Default target
all: $(TARGETS)

# Recorder (main.cpp)
grec: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o grec $(GTKFLAGS) $(X11FLAGS)

# Preferences app (grec_prefs.cpp)
grec_prefs: grec_prefs.cpp
	$(CXX) $(CXXFLAGS) grec_prefs.cpp -o grec_prefs $(GTKFLAGS)

# Clean build files
clean:
	rm -f $(TARGETS)

# Install to /usr/local/bin (optional)
install: $(TARGETS)
	cp grec /usr/local/bin/
	cp grec_prefs /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/grec /usr/local/bin/grec_prefs

# Rebuild everything
rebuild: clean all

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build both grec and grec_prefs (default)"
	@echo "  grec       - Build only the recorder (main.cpp)"
	@echo "  grec_prefs - Build only the preferences app (grec_prefs.cpp)"
	@echo "  clean      - Remove built binaries"
	@echo "  install    - Copy binaries to /usr/local/bin"
	@echo "  uninstall  - Remove binaries from /usr/local/bin"
	@echo "  rebuild    - Clean and rebuild"
	@echo "  help       - Show this help message"

.PHONY: all clean install uninstall rebuild help