all:
	make -C Non_bloquant/Code/
	make -C Bloquant/Code

clean:
	make clean -C Non_bloquant/Code/
	make clean -C Bloquant/Code