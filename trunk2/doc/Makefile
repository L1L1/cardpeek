LATEX=pdflatex

all:			cardpeek_ref.en.pdf cardpeek_ref.fr.pdf

%.pdf:			%.tex
			$(LATEX) $<

clean:
			rm -f *.pdf *.aux *.log
