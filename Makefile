all:
	make -C Bon/Code/
	make -C Bon_Bloquant/Code

clean:
	make clean -C Bon/Code/
	make clean -C Bon_Bloquant/Code