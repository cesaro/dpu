
FIGS:=$(wildcard fig/*.fig fig/*/*.fig)
FIGS:=$(patsubst %.fig,%.pdftex,$(FIGS)) $(patsubst %.fig,%.pdftex_t,$(FIGS))
TEXC:=pdflatex -interaction=nonstopmode

T=main
 
p parse : $(FIGS)
	rubber-info $T
	rubber-info --refs $T

s simple : $(FIGS)
	$(TEXC) $T
	-rubber-info $T

m multi : $(FIGS)
	-$(TEXC) $T
	bibtex $T
	-$(TEXC) $T
	-$(TEXC) $T

c continuous :
	latexmk -pdf -pvc -silent $(shell pwd)/$T > /dev/null 2>&1 &
	#latexmk -pdf -pvc -silent $T &
	ps ax | grep latexmk | grep -v grep

k kill :
	ps ax | grep latexmk | grep -v grep
	kill `ps ax | grep latexmk | grep -v grep | sed 's/^ *//' | tr -s ' ' | cut -d ' ' -f 1`

ps :
	ps ax | grep latexmk | grep -v grep

x xfig :
	cd fig; xfig &

o open :
	open $T.pdf

clean :
	bash -c 'rm -f $T.{aux,bbl,blg,lof,lol,lot,log,out,toc,snm,nav,pag,bcf,run.xml,vtc}'
	bash -c 'rm -f $T-blx.bib'
	bash -c 'rm -f *.fdb_latexmk'
	bash -c 'rm -f fig/*.pdftex fig/*.pdftex_t'
	#bash -c 'rm -f fig/*/*.pdftex fig/*/*.pdftex_t'

.SUFFIXES: .fig .plt .pdftex .pdftex_t
.plt.fig :
	gnuplot < $< > $@
.fig.pdftex :
	fig2dev -L pdftex $< $@
.fig.pdftex_t :
	fig2dev -L pdftex_t -p $(basename $<).pdftex $< $@
