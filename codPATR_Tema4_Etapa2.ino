#include <Arduino_FreeRTOS.h>
#include <semphr.h>

const int ledTraffic = 13;
const int ledEvents = 12;
const int ledStatus = 11;

SemaphoreHandle_t gasLeakSemaphore;
SemaphoreHandle_t fireAlarmSemaphore;
SemaphoreHandle_t panicButtonSemaphore;
SemaphoreHandle_t externalBlockageSemaphore;
SemaphoreHandle_t carCountMutex;
SemaphoreHandle_t tunnelAccessSemaphore;
SemaphoreHandle_t stopTrafficSemaphore;

int carCount = 0;
bool gasLeak = false;
bool fireAlarm = false;
bool panicButton = false;
bool externalBlockage = false;
bool tunnelOpen = true;

void taskSimulateTraffic(void *pvParameters);
void taskSimulateEvents(void *pvParameters);
void taskDisplayStatus(void *pvParameters);

void setup() {
  Serial.begin(9600);
// Crearea semafoarelor pentru controlul accesului și sincronizare
  stopTrafficSemaphore = xSemaphoreCreateBinary();
  gasLeakSemaphore = xSemaphoreCreateBinary();
  fireAlarmSemaphore = xSemaphoreCreateBinary();
  panicButtonSemaphore = xSemaphoreCreateBinary();
  externalBlockageSemaphore = xSemaphoreCreateBinary();
  carCountMutex = xSemaphoreCreateMutex();
  tunnelAccessSemaphore = xSemaphoreCreateMutex();

// Crearea task-urilor pentru simularea traficului, a evenimentelor și afișarea stărilor
  xTaskCreate(taskSimulateTraffic, "SimulateTraffic", 128, NULL, 1, NULL);
  xTaskCreate(taskSimulateEvents, "SimulateEvents", 128, NULL, 1, NULL);
  xTaskCreate(taskDisplayStatus, "DisplayStatus", 128, NULL, 1, NULL);

  pinMode(ledTraffic, OUTPUT);
  pinMode(ledEvents, OUTPUT);
  pinMode(ledStatus, OUTPUT);
}

void loop() {
// Nu se execută nimic aici
}

// Funcția care simulează comportamentul traficului în tunel
void taskSimulateTraffic(void *pvParameters) {
  while (1) {
    // Așteptăm o scurtă perioadă de timp pentru a nu consuma excesiv resursele
    vTaskDelay(pdMS_TO_TICKS(200));

    if (!tunnelOpen) {
      // Dacă tunelul este închis, continuăm bucla infinită
      continue;
    }

    xSemaphoreTake(carCountMutex, portMAX_DELAY);

    if (carCount < 10 && !gasLeak && !fireAlarm && !panicButton) {
      carCount++;
      Serial.println("Intrare in tunel!");
      Serial.print("Stare curenta:\n - Mașini in tunel: ");
      Serial.println(carCount);
      Serial.print(" - Scurgere gaze: ");
      Serial.println(xSemaphoreTake(gasLeakSemaphore, 0) ? "DA" : "NU");
      Serial.print(" - Incendiu: ");
      Serial.println(xSemaphoreTake(fireAlarmSemaphore, 0) ? "DA" : "NU");
      Serial.print(" - Buton de panica: ");
      Serial.println(xSemaphoreTake(panicButtonSemaphore, 0) ? "APASAT" : "NEAPASAT");
      Serial.print(" - Blocare externa: ");
      Serial.println(xSemaphoreTake(externalBlockageSemaphore, 0) ? "ACTIVA" : "NEACTIVA");
      Serial.println("-----------------------");
    } else {
      if (gasLeak || fireAlarm || panicButton) {
        Serial.println("Eveniment detectat! Tunelul se inchide.");
        xSemaphoreGive(externalBlockageSemaphore);
        xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
        tunnelOpen = false;
        Serial.println("Tunelul s-a inchis.");
      } else {
        Serial.println("Limita de masini a fost atinsa!");
        xSemaphoreGive(externalBlockageSemaphore);
        xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
        tunnelOpen = false;
        Serial.println("Tunelul s-a inchis.");
      }
    }

    xSemaphoreGive(carCountMutex);
  }
}



// Funcția care simulează evenimente precum scurgeri de gaze sau incendii
void taskSimulateEvents(void *pvParameters) {
  while (1) {
    digitalWrite(ledEvents, HIGH);

    int eventOccurred = 0; // Variabilă pentru a verifica dacă un eveniment s-a produs

    if (rand() % 10 == 0) {
      xSemaphoreGive(gasLeakSemaphore);
      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      tunnelOpen = false;
      Serial.println("Scurgere de gaze detectata! Tunelul s-a inchis.");
      xSemaphoreGive(tunnelAccessSemaphore);
      eventOccurred = 1;
    }

    if (!eventOccurred && rand() % 15 == 0) {
      xSemaphoreGive(fireAlarmSemaphore);
      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      tunnelOpen = false;
      Serial.println("Incendiu detectat! Tunelul s-a inchis.");
      xSemaphoreGive(tunnelAccessSemaphore);
      eventOccurred = 1;
    }

    if (!eventOccurred && rand() % 20 == 0) {
      xSemaphoreGive(panicButtonSemaphore);
      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      tunnelOpen = false;
      Serial.println("Buton de panica apasat! Tunelul s-a inchis.");
      xSemaphoreGive(tunnelAccessSemaphore);
      eventOccurred = 1;
    }

    if (!eventOccurred && rand() % 25 == 0) {
      xSemaphoreGive(externalBlockageSemaphore);
      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      tunnelOpen = false;
      Serial.println("Blocare externa activata! Tunelul s-a inchis.");
      xSemaphoreGive(tunnelAccessSemaphore);
      eventOccurred = 1;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(ledEvents, LOW);

    if (eventOccurred) {
      // Așteptăm 5 secunde după ce un eveniment s-a produs pentru a simula o perioadă în care tunelul este închis
      vTaskDelay(pdMS_TO_TICKS(5000));

      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      tunnelOpen = true;
      Serial.println("Tunelul s-a redeschis.");
      xSemaphoreGive(tunnelAccessSemaphore);
    }
  }
}

// Funcția care afișează starea curentă a sistemului
void taskDisplayStatus(void *pvParameters) {
  while (1) {
    digitalWrite(ledStatus, HIGH);
    Serial.println("Stare curenta:");

    xSemaphoreTake(carCountMutex, portMAX_DELAY);
    Serial.print(" - Mașini in tunel: ");
    Serial.println(carCount);
    xSemaphoreGive(carCountMutex);

    Serial.print(" - Scurgere gaze: ");
    Serial.println(xSemaphoreTake(gasLeakSemaphore, 0) ? "DA" : "NU");
    Serial.print(" - Incendiu: ");
    Serial.println(xSemaphoreTake(fireAlarmSemaphore, 0) ? "DA" : "NU");
    Serial.print(" - Buton de panica: ");
    Serial.println(xSemaphoreTake(panicButtonSemaphore, 0) ? "APASAT" : "NEAPASAT");
    Serial.print(" - Blocare externa: ");
    Serial.println(xSemaphoreTake(externalBlockageSemaphore, 0) ? "ACTIVA" : "NEACTIVA");
    Serial.println("-----------------------");

    if (!tunnelOpen) {
      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      vTaskSuspendAll();
      xSemaphoreGive(tunnelAccessSemaphore);
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
    digitalWrite(ledStatus, LOW);

    if (!tunnelOpen) {
      xSemaphoreTake(tunnelAccessSemaphore, portMAX_DELAY);
      xTaskResumeAll();
      xSemaphoreGive(tunnelAccessSemaphore);
    }
  }
}
