CXX      := g++
CXXFLAGS := -std=c++23
TARGET   := main
SRCS     := main.cpp Demos.cpp

# En Windows g++ agrega ".exe" al ejecutable; make debe buscar ese nombre
# para saber si ya esta compilado.
ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all run clean sheet sheet-run docs docs-clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run: all
	./$(TARGET)

# Hoja de calculo interactiva (ejecutable aparte)
SHEET_DEPS := sheet.cpp \
              containers/containers/Spreadsheet/Spreadsheet.h \
              containers/containers/Spreadsheet/ExpressionTree.h \
              containers/containers/Trees/BinaryTree.h \
              containers/containers/Lists/linkedlist.h

sheet: sheet$(EXE)

sheet$(EXE): $(SHEET_DEPS)
	$(CXX) $(CXXFLAGS) sheet.cpp -o sheet$(EXE)

sheet-run: sheet$(EXE)
	./sheet$(EXE)

# Documentacion con Doxygen (salida en docs/html/index.html)
docs:
	mkdir -p docs
	doxygen Doxyfile

docs-clean:
	rm -rf docs

clean:
	rm -f $(TARGET) sheet$(EXE)
