NEGRU BOGDAN-CRISTIAN
		325CA




!!	Modul de pornire:
************************************************************************************************
		- se compileaza (make);
		- se porneste server-ul ruland comanda "make run_server";
		- se trimit topicurile de la udp in server, comanda "python3 udp_client.py 127.0.0.1 8080"
		- se conecteaza clientii tcp, utilizand comanda:
		 * "make ID=nume_client run_subscriber"
		  unde nume_client este id-ul ales de dumneavoastra
***************************************************************************************************


	Subscriber (TCP):
		@ Pentru a trimite mesajele catre server am folosit o clasa (Client), pe care am populat-o
		cu sirul de caractere citit de la STDIN si id-ul clientului luat din argv[1] (parametru in linia
		de comanda).

		@ Initial am populat stuctura sockaddr_in cu datele server-ului, apoi m-am conectat, am setat
		socketii de STDIN si socket-ul server-ului si am folosit multiplexarea cu ajutorul functiei
		select pentru acesti 2 socketi in care daca am primit ceva pe socket-ul de STDIN trimit mesajul
		la server cu ajutorul clasei Client, iar daca primesc de la server afisez (de la server primesc
		direct sub forma de string mesajele)

		@ Server-ul cand se inchide trimite mesaje de exit si la clienti asa ca pt inchidere am verificat
		daca am primit exit pt ambii socketi, adica si de la STDIN si de la server + inainte de loop trimit un mesaj cu id-ul clientului catre server

	Server:
		@ Clasa Command are rolul de a lua inputul trimis de subscriber si sa il parseze, salvand
		componentele in cate un camp (type - subscribe/unsubscribe; topic - numele topicului; SF - daca este cazul).
		@ Clasa Client este aceeasi ca cea din subscriber.cpp si are rolul de a se popula la primirea a
		ceva pe socketul unui subscriber
		@ Clasa Subscriber are rolul de a retine tot ce tine de un subscriber, printre care:
		 un vector de topic-uri la care este abonat, date despre sine prin campurile de tip Client si Command, o variabila care indica daca este online, un vector de stringuri in care se retin
		 comenzile cand el este deconectat, string-urile se sterg dupa ce se trimit. Plus socketul pe care se pot trimite mesaje la subscriber

		@ Clasa Topic are rolul sa retina anumite date despre un topic, precum numele, o structura sockaddr_in populata cu datele clientului de pe care s-a trimis topicul, variabile care ajuta la
		parsarea topicurilor + un map de subscriberi (key = nume topic; value = subscriber)
			- metoda pt parsarea topicului: numele il iau cu strtok, tipul de date il iau cu memcpy de
			la un offset de 50 fata de buffer, iar in functie de tipul de date eventula iau semnul tot cu
			memcpy de la un offset de 51 (iau un octet) si valoarea o iau de unde am ramas cu size-ul specificat in cerinta
			- metoda addMessage are rolul de a adauga contentul unui topic in lista fiecarui subscriber de
			mesaje netrimise sub forma de string (string_history), pentru a putea fi trimis direct si afisat direct de catre subscriber. Pentru asta am parcurs map-ul de subscriberi abonati la topicul respectiv si am creat string-ul ce trebuie afisat de subscriber si in functie de tipul de date
			- metoda sendMessage are rolul de a trimite mesajele din vectorul de mesaje netrimise al fiecarui subscriber abonat la topicul curent. Am parcurs lista de subscriberi si am verificat pentru fiecare daca este online, daca are ceva de primit (string_history.size() > 0) si in functie de SF trimit tot vectorul(SF == 1), apoi il sterg sau trimit doar ultimul mesaj, apoi il sterg (SF = 0)

		@ In main am declarat un map de Topicuri cu cheia, numele topicului, si un map de subscriberi cu cheia descriptorul socketului pe care e conectat pentru a tine evidenta conectarii sau deconectarii clientilor. Popoulez structura cu datele server-ului, fac bind pe socketii si de la tcp si de la udp, iar listen doar pe cel tcp. Adaug socketii in multimea read_fds, inclusiv cel pt STDIN (cazul in care dau exit).

		Multiplexez cu select si verific urmatoarele cazuri:
			- Am primit exit de la STDIN, trimit exit tuturor clientilor tcp si ies din loop
		*Incep parcurgerea socketilor
			- daca am primit ceva de pe socketul de conexiuni tcp atunci dau accept si recv(pentru a primi numele clientului), parcurg lista de subscriberi si verific daca mai este vreunul cu acelasi nume, daca mai este si este si online atunci ii trimit exit, astfel daca nu este online ii scot din map clientul vechi (cu socketul vechi) si il adaug pe acesta preluand datele de la cel vechi, mai putin socket-ul. Daca sunt pe cazul de reconectare trebuie sa ii parcurg lista de topicuri si pentru fiecare topic la care era/este abonat il setez ca fiind online doar daca are SF == 1, altfel ii sterg lista de mesaje netrimise. Daca este client nou il inserez in lista de subscriberi ca subscriber nou, altfel (daca e reconectat) apelez sendMessage pe topicurile la care e abonat (fiind online doar pe acele topicuri cu SF==1), dupa aceea il setez ca fiind online si pe topicurile cu SF==0

			- daca am primit o conexiune udp, dau recvfrom, parsez topicul, il adaug in lista de topicuri  (daca nu s-a mai primit pana acum), apelez addMessage (explicatie: linia 3 - Clasa Topic), apoi apelez sendMessage

			- daca am primit ceva pe un socket de la vreun subscriber, verific daca nu cumva a inchis conexiunea si il marchez ca fiind offline, altfel iau comanda primita, o parsez
			- daca comanda este subscribe verific daca sunt corecte urmatoarele date primite (daca topicul exista, daca s-a mai abonat la acel topic) si ii trimit un mesaj daca este vreo astfel de eroare. Daca este ok atunci ii adaug subscriberului topicul in lista proprie de topicuri + il adaug si in lista de subscriberi a topicului respectiv.
			- altfel daca comanda nu este nici unsubscribe inseamna ca a aparut o eroare, asa ca ii trimit un mesaj de eroare, in schimb daca este unsubscribe atunci scot subscriber-ul din lista de subscriberi a topicului respectiv.

