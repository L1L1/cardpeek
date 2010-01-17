LATEX=pdflatex

all:			cardpeek_ref.en.pdf cardpeek_ref.fr.pdf

%.toc:			%.tex
			$(LATEX) $<

%.pdf:			%.tex %.toc
			$(LATEX) $<

clean:
			rm -f *.pdf *.aux *.log
