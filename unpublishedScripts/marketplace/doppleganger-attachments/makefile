all:
	@echo "make dist"

dist: doppleganger-a.svg.json doppleganger-i.svg.json dist/app-doppleganger-marketplace.js
	@echo "OK"

%.svg.json: %.svg
	cat $< | jq -sR '"data:image/svg+xml;xml,"+.' > $@

dist/app-doppleganger-marketplace.js: *.js
	./node_modules/.bin/webpack --verbose app-doppleganger-attachments.js $@
