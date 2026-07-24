#
# makefile
#

all: $(patsubst %.po, %.mo, $(wildcard *.po))

%.mo: %.po
	msgmerge -U $< runewars-na.pot
	msgfmt $< -o $@

clean:
	rm -f *.mo
