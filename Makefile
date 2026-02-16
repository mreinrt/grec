CXX = g++
CXXFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -O2
LIBS = `pkg-config --libs gtk+-3.0` -lX11
TARGET = grec
SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	sudo install -m 755 $(TARGET) /usr/local/bin/

uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
