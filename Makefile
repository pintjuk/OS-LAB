ash: main.c
	gcc -pedantic -Wall -ansi -O4 main.c -o ash

ash_dis: main.c
	gcc -pedantic -Wall -ansi -O4 -DSIGDET main.c -o ash
