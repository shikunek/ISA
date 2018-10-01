Projekt do predmetu ISA
Vypracoval: Petr Polanský
Popis: 	Program se pomoci adres z prikazoveho radku pripojuje 
	do multicastove skupiny a odesíla z ni data na unicastovou
	adresu.
	Adresu multicastu lze zadat jako IPv4 nebo IPv6.
	Adresu unicastu lze zadat jako IPv4, IPv6 nebo jako domenu.

Priklad spusteni: 

	./m2u -g 224.0.0.1:1234 -h [ff02::5]:1234

Odevzdane soubory: 

m2u.c
Makefile
manual.pdf
README
	
	
