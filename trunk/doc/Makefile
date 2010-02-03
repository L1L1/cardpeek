LATEX=pdflatex -halt-on-error

all:			cardpeek_ref.en.pdf cardpeek_ref.fr.pdf

%.toc:			%.tex
			@echo "################"
			@echo "## First pass ##"
			@echo "################"
			$(LATEX) $<

%.pdf:			%.tex %.toc
			@echo "#################"
			@echo "## Second pass ##"
			@echo "#################"
			$(LATEX) $<

clean:
			rm -f *.pdf *.aux *.log
