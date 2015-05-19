ash: main.c
	gcc -pedantic -Wall -ansi -O4 main.c -o ash

ash_sigdet: main.c
	gcc -pedantic -Wall -ansi -O4 -DSIGDET main.c -o ash
