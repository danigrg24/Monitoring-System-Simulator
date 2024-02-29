#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFF_size 6 // Buffer care stocheaza informatii despre masiniile care intra si ies din tunel
#define MAX_MASINI 10 // Variabila si valoarea sa ce reprezinta nr maximi de masini ce pot circula in tunel in acelasi timp
#define INCIDENT_GAZE 1 // Variabila si valoarea sa pe care o returneaza programul la oprirea sistemului in cazul unei scurgeri de gaze
#define INCIDENT_INCENDIU 8 // Variabila si valoarea sa aleasa aleatoriu pentru oprirea sistemului in caz de incendiu
#define LIMITA_SENZOR_GAZE 10 // Variabila si valoarea sa ce reprezinta limita aleasa aleatoriu pentru valoarea depistata de senzorul de gaze

sem_t Sem[6];
int buff[BUFF_size];
int nrMasini = 0;
int incident = 0;
int butonPanicPresat = 0;

void* producator(void*);
void* monitor_intrare_iesire(void*);
void* monitor_gaze_naturale(void*);
void* monitor_fum(void*);
void* buton_panica(void*);
void* operator_extern(void*);

void* (func[])(void) = {producator, monitor_intrare_iesire, monitor_gaze_naturale, monitor_fum, buton_panica, operator_extern};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Adaugam un mutex pentru a proteja accesul la variabila globala nrMasini

int verificare_apasare_buton_panica();
int operator_blocare_intrare_iesire();
sem_t nrMasiniSem;

void gestionare_masini(int intrare) {
    if (intrare) {
        if (incident == 0) {
            printf("Intrare  in tunel. Masini: %d \n", buff[0]);
            nrMasini++;

            if (nrMasini >= MAX_MASINI) {
                printf("Nr. maxim de masini depasit. Sistemul se opreste.\n");
                incident = 1;
            }
        }
    } else {
        if (incident == 0 && nrMasini > 0) {
            printf("Iesire din tunel. Masini: %d \n", buff[0]);
            nrMasini--;
        }
    }
}

int main(void) {
    pthread_t Task[7];
    pthread_attr_t attr;
	sem_init(&nrMasiniSem, 0, 1);

    for (int i = 0; i < 6; i++)
        if (!sem_init(Sem + i, 1, 1)) {}
        else
            printf("Eroare la initializarea semaforului numarul %d \n", i + 1);

    pthread_attr_init(&attr);
    for (int i = 0; i < 6; i++)
        if (pthread_create(Task + i, &attr, (void*)(*(func + i)), NULL) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }

    for (int i = 0; i < 6; i++)
        if (pthread_join(*(Task + i), NULL) != 0) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }

    printf("\nMain: toate taskurile s-au terminat ... exit ... \n");

    for (int i = 0; i < 6; i++)
        sem_destroy(Sem + i);

    exit(0);
}

void* producator(void* v) {
    int i = 0;
    while (1) {
        sem_wait(&nrMasiniSem);  // Semafor pentru a proteja accesul la nrMasini

        if (incident == 0 && nrMasini < MAX_MASINI) {
            sem_wait(Sem + 4);  // Semafor pentru butonul de panica
            sem_wait(Sem + 5);  // Semafor pentru operatorul extern
            sem_wait(Sem + 2);  // Semafor pentru senzorul de intrare/iesire
            sem_wait(Sem);

            if (nrMasini < MAX_MASINI) {
                buff[i % BUFF_size] = i;
                printf("Intrat/iesit in/din tunel. Masini: %d (nrMasini: %d)\n", i, nrMasini);
                sleep(2);
                i++;
                nrMasini++;

                if (nrMasini >= MAX_MASINI) {
                    printf("Nr. maxim de masini atins. Sistemul se opreste.\n");
                    incident = 1;  // Setam incident pentru a opri sistemul
                }
            }

            sem_post(Sem);  // Eliberam semaforul pentru senzorul de intrare/iesire
            sem_post(Sem + 4);  // Semafor pentru butonul de panica
            sem_post(Sem + 5);  // Semafor pentru operatorul extern
        } else {
            printf("Sistemul oprit din cauza producatorului.\n");
            sem_post(&nrMasiniSem);  // Eliberam semaforul pentru nrMasini in caz de incident
            break;  // Iesim din bucla infinita daca sistemul este oprit
        }

        sem_post(&nrMasiniSem);  // Eliberam semaforul pentru nrMasini
        sem_post(Sem + 3);  // Semafor pentru senzorul de gaze
    }
    pthread_exit(NULL);
}



void* monitor_intrare_iesire(void* v) {
    int intrare, iesire;

    while (1) {
        // Simulam logica de detectare a intrarii si iesirii
        intrare = rand() % 2;
        iesire = rand() % 2;

        if (intrare) {
            sem_wait(Sem + 2);  // Semafor pentru senzorul de intrare/iesire
            sem_wait(Sem);

            if (incident == 0) {
                printf("Intrare in tunel. Masini: %d \n", buff[0]);
                nrMasini++;

                if (nrMasini >= MAX_MASINI) {
                    printf("Nr. maxim de masini depasit. Sistemul se opreste.\n");
                    incident = 1;  // Setam incident pentru a opri sistemul
                }
            } 
            sem_post(Sem);
            sem_post(Sem + 4);  // Semafor pentru butonul de panica
        }

        if (iesire) {
            sem_wait(Sem + 2);  // Semafor pentru senzorul de intrare/iesire
            sem_wait(Sem);

            if (incident == 0 && nrMasini > 0) {
                printf("Iesire din tunel. Masini: %d \n", buff[0]);
                nrMasini--;  // Decrementam numarul de masini la iesirea din tunel
            }

            sem_post(Sem);
            sem_post(Sem + 4);  // Semafor pentru butonul de panica
        }

        // Simulam un interval de timp intre verificari
        sleep(1);
    }
}


void* monitor_gaze_naturale(void* v) {
    while (1) {
        sem_wait(Sem + 3);  // Semafor pentru senzorul de gaze
        sem_wait(Sem);

        if (incident == 0) {
            // Simulam o valoare de la senzorul de gaze (valoare intre 0 si 10)
            int valoare_senzor = rand() % 8;
            
            printf("Verificare senzor gaze... Valoare: %d\n", valoare_senzor);

            if (valoare_senzor > LIMITA_SENZOR_GAZE) {
                printf("Scurgere de gaze detectata. Sistemul se opreste.\n");
                incident = INCIDENT_GAZE; //setam tipul de incident
            }
        } 

        sem_post(Sem);
        sem_post(Sem + 5);  // Semafor pentru operatorul extern
    }
}

int detectare_scurgere_gaze() // Functie auxiliara simulata pentru detectarea scurgerii de gaze
{
    // Simulam valoarea unui senzor de gaze
    return rand() % 10;  // Valori simulate intre 0 si 9
}


void* monitor_fum(void* v) {
    while (1) {
        sem_wait(Sem + 4);  // Semafor pentru senzorii de fum
        sem_wait(Sem);

        if (incident == 0) {
            // Simulam o valoare de la senzorii de fum (0 - fara fum, 1 - cu fum)
            int valoare_senzor_fum = rand() % 50;

            printf("Verificare senzori de fum... Valoare: %d\n", valoare_senzor_fum);

            if (valoare_senzor_fum == 1) {
                printf("Detectat fum. Sistemul se opreste.\n");
                incident = INCIDENT_INCENDIU;
            }
        } 

        sem_post(Sem);
        sem_post(Sem + 5);  // Semafor pentru operatorul extern
    }
}


void* buton_panica(void* v) {
    while (1) {
        sem_wait(Sem + 5);  // Semafor pentru butonul de panica
        sem_wait(Sem);

        if (incident == 0) {
			int valoare_buton_panica = rand() % 100;

            printf("Verificare buton de panica... Valoare: %d\n", valoare_buton_panica);
            // Verificam starea butonului de panica
            if (valoare_buton_panica == 1) {
                printf("Buton de panica apasat. Sistemul se opreste.\n");
                incident = INCIDENT_INCENDIU;
            }
        } 

        sem_post(Sem);
        sem_post(Sem + 2);  // Semafor pentru operatorul extern
    }
}

void* operator_extern(void* v) {
    while (1) {
        sem_wait(Sem + 5);  // Semafor pentru operatorul extern
        sem_wait(Sem);

        if (incident == 0) {
            int valoare_operator = rand() % 10;

            printf("Verificare operator extern... Valoare: %d\n", valoare_operator);

            // Verificam daca incidentul este deja setat pe INCIDENT_INCENDIU
            if (incident == INCIDENT_INCENDIU) {
                printf("Sistemul oprit din cauza incendiului.\n");
                sem_post(Sem);  // Eliberam semaforul
                sem_post(Sem + 5);  // Eliberam semaforul pentru operatorul extern
                pthread_exit(NULL);
            }

            // Verificam daca operatorul a blocat intrarea/iesirea in/din tunel si numarul de masini este mai mic de MAX_MASINI
            if (valoare_operator == 1 && nrMasini >= MAX_MASINI) {
                printf("Operatorul extern a blocat intrarea/iesirea in/din tunel si numarul de masini a ajuns la 10. Sistemul se opreste.\n");
                incident = INCIDENT_INCENDIU;
            }


        }

        sem_post(Sem);
        sem_post(Sem + 5);  // Semafor pentru operatorul extern

        // Asteptam putin inainte de a verifica din nou
        usleep(500000);  // 500ms
    }
}



int verificare_apasare_buton_panica() {
    // Simulam logica de verificare a apasarii butonului de panica
    // Returnam 1 cu o probabilitate de 20% pentru a simula apasarea butonului
    // si 0 cu o probabilitate de 80% pentru a simula absenta apasarii

    int probabilitate_apasare = rand() % 100;  // Generam un numar intre 0 si 99

    // Simulam o probabilitate de 20% pentru apasarea butonului
    if (probabilitate_apasare < 20) {
        return 1;  // Butonul de panica este apasat
    } else {
        return 0;  // Butonul de panica nu este apasat
    }
}

int operator_blocare_intrare_iesire() {
    // Simulam logica de verificare a instructiunilor operatorului extern
    // Returnam 1 pentru a semnala blocarea intrarii/iesirii, 0 altfel
      int probabilitate_blocare = rand() % 100;  // Generam un numar intre 0 si 99

    // Simulam o probabilitate de 50% pentru apasarea butonului
    if (probabilitate_blocare < 50) {
        return 1;  // a fost semnalata blocarea intrarii/iesirii
    } else
		{
        return 0;  // nu a fost semnalata blocarea intrarii/iesirii
		}
}