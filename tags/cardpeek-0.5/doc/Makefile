LATEX=pdflatex -halt-on-error

all:			cardpeek_ref.en.pdf cardpeek_ref.fr.pdf

%.toc:			%.tex
			@echo "## First pass ##"
			@$(LATEX) $< | grep "^LaTeX"

%.pdf:			%.tex %.toc
			@echo "## Second pass ##"
			@$(LATEX) $< | grep "^LaTeX"

clean:
			rm -f *.pdf *.aux *.log
