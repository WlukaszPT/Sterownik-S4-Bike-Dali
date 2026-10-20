// =========================================================================
// STEROWNIK S-4 DALI (dawniej „S4 Bike”) (OFFICIAL WERSJA: WIFI + RTC + EMAIL + PROFILE)
// + POPRAWKI STABILNOŚCI (PULLUP DLA CZUJNIKÓW + SPOWOLNIONY SKANER DALI)
// + NOWOŚĆ: ALARM RADAROWY HTTP (/api/stroboskop)
// + RAPORTY: STARTOWY PO ZALOGOWANIU ORAZ RAZ NA DOBĘ O 1:00 W NOCY
// + DODANO: PANCERNY TRYB OFFLINE (WIFI=0/1) + PODGLĄD HASŁA W PANELU
// + OSTATECZNY FIX: SZYBKA KASKADA (Brak pauzy) + STABILNY STRUMIEŃ (delay)
// + FIX: ZABEZPIECZENIE E-MAILA (Czekanie na IP oraz 15s dla zegara NTP)
// + INTELIGENTNE MENU S1 (przeskakuje niepotrzebne parametry dla animacji)
// -------------------------------------------------------------------------
// v1.1: skaner DALI nie przenumerowuje opraw, limit 54 opraw, wysyłka maila
//       bez długich blokad, portal WiFi tylko na żądanie (S2 przy starcie),
//       awaria dopiero po 3 brakach odpowiedzi, przerwa między ramkami DALI,
//       synchronizacja RTC z NTP, unikalny Message-ID. Szczegóły: docs/JAK-DZIALA.md
// v1.2: skaner miga kolejno każdą znalezioną oprawą; oprawy z awarią sprawdzane
//       częściej, więc awaria znika kilka sekund po naprawie.
// v1.3: podpowiedź „S2 = konfiguracja” mniejszą czcionką (była ucięta na ekranie).
// v1.4: skaner znowu znajduje nowe wkładki S4 (ATtiny85 v1.0 nie znają INITIALISE 0xFF) i nie przenumerowuje
//       opraw z adresem; mignięcia robi sama wkładka po WITHDRAW; 35 ms przerwy po odpowiedzi „TAK”
//       (wkładki v1.0 gubiły następną ramkę). Działa z wkładkami ATtiny v1.0 i v1.1.
// v1.5: kontrola linii przed skanem po zgaszeniu opraw i z 300 pomiarów (było: 2 odczyty) – nie zgłasza
//       fałszywego zwarcia; błąd linii NIE kasuje liczby opraw (wcześniej Kaskada i Strumień przestawały
//       działać, a raport pokazywał „brak awarii”); ekran i raport ostrzegają, gdy brak zaprogramowanych opraw.
// v1.6: miejsca L/P (L1 = adres 10, P1 = 11, L2 = 12…) zapamiętane w EEPROM – skan nigdy nie nadaje numeru
//       oprawy, która tylko nie odpowiada (odłączony sznur dawał podwójne numery i ukrywał awarie); nowe
//       oprawy dostają numer tymczasowy i są przypisywane do miejsca: wymiana 1:1 z przycisków albo na
//       telefonie przez stronę serwisową (własna sieć WiFi sterownika, menu SERWIS WiFi). Kod: koniec pliku.
// v1.7: ramka DALI nadawana z przerwaniami WiFi (wyłączone tylko 150 µs przed każdym zboczem) – sieć serwisowa
//       nie zrywa się przy skanie i miganiu; sieć serwisowa tylko AP (bez łączenia z routerem w tym czasie);
//       stałe hasło sieci serwisowej; po restarcie stan oczek odczytany od razu; „Sprawdź podwójne numery”
//       rozdziela oczka z tym samym numerem (oczko z magazynu); przeniesienie oczka na inne miejsce; ponowienie
//       i kontrola zmiany numeru; dziennik serwisowy na stronie.
// v1.8: (uwagi recenzenta do 1.7) operacje na magistrali tylko z kolejki w loop() i jedna naraz – strona zleca,
//       sterownik wykonuje, wynik z numerem operacji w stanie strony; sprawdzenie źródła i celu tuż przed zmianą
//       numeru; potwierdzenie numerów tymczasowych PING-iem; wynik zapisu EEPROM; strona: limit czasu żądań,
//       jeden odczyt naraz, „brak odpowiedzi sterownika”, połączenia telefonów w dzienniku; licznik spóźnionych zboczy.
// v1.9: (2. przegląd) wynik operacji po identyfikatorze zlecenia (historia 8 zleceń, id związane z rodzajem
//       i parametrami), identyfikator uruchomienia sterownika (restart = wynik nieznany), zapis mapy ponawiany do
//       skutku + „Ponów zapis konfiguracji”, miganie źródła po nieudanym przeniesieniu, limit czasu obejmuje treść
//       odpowiedzi, skan sprawdza numer tymczasowy na magistrali przed nadaniem.
// v1.10: (3. przegląd) brak zlecenia w historii = wynik nieznany (strona zleca odczyt stanu oczek, nie twierdzi,
//       że zlecenie nie dotarło); strona ocenia wynik tylko z odczytów rozpoczętych po zakończeniu zlecenia;
//       odpięcie zawsze sprawdza oczko na magistrali; wolny numer tymczasowy szukany wśród wszystkich adresów.
// v1.11: (4. przegląd) strona: tryb odzyskiwania po wyniku nieznanym i po restarcie – odczyt stanu oczek zlecany,
//       gdy sterownik jest wolny, przypisywanie zablokowane do potwierdzonego odczytu; gdy i odczyt ma wynik
//       nieznany – przycisk „Odczytaj stan ponownie” (bez automatycznego powtarzania).
// v1.12: (5. przegląd) odczyt odzyskiwania: 409 (sterownik zajęty) = czekaj i spróbuj, gdy wolny; inny błąd HTTP =
//       przyczyna na stronie i przycisk „Odczytaj stan ponownie” (bez automatycznego ponawiania).
// v1.13: zgłoszenia z testu 1.12: menu SERWIS WiFi z odliczaniem i „Puść przycisk”; wygaszenie oczek po skanie
//       i operacjach (stare wkładki v1.0 zapalały się na odpowiedzi innych oczek); ekran „NOWE OCZKA – przypisz”
//       zamiast „BRAK OPRAW – wykonaj skan”; sterownik obsługuje stronę w czasie komunikatów i wyszukiwania;
//       kreator „Wymień oczko” (oczko z magazynu z podwójnym numerem: wskazanie miejsca i rozdzielenie jednego
//       numeru); zamiana dwóch oczek miejscami; „Mrugnij” 3 s.
// v1.14: kreator „Wymień to oczko”: gdy pod wskazanym numerem jest tylko jedno oczko (oczko z magazynu z numerem
//       miejsca, w którym nie ma drugiego oczka), kreator przenosi je na wymieniane miejsce (wcześniej kończył się
//       komunikatem „Nie znaleziono nowego oczka”).
// v1.15: nazwa produktu „S-4 Dali” na ekranie, w sieciach Wi-Fi (S-4_Dali_WiFi – konfiguracja, S-4_Dali_XXXX – serwis;
//       wcześniej SmartPass_DALI jak sterownik oświetlenia DALI), w raporcie i na stronie serwisowej; „oczka” zamiast
//       „S4”/„opraw S4”. Stan Wi-Fi z przyczyną (złe hasło, nie widzę sieci, odmowa) na ekranie, w dzienniku i na stronie.
//       Kreator: szukanie miejsca o podwójnym numerze krok po kroku – miejsce miga, dopóki serwisant nie odpowie
//       „Tak, mruga” / „Nie – następne” (wcześniej mruganie przechodziło samo co 1,5 s i nie dało się zdążyć).
// v1.16: wysyłka danych do eaCloud co 30 s (POST /update, kontrakt v2): field1 = licznik rowerzystów narastający od
//       uruchomienia (wejścia czujników START1/START2; eaCloud liczy przyrosty, spadek = restart), field2 = czas pracy od uruchomienia [s], measured_at (gdy jest czas
//       z NTP) i message_id (powtórzenie po zerwanym połączeniu nie tworzy duplikatu). Adres serwera i klucz urządzenia
//       w secrets.h (EACLOUD_URL, EACLOUD_KEY) albo w portalu Wi-Fi (S2 przy starcie); pusty adres = wysyłka wyłączona. Wysyłka czeka, gdy oczka świecą;
//       odpowiedź serwera odbierana w tle (sterownik nie czeka na nią i czyta czujniki).
// =========================================================================

#define FW_VERSION "1.16"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h> 
#include <ESP8266WebServer.h> 
#include <DNSServer.h>
#include <RTClib.h>
#include <Dusk2Dawn.h>
#include <ESP_Mail_Client.h> 

// ==========================================
// KONFIGURACJA POCZTY (NADAWCA)
// ==========================================
#include "secrets.h"   // SENDER_EMAIL, SENDER_PASSWORD, EACLOUD_URL, EACLOUD_KEY (plik poza repozytorium, wzór: secrets.example.h)
// v1.16: adres i klucz eaCloud z secrets.h mają pierwszeństwo przed portalem Wi-Fi; puste = z portalu (EEPROM)
#ifndef EACLOUD_URL
#define EACLOUD_URL ""
#endif
#ifndef EACLOUD_KEY
#define EACLOUD_KEY ""
#endif

#define SMTP_HOST           "smtp.gmail.com"
#define SMTP_PORT           587

#define NAZWA_PRODUKTU      "S-4 Dali"
#define SIEC_KONFIGURACJI   "S-4_Dali_WiFi"   // portal Wi-Fi (S2 przy starcie); sieć serwisowa: S-4_Dali_XXXX

String NAZWA_LOKALIZACJI    = "Miasto Ulica";
String recipientEmail       = "pawel.krajewski@euroasfalt.pl"; 

SMTPSession smtp;
ESP8266WebServer server(80); 

// ==========================================
// KONFIGURACJA SPRZĘTOWA
// ==========================================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ D5, /* data=*/ D6);
RTC_DS3231 rtc;

bool rtcDostepny = false;
bool zbuforowanaNoc = true;
unsigned long ostatniPomiarAstro = 0;
bool aktywnySkanerAwarii = true; 
bool trybOffline = false; 
String wifiOpis = "";       // v1.15: stan Wi-Fi z przyczyną (dziennik, strona serwisowa)
String chmuraOpis = "wyłączona (brak adresu)"; // v1.16: stan wysyłki do eaCloud (raport, dziennik, strona serwisowa)

// v1.15: stan Wi-Fi na ekranie (najwyżej 16 znaków) – te same napisy co sterownik oświetlenia DALI
const char* wifiKrotko(int st) {
  if (trybOffline) return "WIFI: OFFLINE";
  if (WiFi.SSID() == "") return "WIFI: BRAK USTAW";
  switch (st) {
    case WL_CONNECTED: return "WIFI: POŁĄCZONO";
    case WL_WRONG_PASSWORD: return "WIFI: ZŁE HASŁO";
    case WL_NO_SSID_AVAIL: return "WIFI: BRAK SIECI";
    case WL_CONNECT_FAILED: return "WIFI: ODMOWA";
    default: return "WIFI: ŁĄCZĘ...";
  }
}

String wifiPrzyczyna(int st) {
  if (trybOffline) return "tryb offline (Wi-Fi wyłączone)";
  if (WiFi.SSID() == "") return "brak zapisanej sieci – S2 przy starcie, sieć " SIEC_KONFIGURACJI;
  String n = "„" + WiFi.SSID() + "”";
  switch (st) {
    case WL_CONNECTED: return "połączono z " + n + ", sygnał " + String(WiFi.RSSI()) + " dBm";
    case WL_WRONG_PASSWORD: return n + ": złe hasło";
    case WL_NO_SSID_AVAIL: return n + ": sterownik nie widzi tej sieci (zasięg, sieć tylko 5 GHz albo inna nazwa – liczą się wielkie litery i spacje)";
    case WL_CONNECT_FAILED: return n + ": router odrzuca połączenie (np. tylko WPA3 albo filtr adresów MAC)";
    case WL_CONNECTION_LOST: return n + ": połączenie utracone";
    default: return n + ": łączę…";
  }
}

const int PIN_START1      = D1;
const int PIN_START2      = D0;
const int PIN_PRZYCISK_S1 = D3;
const int PIN_PRZYCISK_S2 = D7;
const int PIN_PRZYCISK_S3 = D4;

const int DALI_TX_PIN     = 1;
const int DALI_RX_PIN     = D2;
const int AQY_CTRL_PIN    = D8;

const int DALI_ADDR_OFFSET = 10;
const int MAX_LAMPS = 64 - DALI_ADDR_OFFSET;      // adresy krótkie DALI 10..63
const int LAMP_ERROR_THRESHOLD = 3;               // tyle kolejnych braków odpowiedzi = awaria
const unsigned long POLL_INTERVAL_MS = 2000;      // co ile sprawdzana jest jedna oprawa (było 5 s)
const unsigned long DALI_MIN_GAP_US = 12000;      // minimalna przerwa między ramkami (ATtiny85)
const unsigned long RAPORT_PONOW_MS = 15UL * 60UL * 1000UL; // ponowienie raportu o 1:00 co 15 min

// Adresy EEPROM
const int ADDR_LAMPS_COUNT   = 0;
const int ADDR_CYCLE_TIME    = 4;  
const int ADDR_WAVE_MODE     = 8;
const int ADDR_TARGET_CYCLES = 12; 
const int ADDR_HOLD_TIME     = 16; 
const int ADDR_WIFI_MODE     = 44; 
const int ADDR_EMAIL         = 64; 
const int ADDR_LOCATION      = 128;
const int ADDR_MAPA_ZNACZNIK = 200;  // v1.6: 0xB6 = mapa miejsc zapisana
const int ADDR_MAPA          = 201;  // 8 bajtów: bit n = adres DALI n jest miejscem w kaskadzie
const int ADDR_RZAD_L        = 210;  // liczba miejsc w rzędzie L / P pokazywana na stronie serwisowej
const int ADDR_RZAD_P        = 211;
const int ADDR_HASLO_ZNACZNIK = 212; // v1.7: 0xC7 = zapisane stałe hasło sieci serwisowej
const int ADDR_HASLO         = 213;  // 8 cyfr
const uint8_t HASLO_ZNACZNIK = 0xC7;
const int ADDR_CHMURA_URL    = 256;  // v1.16: adres eaCloud, np. http://192.168.1.50:8198/update (96 znaków)
const int ADDR_CHMURA_KLUCZ  = 352;  // v1.16: klucz urządzenia z eaCloud (64 znaki)
const int CHMURA_URL_DL      = 96;
const int CHMURA_KLUCZ_DL    = 64;
const uint8_t MAPA_ZNACZNIK  = 0xB6;

#define DALI_BROADCAST_DAPC 0xFE
#define DALI_100_PERCENT    254

// Zmienne konfiguracji
int LAMPS_COUNT = 0;
int waveMode = 0;
int czasKroku10ms[4] = {10, 10, 10, 10}; 
int targetCycles[4]  = {1, 1, 1, 1};
int holdTimeSec[4]   = {15, 15, 15, 15};
const char* modeNames[] = {"Kaskada", "Strumień", "Wszystkie", "Stroboskop"};

int gpsLat = 5206;
int gpsLon = 1925;
const int SUNSET_OFFSET = 25;
const int SUNRISE_OFFSET = -20;

bool isCascadeActive = false;
unsigned long cascadeStartTime = 0;
int currentCascadeStep = -1;
int currentCycleCount = 0;
bool reverseDirection = false;

// --- ALARM RADAROWY Z API ---
bool isRadarAlarmActive = false;
unsigned long radarAlarmEndTime = 0;

bool wymusOled = false;
bool raportStartowyDoWyslania = false; // raport startowy wysyłany, gdy WiFi połączy się później

unsigned long lastSensorTriggerTime = 0;
bool isHoldTimerActive = false;

unsigned long lastPollTime = 0;
int currentPollAddress = 0;
int lampErrorState[64] = {0};
bool hasAnyError = false;

// v1.6: mapa miejsc i tryb serwisowy (logika na końcu pliku, sekcja MIEJSCA L/P)
uint64_t mapaMiejsc = 0;      // bit n = adres DALI n przypisany do miejsca (L/P) w kaskadzie
bool trybSerwisowy = false;   // sieć WiFi sterownika i strona serwisowa włączone
int mrugajAdr = -1;           // oprawa migająca na życzenie serwisu (inne oprawy i czujniki wstrzymane)
extern uint64_t obecneAdresy;  // v1.8: definicje w sekcji skanu (koniec pliku)
extern uint32_t bootId;        // v1.9: definicja w sekcji kolejki operacji
extern bool skanWykonany;
int testKaskady = 0;          // 1 = jak START1, 2 = jak START2 (test smugi ze strony serwisowej)

enum DisplayMode { DISP_STATUS, DISP_TRYB, DISP_CZAS, DISP_CZAS_SWIECENIA, DISP_CYKLE, DISP_SKANER, DISP_SERWIS, DISP_RESET, DISP_RESET_SYS, DISP_WERSJA };
DisplayMode currentDisplay = DISP_STATUS;
unsigned long displayReturnTimer = 0;
unsigned long lastLcdUpdate = 0;

unsigned long czasOstatniejAktywnosci = 0;
const unsigned long CZAS_DO_USPIENIA = 300000;
bool ekranUspiony = false;

const int Y_LOGO_OFFSET = 2;

// --- DANE LOGO ---
const unsigned char logo_nowe_Euroasfalt[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x03,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0xff,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x38,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xc0,0x00,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xf8,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x07,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x03,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x18,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3c,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7c,0x00,0xe0,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0xe0,0x00,0x00,0x01,0xfc,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0xe3,0x80,
  0x00,0x03,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0xe3,0x80,0x10,0x03,0x02,0x00,0x60,0x3c,0x39,0xcc,0xc7,0xc0,0xf0,0x78,0x70,0xf0,0xef,0xe0,
  0x38,0x00,0x00,0x00,0xe0,0xfe,0x39,0xcf,0xcf,0xe3,0xf8,0xfd,0xfd,0xfc,0xef,0xe0,0x3e,0x00,0x00,0x01,0xe0,0xff,0x39,0xcf,0xdf,0xf3,0xf9,0xfd,0xfd,0xfc,0xef,0xe0,
  0x37,0x80,0x30,0x0f,0x61,0xc7,0x39,0xcf,0x1c,0x70,0x1d,0xc0,0x70,0x1c,0xe3,0x80,0x33,0xf0,0x78,0x7f,0x61,0xff,0x39,0xce,0x00,0x01,0xfd,0xf8,0x71,0xfe,0xe3,0x80,
  0x32,0x7f,0xff,0xf3,0x61,0xff,0x39,0xce,0x1c,0x73,0xfc,0xfc,0x73,0xfe,0xe3,0x80,0x32,0x1f,0xff,0xc2,0x61,0xc0,0x39,0xce,0x1c,0x77,0x1c,0x1e,0x73,0x0e,0xe3,0x80,
  0x3f,0x18,0x00,0xc3,0xe1,0xfe,0x3f,0xce,0x1f,0xf7,0x1c,0x7e,0x73,0x0e,0xe3,0xe0,0x1f,0x18,0x00,0xc7,0xc0,0xfe,0x3f,0x8e,0x0f,0xe7,0xfc,0xfc,0x73,0xfe,0xe3,0xe0,
  0x1f,0x18,0x00,0xcf,0xc0,0x7c,0x1f,0x0e,0x07,0xc3,0xf9,0xf8,0x71,0xfc,0xe1,0xe0,0x1b,0xf8,0x00,0xfe,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x0d,0xfc,0x01,0xfc,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0d,0x9f,0xff,0xcd,0x80,0x80,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,
  0x07,0x8d,0xfd,0x8f,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x20,0x07,0xcc,0x01,0x9f,0x00,0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x04,0x00,0x00,0x20,
  0x03,0xcc,0x01,0x9e,0x01,0x1e,0x79,0xec,0xbc,0xc9,0xe7,0x81,0xe7,0x9e,0x79,0xe0,0x01,0xe6,0x03,0x3c,0x01,0x36,0x49,0x25,0x24,0x89,0x36,0xc3,0x64,0x92,0x92,0x60,
  0x00,0xf6,0x03,0x78,0x01,0x26,0xca,0x25,0x64,0x89,0x34,0xc2,0x6c,0xbc,0x92,0x60,0x00,0x7f,0x07,0xf0,0x01,0x24,0x9a,0x27,0x64,0x89,0x24,0x82,0x49,0xb0,0x92,0x40,
  0x00,0x1f,0x87,0xc0,0x01,0x24,0x93,0x66,0x6c,0x89,0x64,0x83,0x49,0x34,0xb2,0xc0,0x00,0x07,0xff,0x00,0x01,0x24,0x91,0xc6,0x34,0xc0,0xc4,0x81,0x88,0x1c,0x51,0x80,
  0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// --- PROTOTYPY FUNKCJI ---
void sendDaliCommand16(uint8_t address, uint8_t command);
bool checkDaliResponse();
void setSearchAddress(uint32_t addr);
bool checkAddressMatch(uint32_t addr);
void runDaliCommissioning();
void runFactoryReset();
void runSystemReset();
void rysujLogoEuroasfalt();
void ekranPowitalny();
// v1.6: mapa miejsc L/P i strona serwisowa (definicje na końcu pliku)
bool miejsceNadane(int adr);
String etykietaMiejsca(int adr);
int policzMiejsca();
int policzWRzedzie(int rzad);
int nastepneMiejsce(int i);
void wczytajMapeMiejsc();
bool zapiszMapeMiejsc();
void zarejestrujStroneSerwisowa();
void obsluzSerwis();
void wlaczSerwis();
void wylaczSerwis();
void rysujEkranSerwisu();
void zakonczMruganie();
void obsluzMruganie();
bool skanujMagistrale();
bool zmienAdres(int z, int na);
String przypiszMiejsce(int z, int na);
String odepnijMiejsce(int adr);
void obsluzSerwisWTle();
void dziennik(const String& t);
String sprawdzPodwojne();
bool odpowiada(int adr);
bool programujZaznaczone(int cel);
int wolnyTymczasowySprawdzony(uint64_t& zajete);
void odczytajObecne();
void odswiezAwarieMiejsc();
void czekajObslugujac(unsigned long ms);
String zamienMiejsca(int a, int b);
uint64_t doPrzypisania();
int policz64(uint64_t m);

// Krótka podpowiedź mniejszą czcionką (6x12, z polskimi znakami): unifont ma 8 px na znak,
// więc mieści tylko 16 znaków w linii, a dłuższe napisy były ucinane z obu stron.
void rysujPodpowiedz(int y, const char* tekst) {
  u8g2.setFont(u8g2_font_6x12_te);
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(tekst)) / 2, y, tekst);
  u8g2.setFont(u8g2_font_unifont_t_polish);
}

void obudzEkran() {
  czasOstatniejAktywnosci = millis();
  if (ekranUspiony) { u8g2.setPowerSave(0); ekranUspiony = false; }
}

int getOffLevel() { return 0; }

String readEmailFromEEPROM() {
  String email = "";
  for (int i = 0; i < 64; ++i) { char c = char(EEPROM.read(ADDR_EMAIL + i)); if (c == 0 || c == 255) break; email += c; }
  if (email.length() < 5 || email.indexOf('@') == -1) return "pawel.krajewski@euroasfalt.pl";
  return email;
}

void saveEmailToEEPROM(String email) {
  for (int i = 0; i < 64; ++i) EEPROM.write(ADDR_EMAIL + i, i < email.length() ? email[i] : 0);
  EEPROM.commit();
}

String readLocationFromEEPROM() {
  String loc = "";
  for (int i = 0; i < 64; ++i) { char c = char(EEPROM.read(ADDR_LOCATION + i)); if (c == 0 || c == 255) break; loc += c; }
  if (loc.length() < 2) return "Miasto Ulica";
  return loc;
}

void saveLocationToEEPROM(String loc) {
  for (int i = 0; i < 64; ++i) EEPROM.write(ADDR_LOCATION + i, i < loc.length() ? loc[i] : 0);
  EEPROM.commit();
}

// v1.16: tekst z EEPROM (0 albo 0xFF = koniec; czysty EEPROM = pusty tekst)
String czytajTekstEEPROM(int adr, int dl) {
  String t = "";
  for (int i = 0; i < dl; ++i) { char c = char(EEPROM.read(adr + i)); if (c == 0 || c == (char)255) break; t += c; }
  return t;
}

void zapiszTekstEEPROM(int adr, int dl, const String& t) {
  for (int i = 0; i < dl; ++i) EEPROM.write(adr + i, i < (int)t.length() ? t[i] : 0);
  EEPROM.commit();
}

void configModeCallback(WiFiManager *myWiFiManager) {
  u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
  u8g2.drawUTF8((128 - u8g2.getUTF8Width("Skonfiguruj WiFi")) / 2, 0, "Skonfiguruj WiFi");
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(SIEC_KONFIGURACJI)) / 2, 22, SIEC_KONFIGURACJI);
  u8g2.drawUTF8((128 - u8g2.getUTF8Width("IP: 192.168.4.1")) / 2, 44, "IP: 192.168.4.1");
  u8g2.sendBuffer();
}

void handleApiStroboskop() {
  int durationSec = 5; 
  if (server.hasArg("czas")) {
    durationSec = server.arg("czas").toInt();
    if (durationSec <= 0 || durationSec > 60) durationSec = 5;
  }
  isRadarAlarmActive = true;
  radarAlarmEndTime = millis() + (durationSec * 1000UL);
  isCascadeActive = false;
  isHoldTimerActive = false;
  currentCascadeStep = -1;
  wymusOled = true;
  obudzEkran();
  server.send(200, "text/plain", "ALARM ZAAKCEPTOWANY. STROBOSKOP AKTYWNY NA: " + String(durationSec) + "s.");
}

bool lampaUszkodzona(int i) { return miejsceNadane(i + DALI_ADDR_OFFSET) && lampErrorState[i] >= LAMP_ERROR_THRESHOLD; }

// Czas z NTP (strefa Europe/Warsaw z czasem letnim) i ustawienie nim zegara DS3231.
// Czeka tylko przy pierwszym pobraniu (max 15 s); później ESP trzyma czas sam.
void synchronizujCzas() {
  const time_t CZAS_WAZNY = 1700000000; // listopad 2023
  configTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr); int ntpTimeout = 0;
  while (now < CZAS_WAZNY && ntpTimeout < 60) { delay(250); now = time(nullptr); ntpTimeout++; }
  if (now < CZAS_WAZNY || !rtcDostepny) return;
  struct tm lokalny; localtime_r(&now, &lokalny);
  rtc.adjust(DateTime(lokalny.tm_year + 1900, lokalny.tm_mon + 1, lokalny.tm_mday, lokalny.tm_hour, lokalny.tm_min, lokalny.tm_sec));
}

bool wyslijRaportEmail() {
  // Bez czekania na sieć: ESP łączy się ponownie sam, a próba wraca przy następnym terminie.
  if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) return false;

  obudzEkran(); u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish);
  const char* m1 = "Pobieram czas..."; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m1)) / 2, 35, m1); u8g2.sendBuffer();

  synchronizujCzas();

  u8g2.clearBuffer(); const char* m2 = "Wysyłam maila..."; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m2)) / 2, 35, m2); u8g2.sendBuffer();

  MailClient.networkReconnect(true);
  Session_Config config;
  config.server.host_name = SMTP_HOST; config.server.port = SMTP_PORT;
  config.login.email = SENDER_EMAIL; config.login.password = SENDER_PASSWORD;
  config.login.user_domain = F("127.0.0.1"); config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 1; config.time.day_light_offset = 0;

  SMTP_Message message;
  message.sender.name = F("Sterownik " NAZWA_PRODUKTU); message.sender.email = SENDER_EMAIL;
  
  if (hasAnyError || LAMPS_COUNT == 0) message.subject = "[AWARIA] Raport " NAZWA_PRODUKTU ": " + NAZWA_LOKALIZACJI;
  else message.subject = "Raport dobowy " NAZWA_PRODUKTU ": " + NAZWA_LOKALIZACJI;
  
  message.addRecipient(F("Serwis"), recipientEmail.c_str());

  String tresc = "RAPORT DOBOWY " NAZWA_PRODUKTU "\nLokalizacja: " + NAZWA_LOKALIZACJI + "\n";
  if (rtcDostepny) {
    DateTime nowRtc = rtc.now(); char buforGodziny[10]; snprintf(buforGodziny, sizeof(buforGodziny), "%02d:%02d", nowRtc.hour(), nowRtc.minute());
    tresc += "Czas wygenerowania: " + String(buforGodziny) + "\n";
  }
  
  tresc += "\n=== USTAWIENIA STEROWNIKA ===\n";
  tresc += "Przypisanych oczek: " + String(policzMiejsca()) + " (rząd L: " + String(policzWRzedzie(0)) + ", rząd P: " + String(policzWRzedzie(1)) + ")\n";
  tresc += "Tryb animacji: " + String(modeNames[waveMode]) + "\n";
  
  int totalMs = czasKroku10ms[waveMode] * 10; int intPart = totalMs / 1000; int fracPart = (totalMs % 1000) / 10;
  char buforCzasu[10]; snprintf(buforCzasu, sizeof(buforCzasu), "%d.%02d", intPart, fracPart);
  tresc += "Czas kroku kaskady: " + String(buforCzasu) + " sek.\n";
  tresc += "Liczba cykli po detekcji: " + String(targetCycles[waveMode]) + "\n";
  tresc += "Czas podtrzymania (dla str/wsz): " + String(holdTimeSec[waveMode]) + " sek.\n";
  
  tresc += "\n=== STATUS SYSTEMU ===\n";
  if (LAMPS_COUNT == 0) {
    tresc += "!!! BRAK PRZYPISANYCH OCZEK !!!\nKaskada i Strumień nie działają, oczka nie są sprawdzane.\nWykonaj SKANER DALI na sterowniku.\n";
  } else if (hasAnyError) {
    tresc += "!!! WYKRYTO AWARIĘ OCZEK !!!\nUszkodzone oczka (L = rząd lewy, P = prawy, patrząc od szafy): ";
    for (int i = 0; i < LAMPS_COUNT; i++) if (lampaUszkodzona(i)) tresc += etykietaMiejsca(i + DALI_ADDR_OFFSET) + " (oczko " + String(i + 1) + "), ";
    tresc += "\nZalecana inspekcja w terenie.\n";
  } else {
    tresc += "Stan techniczny: 100% sprawny. Brak awarii.\n";
  }

  tresc += "\n-----------------------------------\n";
  tresc += "Wersja oprogramowania: " NAZWA_PRODUKTU " v" FW_VERSION "\n";
  if (trybOffline) tresc += "Tryb pracy: OFFLINE\n";
  tresc += "eaCloud: " + chmuraOpis + "\n";

  message.text.content = tresc.c_str(); message.text.transfer_encoding = "base64"; message.text.charSet = F("utf-8");
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal; 
  // Unikalny Message-ID: przy stałym Gmail traktował kolejne raporty jako duplikaty.
  char messageId[80]; snprintf(messageId, sizeof(messageId), "Message-ID: <s4bike.%06x.%lu.%lu@smartpass>", ESP.getChipId(), (unsigned long)time(nullptr), millis());
  message.addHeader(messageId);

  bool pomyslnaWysylka = false;
  if (smtp.connect(&config)) {
    if (MailClient.sendMail(&smtp, &message)) {
      u8g2.clearBuffer(); const char* okMsg = "Raport wysłany!"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(okMsg)) / 2, 35, okMsg); u8g2.sendBuffer();
      pomyslnaWysylka = true; 
    } else {
      u8g2.clearBuffer(); const char* errMsg = "Błąd wysyłki!"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(errMsg)) / 2, 35, errMsg); u8g2.sendBuffer();
    }
    smtp.sendingResult.clear();
  } else {
    u8g2.clearBuffer(); const char* errSerwera = "Błąd serwera!"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(errSerwera)) / 2, 35, errSerwera); u8g2.sendBuffer();
  }
  delay(1500); u8g2.clearBuffer(); wymusOled = true; return pomyslnaWysylka;
}

// ==========================================
// v1.16: WYSYŁKA DANYCH DO eaCloud (kontrakt v2: docs/INGEST-KONTRAKT.md w repozytorium eaCloud)
// ==========================================
// Co CHMURA_OKRES_MS jedna wiadomość: field1 = rowerzyści od uruchomienia (licznik narastający – eaCloud liczy przyrosty
// między odczytami, a spadek wartości traktuje jak zerowanie), field2 = czas pracy [s].
// Wiadomość bez odpowiedzi serwera jest ponawiana bez zmian (ten sam message_id, serwer nie zapisze jej dwa razy). Odpowiedź „0” (zły klucz albo dane) – bez ponawiania, wpis w dzienniku.
const unsigned long CHMURA_OKRES_MS = 30000UL;
const uint16_t CHMURA_LIMIT_MS = 1000;        // limit czasu nawiązania połączenia (sterownik w tym czasie nie czyta czujników)
const unsigned long CHMURA_ODPOWIEDZ_MS = 15000UL; // odpowiedź serwera odbierana w tle, najwyżej tyle
const unsigned long CHMURA_MAX_PRZERWA_MS = 5UL * 60UL * 1000UL; // serwer nie odpowiada: przerwa rośnie 30 s, 60 s … do 5 min
const unsigned long CZUJNIK_PRZERWA_MS = 500; // krótsza przerwa w sygnale czujnika = ten sam rowerzysta

String chmuraUrl = "", chmuraKlucz = "";
uint32_t rowerzysciRazem = 0;        // od uruchomienia (field1, strona serwisowa)
uint32_t chmuraNr = 0;               // numer wiadomości w tym uruchomieniu
bool chmuraCzeka = false;            // wiadomość wysłana bez odpowiedzi – ponowić
uint32_t chmuraCzekaRowerzysci = 0; unsigned long chmuraCzekaCzasPracy = 0; time_t chmuraCzekaCzas = 0; String chmuraCzekaId = "";
bool sterownikZajety();

// Wejście czujnika (START1 lub START2): nowy rowerzysta po co najmniej CZUJNIK_PRZERWA_MS bez sygnału.
void liczRowerzystow(bool czujnik) {
  static bool byl = false; static unsigned long koniecSygnalu = 0;
  if (czujnik && !byl && millis() - koniecSygnalu >= CZUJNIK_PRZERWA_MS) { rowerzysciRazem++; }
  if (!czujnik && byl) koniecSygnalu = millis();
  byl = czujnik;
}

void ustawOpisChmury(const String& t) {
  if (t != chmuraOpis) { chmuraOpis = t; dziennik("eaCloud: " + t); }
}

// v1.16: adres http://host[:port]/ścieżka (bez HTTPS)
bool rozbierzUrl(const String& u, String& host, uint16_t& port, String& sciezka) {
  if (!u.startsWith("http://")) return false;
  String r = u.substring(7); int ukosnik = r.indexOf('/');
  String hp = ukosnik < 0 ? r : r.substring(0, ukosnik); sciezka = ukosnik < 0 ? "/" : r.substring(ukosnik);
  int dwukropek = hp.indexOf(':');
  host = dwukropek < 0 ? hp : hp.substring(0, dwukropek); port = dwukropek < 0 ? 80 : hp.substring(dwukropek + 1).toInt();
  return host.length() > 0 && port > 0;
}

// Wysyłka bez czekania na odpowiedź: żądanie idzie od razu po połączeniu, odpowiedź serwera jest odbierana w kolejnych
// obiegach loop() (obsluzOdpowiedzChmury). Lokalny serwer odpowiadał po ~2,7 s – czekanie blokowało czujniki.
WiFiClient chmuraKlient;
bool chmuraWToku = false; unsigned long chmuraWyslano = 0; String chmuraOdp = "";
unsigned long chmuraOstatnia = 0, chmuraPrzerwa = CHMURA_OKRES_MS;

void zakonczWysylkeChmury(int kod, const String& tresc) {
  chmuraKlient.stop(); chmuraWToku = false; chmuraOdp = "";
  chmuraPrzerwa = kod > 0 ? CHMURA_OKRES_MS : min(chmuraPrzerwa * 2, CHMURA_MAX_PRZERWA_MS);
  if (kod == 200 && tresc == "1") { chmuraCzeka = false; ustawOpisChmury("wysyłam co 30 s"); }
  else if (kod == 200 && tresc == "0") { chmuraCzeka = false; ustawOpisChmury("serwer odrzucił dane (klucz?)"); }
  else if (kod > 0) ustawOpisChmury("błąd serwera HTTP " + String(kod) + ", ponawiam");
  else ustawOpisChmury("brak odpowiedzi serwera, ponawiam");
}

void obsluzOdpowiedzChmury() {
  if (!chmuraWToku) return;
  while (chmuraKlient.available() && chmuraOdp.length() < 1024) chmuraOdp += (char)chmuraKlient.read();
  bool koniec = !chmuraKlient.connected() && !chmuraKlient.available();
  if (!koniec && millis() - chmuraWyslano < CHMURA_ODPOWIEDZ_MS) return;
  // HTTP/1.0: serwer zamyka połączenie po odpowiedzi; „HTTP/1.1 200 OK\r\n…\r\n\r\n1”
  int kod = chmuraOdp.startsWith("HTTP/") ? chmuraOdp.substring(chmuraOdp.indexOf(' ') + 1).toInt() : 0;
  int poczatek = chmuraOdp.indexOf("\r\n\r\n");
  String tresc = poczatek < 0 ? "" : chmuraOdp.substring(poczatek + 4); tresc.trim();
  zakonczWysylkeChmury(koniec ? kod : 0, tresc);
}

void wyslijDoChmury() {
  if (chmuraUrl.length() == 0 || trybOffline || chmuraWToku) return;
  if (chmuraOstatnia != 0 && millis() - chmuraOstatnia < chmuraPrzerwa) return;
  // nie w trakcie świecenia, alarmu, sieci serwisowej (tylko AP) ani operacji na magistrali – spróbuje w następnym obiegu loop()
  if (isCascadeActive || isHoldTimerActive || isRadarAlarmActive || trybSerwisowy || sterownikZajety()) return;
  chmuraOstatnia = millis();
  if (WiFi.status() != WL_CONNECTED) { ustawOpisChmury("brak Wi-Fi, czekam"); return; }
  String host, sciezka; uint16_t port;
  if (!rozbierzUrl(chmuraUrl, host, port, sciezka)) { ustawOpisChmury("zły adres (tylko http://)"); return; }

  if (!chmuraCzeka) {
    static bool czasNtp = false;
    if (!czasNtp) { configTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov"); czasNtp = true; } // bez czekania: czas w tle
    time_t teraz = time(nullptr);
    chmuraCzekaRowerzysci = rowerzysciRazem;
    chmuraCzekaCzasPracy = millis() / 1000UL;
    chmuraCzekaCzas = teraz > 1700000000 ? teraz : 0;   // bez ważnego czasu serwer przyjmie czas odbioru
    char id[48]; snprintf(id, sizeof(id), "s4dali-%06x-%08x-%lu", ESP.getChipId(), bootId, (unsigned long)++chmuraNr);
    chmuraCzekaId = id; chmuraCzeka = true;
  }

  String tresc = "field1=" + String(chmuraCzekaRowerzysci) + "&field2=" + String(chmuraCzekaCzasPracy) + "&message_id=" + chmuraCzekaId;
  if (chmuraCzekaCzas) tresc += "&measured_at=" + String((unsigned long)chmuraCzekaCzas);

  chmuraKlient.setTimeout(CHMURA_LIMIT_MS);   // limit samego połączenia (serwer w sieci lokalnej: kilka ms)
  if (!chmuraKlient.connect(host.c_str(), port)) {
    chmuraKlient.stop(); chmuraPrzerwa = min(chmuraPrzerwa * 2, CHMURA_MAX_PRZERWA_MS); ustawOpisChmury("brak połączenia z serwerem, ponawiam"); return;
  }
  chmuraKlient.print(String("POST ") + sciezka + " HTTP/1.0\r\nHost: " + host + ":" + String(port) +
                     "\r\nX-SCCLOUDAPIKEY: " + chmuraKlucz + "\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: " +
                     String(tresc.length()) + "\r\nConnection: close\r\n\r\n" + tresc);
  chmuraWToku = true; chmuraWyslano = millis(); chmuraOdp = "";
}

bool isNightTime() {
  if (!rtcDostepny) return true;
  DateTime now = rtc.now(); bool isDST = false;
  int m = now.month(), d = now.day(), dow = now.dayOfTheWeek();
  if (m > 3 && m < 10) isDST = true; else if (m == 3 && (d - dow) >= 25) isDST = true; else if (m == 10 && (d - dow) < 25) isDST = true;
  Dusk2Dawn lokalnyZegar(gpsLat / 100.0, gpsLon / 100.0, 1);
  int sunrise = lokalnyZegar.sunrise(now.year(), now.month(), now.day(), isDST); int sunset  = lokalnyZegar.sunset(now.year(), now.month(), now.day(), isDST);
  if (sunrise < 0 || sunset < 0) { int currentMinute = now.hour() * 60 + now.minute(); return (currentMinute >= 17 * 60 || currentMinute < 6 * 60); }
  sunset += SUNSET_OFFSET; sunrise += SUNRISE_OFFSET; int currentMinute = now.hour() * 60 + now.minute();
  return (currentMinute >= sunset || currentMinute < sunrise);
}

void pewneWygaszenie() {
  int offLvl = getOffLevel();
  sendDaliCommand16(DALI_BROADCAST_DAPC, offLvl); delay(15);
  sendDaliCommand16(DALI_BROADCAST_DAPC, offLvl); delay(15);
  sendDaliCommand16(DALI_BROADCAST_DAPC, offLvl);
}

// POPRAWKA: Usunięcie blokującego 150ms pewnegoWygaszenia na rzecz szybkiego resetu w locie
void endAnimationCycle() {
  currentCycleCount++; wymusOled = true; 
  bool isSensorStillActive = (digitalRead(PIN_START1) == LOW) || (digitalRead(PIN_START2) == LOW);
  if (currentCycleCount >= targetCycles[waveMode]) {
    if (isSensorStillActive) { 
      currentCycleCount = 0; 
      // Szybki reset (jeden broadcast, brak delay) dla Kaskady. Strumień tego nie potrzebuje, bo ma ogon.
      if (waveMode == 0) { sendDaliCommand16(DALI_BROADCAST_DAPC, getOffLevel()); } 
      cascadeStartTime = millis(); currentCascadeStep = -1; 
    } 
    else { 
      // Twarde, ostateczne wygaszenie bezpieczeństwa TYLKO na sam koniec działania
      isCascadeActive = false; pewneWygaszenie(); currentCycleCount = 0; 
    }
  } else { 
    // Szybki reset miedzy kolejnymi cyklami (brak delay) dla Kaskady
    if (waveMode == 0) { sendDaliCommand16(DALI_BROADCAST_DAPC, getOffLevel()); } 
    cascadeStartTime = millis(); currentCascadeStep = -1; 
  }
}

void setup() {
  u8g2.begin(); u8g2.enableUTF8Print(); Wire.begin(D6, D5);
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() == 0) { rtcDostepny = true; rtc.begin(); Wire.begin(D6, D5); if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); } 
  else { rtcDostepny = false; }
  Wire.setClock(400000); 

  pinMode(DALI_RX_PIN, INPUT); pinMode(AQY_CTRL_PIN, OUTPUT); digitalWrite(AQY_CTRL_PIN, HIGH); pinMode(DALI_TX_PIN, OUTPUT); digitalWrite(DALI_TX_PIN, HIGH);
  // D0 (GPIO16) nie ma wewnętrznego podciągnięcia w ESP8266 – czujnik START2 wymaga rezystora 10k do 3V3 na płytce.
  pinMode(PIN_START1, INPUT_PULLUP); pinMode(PIN_START2, INPUT); pinMode(PIN_PRZYCISK_S1, INPUT_PULLUP); pinMode(PIN_PRZYCISK_S2, INPUT_PULLUP); pinMode(PIN_PRZYCISK_S3, INPUT_PULLUP);

  ekranPowitalny();

  if (!rtcDostepny) {
    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish);
    u8g2.drawUTF8((128 - u8g2.getUTF8Width("BŁĄD ZEGARA")) / 2, 10, "BŁĄD ZEGARA"); u8g2.drawUTF8((128 - u8g2.getUTF8Width("Działa awaryjnie")) / 2, 35, "Działa awaryjnie"); u8g2.sendBuffer(); delay(2500);
  }

  EEPROM.begin(512); 
  bootId = ESP.random() | 1;
  dziennik("Start sterownika v" FW_VERSION ", przyczyna: " + ESP.getResetReason());
  LAMPS_COUNT = EEPROM.read(ADDR_LAMPS_COUNT); if (LAMPS_COUNT > MAX_LAMPS) LAMPS_COUNT = 0; // 0xFF = czysty EEPROM, wymaga skanu
  wczytajMapeMiejsc(); // v1.6: przy pierwszym starcie mapa z dotychczasowej liczby opraw (adresy 10..)
  waveMode = EEPROM.read(ADDR_WAVE_MODE); if (waveMode > 3) waveMode = 0;

  for (int i = 0; i < 4; i++) {
    czasKroku10ms[i] = EEPROM.read(ADDR_CYCLE_TIME + i); 
    if (czasKroku10ms[i] < 5 || czasKroku10ms[i] > 100 || czasKroku10ms[i] % 5 != 0) czasKroku10ms[i] = 10; 
    targetCycles[i] = EEPROM.read(ADDR_TARGET_CYCLES + i); 
    if (targetCycles[i] < 1 || targetCycles[i] > 10) targetCycles[i] = 1;
    holdTimeSec[i] = EEPROM.read(ADDR_HOLD_TIME + i); 
    if (holdTimeSec[i] < 1 || holdTimeSec[i] > 30) holdTimeSec[i] = 15;
  }

  trybOffline = (EEPROM.read(ADDR_WIFI_MODE) == 1);
  recipientEmail = readEmailFromEEPROM(); NAZWA_LOKALIZACJI = readLocationFromEEPROM();
  chmuraUrl = czytajTekstEEPROM(ADDR_CHMURA_URL, CHMURA_URL_DL); chmuraKlucz = czytajTekstEEPROM(ADDR_CHMURA_KLUCZ, CHMURA_KLUCZ_DL);
  if (strlen(EACLOUD_URL)) chmuraUrl = EACLOUD_URL;
  if (strlen(EACLOUD_KEY)) chmuraKlucz = EACLOUD_KEY;
  if (chmuraUrl.length()) chmuraOpis = "czekam na pierwszą wysyłkę";

  u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish); 
  
  // Portal konfiguracyjny tylko na żądanie: brak zapisanej sieci albo S2 wciśnięty podczas startu.
  // Wcześniej każdy brak sieci (np. router wstający po zaniku zasilania) blokował sterownik na 3 min.
  bool wymusPortal = (digitalRead(PIN_PRZYCISK_S2) == LOW);

  if (trybOffline && !wymusPortal) {
      const char* msg1 = "TRYB OFFLINE"; 
      u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg1)) / 2, 10, msg1); rysujPodpowiedz(40, "S2 = konfiguracja"); u8g2.sendBuffer();
      WiFi.mode(WIFI_OFF);
      for (int i = 0; i < 40 && !wymusPortal; i++) { delay(50); wymusPortal = (digitalRead(PIN_PRZYCISK_S2) == LOW); }
      u8g2.clearBuffer(); wymusOled = true; u8g2.sendBuffer();
  }

  if (!trybOffline || wymusPortal) {
      const char* msg1 = "Łączenie WiFi..."; u8g2.clearBuffer(); u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg1)) / 2, 22, msg1); u8g2.sendBuffer();

      WiFi.mode(WIFI_STA); 
      WiFi.setSleepMode(WIFI_NONE_SLEEP);
      WiFi.setAutoReconnect(true);

      bool connected = false;
      if (WiFi.SSID() != "" && !wymusPortal) {
          WiFi.begin();
          for (int i = 0; i <= 30; i++) { 
              if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
              if (digitalRead(PIN_PRZYCISK_S2) == LOW) { wymusPortal = true; break; }
              u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
              u8g2.drawUTF8((128 - u8g2.getUTF8Width("Łączenie WiFi...")) / 2, 0, "Łączenie WiFi...");
              String czas = "Czas: " + String(15 - (i / 2)) + "s"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(czas.c_str())) / 2, 22, czas.c_str());
              rysujPodpowiedz(48, "S2 = konfiguracja");
              u8g2.sendBuffer(); delay(500);
          }
      }
      bool otworzPortal = wymusPortal || WiFi.SSID() == "";

      WiFiManager wm;
      wm.setDebugOutput(false);
      wm.setAPCallback(configModeCallback);
      wm.setConfigPortalTimeout(180);        
      
      String showPassJS = "<" "script>window.onload=function(){var p=document.getElementById('p');if(p)p.type='text';};<" "/script>";
      wm.setCustomHeadElement(showPassJS.c_str());
      
      WiFiManagerParameter custom_email("email", "Email do raportow", recipientEmail.c_str(), 64);
      WiFiManagerParameter custom_location("location", "Nazwa Szafy/Lokalizacji", NAZWA_LOKALIZACJI.c_str(), 64);
      WiFiManagerParameter custom_text("<" "hr><" "br><" "b>TRYB PRACY STEROWNIKA:<" "/b><" "br>Wpisz <" "b>0<" "/b> -> Tryb ONLINE (WiFi + Raporty)<" "br>Wpisz <" "b>1<" "/b> -> Tryb OFFLINE (Brak sieci)<" "br><" "br>");
      WiFiManagerParameter custom_mode("w_mode", "0 lub 1", trybOffline ? "1" : "0", 2); 
      WiFiManagerParameter custom_chmura_text("<" "hr><" "b>eaCloud<" "/b> (puste = bez wysyłki)<" "br>");
      WiFiManagerParameter custom_chmura_url("c_url", "Adres, np. http://192.168.1.50:8198/update", chmuraUrl.c_str(), CHMURA_URL_DL);
      WiFiManagerParameter custom_chmura_klucz("c_key", "Klucz urządzenia", chmuraKlucz.c_str(), CHMURA_KLUCZ_DL);

      wm.addParameter(&custom_email); wm.addParameter(&custom_location);
      wm.addParameter(&custom_text); wm.addParameter(&custom_mode);
      wm.addParameter(&custom_chmura_text); wm.addParameter(&custom_chmura_url); wm.addParameter(&custom_chmura_klucz);

      bool portalZapisany = false;
      if (otworzPortal) {
        while (digitalRead(PIN_PRZYCISK_S2) == LOW) { delay(10); }
        portalZapisany = wm.startConfigPortal(SIEC_KONFIGURACJI);
        connected = portalZapisany;
      }

      if (portalZapisany) {
        String newEmail = custom_email.getValue(); if (newEmail != recipientEmail && newEmail.indexOf('@') != -1) { recipientEmail = newEmail; saveEmailToEEPROM(recipientEmail); }
        String newLoc = custom_location.getValue(); if (newLoc != NAZWA_LOKALIZACJI && newLoc.length() > 2) { NAZWA_LOKALIZACJI = newLoc; saveLocationToEEPROM(NAZWA_LOKALIZACJI); }
        
        String newUrl = custom_chmura_url.getValue(); newUrl.trim();
        String newKey = custom_chmura_klucz.getValue(); newKey.trim();
        if (newUrl != chmuraUrl) { chmuraUrl = newUrl; zapiszTekstEEPROM(ADDR_CHMURA_URL, CHMURA_URL_DL, chmuraUrl); }
        if (newKey != chmuraKlucz) { chmuraKlucz = newKey; zapiszTekstEEPROM(ADDR_CHMURA_KLUCZ, CHMURA_KLUCZ_DL, chmuraKlucz); }
        if (strlen(EACLOUD_URL)) chmuraUrl = EACLOUD_URL;
        if (strlen(EACLOUD_KEY)) chmuraKlucz = EACLOUD_KEY;
        chmuraOpis = chmuraUrl.length() ? "czekam na pierwszą wysyłkę" : "wyłączona (brak adresu)";

        int wModeVal = String(custom_mode.getValue()).toInt();
        if (wModeVal == 1) { EEPROM.write(ADDR_WIFI_MODE, 1); EEPROM.commit(); trybOffline = true; WiFi.disconnect(true); delay(200); WiFi.mode(WIFI_OFF); connected = false; } 
        else { EEPROM.write(ADDR_WIFI_MODE, 0); EEPROM.commit(); trybOffline = false; }
      }

      u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
      if (trybOffline) {
        WiFi.mode(WIFI_OFF);
        const char* msg2 = "TRYB OFFLINE"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg2)) / 2, 22, msg2);
        u8g2.sendBuffer(); delay(1500); u8g2.clearBuffer();
      } else if (connected) {
        const char* logMsg1 = "ZALOGOWANO!"; const char* logMsg2 = "Połączono z WiFi"; 
        u8g2.drawUTF8((128 - u8g2.getUTF8Width(logMsg1)) / 2, 10, logMsg1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(logMsg2)) / 2, 35, logMsg2);
        u8g2.sendBuffer(); delay(1500);
        if (!wyslijRaportEmail()) raportStartowyDoWyslania = true;
      } else {
        // Brak sieci: sterownik pracuje normalnie, ESP łączy się w tle, raport startowy pójdzie po połączeniu.
        if (WiFi.SSID() != "") WiFi.begin();
        raportStartowyDoWyslania = true;
        const char* msg1b = wifiKrotko(WiFi.status()); const char* msg2 = "Łączę w tle";   // v1.15: z przyczyną
        u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg1b)) / 2, 10, msg1b); u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg2)) / 2, 35, msg2);
        u8g2.sendBuffer(); delay(1500); u8g2.clearBuffer();
      }
  }

  server.on("/api/stroboskop", HTTP_GET, handleApiStroboskop);
  zarejestrujStroneSerwisowa();
  server.begin();

  zbuforowanaNoc = isNightTime(); 
  pewneWygaszenie(); 
  czasOstatniejAktywnosci = millis();
}

void loop() {
  server.handleClient(); 
  obsluzSerwis(); // v1.6: sieć serwisowa, mruganie, zlecony skan, limit czasu (koniec pliku)
  // v1.15: zmiana stanu Wi-Fi → ekran i dziennik (stany przejściowe „łączę” bez wpisu; ten sam opis tylko raz)
  static unsigned long wifiSprawdzono = 0; static int wifiStan = -99; static String wifiZapisany = "";
  if (!trybSerwisowy && millis() - wifiSprawdzono >= 1000) {
    wifiSprawdzono = millis();
    int st = trybOffline ? -2 : (int)WiFi.status();
    if (st != wifiStan) {
      wifiStan = st; wifiOpis = wifiPrzyczyna(st); wymusOled = true;
      if (st != WL_DISCONNECTED && st != WL_IDLE_STATUS && wifiOpis != wifiZapisany) { wifiZapisany = wifiOpis; dziennik("Wi-Fi: " + wifiOpis); }
    }
  }
  unsigned long currentMillis = millis();
  if (currentMillis - ostatniPomiarAstro > 10000 || ostatniPomiarAstro == 0) { zbuforowanaNoc = isNightTime(); ostatniPomiarAstro = currentMillis; }

  static int ostatniDzienRaportu = -1; 
  static unsigned long ostatniaProbaWysylki = 0;

  // Raport startowy, gdy sieci nie było przy starcie (sprawdzane co 60 s, bez blokowania)
  static unsigned long ostatniaProbaStartowego = 0;
  if (raportStartowyDoWyslania && !trybOffline && WiFi.status() == WL_CONNECTED && millis() - ostatniaProbaStartowego >= 60000UL
      && !isCascadeActive && !isHoldTimerActive && !isRadarAlarmActive) {
    ostatniaProbaStartowego = millis();
    if (wyslijRaportEmail()) raportStartowyDoWyslania = false;
  }
  
  if (rtcDostepny) {
    DateTime now = rtc.now();
    if (now.hour() == 1) { 
      if (ostatniDzienRaportu != now.day() && !isCascadeActive && !isHoldTimerActive && !isRadarAlarmActive && !trybOffline) {
        if (millis() - ostatniaProbaWysylki >= RAPORT_PONOW_MS || ostatniaProbaWysylki == 0) {
          ostatniaProbaWysylki = millis(); 
          if (wyslijRaportEmail()) { ostatniDzienRaportu = now.day(); }
        }
      }
    }
  } else {
    static unsigned long stoperAwaryjnyRaportu = 0;
    if (millis() - stoperAwaryjnyRaportu >= 86400000UL && !isCascadeActive && !isHoldTimerActive && !isRadarAlarmActive && !trybOffline) { 
      stoperAwaryjnyRaportu = millis(); 
      wyslijRaportEmail(); 
    }
  }

  bool start1Active = (digitalRead(PIN_START1) == LOW); bool start2Active = (digitalRead(PIN_START2) == LOW); bool startActive = start1Active || start2Active;
  liczRowerzystow(startActive);   // v1.16: przed testem ze strony serwisowej – test nie jest rowerzystą
  obsluzOdpowiedzChmury();
  wyslijDoChmury();
  if (testKaskady) { start1Active = (testKaskady == 1); start2Active = (testKaskady == 2); startActive = true; testKaskady = 0; }
  if (mrugajAdr >= 0) { start1Active = start2Active = startActive = false; } // serwis miga oprawą – bez animacji

  digitalWrite(AQY_CTRL_PIN, HIGH); int offLevel = getOffLevel(); 
  if (startActive && !isRadarAlarmActive) { lastSensorTriggerTime = millis(); isHoldTimerActive = true; currentDisplay = DISP_STATUS; }
  if (isHoldTimerActive && (millis() - lastSensorTriggerTime >= holdTimeSec[waveMode] * 1000UL)) { isHoldTimerActive = false; }

  if (digitalRead(PIN_PRZYCISK_S1) == LOW) {
    obudzEkran();
    if (currentDisplay == DISP_STATUS) currentDisplay = DISP_TRYB;
    else if (currentDisplay == DISP_TRYB) currentDisplay = DISP_CZAS;
    else if (currentDisplay == DISP_CZAS) {
        if (waveMode < 2) currentDisplay = DISP_CYKLE;
        else currentDisplay = DISP_CZAS_SWIECENIA;
    }
    else if (currentDisplay == DISP_CZAS_SWIECENIA) currentDisplay = DISP_SKANER;
    else if (currentDisplay == DISP_CYKLE) currentDisplay = DISP_SKANER;
    else if (currentDisplay == DISP_SKANER) currentDisplay = DISP_SERWIS;
    else if (currentDisplay == DISP_SERWIS) currentDisplay = DISP_RESET;
    else if (currentDisplay == DISP_RESET) currentDisplay = DISP_RESET_SYS;
    else if (currentDisplay == DISP_RESET_SYS) currentDisplay = DISP_WERSJA;
    else if (currentDisplay == DISP_WERSJA) currentDisplay = DISP_STATUS;
    else currentDisplay = DISP_STATUS;
    
    u8g2.clearBuffer(); displayReturnTimer = millis(); delay(200); 
    while (digitalRead(PIN_PRZYCISK_S1) == LOW) { yield(); server.handleClient(); delay(10); } delay(200);
  }
  if (digitalRead(PIN_PRZYCISK_S2) == LOW) {
    obudzEkran();
    if (currentDisplay == DISP_TRYB) { waveMode--; if (waveMode < 0) waveMode = 3; EEPROM.write(ADDR_WAVE_MODE, waveMode); }
    else if (currentDisplay == DISP_CZAS) { czasKroku10ms[waveMode] -= 5; if (czasKroku10ms[waveMode] < 5) czasKroku10ms[waveMode] = 100; EEPROM.write(ADDR_CYCLE_TIME + waveMode, czasKroku10ms[waveMode]); }
    else if (currentDisplay == DISP_CZAS_SWIECENIA) { holdTimeSec[waveMode]--; if (holdTimeSec[waveMode] < 1) holdTimeSec[waveMode] = 30; EEPROM.write(ADDR_HOLD_TIME + waveMode, holdTimeSec[waveMode]); }
    else if (currentDisplay == DISP_CYKLE) { targetCycles[waveMode]--; if (targetCycles[waveMode] < 1) targetCycles[waveMode] = 10; EEPROM.write(ADDR_TARGET_CYCLES + waveMode, targetCycles[waveMode]); }
    EEPROM.commit(); u8g2.clearBuffer(); displayReturnTimer = millis(); delay(200); while (digitalRead(PIN_PRZYCISK_S2) == LOW) { yield(); server.handleClient(); delay(10); } delay(200);
  }
  if (digitalRead(PIN_PRZYCISK_S3) == LOW) {
    obudzEkran(); unsigned long pressStart = millis(); bool isLongPress = false;
    while (digitalRead(PIN_PRZYCISK_S3) == LOW) {
      yield(); server.handleClient(); unsigned long heldTime = millis() - pressStart;
      if (currentDisplay == DISP_SKANER && heldTime >= 3000) {
        isLongPress = true; u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish); u8g2.drawUTF8(10, 25, "Puść przycisk"); u8g2.sendBuffer();
        while (digitalRead(PIN_PRZYCISK_S3) == LOW) { yield(); delay(10); } runDaliCommissioning(); currentDisplay = DISP_STATUS; break;
      } else if (currentDisplay == DISP_SKANER && heldTime > 500) {
        u8g2.clearBuffer(); u8g2.setDrawColor(1); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
        const char* msg1 = "START SKANERA"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg1)) / 2, 10, msg1); String odp = "Czekaj: " + String(3 - (heldTime / 1000)) + "s"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(odp.c_str())) / 2, 35, odp.c_str()); u8g2.sendBuffer();
      } else if (currentDisplay == DISP_SERWIS && heldTime >= 3000) {
        isLongPress = true; u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish); u8g2.drawUTF8(10, 25, "Puść przycisk"); u8g2.sendBuffer();
        while (digitalRead(PIN_PRZYCISK_S3) == LOW) { yield(); delay(10); }
        if (trybSerwisowy) wylaczSerwis(); else wlaczSerwis();
        currentDisplay = DISP_STATUS; wymusOled = true; break;
      } else if (currentDisplay == DISP_SERWIS && heldTime > 500) {   // v1.13: odliczanie jak przy skanerze
        u8g2.clearBuffer(); u8g2.setDrawColor(1); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
        const char* msg1 = trybSerwisowy ? "WYŁĄCZ SERWIS" : "WŁĄCZ SERWIS"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(msg1)) / 2, 10, msg1);
        String odp = "Czekaj: " + String(3 - (heldTime / 1000)) + "s"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(odp.c_str())) / 2, 35, odp.c_str()); u8g2.sendBuffer();
      } else if (currentDisplay == DISP_RESET && heldTime >= 3000) {
        isLongPress = true; u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish); u8g2.drawUTF8(10, 25, "Puść przycisk"); u8g2.sendBuffer();
        while (digitalRead(PIN_PRZYCISK_S3) == LOW) { yield(); delay(10); } runFactoryReset(); currentDisplay = DISP_STATUS; break;
      } else if (currentDisplay == DISP_RESET_SYS && heldTime >= 3000) {
        isLongPress = true; u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish); u8g2.drawUTF8(10, 25, "Puść przycisk"); u8g2.sendBuffer();
        while (digitalRead(PIN_PRZYCISK_S3) == LOW) { yield(); delay(10); } runSystemReset(); currentDisplay = DISP_STATUS; break;
      } delay(10);
    }
    if (!isLongPress && currentDisplay != DISP_SKANER && currentDisplay != DISP_SERWIS && currentDisplay != DISP_RESET && currentDisplay != DISP_RESET_SYS && currentDisplay != DISP_WERSJA) {
      if (currentDisplay == DISP_TRYB) { waveMode++; if (waveMode > 3) waveMode = 0; EEPROM.write(ADDR_WAVE_MODE, waveMode); }
      else if (currentDisplay == DISP_CZAS) { czasKroku10ms[waveMode] += 5; if (czasKroku10ms[waveMode] > 100) czasKroku10ms[waveMode] = 5; EEPROM.write(ADDR_CYCLE_TIME + waveMode, czasKroku10ms[waveMode]); }
      else if (currentDisplay == DISP_CZAS_SWIECENIA) { holdTimeSec[waveMode]++; if (holdTimeSec[waveMode] > 30) holdTimeSec[waveMode] = 1; EEPROM.write(ADDR_HOLD_TIME + waveMode, holdTimeSec[waveMode]); }
      else if (currentDisplay == DISP_CYKLE) { targetCycles[waveMode]++; if (targetCycles[waveMode] > 10) targetCycles[waveMode] = 1; EEPROM.write(ADDR_TARGET_CYCLES + waveMode, targetCycles[waveMode]); }
      EEPROM.commit(); displayReturnTimer = millis(); u8g2.clearBuffer(); delay(200);
    }
  }

  if (currentDisplay != DISP_STATUS && (millis() - displayReturnTimer > 15000)) { currentDisplay = DISP_STATUS; u8g2.clearBuffer(); }

  if (isRadarAlarmActive) {
    if ((long)(millis() - radarAlarmEndTime) >= 0) {
      isRadarAlarmActive = false; pewneWygaszenie(); currentCascadeStep = -1; wymusOled = true;
    } else {
      obudzEkran(); unsigned long stepDurationMs = 100; int strobeState = (millis() / stepDurationMs) % 2;
      if (strobeState != currentCascadeStep) { 
        currentCascadeStep = strobeState; 
        if (strobeState == 0) { sendDaliCommand16(DALI_BROADCAST_DAPC, 254); delay(10); } 
        else { sendDaliCommand16(DALI_BROADCAST_DAPC, getOffLevel()); delay(10); } 
      }
    }
  }
  else {
    if (waveMode == 2) { 
      if (isHoldTimerActive) {
        obudzEkran(); isCascadeActive = true; unsigned long blinkIntervalMs = czasKroku10ms[waveMode] * 10; int blinkState = (millis() / blinkIntervalMs) % 2;
        if (blinkState != currentCascadeStep) { currentCascadeStep = blinkState; if (blinkState == 0) { sendDaliCommand16(DALI_BROADCAST_DAPC, DALI_100_PERCENT); delay(10); } else { sendDaliCommand16(DALI_BROADCAST_DAPC, offLevel); delay(10); } }
      } else if (isCascadeActive) { pewneWygaszenie(); isCascadeActive = false; currentCascadeStep = -1; }
    }
    else if (waveMode == 3) {
      if (isHoldTimerActive) {
        obudzEkran(); isCascadeActive = true; unsigned long stepDurationMs = czasKroku10ms[waveMode] * 10; int strobeState = (millis() / stepDurationMs) % 2;
        if (strobeState != currentCascadeStep) { currentCascadeStep = strobeState; if (strobeState == 0) { sendDaliCommand16(DALI_BROADCAST_DAPC, 253); delay(10); } else { sendDaliCommand16(DALI_BROADCAST_DAPC, offLevel); delay(10); } }
      } else if (isCascadeActive) { pewneWygaszenie(); isCascadeActive = false; currentCascadeStep = -1; }
    }
    else { 
      if (!isCascadeActive && startActive) { 
        obudzEkran(); 
        if (LAMPS_COUNT > 0) { 
          isCascadeActive = true; cascadeStartTime = millis(); currentCascadeStep = -1; currentCycleCount = 0; reverseDirection = start2Active; 
        } 
      }
      
      if (isCascadeActive) {
        unsigned long stepDurationMs = czasKroku10ms[waveMode] * 10; 
        int paraCount = (LAMPS_COUNT + 1) / 2;
        
        if (waveMode == 0) { 
          unsigned long totalDuration = stepDurationMs * paraCount;
          // Koniec cyklu dopiero po zapaleniu ostatniej pary (gdy pętla nie nadąża, nie gubi kroków)
          if ((millis() - cascadeStartTime) >= totalDuration && currentCascadeStep + 1 >= paraCount) { 
            endAnimationCycle(); 
          } else {
            while (currentCascadeStep + 1 < paraCount) {
              yield(); unsigned long timeElapsed = millis() - cascadeStartTime;
              if (timeElapsed < (currentCascadeStep + 1) * stepDurationMs) break; 
              currentCascadeStep++; int actualStep = reverseDirection ? (paraCount - 1 - currentCascadeStep) : currentCascadeStep;
              int lewaLampa  = (actualStep * 2) + DALI_ADDR_OFFSET; int prawaLampa = (actualStep * 2) + 1 + DALI_ADDR_OFFSET;
              sendDaliCommand16((lewaLampa << 1) | 0x00, DALI_100_PERCENT);
              if ((actualStep * 2) + 1 < LAMPS_COUNT) { sendDaliCommand16((prawaLampa << 1) | 0x00, DALI_100_PERCENT); } // przerwę pilnuje sendDaliCommand16
            }
          }
        }
        else if (waveMode == 1) { 
          // KLASYCZNY STRUMIEŃ (Zapalanie, potem gaszenie - BEZ ZAWIESZEŃ I GUBIENIA KLATEK)
          // Koniec cyklu wyłącznie po zgaszeniu ostatniej pary (niżej). Wcześniej limit czasu przerywał
          // fazę gaszenia przy kroku 0,05 s i ostatnie pary zostawały zapalone.
          {
            while (currentCascadeStep + 1 < paraCount * 2) {
              yield(); unsigned long timeElapsed = millis() - cascadeStartTime;
              if (timeElapsed < (currentCascadeStep + 1) * stepDurationMs) break;
              currentCascadeStep++;
              
              if (currentCascadeStep < paraCount) { 
                // FAZA 1: Rozświetlanie fali
                int actualStep = reverseDirection ? (paraCount - 1 - currentCascadeStep) : currentCascadeStep;
                int lewaLampa  = (actualStep * 2) + DALI_ADDR_OFFSET; int prawaLampa = (actualStep * 2) + 1 + DALI_ADDR_OFFSET;
                
                sendDaliCommand16((lewaLampa << 1) | 0x00, DALI_100_PERCENT); // oddech dla ATtiny85 pilnuje sendDaliCommand16
                if ((actualStep * 2) + 1 < LAMPS_COUNT) { sendDaliCommand16((prawaLampa << 1) | 0x00, DALI_100_PERCENT); }
              } else { 
                // FAZA 2: Gaszenie fali
                int turnOffIndex = currentCascadeStep - paraCount; 
                int actualStep = reverseDirection ? (paraCount - 1 - turnOffIndex) : turnOffIndex;
                int lewaLampa  = (actualStep * 2) + DALI_ADDR_OFFSET; int prawaLampa = (actualStep * 2) + 1 + DALI_ADDR_OFFSET;
                
                sendDaliCommand16((lewaLampa << 1) | 0x00, offLevel);
                if ((actualStep * 2) + 1 < LAMPS_COUNT) { sendDaliCommand16((prawaLampa << 1) | 0x00, offLevel); }
                
                // SKASOWANIE MARTWEJ PRZERWY: natychmiastowe złamanie osi czasu po zgaszeniu ostatniej oprawy
                if (currentCascadeStep == (paraCount * 2) - 1) {
                  endAnimationCycle();
                  break;
                }
              }
            }
          }
        }
      } // <- Koniec if (isCascadeActive)
    } // <- Koniec else (dla waveMode < 2)
  } // <- Koniec else (dla !isRadarAlarmActive)

  static unsigned long lastSweeperTime = 0;
  if (!isCascadeActive && !isHoldTimerActive && !isRadarAlarmActive && LAMPS_COUNT > 0) {
    if (millis() - lastSweeperTime > 10000) { 
      lastSweeperTime = millis();
      if (millis() - lastPollTime > 300) { sendDaliCommand16(DALI_BROADCAST_DAPC, getOffLevel()); }
    }
  } else { lastSweeperTime = millis(); }

  if (aktywnySkanerAwarii && !isCascadeActive && !isHoldTimerActive && !isRadarAlarmActive && LAMPS_COUNT > 0 && mrugajAdr < 0) {
    if (millis() - lastPollTime > POLL_INTERVAL_MS) { 
      lastPollTime = millis();
      // Co drugie sprawdzenie dostaje oprawa z awarią lub podejrzeniem awarii (po kolei, jeśli jest ich kilka).
      // Wcześniej naprawiona oprawa czekała na swoją kolej w pełnym obiegu (liczba opraw × 5 s),
      // więc awaria „wisiała” na ekranie długo po naprawie.
      static bool kolejPodejrzanej = false; static int ostatniaPodejrzana = -1;
      // v1.6: sprawdzane są tylko przypisane miejsca (wolne miejsce w środku rzędu nie jest awarią)
      int nast = nastepneMiejsce(currentPollAddress); if (nast >= 0) currentPollAddress = nast;
      int pollIndex = currentPollAddress; bool zObiegu = true;
      kolejPodejrzanej = !kolejPodejrzanej;
      if (kolejPodejrzanej) {
        for (int k = 1; k <= LAMPS_COUNT; k++) {
          int i = (ostatniaPodejrzana + k) % LAMPS_COUNT; if (i < 0) i += LAMPS_COUNT;
          if (lampErrorState[i] > 0 && miejsceNadane(i + DALI_ADDR_OFFSET)) { pollIndex = i; ostatniaPodejrzana = i; zObiegu = false; break; }
        }
      }
      int actualAddr = pollIndex + DALI_ADDR_OFFSET; 
      uint8_t targetAddress = (actualAddr << 1) | 0x01; 
      
      sendDaliCommand16(targetAddress, 0x91); // PING
      bool isAlive = checkDaliResponse(); delay(15);
      
      bool isLampBroken = false;
      if (isAlive) { sendDaliCommand16(targetAddress, 0x92); isLampBroken = checkDaliResponse(); }
      
      // Awaria dopiero po LAMP_ERROR_THRESHOLD kolejnych błędnych odczytach (pojedyncze zakłócenie nie alarmuje)
      // Jedna poprawna odpowiedź od razu kasuje awarię tej oprawy.
      if (!isAlive || isLampBroken) { if (lampErrorState[pollIndex] < 255) lampErrorState[pollIndex]++; } else { lampErrorState[pollIndex] = 0; }
      // v1.8: odpytywanie aktualizuje też stan „odpowiada” miejsca (oczko podłączone po skanie)
      if (skanWykonany) { if (isAlive) ustawBit64(obecneAdresy, actualAddr, true); else if (lampErrorState[pollIndex] >= LAMP_ERROR_THRESHOLD) ustawBit64(obecneAdresy, actualAddr, false); }
      bool bylaAwaria = hasAnyError;
      hasAnyError = false; for (int i = 0; i < LAMPS_COUNT; i++) { if (lampaUszkodzona(i)) { hasAnyError = true; break; } }
      if (bylaAwaria != hasAnyError) wymusOled = true;
      if (zObiegu) { currentPollAddress++; if (currentPollAddress >= LAMPS_COUNT) currentPollAddress = 0; }
    }
  }

  if (!ekranUspiony && (millis() - czasOstatniejAktywnosci > CZAS_DO_USPIENIA)) { ekranUspiony = true; u8g2.setPowerSave(1); }

  static bool ostatniStanKaskady = false; static int ostatniWyswietlanyCykl = -1;
  if (isCascadeActive != ostatniStanKaskady) { wymusOled = true; ostatniStanKaskady = isCascadeActive; }
  if (isCascadeActive && currentCycleCount != ostatniWyswietlanyCykl) { wymusOled = true; ostatniWyswietlanyCykl = currentCycleCount; }

  if ( wymusOled || (!isCascadeActive && (millis() - lastLcdUpdate > 250)) ) {
    if (digitalRead(PIN_PRZYCISK_S3) == HIGH && !ekranUspiony) {
      lastLcdUpdate = millis(); wymusOled = false; 
      u8g2.clearBuffer(); u8g2.setDrawColor(1); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
      char linia0[40] = ""; char linia1[40] = ""; char linia2[40] = "";

      if (currentDisplay == DISP_STATUS && trybSerwisowy && !isCascadeActive && !isRadarAlarmActive) {
        rysujEkranSerwisu();
      }
      else if (currentDisplay == DISP_STATUS) {
        if (isRadarAlarmActive) {
          snprintf(linia0, sizeof(linia0), "! ALARM RADAR !"); snprintf(linia1, sizeof(linia1), "PRIORYTET WYSOKI"); snprintf(linia2, sizeof(linia2), "Koniec za: %lu s", (radarAlarmEndTime - millis()) / 1000);
        } else if (isCascadeActive || (isHoldTimerActive && waveMode >= 2)) {
          snprintf(linia0, sizeof(linia0), "DETEKCJA RUCHU!"); snprintf(linia1, sizeof(linia1), "Tryb: %s", modeNames[waveMode]);
          if (waveMode < 2) snprintf(linia2, sizeof(linia2), "Cykl: %d/%d", currentCycleCount + 1, targetCycles[waveMode]); else snprintf(linia2, sizeof(linia2), "Oczka: %d", LAMPS_COUNT);
        } else if (LAMPS_COUNT == 0 && policz64(doPrzypisania()) > 0) {
          // v1.13: po skanie nowe oczka mają tylko numery tymczasowe – trzeba je przypisać, nie skanować ponownie
          snprintf(linia0, sizeof(linia0), "NOWE OCZKA: %d", policz64(doPrzypisania())); snprintf(linia1, sizeof(linia1), "Przypisz miejsca:"); snprintf(linia2, sizeof(linia2), "SERWIS WiFi");
        } else if (LAMPS_COUNT == 0) {
          // Bez zaprogramowanych opraw działa tylko „Wszystkie”/„Stroboskop” (polecenie do wszystkich naraz)
          snprintf(linia0, sizeof(linia0), "BRAK OCZEK"); snprintf(linia1, sizeof(linia1), "Wykonaj skan:"); snprintf(linia2, sizeof(linia2), "SKANER DALI");
        } else if (hasAnyError) {
          snprintf(linia0, sizeof(linia0), "WYKRYTO AWARIĘ"); int errorID = -1, ileAwarii = 0; for (int i = 0; i < LAMPS_COUNT; i++) { if (lampaUszkodzona(i)) { if (errorID < 0) errorID = i; ileAwarii++; } }
          snprintf(linia1, sizeof(linia1), "Sprawdź oczko"); snprintf(linia2, sizeof(linia2), "%s%s", etykietaMiejsca(errorID + DALI_ADDR_OFFSET).c_str(), ileAwarii > 1 ? (" +" + String(ileAwarii - 1)).c_str() : "");
        } else {
          if (rtcDostepny) {
            DateTime now = rtc.now(); 
            snprintf(linia0, sizeof(linia0), "%s", wifiKrotko(WiFi.status()));   // v1.15: zamiast „TRYB CAŁODOBOWY”
            snprintf(linia1, sizeof(linia1), "Zegar: %02d:%02d", now.hour(), now.minute());
            if (isHoldTimerActive && waveMode >= 2) {
              unsigned long elapsed = (millis() - lastSensorTriggerTime) / 1000UL; int remaining = holdTimeSec[waveMode] - elapsed; if (remaining < 0) remaining = 0; snprintf(linia2, sizeof(linia2), "Świeci: %ds", remaining);
            } else snprintf(linia2, sizeof(linia2), "Oczka zgaszone");
          } else { 
            if (trybOffline) snprintf(linia0, sizeof(linia0), "OFFLINE bez RTC"); else snprintf(linia0, sizeof(linia0), "BŁĄD ZEGARA"); 
            snprintf(linia1, sizeof(linia1), "Działa awaryjnie"); snprintf(linia2, sizeof(linia2), "Oczka zgaszone"); 
          }
        }
        u8g2.drawUTF8((128 - u8g2.getUTF8Width(linia0)) / 2, 0, linia0); u8g2.drawUTF8((128 - u8g2.getUTF8Width(linia1)) / 2, 22, linia1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(linia2)) / 2, 44, linia2);
      }
      else if (currentDisplay == DISP_TRYB) { snprintf(linia1, sizeof(linia1), "TRYB ANIMACJI:"); snprintf(linia2, sizeof(linia2), "> %s", modeNames[waveMode]); }
      else if (currentDisplay == DISP_CZAS) { int totalMs = czasKroku10ms[waveMode] * 10; int intPart = totalMs / 1000; int fracPart = (totalMs % 1000) / 10; snprintf(linia1, sizeof(linia1), "CZAS KROKU:"); snprintf(linia2, sizeof(linia2), "> %d.%02d sek.", intPart, fracPart); }
      else if (currentDisplay == DISP_CZAS_SWIECENIA) { snprintf(linia1, sizeof(linia1), "CZAS TRWANIA"); snprintf(linia2, sizeof(linia2), "> %d sek.", holdTimeSec[waveMode]); }
      else if (currentDisplay == DISP_CYKLE) { snprintf(linia1, sizeof(linia1), "LICZBA CYKLI:"); snprintf(linia2, sizeof(linia2), "> %d razy", targetCycles[waveMode]); }
      else if (currentDisplay == DISP_SKANER) { snprintf(linia1, sizeof(linia1), "SKANER DALI:"); snprintf(linia2, sizeof(linia2), "Przytrzymaj +"); }
      else if (currentDisplay == DISP_SERWIS) { snprintf(linia1, sizeof(linia1), trybSerwisowy ? "WYŁĄCZ SERWIS:" : "SERWIS WiFi:"); snprintf(linia2, sizeof(linia2), "Przytrzymaj +"); }
      else if (currentDisplay == DISP_RESET) { snprintf(linia1, sizeof(linia1), "RESET OCZEK:"); snprintf(linia2, sizeof(linia2), "Przytrzymaj +"); }
      else if (currentDisplay == DISP_RESET_SYS) { snprintf(linia1, sizeof(linia1), "RESET SYSTEMU:"); snprintf(linia2, sizeof(linia2), "Przytrzymaj +"); }
      else if (currentDisplay == DISP_WERSJA) { snprintf(linia1, sizeof(linia1), "Euroasfalt"); snprintf(linia2, sizeof(linia2), NAZWA_PRODUKTU " v" FW_VERSION); }
      
      if (currentDisplay == DISP_WERSJA) {
          u8g2.drawUTF8((128 - u8g2.getUTF8Width(linia1)) / 2, 15, linia1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(linia2)) / 2, 40, linia2);
      } else if (currentDisplay != DISP_STATUS) {
          u8g2.drawUTF8(0, 10, linia1);
          if (currentDisplay == DISP_SKANER || currentDisplay == DISP_SERWIS || currentDisplay == DISP_RESET || currentDisplay == DISP_RESET_SYS) { u8g2.drawUTF8((128 - u8g2.getUTF8Width(linia2)) / 2, 35, linia2); } else { u8g2.drawUTF8(5, 35, linia2); }
      }
      u8g2.sendBuffer();
    }
  }
}

unsigned long ostatniaRamkaDaliUs = 0;
uint32_t daliMaxSpoznienieUs = 0;   // v1.7: największe spóźnienie zbocza ramki (diagnostyka na stronie serwisowej)
uint32_t daliSpoznioneRamki = 0;    // v1.8: ramki ze zboczem spóźnionym ponad DALI_SPOZNIENIE_ALARM_US
const uint32_t DALI_SPOZNIENIE_ALARM_US = 100;   // wkładki przyjmują do ok. 150 µs (symulacja) – zapas
// v1.8: 0 = nadawanie jak do 1.6 (cała ramka bez przerwań), gdy pomiar analizatorem pokaże niestabilne zbocza.
#define DALI_NADAWANIE_Z_PRZERWANIAMI 1

void sendDaliCommand16(uint8_t address, uint8_t command) {
  // Minimalna przerwa między ramkami: wcześniej Kaskada miała 5 ms, a przy nadrabianiu kroków 0 ms.
  while (micros() - ostatniaRamkaDaliUs < DALI_MIN_GAP_US) { yield(); }
  // v1.7: zbocza w dokładnych chwilach z licznika cykli; przerwania wyłączone tylko na 150 µs przed każdym zboczem.
  // Wcześniej cała ramka (14 ms) szła bez przerwań – przy skanie i miganiu telefon tracił sieć serwisową.
  // Spóźnienie zbocza o czas obsługi przerwania nie kumuluje się; wkładki przyjmują zbocza spóźnione do ok. 150 µs.
#if !DALI_NADAWANIE_Z_PRZERWANIAMI
  { uint16_t f16 = (address << 8) | command; noInterrupts();
    digitalWrite(DALI_TX_PIN, LOW); delayMicroseconds(416); digitalWrite(DALI_TX_PIN, HIGH); delayMicroseconds(416);
    for (int i = 15; i >= 0; i--) { bool b = (f16 >> i) & 1; digitalWrite(DALI_TX_PIN, b ? LOW : HIGH); delayMicroseconds(416); digitalWrite(DALI_TX_PIN, b ? HIGH : LOW); delayMicroseconds(416); }
    digitalWrite(DALI_TX_PIN, HIGH); interrupts(); ostatniaRamkaDaliUs = micros(); return; }
#endif
  const uint32_t CYKLE_US = F_CPU / 1000000UL, POL_BITU = 416UL * CYKLE_US, OKNO = 150UL * CYKLE_US;
  uint32_t spozRamki = 0;
  uint32_t frame = 0x10000UL | ((uint32_t)address << 8) | command;   // bit startu = 1
  noInterrupts(); uint32_t t = ESP.getCycleCount() + OKNO; interrupts();
  for (int i = 16; i >= 0; i--) {
    bool jedynka = (frame >> i) & 0x01;   // 1 = połowa LOW, potem HIGH; 0 = HIGH, potem LOW
    for (int pol = 0; pol < 2; pol++) {
      uint32_t c;
      while ((int32_t)(ESP.getCycleCount() - (t - OKNO)) < 0) { }      // tu działają przerwania WiFi
      noInterrupts();
      while ((int32_t)((c = ESP.getCycleCount()) - t) < 0) { }
      digitalWrite(DALI_TX_PIN, (jedynka != (pol == 1)) ? LOW : HIGH);
      interrupts();
      uint32_t spoz = (c - t) / CYKLE_US; if (spoz > spozRamki) spozRamki = spoz;
      t += POL_BITU;
    }
  }
  while ((int32_t)(ESP.getCycleCount() - (t - OKNO)) < 0) { }
  noInterrupts(); while ((int32_t)(ESP.getCycleCount() - t) < 0) { } digitalWrite(DALI_TX_PIN, HIGH); interrupts();   // spoczynek
  // Licznik mierzy chwilę przed digitalWrite (stały czas ok. 1 µs) – nie zastępuje pomiaru analizatorem na linii.
  if (spozRamki > daliMaxSpoznienieUs) daliMaxSpoznienieUs = spozRamki;
  if (spozRamki > DALI_SPOZNIENIE_ALARM_US) daliSpoznioneRamki++;
  ostatniaRamkaDaliUs = micros();
}
// Po odpowiedzi "TAK" 35 ms przerwy (wcześniej 25 ms): wkładki ATtiny v1.0 biorą cudzą odpowiedź za początek
// ramki i przez ok. 30 ms dekodują śmieci – następna ramka po 25 ms ginęła (np. PING kolejnej oprawy przy skanie).
const unsigned long PRZERWA_PO_ODPOWIEDZI_MS = 35;
bool checkDaliResponse() {
  unsigned long startTime = millis(); while (millis() - startTime < 20) { if (digitalRead(DALI_RX_PIN) == LOW) { delayMicroseconds(200); if (digitalRead(DALI_RX_PIN) == LOW) { delay(PRZERWA_PO_ODPOWIEDZI_MS); return true; } } }
  return false;
}
void setSearchAddress(uint32_t addr) { sendDaliCommand16(0xB1, (addr >> 16) & 0xFF); delay(12); sendDaliCommand16(0xB3, (addr >> 8) & 0xFF); delay(12); sendDaliCommand16(0xB5, addr & 0xFF); delay(12); }
bool checkAddressMatch(uint32_t addr) { setSearchAddress(addr); sendDaliCommand16(0xA9, 0x00); return checkDaliResponse(); }

void runFactoryReset() {
  u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
  const char* m1 = "RESET OCZEK..."; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m1)) / 2, 25, m1); u8g2.sendBuffer();

  // 1. Otwarcie sesji programowania DALI
  sendDaliCommand16(0xA1, 0x00); delay(30);
  sendDaliCommand16(0xA5, 0x00); delay(15);
  sendDaliCommand16(0xA5, 0x00); delay(15);
  sendDaliCommand16(0xA7, 0x00); delay(15);
  sendDaliCommand16(0xA7, 0x00); delay(120);

  int cleanedCount = 0;
  while (cleanedCount < 64) { // limit: przy zakłóconej magistrali pętla nie kończyła się nigdy
    uint32_t low = 0, high = 0xFFFFFF;
    if (!checkAddressMatch(high)) break; // Brak kolejnych opraw na kablu

    // Wyszukiwanie binarne 24-bitowego ID oprawy
    while (low < high) {
      uint32_t mid = low + ((high - low) / 2);
      if (checkAddressMatch(mid)) high = mid; else low = mid + 1;
    }

    // Zaznaczenie odnalezionej wkładki ATtiny
    setSearchAddress(low);
    sendDaliCommand16(0xA9, 0x00); delay(30);

    // TWARDE KASOWANIE: Wpisanie 0xFF (czysta oprawa fabryczna)
    sendDaliCommand16(0xB7, 0xFF); delay(50);
    sendDaliCommand16(0xB7, 0xFF); delay(100);

    // Wycofanie skasowanej oprawy z pętli
    sendDaliCommand16(0xAB, 0x00); delay(20);
    sendDaliCommand16(0xAB, 0x00); delay(50);

    cleanedCount++;
  }

  // Zamknięcie sesji DALI
  sendDaliCommand16(0xA1, 0x00); delay(100);

  // Czyszczenie pamięci Wemosa (v1.6: także mapa miejsc L/P)
  mapaMiejsc = 0; zapiszMapeMiejsc();
  hasAnyError = false; for(int i = 0; i < 64; i++) lampErrorState[i] = 0; currentPollAddress = 0;

  // Fizyczne mignięcie potwierdzające wyczyszczenie
  sendDaliCommand16(DALI_BROADCAST_DAPC, 254); delay(500);
  sendDaliCommand16(DALI_BROADCAST_DAPC, getOffLevel()); delay(500);

  u8g2.clearBuffer();
  char okBuf[30]; snprintf(okBuf, sizeof(okBuf), "Czystych: %d", cleanedCount);
  u8g2.drawUTF8((128 - u8g2.getUTF8Width("ZRESETOWANO!")) / 2, 10, "ZRESETOWANO!");
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(okBuf)) / 2, 35, okBuf);
  u8g2.sendBuffer(); delay(3000); u8g2.clearBuffer();
}

void runSystemReset() {
  u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
  const char* m1 = "KASOWANIE..."; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m1)) / 2, 25, m1); u8g2.sendBuffer();
  WiFiManager wm; wm.resetSettings(); WiFi.disconnect(true); delay(200);
  EEPROM.write(ADDR_LAMPS_COUNT, 0); EEPROM.write(ADDR_WAVE_MODE, 0); EEPROM.write(ADDR_WIFI_MODE, 0);
  EEPROM.write(ADDR_MAPA_ZNACZNIK, MAPA_ZNACZNIK); for (int i = 0; i < 8; i++) EEPROM.write(ADDR_MAPA + i, 0); EEPROM.write(ADDR_RZAD_L, 0xFF); EEPROM.write(ADDR_RZAD_P, 0xFF);
  for(int i=0; i<4; i++) { EEPROM.write(ADDR_CYCLE_TIME + i, 10); EEPROM.write(ADDR_TARGET_CYCLES + i, 1); EEPROM.write(ADDR_HOLD_TIME + i, 15); }
  saveEmailToEEPROM("pawel.krajewski@euroasfalt.pl"); saveLocationToEEPROM("Miasto Ulica");
  zapiszTekstEEPROM(ADDR_CHMURA_URL, CHMURA_URL_DL, ""); zapiszTekstEEPROM(ADDR_CHMURA_KLUCZ, CHMURA_KLUCZ_DL, ""); EEPROM.commit();
  u8g2.clearBuffer(); const char* okStr = "ZRESETOWANO!"; const char* okStr2 = "RESTART..."; u8g2.drawUTF8((128 - u8g2.getUTF8Width(okStr)) / 2, 15, okStr); u8g2.drawUTF8((128 - u8g2.getUTF8Width(okStr2)) / 2, 35, okStr2); u8g2.sendBuffer(); delay(3000); ESP.restart(); 
}


void rysujLogoEuroasfalt() {
  u8g2.setDrawColor(0); u8g2.drawBox(0, 0, 128, 64); u8g2.setDrawColor(1);
  for (int y = 0; y < 50; y++) { for (int x = 0; x < 128; x++) { int bajtIndex = (y * 16) + (x / 8); int bitIndex = 7 - (x % 8); uint8_t bajt = pgm_read_byte(&(logo_nowe_Euroasfalt[bajtIndex])); if ((bajt >> bitIndex) & 0x01) { int newY = y + Y_LOGO_OFFSET; if (newY >= 0 && newY < 64) u8g2.drawPixel(x, newY); } } }
}

void ekranPowitalny() {
  u8g2.setContrast(0); u8g2.clearBuffer(); rysujLogoEuroasfalt(); u8g2.sendBuffer();
  for (int i = 0; i <= 255; i += 5) { u8g2.setContrast(i); delay(12); } delay(2500);
  for (int i = 255; i >= 0; i -= 5) { u8g2.setContrast(i); delay(8); }
  u8g2.clearBuffer(); u8g2.setDrawColor(1); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
  u8g2.drawUTF8((128 - u8g2.getUTF8Width("STEROWNIK")) / 2, 10, "STEROWNIK"); u8g2.drawUTF8((128 - u8g2.getUTF8Width(NAZWA_PRODUKTU " v" FW_VERSION)) / 2, 35, NAZWA_PRODUKTU " v" FW_VERSION);
  u8g2.sendBuffer(); for (int i = 0; i <= 255; i += 5) { u8g2.setContrast(i); delay(8); } u8g2.setContrast(255); delay(2000);
}

// =========================================================================
// v1.6: MIEJSCA L/P, SKANER BEZ PODWÓJNYCH NUMERÓW, WYMIANA OPRAW, STRONA SERWISOWA
// =========================================================================
// Numer (adres DALI) należy do MIEJSCA na drodze, nie do oprawy: L1 = 10, P1 = 11, L2 = 12, P2 = 13…
// (kaskada zapala parami adres parzysty + nieparzysty, czyli L_k razem z P_k). L/P patrząc od strony szafy.
//
// Zasady:
// 1. Mapa przypisanych miejsc jest w EEPROM (mapaMiejsc). Numer miejsca nie zmienia się sam.
// 2. Miejsce, które nie odpowiada, jest zarezerwowane – skan nie daje jego numeru nowej oprawie
//    (v1.5 dawało: odłączony sznur + skan drugiego sznura = podwójne numery i ukryte awarie).
// 3. Nowa oprawa dostaje numer TYMCZASOWY (0–9, a gdy brak – najwyższe wolne adresy) i nie świeci w kaskadzie,
//    dopóki ktoś nie wskaże jej miejsca: wymiana 1:1 przyciskiem na sterowniku albo na telefonie.
// 4. Zmiana numeru oprawy o adresie X (działa z wkładkami ATtiny v1.0 i v1.1): INITIALISE 0x00 + RANDOMISE –
//    wkładka z adresem ustawia adres losowy = adres krótki, więc SEARCH = X wskazuje tylko ją; PROGRAM SHORT.

// Strona serwisowa: tablica STRONA_SERWIS na samym końcu pliku (długi tekst HTML na końcu nie przeszkadza
// Arduino IDE w generowaniu deklaracji funkcji).
extern const char STRONA_SERWIS[] PROGMEM;


// ------------------------------------------------------------------ mapa miejsc
const int MAX_W_RZEDZIE = (64 - DALI_ADDR_OFFSET) / 2;   // 27

bool bit64(uint64_t m, int a) { return a >= 0 && a < 64 && ((m >> a) & 1ULL); }
void ustawBit64(uint64_t &m, int a, bool v) { if (a < 0 || a > 63) return; if (v) m |= (1ULL << a); else m &= ~(1ULL << a); }
int policz64(uint64_t m) { int n = 0; for (int a = 0; a < 64; a++) if (bit64(m, a)) n++; return n; }
int pierwszy64(uint64_t m) { for (int a = 0; a < 64; a++) if (bit64(m, a)) return a; return -1; }

bool miejsceNadane(int adr) { return adr >= DALI_ADDR_OFFSET && adr < 64 && bit64(mapaMiejsc, adr); }
int rzadAdresu(int adr) { return (adr - DALI_ADDR_OFFSET) % 2; }            // 0 = L, 1 = P
int numerWRzedzie(int adr) { return (adr - DALI_ADDR_OFFSET) / 2 + 1; }
int adresMiejsca(int rzad, int k) { return DALI_ADDR_OFFSET + (k - 1) * 2 + rzad; }

String etykietaMiejsca(int adr) {
  if (adr < DALI_ADDR_OFFSET || adr > 63) return "T" + String(adr);       // numer tymczasowy
  return String(rzadAdresu(adr) == 0 ? "L" : "P") + String(numerWRzedzie(adr));
}

int policzMiejsca() { return policz64(mapaMiejsc); }
int policzWRzedzie(int rzad) { int n = 0; for (int a = DALI_ADDR_OFFSET + rzad; a < 64; a += 2) if (miejsceNadane(a)) n++; return n; }
int najdalszeWRzedzie(int rzad) { int k = 0; for (int a = DALI_ADDR_OFFSET + rzad; a < 64; a += 2) if (miejsceNadane(a)) k = numerWRzedzie(a); return k; }

// Indeks (adres − 10) następnego przypisanego miejsca od i (z zawinięciem); −1 gdy brak.
int nastepneMiejsce(int i) {
  if (LAMPS_COUNT <= 0) return -1;
  for (int k = 0; k < LAMPS_COUNT; k++) { int j = (i + k) % LAMPS_COUNT; if (miejsceNadane(j + DALI_ADDR_OFFSET)) return j; }
  return -1;
}

void przeliczLiczbeOpraw() {
  int maxAdr = DALI_ADDR_OFFSET - 1;
  for (int a = DALI_ADDR_OFFSET; a < 64; a++) if (miejsceNadane(a)) maxAdr = a;
  LAMPS_COUNT = maxAdr - DALI_ADDR_OFFSET + 1;   // kaskada: pary do najdalszego miejsca
  if (currentPollAddress >= LAMPS_COUNT) currentPollAddress = 0;
}

// v1.8: wynik EEPROM.commit() zwracany (false = mapa w RAM inna niż po restarcie).
// v1.9: commit() zawsze – EEPROM.read() czyta bufor RAM, więc po nieudanym zapisie porównanie nie widziało różnicy
// i zapis nie był ponawiany. Biblioteka (esp8266 3.1.2) po błędzie zostawia bufor „brudny”, a bez zmian nic nie zapisuje.
bool zapisMapyOczekuje = false;   // mapa w RAM nie jest jeszcze w pamięci trwałej (strona: „Ponów zapis konfiguracji”)

bool zapiszMapeMiejsc() {
  przeliczLiczbeOpraw();
  hasAnyError = false; for (int i = 0; i < LAMPS_COUNT; i++) if (lampaUszkodzona(i)) { hasAnyError = true; break; }
  wymusOled = true;
  EEPROM.write(ADDR_MAPA_ZNACZNIK, MAPA_ZNACZNIK);
  for (int i = 0; i < 8; i++) EEPROM.write(ADDR_MAPA + i, (uint8_t)(mapaMiejsc >> (8 * i)));
  EEPROM.write(ADDR_LAMPS_COUNT, LAMPS_COUNT);   // zgodność wstecz (powrót do v1.5)
  bool ok = EEPROM.commit();
  if (!ok) dziennik("BŁĄD zapisu mapy miejsc w pamięci sterownika (EEPROM)");
  else if (zapisMapyOczekuje) dziennik("Zapis mapy miejsc ponowiony – udany");
  zapisMapyOczekuje = !ok;
  return ok;
}

void wczytajMapeMiejsc() {
  if (EEPROM.read(ADDR_MAPA_ZNACZNIK) == MAPA_ZNACZNIK) {
    mapaMiejsc = 0;
    for (int i = 0; i < 8; i++) mapaMiejsc |= ((uint64_t)EEPROM.read(ADDR_MAPA + i)) << (8 * i);
    mapaMiejsc &= ~((1ULL << DALI_ADDR_OFFSET) - 1);   // adresy 0–9 nigdy nie są miejscami
    przeliczLiczbeOpraw();
  } else {
    // Pierwszy start v1.6: dotychczasowe oprawy 10..10+N−1 (montaż naprzemienny L1, P1, L2…) stają się miejscami.
    mapaMiejsc = 0;
    for (int i = 0; i < LAMPS_COUNT && i < MAX_LAMPS; i++) ustawBit64(mapaMiejsc, i + DALI_ADDR_OFFSET, true);
    zapiszMapeMiejsc();
  }
}

int dlugoscRzedu(int rzad) {
  int zapisana = EEPROM.read(rzad == 0 ? ADDR_RZAD_L : ADDR_RZAD_P);
  if (zapisana < 1 || zapisana > MAX_W_RZEDZIE) zapisana = 0;
  return max(zapisana, najdalszeWRzedzie(rzad));
}

// ------------------------------------------------------------------ stan skanu i serwisu
uint64_t obecneAdresy = 0;   // adresy 0–63, które odpowiadają (po skanie, aktualizowane przy przypisaniu)
bool skanWykonany = false, skanTrwa = false, skanBladLinii = false, skanBrakAdresow = false;
int skanNiepotwierdzone = 0;   // v1.8: oczka, które po nadaniu numeru tymczasowego nie odpowiedziały
int skanZnalezione = 0;
String serwisKomunikat = "";
String skanEtap = "";        // v1.7: opis etapu sprawdzania podwójnych numerów (strona i OLED)

uint64_t brakujaceMiejsca() { return skanWykonany ? (mapaMiejsc & ~obecneAdresy) : 0; }
uint64_t doPrzypisania() { return skanWykonany ? (obecneAdresy & ~mapaMiejsc) : 0; }

// Najmniejsze wolne miejsce (dopisanie w kolejności L1, P1, L2… – jak dotychczasowy montaż po jednej oprawie).
int pierwszeWolneMiejsce() { for (int a = DALI_ADDR_OFFSET; a < 64; a++) if (!miejsceNadane(a)) return a; return -1; }

// Numer tymczasowy: najpierw 0–9 (poza kaskadą), potem najwyższe adresy, które nie są miejscami.
int wolnyAdresTymczasowy(uint64_t zajete) {
  for (int a = 0; a < DALI_ADDR_OFFSET; a++) if (!bit64(zajete, a)) return a;
  for (int a = 63; a >= DALI_ADDR_OFFSET; a--) if (!bit64(zajete, a) && !miejsceNadane(a)) return a;
  return -1;
}

// Ostrzeżenie: nie odpowiada większość rzędu (raczej odłączony sznur niż awaria oczek).
String ostrzezenieSznura() {
  String w = "";
  uint64_t brak = brakujaceMiejsca();
  for (int r = 0; r < 2; r++) {
    int nadane = policzWRzedzie(r), b = 0;
    for (int a = DALI_ADDR_OFFSET + r; a < 64; a += 2) if (bit64(brak, a)) b++;
    if (b >= 3 && b * 2 >= nadane) w += String(w.length() ? " " : "") + "Nie odpowiada " + String(b) + " z " + String(nadane) +
      " oczek rzędu " + (r == 0 ? "L" : "P") + " – sprawdź podłączenie sznura " + (r == 0 ? "L." : "P.");
  }
  return w;
}

// ------------------------------------------------------------------ kolejka operacji (v1.8)
// Operacje na magistrali (skan, odczyt, podwójne numery, przypisanie, odpięcie) wykonuje tylko loop(), po jednej.
// Strona zleca operację (202 + numer), a wynik czyta ze stanu – także po zerwaniu połączenia. W 1.7 przypisanie szło
// wewnątrz obsługi żądania HTTP, a w trakcie programowania obsługiwane były kolejne żądania: mogły uruchomić drugą
// operację na tej samej magistrali, a serwer WWW (jeden „bieżący klient”) gubił odpowiedź na pierwsze żądanie.
enum Operacja : uint8_t { OP_BRAK, OP_SKAN, OP_ODCZYT, OP_PODWOJNE, OP_PRZYPISZ, OP_ODEPNIJ, OP_WYMIEN, OP_ROZDZIEL, OP_ZAMIEN };
const char* const OP_NAZWA[] = { "", "skan", "odczyt", "podwojne", "przypisz", "odepnij", "wymien", "rozdziel", "zamien" };
Operacja opZlecona = OP_BRAK, opTrwa = OP_BRAK, opWynikRodzaj = OP_BRAK;
int opZ = -1, opNa = -1, opWynikZ = -1, opWynikNa = -1;
uint32_t opNr = 0, opWynikNr = 0;      // numer ostatnio przyjętej / zakończonej operacji
bool opWynikOk = true;
String opWynikTekst = "";
uint32_t bootId = 0;                   // v1.9: losowy przy starcie – strona rozpoznaje restart sterownika
// v1.9: historia ostatnich zleceń ze strony – wynik wyszukiwany po identyfikatorze zlecenia (nie po numerze).
// GRANICA: historia ma HISTORIA_N wpisów. Wynik zlecenia i ochrona przed jego powtórnym wykonaniem (ten sam id)
// działają, dopóki po nim nie przyjęto HISTORIA_N kolejnych zleceń; restart sterownika czyści historię (strona
// wykrywa restart po bootId). Starszego zlecenia strona nie uznaje ani za wykonane, ani za niewykonane: „wynik
// nieznany”, odczyt stanu oczek, świadome ponowienie przez montera.
struct Zlecenie { String id; uint32_t nr; uint8_t rodzaj; int z, na; uint8_t stan; String tekst; String kod; };   // stan: 0 czeka, 1 trwa, 2 ok, 3 błąd; kod – wynik kreatora
const int HISTORIA_N = 8;
Zlecenie historia[HISTORIA_N];
int historiaPoz = 0;
// indeks zamiast wskaźnika na Zlecenie – Arduino IDE generuje prototypy przed definicją struktury
int zlecenieNr(uint32_t nr) { for (int i = 0; i < HISTORIA_N; i++) if (historia[i].nr == nr && nr) return i; return -1; }
int magistralaZajeta = 0;              // > 0: trwa operacja na magistrali (ze strony albo z menu sterownika)
struct ZajetaMagistrala { ZajetaMagistrala() { magistralaZajeta++; } ~ZajetaMagistrala() { magistralaZajeta--; } };
bool sterownikZajety() { return magistralaZajeta > 0 || opZlecona != OP_BRAK || skanTrwa; }
// uint8_t zamiast Operacja i bez argumentów domyślnych – Arduino IDE generuje prototypy przed definicją typu
void zlecOperacje(uint8_t op, int z, int na) { opZlecona = (Operacja)op; opZ = z; opNa = na; opNr++; }

// Połączenia telefonów z siecią serwisową (zdarzenia WiFi → dziennik w loop()).
WiFiEventHandler hTelefonPolaczony, hTelefonRozlaczony;
volatile uint16_t apPolaczenia = 0, apRozlaczenia = 0;

// ------------------------------------------------------------------ sieć serwisowa
DNSServer dnsSerwis;
char serwisSsid[24] = "";
char serwisHaslo[12] = "";
unsigned long serwisOstatniaAktywnosc = 0;
const unsigned long SERWIS_LIMIT_MS = 30UL * 60UL * 1000UL;   // wyłączenie po 30 min bez użycia strony
const IPAddress SERWIS_IP(192, 168, 4, 1);
bool serwisStronaOtwarta = false;   // po otwarciu strony telefon dostaje odpowiedź „sieć działa” i nie zrywa połączenia

// v1.7: stałe hasło – losowane raz i zapisane w EEPROM. W v1.6 było nowe przy każdym włączeniu serwisu,
// więc po zerwaniu sieci telefon nie łączył się ponownie zapamiętanym hasłem.
void wczytajHasloSerwisu() {
  bool ok = EEPROM.read(ADDR_HASLO_ZNACZNIK) == HASLO_ZNACZNIK;
  for (int i = 0; i < 8 && ok; i++) { char c = (char)EEPROM.read(ADDR_HASLO + i); if (c < '0' || c > '9') ok = false; else serwisHaslo[i] = c; }
  if (!ok) {
    snprintf(serwisHaslo, sizeof(serwisHaslo), "%08lu", (unsigned long)(ESP.random() % 100000000UL));
    EEPROM.write(ADDR_HASLO_ZNACZNIK, HASLO_ZNACZNIK);
    for (int i = 0; i < 8; i++) EEPROM.write(ADDR_HASLO + i, serwisHaslo[i]);
    EEPROM.commit();
  }
  serwisHaslo[8] = 0;
}

void wlaczSerwis() {
  snprintf(serwisSsid, sizeof(serwisSsid), "S-4_Dali_%04X", (unsigned)(ESP.getChipId() & 0xFFFF));
  wczytajHasloSerwisu();
  // v1.7: tylko punkt dostępowy. W v1.6 (AP+STA) sterownik równolegle łączył się z routerem – szukanie routera
  // przełącza kanał radia i telefon tracił sieć po kilku minutach. Raporty e-mail czekają do końca serwisu.
  WiFi.persistent(false);                  // bez zapisu ustawień AP do flash; dane routera zostają
  WiFi.mode(WIFI_AP);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.softAPConfig(SERWIS_IP, SERWIS_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(serwisSsid, serwisHaslo, 6, 0, 4);
  dnsSerwis.setErrorReplyCode(DNSReplyCode::NoError);
  dnsSerwis.start(53, "*", SERWIS_IP);     // każdy adres → strona serwisowa (okno logowania w telefonie)
  trybSerwisowy = true; serwisOstatniaAktywnosc = millis(); serwisKomunikat = ""; serwisStronaOtwarta = false;
  if (!skanWykonany && policzMiejsca() > 0 && !sterownikZajety()) zlecOperacje(OP_ODCZYT, -1, -1);   // stan oczek od razu, bez ponownego skanu
  hTelefonPolaczony = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected&) { apPolaczenia++; });
  hTelefonRozlaczony = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected&) { apRozlaczenia++; });
  dziennik(String("Sieć serwisowa ") + serwisSsid + " włączona");
  obudzEkran(); wymusOled = true;
}

void wylaczSerwis() {
  if (mrugajAdr >= 0) zakonczMruganie();
  dnsSerwis.stop(); WiFi.softAPdisconnect(true);
  if (trybOffline) WiFi.mode(WIFI_OFF);
  else { WiFi.mode(WIFI_STA); WiFi.begin(); }   // powrót do routera (zapisane dane sieci)
  WiFi.persistent(true);
  trybSerwisowy = false; wymusOled = true;
  dziennik("Sieć serwisowa wyłączona");
}

// ------------------------------------------------------------------ dziennik serwisowy (ostatnie zdarzenia)
const int DZIENNIK_N = 12;
String dziennikWpisy[DZIENNIK_N];
int dziennikPoz = 0;

void dziennik(const String& t) {
  char cz[12];
  if (rtcDostepny) { DateTime n = rtc.now(); snprintf(cz, sizeof(cz), "%02d:%02d:%02d", n.hour(), n.minute(), n.second()); }
  else { unsigned long s = millis() / 1000; snprintf(cz, sizeof(cz), "+%lu:%02lu", s / 60, s % 60); }
  dziennikWpisy[dziennikPoz] = String(cz) + " " + t;
  dziennikPoz = (dziennikPoz + 1) % DZIENNIK_N;
}

void obsluzSerwisWTle() { server.handleClient(); if (trybSerwisowy) dnsSerwis.processNextRequest(); yield(); }

// v1.13: odczekanie z obsługą strony serwisowej (komunikaty na OLED trzymały sterownik do 16 s bez odpowiedzi stronie).
void czekajObslugujac(unsigned long ms) { unsigned long t0 = millis(); while (millis() - t0 < ms) { obsluzSerwisWTle(); delay(5); } }

// Ekran OLED w trybie serwisowym (zamiast ekranu stanu).
void rysujEkranSerwisu() {
  const char* t = skanTrwa ? "SKANOWANIE..." : "SERWIS WiFi";
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(t)) / 2, 0, t);
  u8g2.setFont(u8g2_font_6x12_te);
  char l[32];
  snprintf(l, sizeof(l), "Sieć: %s", serwisSsid); u8g2.drawUTF8(0, 18, l);
  snprintf(l, sizeof(l), "Hasło: %s", serwisHaslo); u8g2.drawUTF8(0, 31, l);
  if (skanTrwa && skanEtap.length()) snprintf(l, sizeof(l), "%s", skanEtap.c_str());
  else if (skanTrwa) snprintf(l, sizeof(l), "Znaleziono: %d", skanZnalezione);
  else if (mrugajAdr >= 0) snprintf(l, sizeof(l), "Miga: %s", etykietaMiejsca(mrugajAdr).c_str());
  else snprintf(l, sizeof(l), "Strona: 192.168.4.1");
  u8g2.drawUTF8(0, 44, l);
  u8g2.drawUTF8(0, 54, "Koniec: menu SERWIS");
  u8g2.setFont(u8g2_font_unifont_t_polish);
}

// ------------------------------------------------------------------ mruganie oprawą
unsigned long mrugajDoMs = 0, mrugajOstatnio = 0; bool mrugajStan = false;

void zacznijMruganie(int adr, unsigned long ms) {
  if (adr < 0 || adr > 63) return;
  if (mrugajAdr >= 0 && mrugajAdr != adr) sendDaliCommand16((uint8_t)(mrugajAdr << 1), 0);
  if (mrugajAdr < 0) { isCascadeActive = false; isHoldTimerActive = false; currentCascadeStep = -1; pewneWygaszenie(); }
  mrugajAdr = adr; mrugajDoMs = millis() + ms; mrugajOstatnio = 0; mrugajStan = false; wymusOled = true;
}

void zakonczMruganie() {
  if (mrugajAdr >= 0) { sendDaliCommand16((uint8_t)(mrugajAdr << 1), 0); }
  mrugajAdr = -1; wymusOled = true;
}

void obsluzMruganie() {
  if (mrugajAdr < 0) return;
  if ((long)(millis() - mrugajDoMs) >= 0) { zakonczMruganie(); return; }
  if (millis() - mrugajOstatnio >= 350) {
    mrugajOstatnio = millis(); mrugajStan = !mrugajStan;
    sendDaliCommand16((uint8_t)(mrugajAdr << 1), mrugajStan ? DALI_100_PERCENT : 0);
  }
}

void wykonajOperacje();

// Wywoływane z loop().
void obsluzSerwis() {
  if (trybSerwisowy) {
    dnsSerwis.processNextRequest();
    if (millis() - serwisOstatniaAktywnosc > SERWIS_LIMIT_MS && !sterownikZajety()) wylaczSerwis();
    static uint16_t widzianeP = 0, widzianeR = 0;
    if (apPolaczenia != widzianeP) { widzianeP = apPolaczenia; dziennik("Telefon połączony z siecią serwisową (połączonych: " + String(WiFi.softAPgetStationNum()) + ")"); }
    if (apRozlaczenia != widzianeR) { widzianeR = apRozlaczenia; dziennik("Telefon rozłączony z siecią serwisową (połączonych: " + String(WiFi.softAPgetStationNum()) + ")"); }
  }
  obsluzMruganie();
  if (opZlecona != OP_BRAK && magistralaZajeta == 0) wykonajOperacje();
}

// ------------------------------------------------------------------ skaner
const unsigned long CZAS_MIGNIECIA_WKLADKI_MS = 500;   // wkładka miga po WITHDRAW (v1.0 nie słucha linii w tym czasie)

void pokazPostepSkanu(int znalezione) {
  char scanProgress[30]; snprintf(scanProgress, sizeof(scanProgress), "Znaleziono: %d", znalezione);
  u8g2.setDrawColor(0); u8g2.drawBox(0, 35, 128, 25); u8g2.setDrawColor(1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(scanProgress)) / 2, 35, scanProgress); u8g2.sendBuffer();
}

// Skan: kontrola linii (v1.5), PING wszystkich adresów, wyszukiwanie opraw. Oprawy z adresem zostają z nim,
// oprawy bez adresu dostają numer tymczasowy. Nic nie trafia automatycznie na miejsce w kaskadzie.
bool skanujMagistrale() {
  ZajetaMagistrala zajeta;
  skanTrwa = true; skanZnalezione = 0; skanBladLinii = false; skanBrakAdresow = false; skanNiepotwierdzone = 0; serwisKomunikat = "";
  if (mrugajAdr >= 0) zakonczMruganie();
  isCascadeActive = false; isHoldTimerActive = false;
  obudzEkran();
  u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
  const char* m1 = "SKANOWANIE..."; const char* m2 = "Kontrola linii"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m1)) / 2, 10, m1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(m2)) / 2, 35, m2); u8g2.sendBuffer();
  pinMode(DALI_RX_PIN, INPUT);

  // Kontrola linii (v1.5): zgaszenie opraw, 300 odczytów co 1 ms, zwarcie = linia niska ≥ 90% czasu.
  sendDaliCommand16(DALI_BROADCAST_DAPC, getOffLevel()); delay(300);
  int odczytyNiskie = 0;
  for (int i = 0; i < 300; i++) { if (digitalRead(DALI_RX_PIN) == LOW) odczytyNiskie++; delay(1); }
  int procentNiski = odczytyNiskie / 3;
  if (procentNiski >= 90) {
    skanBladLinii = true; skanTrwa = false;
    serwisKomunikat = "Błąd linii DALI (linia niska " + String(procentNiski) + "% czasu) – sprawdź zwarcie. Oprawy bez zmian.";
    u8g2.clearBuffer(); const char* err1 = "BŁĄD LINII DALI"; char err2[24]; snprintf(err2, sizeof(err2), "Niska: %d%%", procentNiski); const char* err3 = "Oprawy bez zmian";
    u8g2.drawUTF8((128 - u8g2.getUTF8Width(err1)) / 2, 0, err1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(err2)) / 2, 22, err2); u8g2.drawUTF8((128 - u8g2.getUTF8Width(err3)) / 2, 44, err3);
    u8g2.sendBuffer(); czekajObslugujac(5000); u8g2.clearBuffer(); wymusOled = true;
    return false;
  }

  // Oprawy z adresem (także tymczasowym 0–9), które odpowiadają przed skanem.
  uint64_t obecne = 0;
  for (int a = 0; a < 64; a++) {
    sendDaliCommand16((uint8_t)((a << 1) | 0x01), 0x91);
    if (checkDaliResponse()) ustawBit64(obecne, a, true);
    delay(15);
    if ((a & 7) == 7) obsluzSerwisWTle();
  }
  u8g2.clearBuffer(); m2 = "Szukam nowych..."; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m1)) / 2, 10, m1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(m2)) / 2, 35, m2); u8g2.sendBuffer();

  // INITIALISE 0x00 (wkładki ATtiny v1.0 nie znają 0xFF) + RANDOMISE: oprawa z adresem ma adres losowy = adres krótki.
  sendDaliCommand16(0xA1, 0x00); delay(30);
  sendDaliCommand16(0xA5, 0x00); delay(15); sendDaliCommand16(0xA5, 0x00); delay(15);
  sendDaliCommand16(0xA7, 0x00); delay(15); sendDaliCommand16(0xA7, 0x00); delay(120);

  int znalezione = 0;
  while (znalezione < 64) { // limit: przy zakłóconej magistrali pętla nie kończyła się nigdy
    uint32_t low = 0, high = 0xFFFFFF; if (!checkAddressMatch(high)) break;
    while (low < high) { uint32_t mid = low + ((high - low) / 2); if (checkAddressMatch(mid)) high = mid; else low = mid + 1; obsluzSerwisWTle(); }
    setSearchAddress(low); sendDaliCommand16(0xA9, 0x00); delay(30);
    if (low < 64) {
      ustawBit64(obecne, (int)low, true);   // ma adres – zostaje (także gdy nie odpowiedziała na PING)
    } else {
      int t = wolnyTymczasowySprawdzony(obecne);   // v1.9: numer sprawdzony na magistrali (oczko spoza PING-u)
      if (t < 0) { skanBrakAdresow = true; break; }
      // v1.8: numer tymczasowy wpisany do stanu dopiero po odpowiedzi oczka pod tym numerem (1 ponowienie)
      if (programujZaznaczone(t)) ustawBit64(obecne, t, true);
      else { skanNiepotwierdzone++; dziennik("Skan: oczko nie potwierdziło numeru " + etykietaMiejsca(t)); }
    }
    sendDaliCommand16(0xAB, 0x00); delay(20); sendDaliCommand16(0xAB, 0x00);
    znalezione++; skanZnalezione = znalezione; pokazPostepSkanu(znalezione);
    unsigned long t0 = millis(); while (millis() - t0 < CZAS_MIGNIECIA_WKLADKI_MS) { obsluzSerwisWTle(); delay(5); }
  }
  sendDaliCommand16(0xA1, 0x00); delay(100);
  // v1.13: wygaszenie wszystkich oczek. Stara wkładka v1.0 nie sprawdza poprawności ramki i potrafi odczytać odpowiedź
  // innego oczka jako polecenie zapalenia – po skanie świeciły losowe oczka (symulacja: v113_montaz10).
  pewneWygaszenie();

  obecneAdresy = obecne; skanWykonany = true;
  odswiezAwarieMiejsc();
  zapiszMapeMiejsc();
  int nowe = policz64(doPrzypisania()), brak = policz64(brakujaceMiejsca());
  serwisKomunikat = "Skan: odpowiada " + String(policz64(obecne)) + ", nowe do przypisania " + String(nowe) + ", nie odpowiada " + String(brak) + ".";
  if (skanBrakAdresow) serwisKomunikat += " Brak wolnych numerów tymczasowych – przypisz nowe oczka i skanuj ponownie.";
  if (skanNiepotwierdzone) serwisKomunikat += " " + String(skanNiepotwierdzone) + " oczek nie potwierdziło numeru – skanuj ponownie.";
  dziennik(serwisKomunikat);
  skanTrwa = false;
  return true;
}

// Brakujące miejsca od razu jako awaria (nie czekając na 3 braki odpowiedzi), obecne – sprawne.
void odswiezAwarieMiejsc() {
  for (int i = 0; i < 64 - DALI_ADDR_OFFSET; i++) {
    int a = i + DALI_ADDR_OFFSET;
    lampErrorState[i] = miejsceNadane(a) && !bit64(obecneAdresy, a) ? LAMP_ERROR_THRESHOLD : 0;
  }
}

// v1.7: szybki odczyt (PING 0–63, ok. 3 s) po włączeniu serwisu – po restarcie sterownika nie trzeba skanować
// od nowa. Nie znajduje oczek bez numeru (fabrycznie nowych) – do tego jest skan.
void odczytajObecne() {
  ZajetaMagistrala zajeta;
  skanTrwa = true; skanZnalezione = 0;
  uint64_t ob = 0;
  for (int a = 0; a < 64; a++) {
    sendDaliCommand16((uint8_t)((a << 1) | 0x01), 0x91);
    if (checkDaliResponse()) { ustawBit64(ob, a, true); skanZnalezione++; }
    delay(15);
    if ((a & 7) == 7) obsluzSerwisWTle();
  }
  obecneAdresy = ob; skanWykonany = true;
  odswiezAwarieMiejsc();
  serwisKomunikat = "Odczytano oczka: odpowiada " + String(policz64(ob)) + ", nie odpowiada " + String(policz64(brakujaceMiejsca())) +
                    ". Nowe oczko bez numeru? Użyj „Skanuj ponownie”.";
  dziennik(serwisKomunikat);
  skanTrwa = false;
}

// Skan z menu sterownika: podsumowanie i – przy jednej nowej oprawie – pytanie o miejsce (wymiana / dopisanie).
void runDaliCommissioning() {
  ZajetaMagistrala zajeta;   // w czasie skanu i pytania „WYMIANA?” strona nie zleci innej operacji
  if (!skanujMagistrale()) return;
  int nowe = policz64(doPrzypisania()), brak = policz64(brakujaceMiejsca());
  u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish);
  const char* okStr = "SKAN GOTOWY!"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(okStr)) / 2, 0, okStr);
  char l1[30], l2[30]; snprintf(l1, sizeof(l1), "Oczek: %d", policzMiejsca() - brak); snprintf(l2, sizeof(l2), "Nowe %d  Brak %d", nowe, brak);
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(l1)) / 2, 22, l1); u8g2.drawUTF8((128 - u8g2.getUTF8Width(l2)) / 2, 44, l2);
  u8g2.sendBuffer(); czekajObslugujac(3000);

  String ostrz = ostrzezenieSznura();
  if (ostrz.length()) {
    u8g2.clearBuffer(); u8g2.drawUTF8((128 - u8g2.getUTF8Width("UWAGA: SZNUR?")) / 2, 0, "UWAGA: SZNUR?");
    rysujPodpowiedz(22, "Nie odpowiada wiele"); rysujPodpowiedz(34, "oczek jednego rzędu."); rysujPodpowiedz(46, "Sprawdź podłączenie.");
    u8g2.sendBuffer(); czekajObslugujac(5000);
  }

  if (nowe == 1 && brak <= 1) {
    int z = pierwszy64(doPrzypisania());
    int na = brak == 1 ? pierwszy64(brakujaceMiejsca()) : pierwszeWolneMiejsce();
    if (na >= 0) {
      zacznijMruganie(z, 120000UL);
      bool tak = false, decyzja = false; unsigned long t0 = millis();
      u8g2.clearBuffer(); u8g2.setFont(u8g2_font_unifont_t_polish);
      String pyt = String(brak == 1 ? "WYMIANA " : "DOPISZ ") + etykietaMiejsca(na) + "?";
      u8g2.drawUTF8((128 - u8g2.getUTF8Width("Nowe oczko miga")) / 2, 0, "Nowe oczko miga");
      u8g2.drawUTF8((128 - u8g2.getUTF8Width(pyt.c_str())) / 2, 22, pyt.c_str());
      rysujPodpowiedz(46, "+ = tak    S1 = nie"); u8g2.sendBuffer();
      while (!decyzja && millis() - t0 < 120000UL) {
        obsluzMruganie(); obsluzSerwisWTle();
        if (digitalRead(PIN_PRZYCISK_S3) == LOW) { tak = true; decyzja = true; }
        else if (digitalRead(PIN_PRZYCISK_S1) == LOW) { decyzja = true; }
        delay(10);
      }
      while (digitalRead(PIN_PRZYCISK_S3) == LOW || digitalRead(PIN_PRZYCISK_S1) == LOW) { delay(10); }
      zakonczMruganie();
      if (tak) {
        String blad = przypiszMiejsce(z, na);
        u8g2.clearBuffer();
        if (blad.length()) { u8g2.drawUTF8((128 - u8g2.getUTF8Width("NIE UDAŁO SIĘ")) / 2, 10, "NIE UDAŁO SIĘ"); rysujPodpowiedz(40, "Powtórz skan"); }
        else { String ok = "Zapisano " + etykietaMiejsca(na); u8g2.drawUTF8((128 - u8g2.getUTF8Width(ok.c_str())) / 2, 22, ok.c_str()); }
        u8g2.sendBuffer(); czekajObslugujac(3000);
      }
      nowe = policz64(doPrzypisania());
    }
  }
  if (nowe > 0) {
    u8g2.clearBuffer(); char l3[30]; snprintf(l3, sizeof(l3), "Nowe oczka: %d", nowe);
    u8g2.drawUTF8((128 - u8g2.getUTF8Width(l3)) / 2, 0, l3);
    rysujPodpowiedz(22, "Przypisz miejsca"); rysujPodpowiedz(34, "na telefonie:"); rysujPodpowiedz(46, "menu SERWIS WiFi");
    u8g2.sendBuffer(); czekajObslugujac(5000);
  }
  u8g2.clearBuffer(); wymusOled = true;
}

// ------------------------------------------------------------------ zmiana numeru oprawy
// Oprawa o adresie z dostaje adres na. Działa z wkładkami v1.0 i v1.1 (INITIALISE 0x00 + RANDOMISE: adres losowy
// oprawy z adresem = adres krótki). Sprawdzenie: na odpowiada, z – już nie.
// v1.7: do 3 prób; wynik każdej nieudanej próby w dzienniku serwisowym.
String bladZmiany = "";   // opis ostatniego błędu zmianyAdres (pusty = sukces)

bool odpowiada(int adr) {
  for (int i = 0; i < 2; i++) { sendDaliCommand16((uint8_t)((adr << 1) | 0x01), 0x91); bool t = checkDaliResponse(); delay(15); if (t) return true; }
  return false;
}

// v1.8: PROGRAM SHORT ADDRESS dla oczka zaznaczonego wyszukiwaniem (adres losowy = adres wyszukiwania) i kontrola PING
// pod nowym numerem, z jednym ponowieniem. Wywoływane w trakcie sesji INITIALISE (przed WITHDRAW).
bool programujZaznaczone(int cel) {
  uint8_t cmd = (uint8_t)((cel << 1) | 0x01);
  for (int proba = 0; proba < 2; proba++) {
    sendDaliCommand16(0xB7, cmd); delay(50); sendDaliCommand16(0xB7, cmd); delay(200);
    if (odpowiada(cel)) return true;
  }
  return false;
}

// v1.8: wolny numer tymczasowy sprawdzony na magistrali – numer, pod którym ktoś odpowiada (oczko podłączone po
// skanie), jest oznaczany jako zajęty (w zajete i obecneAdresy) i pomijany. v1.9: także w skanie.
int wolnyTymczasowySprawdzony(uint64_t& zajete) {
  for (int i = 0; i < 64; i++) {   // v1.10: wszystkie adresy (było 8 kandydatów)
    if ((i & 7) == 7) obsluzSerwisWTle();
    int t = wolnyAdresTymczasowy(zajete);
    if (t < 0) return -1;
    if (!odpowiada(t)) return t;
    ustawBit64(zajete, t, true); ustawBit64(obecneAdresy, t, true);
    dziennik("Numer " + etykietaMiejsca(t) + " zajęty przez oczko spoza skanu – pominięty");
  }
  return -1;
}

// Kontrola po zmianie: nowy numer odpowiada, stary milczy. UWAGA: gdy pod starym numerem były dwa oczka i OBA przyjęły
// nowy numer, kontrola też wypada poprawnie – nie wykrywa takiego bliźniaka (wykrywa tylko przypadek, gdy przeszło
// jedno z nich). Bliźniaki rozdziela dopiero „Sprawdź podwójne numery”.
bool zmienAdres(int z, int na) {
  uint8_t cmd = (uint8_t)((na << 1) | 0x01);
  bladZmiany = "";
  for (int proba = 1; proba <= 3; proba++) {
    sendDaliCommand16(0xA1, 0x00); delay(30);
    sendDaliCommand16(0xA5, 0x00); delay(15); sendDaliCommand16(0xA5, 0x00); delay(15);
    sendDaliCommand16(0xA7, 0x00); delay(15); sendDaliCommand16(0xA7, 0x00); delay(120);
    setSearchAddress((uint32_t)z);
    sendDaliCommand16(0xB7, cmd); delay(50); sendDaliCommand16(0xB7, cmd); delay(200);
    sendDaliCommand16(0xA1, 0x00); delay(100);
    bool jestNowy = odpowiada(na), jestStary = odpowiada(z);
    if (jestNowy && !jestStary) {
      if (proba > 1) dziennik("Numer " + etykietaMiejsca(z) + " → " + etykietaMiejsca(na) + ": udane za " + String(proba) + ". razem");
      return true;
    }
    dziennik("Numer " + etykietaMiejsca(z) + " → " + etykietaMiejsca(na) + ", próba " + String(proba) + ": nowy " +
             (jestNowy ? "odpowiada" : "milczy") + ", stary " + (jestStary ? "odpowiada" : "milczy"));
    if (jestNowy && jestStary) {   // jedno oczko przeszło, drugie zostało: pod numerem z były co najmniej dwa
      bladZmiany = "Pod numerem " + etykietaMiejsca(z) + " są co najmniej dwa oczka. Użyj „Sprawdź podwójne numery”.";
      ustawBit64(obecneAdresy, na, true);
      return false;
    }
    if (!jestStary) { bladZmiany = "Oczko " + etykietaMiejsca(z) + " przestało odpowiadać – skanuj ponownie."; ustawBit64(obecneAdresy, z, false); return false; }
    obsluzSerwisWTle();
  }
  bladZmiany = "Oczko nie przyjęło nowego numeru (3 próby) – sprawdź zasilanie oczka i spróbuj ponownie.";
  return false;
}

// Przypisanie oprawy z (numer tymczasowy albo inne miejsce) do miejsca na. Pusty tekst = sukces.
const char* BLAD_ZAPISU = "Numer oczka zmieniony, ale zapis konfiguracji w pamięci sterownika nie powiódł się – użyj „Ponów zapis konfiguracji” (bez ponownej zmiany numeru).";

String przypiszMiejsce(int z, int na) {
  ZajetaMagistrala zajeta;
  if (skanTrwa) return "Trwa skan.";
  if (z < 0 || z > 63 || !bit64(obecneAdresy, z)) return "Ta oprawa nie odpowiada – zrób skan.";
  if (na < DALI_ADDR_OFFSET || na > 63) return "Nieprawidłowe miejsce.";
  // v1.8: źródło i cel sprawdzane na magistrali tuż przed zmianą (stan ze skanu mógł się zdezaktualizować)
  if (!odpowiada(z)) { ustawBit64(obecneAdresy, z, false); return "Oczko " + etykietaMiejsca(z) + " nie odpowiada – sprawdź podłączenie i skanuj ponownie."; }
  if (z == na) {   // oczko ma już numer tego miejsca (oczko z magazynu albo zanik zasilania przed zapisem mapy)
    ustawBit64(mapaMiejsc, na, true); lampErrorState[na - DALI_ADDR_OFFSET] = 0;
    dziennik("Przypisano " + etykietaMiejsca(na) + " (oczko miało już ten numer)");
    return zapiszMapeMiejsc() ? "" : String(BLAD_ZAPISU);
  }
  if (!bit64(obecneAdresy, na) && odpowiada(na)) {
    ustawBit64(obecneAdresy, na, true);
    return "Pod numerem " + etykietaMiejsca(na) + " odpowiada oczko, którego nie było przy skanie (podłączone później?). Skanuj ponownie.";
  }
  if (bit64(obecneAdresy, na)) {
    if (miejsceNadane(na)) return "Miejsce " + etykietaMiejsca(na) + " ma działające oczko. Najpierw je odepnij.";
    // Adres zajęty przez inną nieprzypisaną oprawę (numer tymczasowy) – przeniesienie jej na inny wolny.
    uint64_t zajete = obecneAdresy; ustawBit64(zajete, na, true); ustawBit64(zajete, z, true);
    int t = wolnyTymczasowySprawdzony(zajete);
    if (t < 0 || !zmienAdres(na, t)) return "Nie udało się zwolnić numeru " + etykietaMiejsca(na) + ". " + bladZmiany;
    ustawBit64(obecneAdresy, na, false); ustawBit64(obecneAdresy, t, true);
  }
  bool przeniesienie = miejsceNadane(z);
  if (!zmienAdres(z, na)) return bladZmiany;
  ustawBit64(obecneAdresy, z, false); ustawBit64(obecneAdresy, na, true);
  // v1.7: przy przeniesieniu miejsce z zostaje w kaskadzie bez oczka („nie odpowiada”) – wymiana albo odpięcie.
  ustawBit64(mapaMiejsc, na, true);
  if (z >= DALI_ADDR_OFFSET) lampErrorState[z - DALI_ADDR_OFFSET] = przeniesienie ? LAMP_ERROR_THRESHOLD : 0;
  lampErrorState[na - DALI_ADDR_OFFSET] = 0;
  dziennik((przeniesienie ? "Przeniesiono " : "Przypisano ") + etykietaMiejsca(z) + " → " + etykietaMiejsca(na));
  return zapiszMapeMiejsc() ? "" : String(BLAD_ZAPISU);
}

// Odpięcie miejsca: działająca oprawa dostaje numer tymczasowy (do ponownego przypisania), a nieodpowiadające
// miejsce jest zwalniane (oczko usunięte z drogi).
String odepnijMiejsce(int adr) {
  ZajetaMagistrala zajeta;
  if (skanTrwa) return "Trwa skan.";
  if (!miejsceNadane(adr)) return "Miejsce " + etykietaMiejsca(adr) + " nie jest przypisane.";
  if (!skanWykonany) return "Najpierw zrób skan.";
  ustawBit64(obecneAdresy, adr, odpowiada(adr));   // v1.10: zawsze stan na teraz (oczko mogło wrócić po skanie)
  if (bit64(obecneAdresy, adr)) {
    uint64_t zaj = obecneAdresy;
    int t = wolnyTymczasowySprawdzony(zaj);
    if (t < 0) return "Brak wolnych numerów tymczasowych.";
    if (!zmienAdres(adr, t)) return "Oczko " + etykietaMiejsca(adr) + ": " + bladZmiany;
    ustawBit64(obecneAdresy, adr, false); ustawBit64(obecneAdresy, t, true);
  }
  ustawBit64(mapaMiejsc, adr, false);
  lampErrorState[adr - DALI_ADDR_OFFSET] = 0;
  return zapiszMapeMiejsc() ? "" : String(BLAD_ZAPISU);
}

// v1.13: zamiana dwóch przypisanych (działających) oczek przez numer tymczasowy: a → T, b → a, T → b. Przy błędzie
// w połowie – opis, gdzie zostało oczko (bez zgadywania).
String zamienMiejsca(int a, int b) {
  ZajetaMagistrala zajeta;
  if (a == b || !miejsceNadane(a) || !miejsceNadane(b)) return "Wybierz dwa różne przypisane miejsca.";
  if (!odpowiada(a)) return "Oczko " + etykietaMiejsca(a) + " nie odpowiada – zamiana niemożliwa.";
  if (!odpowiada(b)) return "Oczko " + etykietaMiejsca(b) + " nie odpowiada – zamiana niemożliwa.";
  uint64_t zaj = obecneAdresy; ustawBit64(zaj, a, true); ustawBit64(zaj, b, true);
  int t = wolnyTymczasowySprawdzony(zaj);
  if (t < 0) return "Brak wolnego numeru tymczasowego.";
  if (!zmienAdres(a, t)) return "Oczko " + etykietaMiejsca(a) + ": " + bladZmiany;
  if (!zmienAdres(b, a)) {
    bool wrocilo = zmienAdres(t, a);
    return "Oczko " + etykietaMiejsca(b) + ": " + bladZmiany + (wrocilo ? " Oczko " + etykietaMiejsca(a) + " zostało na swoim miejscu." :
           " Oczko z " + etykietaMiejsca(a) + " jest pod numerem tymczasowym " + etykietaMiejsca(t) + " – zrób skan i przypisz je.");
  }
  if (!zmienAdres(t, b)) {
    ustawBit64(obecneAdresy, t, true); ustawBit64(obecneAdresy, b, false);
    return "Oczko z " + etykietaMiejsca(a) + " zostało pod numerem tymczasowym " + etykietaMiejsca(t) + " – przypisz je do " + etykietaMiejsca(b) + ".";
  }
  lampErrorState[a - DALI_ADDR_OFFSET] = 0; lampErrorState[b - DALI_ADDR_OFFSET] = 0;
  dziennik("Zamieniono " + etykietaMiejsca(a) + " ↔ " + etykietaMiejsca(b));
  return "";
}

// ------------------------------------------------------------------ podwójne numery (v1.7)
// Oczko z magazynu może mieć numer, który na tej drodze ma już inne oczko. Dwa oczka z jednym numerem
// odpowiadają razem, więc skan ich nie rozróżni: brakujące miejsce nie ma „nowego” oczka do przypisania.
// Rozdzielenie numeru A: oczka z A tracą numer (PROGRAM 0xFF), RANDOMISE daje im różne adresy losowe, inne oczka
// z numerem są wycofane (WITHDRAW), wyszukiwanie znajduje oczka z A po kolei. Jedno = wraca na A; dwa i więcej =
// wszystkie dostają numery tymczasowe do przypisania (miejsce A „nie odpowiada”, dopóki monter go nie wskaże).
// Sprawdzone w symulatorze na kodzie wkładek v1.0 i v1.1 (repo wkładek, tests: v17_blizniaki_*).
String rozdzielUwagi = "";   // v1.8: nieudane fazy rozdzielania (sprawdzPodwojne pokazuje je w wyniku)

int rozdzielNumer(int A) {
  ZajetaMagistrala zajeta;
  sendDaliCommand16(0xA1, 0x00); delay(30);
  sendDaliCommand16(0xA5, 0x00); delay(15); sendDaliCommand16(0xA5, 0x00); delay(15);
  sendDaliCommand16(0xA7, 0x00); delay(15); sendDaliCommand16(0xA7, 0x00); delay(120);
  setSearchAddress((uint32_t)A);
  sendDaliCommand16(0xB7, 0xFF); delay(50); sendDaliCommand16(0xB7, 0xFF); delay(100);
  sendDaliCommand16(0xA7, 0x00); delay(15); sendDaliCommand16(0xA7, 0x00); delay(120);
  for (int a = 0; a < 64; a++) if (bit64(obecneAdresy, a) && a != A) {
    setSearchAddress((uint32_t)a); sendDaliCommand16(0xAB, 0x00); delay(20);
    if ((a & 7) == 7) obsluzSerwisWTle();
  }
  int n = 0;
  for (int it = 0; it < 16; it++) {
    uint32_t low = 0, high = 0xFFFFFF; if (!checkAddressMatch(high)) break;
    while (low < high) { uint32_t mid = low + ((high - low) / 2); if (checkAddressMatch(mid)) high = mid; else low = mid + 1; obsluzSerwisWTle(); }
    setSearchAddress(low); sendDaliCommand16(0xA9, 0x00); delay(30);
    if (low >= 64) {   // oczko z numeru A (bez numeru); low < 64 = oczko z numerem, które nie wycofało się wcześniej
      int cel = A;
      if (n > 0) {
        uint64_t zaj = obecneAdresy; ustawBit64(zaj, A, true);
        cel = wolnyTymczasowySprawdzony(zaj);
        if (cel < 0) { rozdzielUwagi += " " + etykietaMiejsca(A) + ": brak wolnego numeru tymczasowego."; sendDaliCommand16(0xAB, 0x00); break; }
      }
      if (programujZaznaczone(cel)) { ustawBit64(obecneAdresy, cel, true); n++; }
      else rozdzielUwagi += " " + etykietaMiejsca(A) + ": oczko nie przyjęło numeru " + etykietaMiejsca(cel) + " (zostało bez numeru – znajdzie je skan).";
    }
    sendDaliCommand16(0xAB, 0x00); delay(20); sendDaliCommand16(0xAB, 0x00);
    unsigned long t0 = millis(); while (millis() - t0 < CZAS_MIGNIECIA_WKLADKI_MS) { obsluzSerwisWTle(); delay(5); }
  }
  sendDaliCommand16(0xA1, 0x00); delay(100);
  if (n >= 2) {   // pierwsze znalezione też na numer tymczasowy – nie wiadomo, które z nich stoi na miejscu A
    uint64_t zaj = obecneAdresy; ustawBit64(zaj, A, true);
    int t = wolnyTymczasowySprawdzony(zaj);
    if (t >= 0 && zmienAdres(A, t)) { ustawBit64(obecneAdresy, A, false); ustawBit64(obecneAdresy, t, true); }
    else rozdzielUwagi += " " + etykietaMiejsca(A) + ": jedno z rozdzielonych oczek zostało pod numerem " + etykietaMiejsca(A) + " – sprawdź je mrugnięciem.";
  }
  if (n == 0) ustawBit64(obecneAdresy, A, false);   // oczko bez numeru – znajdzie je następny skan
  return n;
}

void pokazEtap(const String& t) {
  u8g2.clearBuffer(); u8g2.setFontPosTop(); u8g2.setFont(u8g2_font_unifont_t_polish);
  const char* m1 = "PODWÓJNE NUMERY"; u8g2.drawUTF8((128 - u8g2.getUTF8Width(m1)) / 2, 10, m1);
  rysujPodpowiedz(38, t.c_str()); u8g2.sendBuffer();
}

// Pełny skan (oczka bez numeru dostają numery tymczasowe), potem rozdzielenie każdego numeru. Ok. 6 s na oczko.
String sprawdzPodwojne() {
  ZajetaMagistrala zajeta;
  dziennik("Sprawdzanie podwójnych numerów");
  skanEtap = "Skan przed sprawdzeniem";
  if (!skanujMagistrale()) { skanEtap = ""; return serwisKomunikat; }
  skanTrwa = true; rozdzielUwagi = "";
  uint64_t doSpr = obecneAdresy;
  int wszystkie = policz64(doSpr), nr = 0;
  String podwojne = "", zgubione = "";
  for (int a = 0; a < 64; a++) if (bit64(doSpr, a)) {
    nr++; skanEtap = "Sprawdzam " + etykietaMiejsca(a) + " (" + String(nr) + "/" + String(wszystkie) + ")";
    pokazEtap(skanEtap);
    int n = rozdzielNumer(a);
    if (n >= 2) { podwojne += String(podwojne.length() ? ", " : "") + etykietaMiejsca(a) + " (" + String(n) + " oczka)"; dziennik("Numer " + etykietaMiejsca(a) + ": " + String(n) + " oczka – rozdzielone"); }
    if (n == 0) { zgubione += String(zgubione.length() ? ", " : "") + etykietaMiejsca(a); dziennik("Numer " + etykietaMiejsca(a) + ": oczko nie odnalezione"); }
    unsigned long t0 = millis(); while (millis() - t0 < CZAS_MIGNIECIA_WKLADKI_MS) { obsluzSerwisWTle(); delay(5); }   // wkładki v1.0 po WITHDRAW
  }
  skanEtap = "";
  odswiezAwarieMiejsc();
  if (podwojne.length()) serwisKomunikat = "Podwójne numery: " + podwojne + ". Te oczka dostały numery tymczasowe – przypisz migające oczka do miejsc.";
  else serwisKomunikat = "Nie ma podwójnych numerów (sprawdzono " + String(wszystkie) + " oczek).";
  if (zgubione.length()) serwisKomunikat += " Nie odnaleziono: " + zgubione + " – skanuj ponownie.";
  if (rozdzielUwagi.length()) serwisKomunikat += " Niepełne:" + rozdzielUwagi;
  dziennik(serwisKomunikat);
  skanTrwa = false;
  u8g2.clearBuffer();
  return (zgubione.length() || rozdzielUwagi.length()) ? serwisKomunikat : String("");
}

// v1.8: wykonanie zleconej operacji (tylko z loop(), nigdy z obsługi żądania HTTP).
void wykonajOperacje() {
  ZajetaMagistrala zajeta;
  Operacja op = opZlecona; uint32_t nr = opNr; int z = opZ, na = opNa;
  opTrwa = op; opZlecona = OP_BRAK;
  { int h = zlecenieNr(nr); if (h >= 0) historia[h].stan = 1; }
  String blad = "", kod = "";
  switch (op) {
    case OP_SKAN: if (!skanujMagistrale()) blad = serwisKomunikat; break;
    case OP_ODCZYT: odczytajObecne(); break;
    case OP_PODWOJNE: if (mrugajAdr >= 0) zakonczMruganie(); blad = sprawdzPodwojne(); break;
    case OP_PRZYPISZ: {
      if (mrugajAdr == z) zakonczMruganie();
      bool przeniesienie = miejsceNadane(z);
      blad = przypiszMiejsce(z, na);
      if (!blad.length()) {
        serwisKomunikat = przeniesienie ? "Przeniesiono " + etykietaMiejsca(z) + " → " + etykietaMiejsca(na) + ". Miejsce " + etykietaMiejsca(z) +
                                          " jest teraz bez oczka – podłącz tam oczko i skanuj albo je odepnij." : "Zapisano " + etykietaMiejsca(na) + ".";
        zacznijMruganie(na, 1500);
      } else {
        dziennik("Błąd przypisania: " + blad);
        // v1.9: to samo oczko miga dalej (także przy przenoszeniu przypisanego), o ile odpowiada pod numerem z.
        // Gdy nie – nie zgadujemy: komunikat każe skanować ponownie.
        if (odpowiada(z)) zacznijMruganie(z, 300000UL);
      }
      break;
    }
    case OP_ODEPNIJ: blad = odepnijMiejsce(z); if (!blad.length()) serwisKomunikat = "Odpięto " + etykietaMiejsca(z) + "."; break;
    case OP_WYMIEN: {   // v1.13 kreator „Wymień oczko”: skan i – przy jednym nowym oczku – zapis w miejscu na
      if (mrugajAdr >= 0) zakonczMruganie();
      if (!skanujMagistrale()) { blad = serwisKomunikat; break; }
      uint64_t nw = doPrzypisania(); int ile = policz64(nw);
      if (bit64(obecneAdresy, na)) { kod = "ok"; serwisKomunikat = "Oczko " + etykietaMiejsca(na) + " odpowiada."; }
      else if (ile == 1) {
        blad = przypiszMiejsce(pierwszy64(nw), na);
        if (!blad.length()) { kod = "ok"; serwisKomunikat = "Oczko " + etykietaMiejsca(na) + " wymienione."; }
      } else if (ile == 0) { kod = "brak_nowego"; serwisKomunikat = "Skan nie znalazł nowego oczka – nowe oczko ma prawdopodobnie numer innego miejsca."; }
      else { kod = "wiele"; serwisKomunikat = "Nowych oczek: " + String(ile) + " – wskaż, gdzie miga każde."; }
      break;
    }
    case OP_ROZDZIEL: {   // v1.13 kreator: rozdzielenie jednego numeru wskazanego przez montera (zamiast sprawdzania wszystkich)
      if (mrugajAdr >= 0) zakonczMruganie();
      if (z < 0 || z > 63) { blad = "Nieprawidłowy numer."; break; }
      rozdzielUwagi = ""; skanEtap = "Rozdzielam " + etykietaMiejsca(z); skanTrwa = true; pokazEtap(skanEtap);
      int n = rozdzielNumer(z);
      skanEtap = ""; skanTrwa = false; u8g2.clearBuffer();
      odswiezAwarieMiejsc();
      if (n >= 2) { kod = "rozdzielone"; serwisKomunikat = "Pod numerem " + etykietaMiejsca(z) + " były " + String(n) + " oczka – przypisz migające oczka do miejsc."; }
      else if (n == 1 && na >= DALI_ADDR_OFFSET && na != z) {
        // v1.14: monter wskazał, że oczko z wymienianego miejsca odpowiada pod numerem z i jest tam jedyne –
        // przeniesienie z → na (miejsce z zostaje bez oczka: „nie odpowiada”, do wymiany albo odpięcia)
        blad = przypiszMiejsce(z, na);
        if (!blad.length()) { kod = "przeniesione"; serwisKomunikat = "Oczko miało numer " + etykietaMiejsca(z) + " – zapisane jako " + etykietaMiejsca(na) + "."; zacznijMruganie(na, 1500); }
      }
      else if (n == 1) { kod = "pojedyncze"; serwisKomunikat = "Pod numerem " + etykietaMiejsca(z) + " jest tylko jedno oczko."; }
      else blad = "Pod numerem " + etykietaMiejsca(z) + " nie znaleziono oczka – skanuj ponownie.";
      if (rozdzielUwagi.length()) serwisKomunikat += " Uwagi:" + rozdzielUwagi;
      dziennik(serwisKomunikat);
      break;
    }
    case OP_ZAMIEN: {   // v1.13: zamiana dwóch przypisanych oczek miejscami
      if (mrugajAdr >= 0) zakonczMruganie();
      blad = zamienMiejsca(z, na);
      if (!blad.length()) { serwisKomunikat = "Zamieniono " + etykietaMiejsca(z) + " ↔ " + etykietaMiejsca(na) + " – sprawdź test smugi."; zacznijMruganie(z, 1500); }
      break;
    }
    default: break;
  }
  // v1.13: wygaszenie wszystkich oczek po operacji (wkładki v1.0 zapalają się na odpowiedzi innych oczek); miganie
  // potwierdzające zapala oczko ponownie w następnym kroku migania.
  pewneWygaszenie();
  if (blad.length()) serwisKomunikat = blad;
  opWynikRodzaj = op; opWynikZ = z; opWynikNa = na; opWynikOk = blad.length() == 0; opWynikTekst = blad; opWynikNr = nr;
  { int h = zlecenieNr(nr); if (h >= 0) { historia[h].stan = blad.length() ? 3 : 2; historia[h].tekst = blad; historia[h].kod = kod; } }
  opTrwa = OP_BRAK; wymusOled = true;
}

// ------------------------------------------------------------------ strona i API (tylko z sieci serwisowej)
bool zSieciSerwisowej() {
  if (!trybSerwisowy || !(server.client().localIP() == SERWIS_IP)) {
    server.send(403, "text/plain; charset=utf-8", "Strona serwisowa: włącz na sterowniku menu SERWIS WiFi i połącz się z jego siecią.");
    return false;
  }
  serwisOstatniaAktywnosc = millis();
  return true;
}

void odpowiedz(const String& blad) {
  if (blad.length()) { serwisKomunikat = blad; server.send(409, "text/plain; charset=utf-8", blad); }
  else server.send(200, "text/plain; charset=utf-8", "OK");
}

int argInt(const char* n, int d) { return server.hasArg(n) ? server.arg(n).toInt() : d; }

String jsonTekst(const String& s) {
  String o = "\"";
  for (unsigned i = 0; i < s.length(); i++) { char c = s[i]; if (c == '"' || c == '\\') o += '\\'; if ((uint8_t)c >= 0x20) o += c; }
  return o + "\"";
}

void apiStan() {
  if (!zSieciSerwisowej()) return;
  uint64_t brak = brakujaceMiejsca(), nowe = doPrzypisania();
  String j; j.reserve(5200);
  j += "{\"wersja\":\"" FW_VERSION "\",\"lokalizacja\":" + jsonTekst(NAZWA_LOKALIZACJI);
  j += ",\"skan\":{\"trwa\":" + String(skanTrwa ? "true" : "false") + ",\"wykonany\":" + String(skanWykonany ? "true" : "false") +
       ",\"znalezione\":" + String(skanZnalezione) + ",\"etap\":" + jsonTekst(skanEtap) + "}";
  j += ",\"siec\":" + jsonTekst(serwisSsid);
  j += ",\"rzedy\":[" + String(dlugoscRzedu(0)) + "," + String(dlugoscRzedu(1)) + "]";
  j += ",\"mrugaj\":" + String(mrugajAdr);
  j += ",\"miejsca\":{";
  bool pierwsze = true;
  for (int r = 0; r < 2; r++) for (int k = 1; k <= dlugoscRzedu(r); k++) {
    int a = adresMiejsca(r, k);
    const char* s = !miejsceNadane(a) ? "wolne" : bit64(brak, a) ? "brak" : lampaUszkodzona(a - DALI_ADDR_OFFSET) ? "awaria" : "ok";
    j += String(pierwsze ? "" : ",") + "\"" + String(a) + "\":\"" + s + "\""; pierwsze = false;
  }
  j += "},\"nowe\":[";
  pierwsze = true;
  for (int a = 0; a < 64; a++) if (bit64(nowe, a)) { j += String(pierwsze ? "" : ",") + String(a); pierwsze = false; }
  j += "]";
  // Propozycja z eliminacji: jedno nowe oczko i jedno brakujące miejsce = wymiana.
  if (policz64(nowe) == 1 && policz64(brak) == 1) j += ",\"wymiana\":{\"z\":" + String(pierwszy64(nowe)) + ",\"na\":" + String(pierwszy64(brak)) + "}";
  j += ",\"ostrzezenie\":" + jsonTekst(ostrzezenieSznura());
  j += ",\"komunikat\":" + jsonTekst(serwisKomunikat);
  j += ",\"boot\":" + String(bootId) + ",\"zapis_oczekuje\":" + String(zapisMapyOczekuje ? "true" : "false");
  j += ",\"historia_n\":" + String(HISTORIA_N) + ",\"historia\":[";
  for (int i = 0, n = 0; i < HISTORIA_N; i++) {
    const Zlecenie& h = historia[i]; if (!h.nr || !h.id.length()) continue;
    j += String(n++ ? "," : "") + "{\"id\":" + jsonTekst(h.id) + ",\"nr\":" + String(h.nr) + ",\"rodzaj\":\"" + OP_NAZWA[h.rodzaj] +
         "\",\"z\":" + String(h.z) + ",\"na\":" + String(h.na) + ",\"stan\":" + String(h.stan) + ",\"tekst\":" + jsonTekst(h.tekst) + ",\"kod\":\"" + h.kod + "\"}";
  }
  j += "]";
  j += ",\"op\":{\"trwa\":\"" + String(OP_NAZWA[opTrwa != OP_BRAK ? opTrwa : opZlecona]) + "\",\"nr\":" + String(opNr) +
       ",\"zajety\":" + String(sterownikZajety() ? "true" : "false") +
       ",\"wynik\":{\"nr\":" + String(opWynikNr) + ",\"rodzaj\":\"" + OP_NAZWA[opWynikRodzaj] + "\",\"ok\":" + String(opWynikOk ? "true" : "false") +
       ",\"z\":" + String(opWynikZ) + ",\"na\":" + String(opWynikNa) + ",\"tekst\":" + jsonTekst(opWynikTekst) + "}}";
  j += ",\"dziennik\":[";
  for (int i = 1; i <= DZIENNIK_N; i++) {
    const String& w = dziennikWpisy[(dziennikPoz - i + DZIENNIK_N) % DZIENNIK_N];
    if (w.length()) j += String(i > 1 ? "," : "") + jsonTekst(w);
  }
  j += "],\"diag\":{\"dali_us\":" + String(daliMaxSpoznienieUs) + ",\"spoznione\":" + String(daliSpoznioneRamki) + ",\"telefony\":" + String(WiFi.softAPgetStationNum()) + ",\"wifi\":" + jsonTekst(wifiOpis) + ",\"chmura\":" + jsonTekst(chmuraOpis) + ",\"rowerzysci\":" + String(rowerzysciRazem) + ",\"ram\":" + String(ESP.getFreeHeap()) + ",\"czas_s\":" + String(millis() / 1000) + "}}";
  server.send(200, "application/json; charset=utf-8", j);
}

const char* ZAJETY = "Sterownik wykonuje operację na oczkach – poczekaj, aż się skończy.";

// Identyfikator zlecenia ze strony (id): powtórzone żądanie (przeglądarka ponawia POST po zerwanym połączeniu)
// dostaje numer już przyjętej operacji zamiast „zajęty” – operacja nie wykona się dwa razy.
// v1.9: identyfikator szukany w historii 8 zleceń i związany z rodzajem i parametrami operacji.
void zlecZeStrony(uint8_t op, int z, int na) {
  String id = server.arg("id");
  if (id.length() > 24) id = id.substring(0, 24);
  if (id.length()) for (int i = 0; i < HISTORIA_N; i++) if (historia[i].nr && historia[i].id == id) {
    if (historia[i].rodzaj != op || historia[i].z != z || historia[i].na != na) { server.send(409, "text/plain; charset=utf-8", "Identyfikator zlecenia użyty dla innej operacji."); return; }
    server.send(202, "text/plain; charset=utf-8", String(historia[i].nr)); return;   // powtórzone żądanie
  }
  if (sterownikZajety()) { server.send(409, "text/plain; charset=utf-8", ZAJETY); return; }
  zlecOperacje(op, z, na);
  Zlecenie& h = historia[historiaPoz]; historiaPoz = (historiaPoz + 1) % HISTORIA_N;
  h.id = id; h.nr = opNr; h.rodzaj = op; h.z = z; h.na = na; h.stan = 0; h.tekst = ""; h.kod = "";
  server.send(202, "text/plain; charset=utf-8", String(opNr));
}

void zarejestrujStroneSerwisowa() {
  server.on("/", HTTP_GET, []() {
    if (!zSieciSerwisowej()) return;
    server.sendHeader("Cache-Control", "no-store");
    serwisStronaOtwarta = true;
    server.send_P(200, "text/html; charset=utf-8", STRONA_SERWIS);
  });
  server.on("/api/serwis/stan", HTTP_GET, apiStan);
  // v1.8: operacje na magistrali tylko zlecane (202 + numer operacji); wykonuje je loop(), wynik w /api/serwis/stan.
  server.on("/api/serwis/skan", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_SKAN, -1, -1); });
  server.on("/api/serwis/podwojne", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_PODWOJNE, -1, -1); });
  server.on("/api/serwis/odczyt", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_ODCZYT, -1, -1); });   // v1.10: po wyniku nieznanym
  server.on("/api/serwis/przypisz", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_PRZYPISZ, argInt("z", -1), argInt("na", -1)); });
  server.on("/api/serwis/odepnij", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_ODEPNIJ, argInt("adr", -1), -1); });
  server.on("/api/serwis/wymien", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_WYMIEN, -1, argInt("na", -1)); });     // v1.13 kreator
  server.on("/api/serwis/rozdziel", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_ROZDZIEL, argInt("adr", -1), argInt("na", -1)); });
  server.on("/api/serwis/zamien", HTTP_POST, []() { if (zSieciSerwisowej()) zlecZeStrony(OP_ZAMIEN, argInt("z", -1), argInt("na", -1)); });
  server.on("/api/serwis/zapisz", HTTP_POST, []() {   // v1.9: ponowienie zapisu konfiguracji bez zmiany numerów oczek
    if (!zSieciSerwisowej()) return;
    if (sterownikZajety()) { odpowiedz(ZAJETY); return; }
    odpowiedz(zapiszMapeMiejsc() ? "" : "Zapis konfiguracji znowu się nie powiódł – zgłoś to (zrzut dziennika). Do restartu sterownik pracuje z konfiguracją z pamięci RAM.");
  });
  server.on("/api/serwis/mrugaj", HTTP_POST, []() {
    if (!zSieciSerwisowej()) return;
    if (sterownikZajety()) { odpowiedz(ZAJETY); return; }
    int a = argInt("adr", -1);
    if (a < 0) zakonczMruganie(); else zacznijMruganie(a, (unsigned long)constrain(argInt("s", 60), 1, 300) * 1000UL);
    odpowiedz("");
  });
  server.on("/api/serwis/test", HTTP_POST, []() {
    if (!zSieciSerwisowej()) return;
    if (sterownikZajety()) { odpowiedz(ZAJETY); return; }
    if (LAMPS_COUNT == 0) { odpowiedz("Brak przypisanych oczek."); return; }
    zakonczMruganie(); testKaskady = argInt("kier", 1) == 2 ? 2 : 1;
    odpowiedz("");
  });
  server.on("/api/serwis/rzedy", HTTP_POST, []() {
    if (!zSieciSerwisowej()) return;
    int l = constrain(argInt("l", 0), 0, MAX_W_RZEDZIE), p = constrain(argInt("p", 0), 0, MAX_W_RZEDZIE);
    EEPROM.write(ADDR_RZAD_L, l); EEPROM.write(ADDR_RZAD_P, p);
    odpowiedz(EEPROM.commit() ? "" : "Zapis liczby oczek w pamięci sterownika nie powiódł się – spróbuj ponownie.");
  });
  server.on("/api/serwis/koniec", HTTP_POST, []() {
    if (!zSieciSerwisowej()) return;
    if (sterownikZajety()) { odpowiedz("Sterownik kończy operację na oczkach – zakończ serwis, gdy się skończy."); return; }
    server.send(200, "text/plain; charset=utf-8", "OK");
    delay(200); wylaczSerwis();
  });
  // Okno logowania sieci w telefonie (captive portal): każdy inny adres → strona serwisowa.
  server.onNotFound([]() {
    if (trybSerwisowy && server.client().localIP() == SERWIS_IP) {
      // v1.7: po otwarciu strony sprawdzenia „czy jest internet” dostają odpowiedź poprawną – telefon nie zamyka
      // okna logowania i nie rozłącza sieci (iPhone: „Anuluj” w oknie logowania rozłączał sieć).
      String u = server.uri();
      if (serwisStronaOtwarta) {
        if (u == "/generate_204" || u == "/gen_204") { server.send(204, "text/plain", ""); return; }
        if (u.indexOf("hotspot-detect") >= 0 || u.indexOf("success.htm") >= 0) {
          server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"); return;
        }
        if (u == "/connecttest.txt") { server.send(200, "text/plain", "Microsoft Connect Test"); return; }
        if (u == "/ncsi.txt") { server.send(200, "text/plain", "Microsoft NCSI"); return; }
      }
      server.sendHeader("Location", "http://192.168.4.1/", true); server.send(302, "text/plain", "");
    } else server.send(404, "text/plain", "Not found");
  });
}

// =========================================================================
// Strona serwisowa (HTML + JavaScript w pamięci programu): mapa przejścia z rzędami L/P widzianymi od strony szafy,
// skan, przypisywanie migającego oczka do miejsca, wymiana, test smugi. Podgląd bez sterownika: tools/podglad-strony.mjs.
const char STRONA_SERWIS[] PROGMEM = R"=====(<!doctype html>
<html lang="pl"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>S-4 Dali – serwis</title>
<style>
:root{--n:#112546;--o:#FE7325;--g:#1E9E5A;--r:#D93025;--s:#58595B;--l:#E6E7E7}
*{box-sizing:border-box}body{margin:0;font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;color:#111;background:#F5F6F8}
header{background:var(--n);color:#fff;padding:10px 14px}header b{font-size:17px}header small{display:block;opacity:.8}
main{padding:10px;max-width:520px;margin:auto}
.k{background:#fff;border-radius:10px;padding:12px;margin:0 0 10px;box-shadow:0 1px 2px #0002}
.k h2{margin:0 0 6px;font-size:17px;color:var(--n)}
.w{background:#FFF4E5;border-left:5px solid var(--o)}.e{background:#FDECEA;border-left:5px solid var(--r)}
button{font:inherit;border:0;border-radius:8px;padding:11px 14px;margin:4px 4px 0 0;background:var(--n);color:#fff;font-weight:600}
button.o{background:var(--o)}button.j{background:#fff;color:var(--n);box-shadow:inset 0 0 0 2px var(--n)}button:disabled{opacity:.4}
svg{width:100%;height:auto;display:block;touch-action:manipulation}
.leg{display:flex;flex-wrap:wrap;gap:4px 12px;font-size:13px;color:var(--s)}.leg i{display:inline-block;width:12px;height:12px;border-radius:6px;margin-right:4px;vertical-align:-1px}
input{font:inherit;width:64px;padding:8px;border:1px solid #bbb;border-radius:6px}
#sheet{position:fixed;left:0;right:0;bottom:0;background:#fff;padding:14px;box-shadow:0 -2px 12px #0004;border-radius:14px 14px 0 0;display:none}
@keyframes p{50%{opacity:.25}}.mig{animation:p .7s infinite}
</style></head><body>
<header><b>S-4 Dali · serwis</b><small id="lok">…</small></header>
<main>
<div id="msg"></div>
<div class="k" id="krok"></div>
<div class="k"><h2>Przejście – widok od szafy</h2><div id="mapa"></div>
<div class="leg"><span><i style="background:var(--g)"></i>działa</span><span><i style="background:var(--r)"></i>awaria / nie odpowiada</span><span><i style="background:#fff;box-shadow:inset 0 0 0 2px #999"></i>wolne</span><span><i style="background:#FFC400"></i>miga</span></div></div>
<div class="k"><h2>Liczba oczek w rzędzie</h2>L <input id="nl" type="number" min="0" max="27"> &nbsp; P <input id="np" type="number" min="0" max="27"> <button class="j" onclick="rzedy()">Zapisz</button></div>
<div class="k"><button class="j" onclick="skan()">Skanuj ponownie</button><button class="j" onclick="podwojne()">Sprawdź podwójne numery</button><button class="j" onclick="post('mrugaj?adr=-1')">Zgaś miganie</button><button class="j" onclick="koniec()">Zakończ serwis</button><small id="wer" style="display:block;color:var(--s);margin-top:8px"></small></div>
<details class="k"><summary>Dziennik sterownika</summary><pre id="dz" style="white-space:pre-wrap;font-size:12px;margin:8px 0 0"></pre></details>
</main>
<div id="sheet"></div>
<script>
let S=null,zajety=false,bledy=0,blad='',przenies=-1,zamien=-1,kreator=null,czekamId='',czekamRodzaj='',czekamBoot=0,tm=0,odczyt=0,odczytPo=0,braki=0,odzysk='';
const $=i=>document.getElementById(i);
const wstaw=(i,h)=>{const e=$(i);if(e._h!==h){e._h=h;e.innerHTML=h;}};
const et=a=>a<10?'T'+a:((a-10)%2?'P':'L')+(((a-10)>>1)+1);
const adr=(r,k)=>10+(k-1)*2+r;
// v1.9: limit czasu obejmuje całe żądanie razem z odczytem treści; flagi zwalniane w finally.
async function zapytaj(u,o,ms,json){const c=new AbortController(),t=setTimeout(()=>c.abort(),ms);
 try{const r=await fetch(u,Object.assign({signal:c.signal,cache:'no-store'},o||{}));const body=json&&r.ok?await r.json():await r.text();return{ok:r.ok,status:r.status,body};}finally{clearTimeout(t);}}
// Krótkie polecenia (mruganie, test, liczba oczek, ponowienie zapisu). Wynik: true = OK; błąd w ramce, bez okienek.
async function post(u){if(zajety)return false;zajety=true;let ok=false;
 try{const r=await zapytaj('/api/serwis/'+u,{method:'POST'},5000);ok=r.ok;blad=ok?'':r.body;}
 catch(e){blad='Brak odpowiedzi sterownika – polecenie mogło nie dotrzeć. Spróbuj ponownie.';}
 finally{zajety=false;}teraz();return ok;}
// Operacje na oczkach: sterownik przyjmuje zlecenie z identyfikatorem strony i wykonuje je sam. Wynik strona szuka
// w historii zleceń sterownika po tym identyfikatorze (nie po numerze – inny telefon mógł zlecić coś w międzyczasie);
// zmiana identyfikatora uruchomienia (boot) = sterownik się zrestartował, wynik nieznany.
async function zlec(u){if(zajety||czekamId||!S)return;if(!u.startsWith('przypisz'))przenies=-1;if(!u.startsWith('zamien'))zamien=-1;if(S.op.zajety){blad='Sterownik jeszcze pracuje – poczekaj.';rysuj();return;}
 const id=Date.now().toString(36)+Math.random().toString(36).slice(2,8),rodzaj=u.split('?')[0];zajety=true;czekamId=id;czekamRodzaj=rodzaj;czekamBoot=S.boot;
 try{const r=await zapytaj('/api/serwis/'+u+(u.includes('?')?'&':'?')+'id='+id,{method:'POST'},8000);
  if(r.status!=202){czekamId='';
   if(rodzaj=='odczyt'&&odzysk){if(r.status==409)odzysk='potrzebny';else{odzysk='nieznany';blad='Sterownik odrzucił odczyt stanu (HTTP '+r.status+'): '+r.body;}}   // 409 = zajęty: spróbuje, gdy wolny
   else blad=r.body;}
  else if(rodzaj!='odczyt')blad='';}
 catch(e){}   // odpowiedź zaginęła: wynik (albo brak zlecenia) ustali historia w stanie
 finally{zajety=false;odczytPo=odczyt+1;braki=0;}teraz();}
// v1.10: brak wpisu w historii liczy się tylko w odczytach rozpoczętych PO zakończeniu zlecenia (nr>=odczytPo) i dopiero
// w dwóch kolejnych; oznacza „wynik nieznany” (wypadł z historii albo zlecenie nie dotarło) – nie „nie dotarło”.
// v1.11: wynik nieznany i restart sterownika włączają tryb odzyskiwania (odzysk): 'potrzebny' – odczyt stanu oczek
// zostanie zlecony, gdy sterownik będzie wolny; 'trwa' – zlecony; '' – potwierdzony, przypisywanie odblokowane;
// 'nieznany' – i odczyt ma wynik nieznany albo błąd: przycisk „Odczytaj stan ponownie” (bez automatycznego powtarzania).
const NIEZNANY='Wynik zlecenia jest nieznany (sterownik już go nie pamięta albo zlecenie do niego nie dotarło).';
function nieznany(tekst){const bylOdczyt=czekamRodzaj=='odczyt'&&odzysk;czekamId='';przenies=-1;
 if(bylOdczyt){odzysk='nieznany';blad='Nie udało się potwierdzić odczytu stanu oczek.';}else{odzysk='potrzebny';blad=tekst;}}
function wynik(nr){if(!czekamId)return;
 if(S.boot!==czekamBoot){nieznany('Sterownik uruchomił się ponownie. Wynik operacji jest nieznany.');return;}
 const h=(S.historia||[]).find(x=>x.id===czekamId);
 if(!h){if(zajety||nr<odczytPo)return;if(++braki<2)return;nieznany(NIEZNANY);return;}
 if(h.stan<2)return;
 czekamId='';
 if(kreator&&(h.rodzaj=='wymien'||h.rodzaj=='rozdziel')){krWynik(h);return;}
 if(h.rodzaj=='zamien'){zamien=-1;if(h.stan==3)blad=h.tekst;return;}
 if(h.rodzaj=='odczyt'&&odzysk){if(h.stan==2){odzysk='';blad='Stan oczek odczytany – sprawdź rysunek; jeśli oczko nie ma miejsca, przypisz je jeszcze raz.';}else{odzysk='nieznany';blad=h.tekst;}return;}
 if(h.stan==3)blad=h.tekst;else if(h.rodzaj=='przypisz'){przenies=-1;blad='';if(S.nowe.length)setTimeout(()=>pokaz(0),1600);}}
function odzyskuj(){if(odzysk=='potrzebny'&&S&&!S.op.zajety&&!czekamId&&!zajety){odzysk='trwa';zlec('odczyt');}}
async function stan(){const nr=++odczyt;try{const r=await zapytaj('/api/serwis/stan',{},4000,true);if(!r.ok){$('krok').innerHTML='<h2>Brak dostępu</h2>'+r.body;return;}S=r.body;bledy=0;wynik(nr);odzyskuj();}catch(e){bledy++;}if(S)rysuj();}
let wToku=false,znowu=false;
async function petla(){if(wToku){znowu=true;return;}wToku=true;clearTimeout(tm);await stan();wToku=false;if(znowu){znowu=false;return petla();}tm=setTimeout(petla,1500);}
function teraz(){petla();}
function skan(){if(confirm('Skanować magistralę? Trwa ok. 3 s na każde oczko.'))zlec('skan');}
function podwojne(){if(confirm('Sprawdzić, czy dwa oczka nie mają tego samego numeru (np. oczko z magazynu)? Oczka będą mrugać. Trwa ok. 6 s na każde oczko.'))zlec('podwojne');}
function koniec(){if(confirm('Wyłączyć sieć serwisową?'))post('koniec');}
function rzedy(l,p){post('rzedy?l='+(+$(l||'nl').value||0)+'&p='+(+$(p||'np').value||0));}
function nowaMiga(){return S.nowe.includes(S.mrugaj)?S.mrugaj:-1;}
function pokaz(i){przenies=-1;post('mrugaj?adr='+S.nowe[i]+'&s=300');}
function inne(){const i=S.nowe.indexOf(S.mrugaj);pokaz((i+1)%S.nowe.length);}
function przenos(a){przenies=a;zamknij();post('mrugaj?adr='+a+'&s=300');}
function anulujPrz(){przenies=-1;post('mrugaj?adr=-1');}
function przypisz(z,a){zlec('przypisz?z='+z+'&na='+a);}
// v1.13: kreator „Wymień oczko”: zaloz → szukam (skan) → [gotowe | wskaz → rozdzielam → przypisz → gotowe | brak]
function wymien(a){zamknij();kreator={m:a,krok:'zaloz'};blad='';rysuj();}
function przerwijKr(){kreator=null;post('mrugaj?adr=-1');}
function krDalej(){kreator.krok='szukam';zlec('wymien?na='+kreator.m);}
function krWynik(h){if(h.stan==3){kreator.krok='blad';kreator.tekst=h.tekst;return;}
 if(h.rodzaj=='wymien'){if(h.kod=='ok')kreator.krok='gotowe';else if(h.kod=='brak_nowego')krWskaz(0);else{kreator.krok='przypisz';setTimeout(()=>pokaz(0),800);}}
 else if(h.kod=='rozdzielone'){kreator.krok='przypisz';setTimeout(()=>pokaz(0),800);}else if(h.kod=='przeniesione')kreator.krok='gotowe';else kreator.krok='brak';}
// Miejsca do sprawdzenia: działające oczka (oczko z magazynu ma numer jednego z nich).
// v1.15: krok po kroku – miejsce miga (do 2 min), dopóki serwisant nie odpowie; wcześniej mruganie przechodziło samo co 1,5 s.
function krWskaz(od){const k=Object.keys(S.miejsca).map(Number).filter(a=>a!=kreator.m&&(S.miejsca[a]=='ok'||S.miejsca[a]=='awaria'));
 kreator.krok='wskaz';kreator.kand=k;kreator.i=od;krKrok();}
function krKrok(){if(!kreator||kreator.krok!='wskaz')return;if(kreator.i>=kreator.kand.length){kreator.krok='brak';kreator.x=-1;post('mrugaj?adr=-1');return;}
 kreator.x=kreator.kand[kreator.i];post('mrugaj?adr='+kreator.x+'&s=120');}
function krTak(){kreator.krok='rozdzielam';zlec('rozdziel?adr='+kreator.x+'&na='+kreator.m);}
function krNie(){kreator.i++;krKrok();rysuj();}
function krWstecz(){if(kreator.i>0){kreator.i--;krKrok();rysuj();}}
function krRysuj(){const m=et(kreator.m),x=kreator.x>=0?et(kreator.x):'',w='<button class="j" onclick="przerwijKr()">Przerwij</button>',
 kr={zaloz:'1. Załóż nowe oczko w miejscu <b>'+m+'</b> i podłącz przewody.<br>2. Stuknij <b>Dalej</b> – sterownik sam je znajdzie (ok. 3 s na oczko).<br><button class="o" onclick="krDalej()">Dalej</button>'+w,
  szukam:'Szukam nowego oczka…'+(S.skan.trwa?' Znaleziono oczek: <b>'+S.skan.znalezione+'</b>.':''),
  wskaz:'Nowe oczko ma numer innego miejsca – szukamy którego. <b>Stań przy '+m+'</b> i patrz tylko na to oczko.<br>Teraz mruga miejsce <b>'+x+'</b> (sprawdzane '+(kreator.i+1)+' z '+(kreator.kand||[]).length+'). Miga, dopóki nie odpowiesz – bez pośpiechu.<br><b>Czy oczko w '+m+' mruga?</b><br><button class="o" onclick="krTak()">Tak, mruga</button><button onclick="krNie()">Nie – następne miejsce</button>'+(kreator.i>0?'<button class="j" onclick="krWstecz()">Wróć do poprzedniego</button>':'')+w,
  rozdzielam:'Sprawdzam numer '+x+' i zapisuję oczko w '+m+' (ok. 10 s)…',
  przypisz:(nowaMiga()>=0?'Miga jedno z oczek. Stuknij na rysunku miejsce, w którym miga'+(x?' (<b>'+m+'</b> albo <b>'+x+'</b>)':'')+'.':'Za chwilę zacznie migać oczko do przypisania…<br><button class="o" onclick="pokaz(0)">Mrugnij</button>')+w,
  brak:'Nie znaleziono nowego oczka. Sprawdź: zasilanie i przewody nowego oczka w '+m+', czy świeci w teście smugi.<br><button class="o" onclick="kreator.krok=\'zaloz\';rysuj()">Spróbuj ponownie</button><button class="j" onclick="podwojne()">Sprawdź wszystkie numery</button>'+w,
  blad:(kreator.tekst||'Błąd')+'<br><button class="o" onclick="kreator.krok=\'zaloz\';rysuj()">Spróbuj ponownie</button>'+w,
  gotowe:'✓ Oczko <b>'+m+'</b> wymienione. Sprawdź smugę.<br><button class="o" onclick="post(\'test?kier=1\')">Test smugi</button><button class="j" onclick="kreator=null;rysuj()">Zamknij</button>'};
 return '<h2>Wymiana oczka '+m+'</h2>'+kr[kreator.krok];}
function klik(a){
 if(czekamId||S.op.zajety||odzysk){blad=odzysk?'Najpierw odczyt stanu oczek – poczekaj albo użyj „Odczytaj stan ponownie”.':'Sterownik jeszcze pracuje – poczekaj.';rysuj();return;}
 const st=S.miejsca[a]||'wolne';
 if(zamien>=0){if(a==zamien)return;if(st!='ok'&&st!='awaria'){blad='Zamienić można tylko z działającym (zielonym) oczkiem. Na wolne albo czerwone miejsce użyj „Przenieś”.';rysuj();return;}
  if(confirm('Zamienić oczka '+et(zamien)+' i '+et(a)+' miejscami?'))zlec('zamien?z='+zamien+'&na='+a);return;}
 const z=przenies>=0?przenies:nowaMiga();
 if(z>=0){
  if(przenies>=0&&z==a)return;
  if(st=='ok'||st=='awaria'){blad=et(a)+' ma działające oczko. Jeśli to pomyłka, odepnij najpierw '+et(a)+'.';rysuj();return;}
  if(confirm(przenies>=0?'Przenieść oczko '+et(z)+' na miejsce '+et(a)+'? Miejsce '+et(z)+' zostanie bez oczka.':'Migające oczko to '+et(a)+'?'))przypisz(z,a);
  return;}
 const sh=$('sheet');sh.style.display='block';
 sh.innerHTML='<h2 style="margin:0 0 8px;color:var(--n)">Miejsce '+et(a)+'</h2><div style="color:var(--s);margin-bottom:6px">'+({ok:'Oczko działa.',awaria:'Awaria oczka.',brak:'Oczko nie odpowiada (odłączone lub uszkodzone).',wolne:'Miejsce wolne.'})[st]+'</div>'+
 (st=='brak'||st=='awaria'?'<button class="o" onclick="wymien('+a+')">Wymień to oczko</button>':'')+
 (st!='wolne'&&st!='brak'?'<button onclick="post(\'mrugaj?adr='+a+'&s=3\');zamknij()">Mrugnij 3 s</button><button onclick="przenos('+a+')">Przenieś na inne miejsce</button><button onclick="zamien='+a+';zamknij();post(\'mrugaj?adr='+a+'&s=300\')">Zamień z innym</button>':'')+
 (st!='wolne'?'<button class="j" onclick="if(confirm(\'Odpiąć '+et(a)+'? Działające oczko dostanie numer tymczasowy do ponownego przypisania.\'))zlec(\'odepnij?adr='+a+'\');zamknij()">Odepnij</button>':'')+
 '<button class="j" onclick="zamknij()">Zamknij</button>';}
function zamknij(){$('sheet').style.display='none';}
function rysuj(){
 $('lok').textContent=S.lokalizacja;$('wer').textContent='S-4 Dali v'+S.wersja+(S.diag.wifi?' · Wi-Fi (przed serwisem): '+S.diag.wifi:'')+' · eaCloud: '+S.diag.chmura+' · rowerzyści od startu: '+S.diag.rowerzysci+' · sieć '+S.siec+' (hasło stałe, na ekranie sterownika) · telefony '+S.diag.telefony+' · DALI '+S.diag.dali_us+' µs, spóźnione ramki '+S.diag.spoznione+' · RAM '+S.diag.ram;
 if(document.activeElement.tagName!='INPUT'){$('nl').value=S.rzedy[0];$('np').value=S.rzedy[1];}
 let m='';
 if(bledy>=2)m+='<div class="k e"><b>Brak odpowiedzi sterownika.</b> Sprawdź w ustawieniach WiFi, czy telefon jest połączony z <b>'+S.siec+'</b> (hasło na ekranie sterownika), i czy ekran sterownika działa (restart?). Android: wyłącz dane komórkowe. Strona wróci sama'+(czekamId?'; wynik zleconej operacji pokaże się po odzyskaniu połączenia':'')+'.</div>';
 if(S.zapis_oczekuje)m+='<div class="k e"><b>Konfiguracja nie jest zapisana w pamięci sterownika</b> (po restarcie wróciłaby poprzednia). Numery oczek są już zmienione. <button class="o" onclick="post(\'zapisz\')">Ponów zapis konfiguracji</button></div>';
 if(blad)m+='<div class="k e">'+blad+' <button class="j" onclick="blad=\'\';rysuj()">OK</button></div>';
 if(S.ostrzezenie)m+='<div class="k w"><b>Uwaga:</b> '+S.ostrzezenie+'</div>';if(S.komunikat)m+='<div class="k">'+S.komunikat+'</div>';wstaw('msg',m);
 const z=nowaMiga(),brak=Object.keys(S.miejsca).filter(a=>S.miejsca[a]=='brak').map(Number);let k='';
 const op={przypisz:'Zapisuję miejsce oczka',odepnij:'Odpinam oczko',odczyt:'Odczytuję stan oczek',skan:'Skanowanie',podwojne:'Sprawdzanie podwójnych numerów'}[S.op.trwa];
 if(kreator&&kreator.krok=='przypisz'&&!czekamId&&!S.op.zajety&&!S.nowe.length)kreator.krok='gotowe';
 if(kreator)k=krRysuj();
 else if(zamien>=0)k='<h2>Zamień oczko '+et(zamien)+'</h2>Oczko '+et(zamien)+' miga. Stuknij drugie działające (zielone) oczko – zamienią się miejscami.<br><button class="j" onclick="zamien=-1;post(\'mrugaj?adr=-1\')">Anuluj</button>';
 else if(S.skan.trwa)k='<h2>'+(S.skan.etap?'Sprawdzanie podwójnych numerów…':'Skanowanie…')+'</h2>'+(S.skan.etap?S.skan.etap+'. Oczka mrugają przy sprawdzaniu.':'Znaleziono oczek: <b>'+S.skan.znalezione+'</b>. Nowe oczka mrugają dwa razy, gdy skaner je znajdzie.');
 else if(odzysk=='nieznany')k='<h2>Stan oczek niepotwierdzony</h2>Przypisywanie jest wstrzymane, dopóki sterownik nie odczyta stanu oczek.<br><button class="o" onclick="odzysk=\'potrzebny\';blad=\'\';teraz()">Odczytaj stan ponownie</button>';
 else if(odzysk)k='<h2>Odczytuję stan oczek…</h2>'+(S.op.zajety?'Sterownik kończy inną operację – odczyt ruszy zaraz po niej.':'Poczekaj na wynik.')+' Przypisywanie jest wstrzymane do potwierdzenia stanu.';
 else if(S.op.zajety||czekamId)k='<h2>'+(op||'Sterownik pracuje')+'…</h2>Poczekaj na wynik – nie stukaj ponownie.';
 else if(przenies>=0)k='<h2>Przenieś oczko '+et(przenies)+'</h2>Oczko '+et(przenies)+' miga. Stuknij na rysunku miejsce, w którym naprawdę stoi.<br><button class="j" onclick="anulujPrz()">Anuluj</button>';
 else if(!S.rzedy[0]&&!S.rzedy[1])k='<h2>Ile oczek ma każdy rząd?</h2>Patrząc od szafy na drogę: rząd <b>L</b> po lewej, rząd <b>P</b> po prawej.<br>L <input id="nl0" type="number" min="0" max="27"> &nbsp; P <input id="np0" type="number" min="0" max="27"> <button class="o" onclick="rzedy(\'nl0\',\'np0\')">Zapisz</button>';
 else if(!S.skan.wykonany)k='<h2>1. Skan</h2>Podłącz wszystkie oczka i zrób skan. Skaner nie zmienia numerów oczek, które już mają miejsce.<br><button class="o" onclick="skan()">Skanuj</button>';
 else if(S.wymiana&&z<0)k='<h2>Wymiana oczka</h2>Jedno nowe oczko, nie odpowiada <b>'+et(S.wymiana.na)+'</b>.<br><button class="o" onclick="if(confirm(\'Nowe oczko na miejsce '+et(S.wymiana.na)+'?\'))przypisz('+S.wymiana.z+','+S.wymiana.na+')">Tak, to wymiana '+et(S.wymiana.na)+'</button><button class="j" onclick="pokaz(0)">Nie – wskażę miejsce</button>';
 else if(S.nowe.length&&z<0)k='<h2>2. Przypisz nowe oczka ('+S.nowe.length+')</h2>Sterownik będzie migał po jednym nowym oczku. Stań przy drodze, znajdź migające oczko i stuknij jego miejsce na rysunku.<br><button class="o" onclick="pokaz(0)">Zacznij</button>';
 else if(z>=0)k='<h2>Miga nowe oczko '+et(z)+'</h2>Zostało do przypisania: <b>'+S.nowe.length+'</b>. Stuknij na rysunku miejsce, w którym miga.<br><button class="j" onclick="inne()">Nie widzę – mrugnij innym</button>';
 else if(brak.length)k='<h2>Nie odpowiada: '+brak.map(et).join(', ')+'</h2>Sprawdź przewody i zasilanie tych oczek (cały rząd = sznur), potem skanuj ponownie. Wymienione oczko: podłącz nowe i skanuj – sterownik zaproponuje jego miejsce.<br><button class="o" onclick="skan()">Skanuj ponownie</button>'+
  '<p style="margin:10px 0 0"><b>Wymieniasz oczko?</b> Stuknij czerwone miejsce na rysunku → <b>Wymień to oczko</b> – kreator poprowadzi krok po kroku (także oczko z magazynu).</p>';
 else k='<h2>3. Test smugi</h2>Wszystkie oczka mają miejsca. Sprawdź, czy smuga idzie równo w obu rzędach.<br><button class="o" onclick="post(\'test?kier=1\')">Test 1 → koniec</button><button class="o" onclick="post(\'test?kier=2\')">Test koniec → 1</button>';
 wstaw('krok',k);
 wstaw('dz',(S.dziennik||[]).join('\n'));
 // mapa: dół = szafa i serwisant, rzędy L (lewy) i P (prawy) od 1 przy szafie do końca po drugiej stronie
 const n=Math.max(S.rzedy[0],S.rzedy[1],1),dy=34,top=40,H=top+n*dy+20,W=360,xL=120,xP=240;
 let s='<svg viewBox="0 0 '+W+' '+(H+80)+'" font-family="system-ui,sans-serif">';
 s+='<rect x="0" y="0" width="'+W+'" height="26" fill="#cfd8dc"/><text x="'+W/2+'" y="17" text-anchor="middle" font-size="12" fill="#455a64">drugi brzeg drogi</text>';
 s+='<rect x="0" y="26" width="'+W+'" height="'+(H-26)+'" fill="#5f6368"/>';
 for(let y=top;y<H-10;y+=18)s+='<rect x="'+(xL+16)+'" y="'+y+'" width="'+(xP-xL-32)+'" height="9" fill="#fff" opacity=".85"/>';
 s+='<line x1="0" x2="'+(xL-14)+'" y1="'+(H/2+13)+'" y2="'+(H/2+13)+'" stroke="#fff" stroke-dasharray="14 10" stroke-width="3"/><line x1="'+(xP+14)+'" x2="'+W+'" y1="'+(H/2+13)+'" y2="'+(H/2+13)+'" stroke="#fff" stroke-dasharray="14 10" stroke-width="3"/>';
 for(let r=0;r<2;r++)for(let i=1;i<=S.rzedy[r];i++){const a=adr(r,i),st=S.miejsca[a]||'wolne',x=r?xP:xL,y=H-14-i*dy+dy/2,
  c=st=='ok'?'var(--g)':(st=='wolne'?'#fff':'var(--r)'),mig=S.mrugaj==a;
  s+='<g onclick="klik('+a+')" style="cursor:pointer"><circle cx="'+x+'" cy="'+y+'" r="15" fill="'+(mig?'#FFC400':c)+'" stroke="'+(st=='wolne'?'#bbb':'#fff')+'" stroke-width="2"'+(st=='brak'?' stroke-dasharray="4 3"':'')+(mig?' class="mig"':'')+'/>'+
  '<text x="'+x+'" y="'+(y+4)+'" text-anchor="middle" font-size="11" font-weight="700" fill="'+(st=='wolne'?'#444':'#fff')+'">'+et(a)+'</text>'+
  '<rect x="'+(x-30)+'" y="'+(y-17)+'" width="60" height="34" fill="transparent"/></g>';}
 s+='<rect x="0" y="'+H+'" width="'+W+'" height="80" fill="#cfd8dc"/>';
 s+='<text x="'+(xL-48)+'" y="'+(H+22)+'" text-anchor="middle" font-size="15" font-weight="800" fill="var(--n)">L</text><text x="'+(xL-48)+'" y="'+(H+38)+'" text-anchor="middle" font-size="11" fill="#455a64">lewy</text>';
 s+='<text x="'+(xP+48)+'" y="'+(H+22)+'" text-anchor="middle" font-size="15" font-weight="800" fill="var(--n)">P</text><text x="'+(xP+48)+'" y="'+(H+38)+'" text-anchor="middle" font-size="11" fill="#455a64">prawy</text>';
 s+='<rect x="'+(W/2-30)+'" y="'+(H+8)+'" width="60" height="30" rx="3" fill="var(--n)"/><text x="'+W/2+'" y="'+(H+28)+'" text-anchor="middle" font-size="11" fill="#fff" font-weight="700">SZAFA</text>';
 s+='<circle cx="'+W/2+'" cy="'+(H+50)+'" r="6" fill="var(--o)"/><text x="'+W/2+'" y="'+(H+72)+'" text-anchor="middle" font-size="12" font-weight="700" fill="var(--o)">TU STOISZ – twarzą do drogi</text>';
 s+='</svg>';wstaw('mapa',s);
}
petla();
</script></body></html>)=====";
