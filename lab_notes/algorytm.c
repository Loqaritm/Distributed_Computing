#include "mpi.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 
#define MAX_ILOSC_SZAFEK M;
#define ILOSC_PROCESOW N;


void thread_listen{
    while(1){
        wiadomosc = receive();
        if(wiadomosc.tag == CHCE_WEJSC_DO_SZATNI){
            kolejka_wejscie += wiadomosc.kolejka_wejscie;
            kolejka_wejscie.sort();
            if(wiadomosc.value > my_value){
                send(wiadomosc.sender, ZGODA);
            }
            else{
                kolejka_zgody.append(wiadomosc.sender);
                kolejka_zgody.sort();
            }
        }

        if(wiadomosc.tag == DOM_DO_SZATNIA){
            szatnie_plec_ilosc += wiadomosc.szatnie_plec_ilosc;
            szatnie_ilosc_szafek_zajetych += wiadomosc.szatnie_ilosc_szafek_zajetych;
            kolejka_wejscie.remove(wiadomosc.sender);
        }

        if(wiadomosc.tag == SZATNIA_DO_BASEN){
            szatnie_plec_ilosc += wiadomosc.szatnie_plec_ilosc;
        }

        if(wiadomosc.tag == CHCE_WROCIC_Z_BASENU){
            kolejki_baseny[wiadomosc.i].append(wiadomosc.sender);
            if(kolejki_baseny[wiadomosc.i].contains(my_id)){
                if(wiadomosc.value > my_value){
                    send(wiadomosc.sender, ZGODA)
                }
                else{
                    kolejka_zgody_powroty.append(wiadomosc.sender);
                    kolejka_zgody_powroty.sort();
                }
            }
            else
            {
                send(wiadomosc.sender, ZGODA);
            }
            
        }

        if(wiadomosc.tag == BASEN_DO_SZATNIA){
            szatnie_plec_ilosc += wiadomosc.szatnie_plec_ilosc;
        }

        if(wiadomosc.tag == SZATNIA_DO_DOM){
            szatnie_plec_ilosc += wiadomosc.szatnie_plec_ilosc;
            szatnie_ilosc_szafek_zajetych += wiadomosc.szatnie_ilosc_szafek_zajetych;            
        }


        semafor_V;
    }
}

void chce_wejsc_do_szatni(int moje_id){
    kolejka_wejscie.append(moje_id);
    broadcast(kolejka_wejscie, CHCE_WEJSC_DO_SZATNI);
    czekaj_na_potwierdzenie_od_wszystkich();
}

void dom_do_szatnia(int i, char plec){
    if(plec == 'm'){
        szatnie_plec_ilosc[i]++;
    }
    else{
        szatnie_plec_ilosc[i]--;
    }
    szatnie_ilosc_szafek_zajetych[i]++;
    msg = [szatnie_plec_ilosc, szatnie_ilosc_szafek_zajetych];
    broadcast(msg, DOM_DO_SZATNIA);
    send(kolejka_zgody[0], ZGODA);
    return 0;
}

void szatnia_do_basen(int i, char plec){
    if(plec == 'm'){
        szatnie_plec_ilosc[i]--;
    }
    else{
        szatnie_plec_ilosc[i]++;
    }
    msg = [szatnie_plec_ilosc];
    broadcast(msg,SZATNIA_DO_BASEN);
}

void chce_wrocic_z_basenu(int i){
    kolejki_powroty[szatnia_w_ktorej_jestem].append(moje_id);
    msg = [kolejki_powroty];
    broadcast(msg,CHCE_WROCIC_Z_BASENU);
    czekaj_na_potwierdzenie_od_wszystkich();
}


void basen_do_szatnia(int i, char plec){
    if (plec == 'm'){
        szatnia_plec_ilosc[i]++;
    }
    else{
        szatnie_plec_ilosc[i]--;
    }
    msg = [szatnie_plec_ilosc];
    broadcast(msg,BASEN_DO_SZATNIA);
    send(kolejka_zgody_powroty[0], ZGODA);
}

void szatnia_do_dom(int i, char plec){
    if(plec == 'm'){
        szatnia_plec_ilosc[i]--;
    }
    else{
        szatnia_plec_ilosc[i]++;
    }
    szatnie_ilosc_szafek_zajetych[i]--;
    msg = [szatnie_plec_ilosc, szatnie_ilosc_szafek_zajetych];
    broadcast(msg,SZATNIA_DO_DOM);
}


main(){
    int szatnie_plec_ilosc[3] = {0, 0, 0};
    int szatnie_ilosc_szafek_zajetych[3] = {0, 0, 0};
    FIFO kolejki_powroty[3] = {null, null, null};
    FIFO kolejka_wejscie;
    char plec = random( 'm' -- 'k' );
    int szatnia_w_ktorej_jestem = -1;

    //wejscie do szatni

    chce_wejsc_do_szatni(moje_id);
    while (1) {
        semafor_P(SEMAFOR_1);
        for(int i=0; i<3; i++){
            if(plec == 'm'){
                if(szatnie_plec_ilosc[i] >= 0 && 
                szatnie_ilosc_szafek_zajetych[i] < MAX_ILOSC_SZAFEK &&
                kolejki_powroty[i].length == 0 && kolejka_wejscie[0] == moje_id){
                    szatnia_w_ktorej_jestem = i;
                    // broadcast(WCHODZE);
                    dom_do_szatnia(i, plec);
                    goto dalej;
                }
            }
            if(plec == 'k'){
                if(szatnie_plec_ilosc[i] <= 0 && 
                szatnie_ilosc_szafek_zajetych[i] < MAX_ILOSC_SZAFEK &&
                kolejki_powroty[i].length == 0 && kolejka_wejscie[0] == moje_id){
                    szatnia_w_ktorej_jestem = i;
                    // broadcast(WCHODZE);
                    dom_do_szatnia(i, plec);
                    goto dalej;
                }
            }
        }
    }
    dalej: 
    //czas w szatni
    sleep(rand);
    // broadcast(WYCHODZE_NA_BASEN);
    szatnia_do_basen(i,plec);
    //czas na basenie
    sleep(rand);

    //wychodzenie z basenu do szatni
    chce_wrocic_z_basenu(szatnia_w_ktorej_jestem);
    
    while(1){
        semafor_P(SEMAFOR_2);
        if(plec == 'm'){
            if (szatnie_plec_ilosc[szatnia_w_ktorej_jestem] >=0 && kolejki_powroty[szatnia_w_ktorej_jestem][0] == moje_id){
                // broadcast(WYCHODZE_Z_BASENU);
                basen_do_szatnia(int i, plec);
                goto dalej2;
            }
        }
        else if(plec == 'k'){
            if (szatnie_plec_ilosc[szatnia_w_ktorej_jestem] <=0 && kolejki_powroty[szatnia_w_ktorej_jestem][0] == moje_id){
                // broadcast(WYCHODZE_Z_BASENU);
                basen_do_szatnia(int i, plec);
                goto dalej2;
            }
        }
    }
    dalej2:
    //czas w szatni
    sleep(rand);
    // broadcast(WYCHODZE_Z_SZATNI);
    szatnia_do_dom(i, plec);
}



/*

PARAMETRY:
int szatnie_plec_ilosc[3] = {0, 0, 0};
int szatnie_ilosc_szafek_zajetych[3] = {0, 0, 0};
FIFO kolejki_powroty[3] = {null, null, null};
FIFO kolejka_wejscie;
char plec = random( 'm' -- 'k' );
int szatnia_w_ktorej_jestem = -1;


SCHEMAT DZIALANIA:
1. Proces wysyła wiadomość z żądaniem wejscia i czeka na zgody od wszystkich innych procesów.
2a. Jeżeli proces odbierający jest gorszy (priorytet + zegar lamporta), wysyła potwierdzenie, albo:
2b. jeżeli proces odbierający jest lepszy, zapisuje procesy chcące od niego pozwolenie w tablicy.

3. Proces, który dostał cały zestaw zgód ma pewność że jest najlepszy, to znaczy: wszyscy wiedzą że jest na początku kolejki.
4. Proces sprawdza czy może wejść do którejś z szatni.
5a. Jeżeli nie - czeka na nową wiadomość (odbierane w drugim wątku), po czym powtarza próbę.
5b. Jeżeli tak - wchodzi do sekcji krytycznej, czyli szatni i wysyła wiadomość o tym (zajmuje szafkę i miejsce w szatni
    - mężczyźni dodają +1 do licznika płci w szatni, kobiety -1). 
    Następnie udziela pozwolenia następnemu najlepszemu procesowi, efektywnie przesuwając kolejkę o jedno miejsce do przodu.

6. Proces zostaje w sekcji krytycznej jakis czas (losowy), po czym wysyła wiadomość o swoim wyjsciu z szatni na basen.
    Pozwala to np. odblokować procesy czekające na wejście do szatni. (zwalnia to miejsce w szatni, a właściwie zmienia ilość płci w szatni)
7. Proces spędza losowy czas na basenie (w swojej sekcji lokalnej).

8. Proces wysyła żądanie wejscia do kolejki powrotnej do swojej szatni. Wszystkie procesy które nie czekają w jednej z powrotnych kolejek
    wysyłają mu ZGODĘ. Procesy w innych kolejkach wysyłają również ZGODĘ. Jedynie ewentualne procesy w kolejce powrotnej[i] mogą
    powstrzymać proces przed wejściem do kolejki powrotnej. Wymagane jest to do odpowiedniej synchronizacji i sortowania kolejki.
9. Proces wysyła wiadomość o swoim wejsciu do kolejki powrotnej. w ten sposób blokuje procesy, chcące wejść do jego szatni
    (każdy proces zapisuje tę informację w swojej tablicy powrotnych przed odesłaniem zgody, co powoduje blokadę).

10. Proces sprawdza warunki wejścia do szatni (w tym wypadku tylko czy jest to jego płeć)
11a. Jeżeli nie - czeka na nowe wiadomości.
11b. Jeżeli tak - wchodzi do sekcji krytycznej, wysyła wiadomość o wejsciu do szatni, a następnie wysyła wiadomość o wyjściu z kolejki powrotnej.

12. Proces spędza losowy czas w sekcji krytycznej, a następnie wysyła wiadomość o wyjsciu z szatni i zwolnieniu szafki.
*/