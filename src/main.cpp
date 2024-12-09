// Librerías necesarias
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h" 
#include "Sprite.h"

// Pines de conexión entre el Arduino Mega y el TFT LCD ILI9341
#define TFT_DC 7
#define TFT_CS 6
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 10
#define TFT_MISO 12

#define botonRight 18
#define botonLeft 19
#define botonUp 20
#define botonDown 21

#define BUZZER_PIN 9 


// Definición del laberinto (1 = muro, 0 = pasillo)
//matriz 10x10
#define LABERINTO_ANCHO  10 
#define LABERINTO_ALTO 10

int laberinto[LABERINTO_ALTO][LABERINTO_ANCHO] = {

    {1, 0, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 1},
    {1, 1, 0, 0, 1, 0, 1, 1, 0, 0},
    {1, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {1, 0, 1, 0, 1, 1, 1, 0, 1, 1},
    {1, 0, 1, 0, 0, 0, 1, 0, 1, 0},
    {1, 0, 0, 0, 1, 0, 1, 0, 1, 0},
    {1, 1, 0, 1, 0, 0, 1, 0, 0, 0},
    {1, 1, 0, 1, 1, 1, 1, 1, 0, 1}
};

// Define las dimensiones del display 
const int XMAX = 240;
const int YMAX = 320;

// Variables para Player1
//Posición actual del jugador
int x1 = 0;               
int y1 = 0;

//Posición anterior del jugador
int x1_prev = 0;
int y1_prev = 0;

// Variables para los objetos amarillos
const int numObjetos = 6;                               //numero de objetos
int objetosX[numObjetos];                               //almacena la posicion X de los objetos amarillos
int objetosY[numObjetos];                               //almacena la posicion Y de los objetos amarillos
bool objetoExistente[numObjetos];                      
int objetosRecolectados = 0;                            //Contador de objetos recolectados

// Variables para los objetos verdes
unsigned long ultimaAparicionObjetosVerdes = 0;
const unsigned long intervaloObjetosVerdes = 20000;     //20 segundos entre apariciones
const unsigned long duracionObjetosVerdes = 15000;      //15 segundos de duración
const int numObjetosVerdes = 1;                         //numero de objetos verdes
int objetosVerdesX[numObjetosVerdes];                   
int objetosVerdesY[numObjetosVerdes];                   
bool objetoVerdesExistente[numObjetosVerdes];           


// Direcciones de movimiento
const uint8_t UP = 0;
const uint8_t DOWN = 1;
const uint8_t RIGHT = 2;
const uint8_t LEFT = 3;

Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
GFXcanvas1 canvas1(32, 32); 

//Se declaran las funciones

void setPlayer1Position(int x, int y); 
void animatePlayers(void);
void moverPlayer1(uint8_t direccion);
void moverPlayerDerecha(void);
void moverPlayerIzquierda(void);
void moverPlayerArriba(void);
void moverPlayerAbajo(void);
void mostrarTiempoDeJuego(void);
void generarPosicionesAleatoriasObjetos(void);
void generarPosicionesAleatoriasObjetosVerdes(void);
void dibujarObjetos(void);
void dibujarObjetosVerdes(void);
void comprobarColision(void);
void comprobarColisionVerde(void);
void mostrarObjetosRecolectados(void); 
void mostrarGanaste(void);  
void mostrarPerdiste(void);  
void borrarObjetosVerdes(void);
void dibujarLaberinto(void);

unsigned long startTime;
unsigned long gameTime = 180000;                    // Tiempo de juego 3 minutos en milisegundos (180000 ms)
bool juegoTerminado = false;                        // Variable para controlar si el juego terminó

void setup() {
    // Inicializar la semilla aleatoria de manera más robusta, diferente en cada ejecución
    randomSeed(analogRead(0) + millis()); 

    //Inicializa la comunicación serial
    Serial.begin(9600);
    Serial.println("Serial inicializado");

    //Manejo de las interrupciones
    attachInterrupt(digitalPinToInterrupt(botonRight), moverPlayerDerecha, HIGH);
    attachInterrupt(digitalPinToInterrupt(botonLeft), moverPlayerIzquierda, HIGH);
    attachInterrupt(digitalPinToInterrupt(botonUp), moverPlayerArriba, HIGH);
    attachInterrupt(digitalPinToInterrupt(botonDown), moverPlayerAbajo, HIGH);

    //Inicializar y limpiar la pantalla
    screen.begin();
    screen.fillScreen(ILI9341_BLACK);

    //Configura el pin como salida
    pinMode(BUZZER_PIN, OUTPUT);
    
    digitalWrite(BUZZER_PIN, LOW);

    dibujarLaberinto();       

    
    delay(1000);

    startTime = millis();         
    sei();                   //activa interrupciones globales

    generarPosicionesAleatoriasObjetos();
    dibujarObjetos(); 
    generarPosicionesAleatoriasObjetosVerdes();

    
}

void loop() {   
    
    setPlayer1Position(0, YMAX - 32);  //Coloca al jugador en la parte inferior de la pantalla.
    animatePlayers();                  //Controla el movimiento, la animación...
}

// Fija la posición del jugador
void setPlayer1Position(int x, int y) {
    x1 = x;
    y1 = y;
}


//Generear posiciones aleatorias de los objetos amarillos
void generarPosicionesAleatoriasObjetos() {

    //Define variables para ajustar las posiciones de los objetos
    int objetoSize = 16;               //Tamaño del objeto en pixeles
    int margenSuperior = 36;           //Margen en píxeles desde la parte superior de la pantalla 
    int cellSize = 24;                 //Tamaño de cada celda del laberinto en píxeles.

    for (int k = 0; k < numObjetos; k++) {  
        
        bool posicionValida = false;        
        
        while (!posicionValida) {                  

        //Genera números aleatorios para las coordenadas de la fila y la columna 
            int fila = random(LABERINTO_ALTO);
            int columna = random(LABERINTO_ANCHO);

            // Verificar si la celda es un pasillo (0)
            if (laberinto[fila][columna] == 0) {
                // Calcular coordenadas para centrar el objeto en el pasillo
                int x = columna * cellSize + (cellSize - objetoSize) / 2;
                int y = fila * cellSize + margenSuperior + (cellSize - objetoSize) / 2;

                //Guarda las coordenadas calculadas
                objetosX[k] = x;
                objetosY[k] = y;

                // Verificar que no haya otro objeto en esta posición
                posicionValida = true;
                for (int j = 0; j < k; j++) {      
                    if (abs(objetosX[k] - objetosX[j]) < objetoSize &&   
                        abs(objetosY[k] - objetosY[j]) < objetoSize) {

                        //Si los objetos están demasiado cerca o superpuestos marca como false 
                        posicionValida = false;
                        break;
                    }
                }
            }
        }
        
        objetoExistente[k] = true;
    }
}

// Dibujar objetos amarillos
void dibujarObjetos() {
    int objetoSize = 16; 
    for (int i = 0; i < numObjetos; i++) {  
        if (objetoExistente[i]) { 
            
            screen.fillRect(objetosX[i], objetosY[i], objetoSize, objetoSize, ILI9341_YELLOW); 
        }
    }
}

//Generar posiciones aleatorias de los objetos verdes 
void generarPosicionesAleatoriasObjetosVerdes() {
    int objetoVerdeSize = 16;
    int margenSuperior = 36;
    int cellSize = 24;

    for (int i = 0; i < numObjetosVerdes; i++) {
        bool posicionValida = false;  
        
        while (!posicionValida) {
            int fila = random(LABERINTO_ALTO);
            int columna = random(LABERINTO_ANCHO);

            // Verificar si la celda es un pasillo (0)
            if (laberinto[fila][columna] == 0) {
                // Calcular coordenadas para centrar el objeto en el pasillo
                int x = columna * cellSize + (cellSize - objetoVerdeSize) / 2;
                int y = fila * cellSize + margenSuperior + (cellSize - objetoVerdeSize) / 2;

                objetosVerdesX[i] = x;
                objetosVerdesY[i] = y;

                // Verificar que no haya otro objeto en esta posición
                posicionValida = true;
                
                // Verificar con objetos amarillos
                for (int j = 0; j < numObjetos; j++) {
                    if (objetoExistente[j] &&
                        abs(objetosVerdesX[i] - objetosX[j]) < objetoVerdeSize && 
                        abs(objetosVerdesY[i] - objetosY[j]) < objetoVerdeSize) {
                        posicionValida = false;
                        break;
                    }
                }

                // Verificar con otros objetos verdes
                for (int j = 0; j < i; j++) {
                    if (abs(objetosVerdesX[i] - objetosVerdesX[j]) < objetoVerdeSize && 
                        abs(objetosVerdesY[i] - objetosVerdesY[j]) < objetoVerdeSize) {
                        posicionValida = false;
                        break;
                    }
                }
            }
        }
        
        objetoVerdesExistente[i] = true;
    }
}


void dibujarObjetosVerdes() {
    int objetoVerdeSize = 16;
    for (int i = 0; i < numObjetosVerdes; i++) {
        if (objetoVerdesExistente[i]) {  
            screen.fillRect(objetosVerdesX[i], objetosVerdesY[i], objetoVerdeSize, objetoVerdeSize, ILI9341_GREEN); 
        }
    }
}


// Anima al jugador
void animatePlayers(void) {
    int count1 = 0;    
    while (true) {
        // Borra la posición anterior del jugador (rellena con el fondo)
        screen.fillRect(x1_prev, y1_prev, 18, 18, ILI9341_BLACK);

        // Dibuja al jugador en la nueva posición

        canvas1.fillScreen(0); //limpia
        canvas1.drawBitmap(0, 0, dino[count1], 18, 18, ILI9341_WHITE);
        screen.drawBitmap(x1, y1, canvas1.getBuffer(), canvas1.width(), canvas1.height(), ILI9341_WHITE);  //Transfiere el contenido del canvas al lugar correcto en la pantalla

        //Actualiza las coordenadas
        x1_prev = x1;  
        y1_prev = y1;

        
        count1++;    
        if(count1 == 2)   
            count1 = 0;

        // Comprobar colisiones
        comprobarColision();  
        comprobarColisionVerde();  

        mostrarTiempoDeJuego();
        mostrarObjetosRecolectados();

        // Verificar si el jugador ha ganado
        if (objetosRecolectados >= numObjetos) {
            mostrarGanaste();
            juegoTerminado = true;
            break;
        }

        // Si el tiempo se ha agotado
        if (millis() - startTime >= gameTime) {   
            mostrarPerdiste();
            juegoTerminado = true;
            break;
        }

        delay(50);  
    }
}


//Dibujar laberinto
void dibujarLaberinto() {
    int cellSize = 24; // Tamaño de cada celda (más grande para pasillos anchos)
    int muroGrosor = 4; // Grosor de los muros (delgados)
    int margenSuperior = 36; 

    for (int i = 0; i < LABERINTO_ALTO; i++) {
        for (int j = 0; j < LABERINTO_ANCHO; j++) {  
            if (laberinto[i][j] == 1) {     
                // Dibujar los muros
                // Pared superior
                screen.fillRect(j * cellSize, i * cellSize + margenSuperior, cellSize, muroGrosor, ILI9341_WHITE);
                // Pared inferior
                screen.fillRect(j * cellSize, (i + 1) * cellSize - muroGrosor + margenSuperior, cellSize, muroGrosor, ILI9341_WHITE);
                // Pared izquierda
                screen.fillRect(j * cellSize, i * cellSize + margenSuperior, muroGrosor, cellSize, ILI9341_WHITE);
                // Pared derecha
                screen.fillRect((j + 1) * cellSize - muroGrosor, i * cellSize + margenSuperior, muroGrosor, cellSize, ILI9341_WHITE);
            } else {
                // Si no es muro, rellenar el interior de la celda con rectangulo negro(pasillo)
                //se ajusta la posición para que no se solapen con los muros
                screen.fillRect(j * cellSize + muroGrosor, i * cellSize + muroGrosor + margenSuperior, 
                                cellSize - 2 * muroGrosor, cellSize - 2 * muroGrosor, ILI9341_BLACK);
            }
        }
    }
}



// Función para mover el jugador 
void moverPlayer1(uint8_t direccion) {
    int delta = 1;  
    int newX = x1;
    int newY = y1;
    int playerWidth = 18;
    int playerHeight = 18;

    // Determinar nueva posición
    switch (direccion) {
    case UP:
        newY -= delta;              
        break;
    case DOWN:
        newY += delta;
        break;
    case RIGHT:
        newX += delta;
        break;
    case LEFT:
        newX -= delta;
        break;
    }

    // Comprobación básica de límites de pantalla
    if (newX < 0 || newX > XMAX - playerWidth || newY < 0 || newY > YMAX - playerHeight) {
        return;
    }

    // Detección de colisiones más precisa
    int cellSize = 24;
    int margenSuperior = 36;
    
    //Cálculo de las celdas en la que el jugador se encuentra
    int gridX1 = newX / cellSize;  
    int gridY1 = (newY - margenSuperior) / cellSize;  
    int gridX2 = (newX + playerWidth - 1) / cellSize;   
    int gridY2 = (newY + playerHeight - 1 - margenSuperior) / cellSize;  

    // Compruebe la colisión con una penetración mínima
    bool canMove = true;
    for (int y = gridY1; y <= gridY2; y++) {   
        for (int x = gridX1; x <= gridX2; x++) {
            if (y >= 0 && y < LABERINTO_ALTO && x >= 0 && x < LABERINTO_ANCHO) {
                if (laberinto[y][x] == 1) {   
                    // Prevención de colisiones más precisa
                    //Calcula las coordenadas de los muros de la celda
                    int wallLeft = x * cellSize;
                    int wallTop = y * cellSize + margenSuperior;
                    int wallRight = wallLeft + cellSize;
                    int wallBottom = wallTop + cellSize;

                    // Compruebe si el jugador está demasiado cerca de la pared
                    if (newX < wallRight && newX + playerWidth > wallLeft &&
                        newY < wallBottom && newY + playerHeight > wallTop) {
                        canMove = false;
                        break;
                    }
                }
            }
        }
        if (!canMove) break;
    }

    // Actualizar posición si el movimiento es válido
    if (canMove) {
        x1 = newX;
        y1 = newY;
        setPlayer1Position(x1, y1);
    }
}

// Interrupciones para cada dirección
void moverPlayerDerecha() {
    delay(50);  
    if(digitalRead(botonRight) == HIGH) {
        Serial.println("Boton RIGHT");
        moverPlayer1(RIGHT);
    }    
}

void moverPlayerIzquierda() {
    delay(50);  
    if(digitalRead(botonLeft) == HIGH) {
        Serial.println("Boton LEFT");
        moverPlayer1(LEFT);
    }    
}

void moverPlayerArriba() {
    delay(50);  
    if(digitalRead(botonUp) == HIGH) {
        Serial.println("Boton UP");
        moverPlayer1(UP);
    }    
}

void moverPlayerAbajo() {
    delay(50);  
    if(digitalRead(botonDown) == HIGH) {
        Serial.println("Boton DOWN");
        moverPlayer1(DOWN);
    }    
}


//Borrar objetos verdes
void borrarObjetosVerdes() {
    for (int i = 0; i < numObjetosVerdes; i++) {
        if (objetoVerdesExistente[i]) { //si es true lo borra
            screen.fillRect(objetosVerdesX[i], objetosVerdesY[i], 16, 16, ILI9341_BLACK); 
            objetoVerdesExistente[i] = false;
        }
    }
}


// Muestra el tiempo de juego
void mostrarTiempoDeJuego(void) {        
    unsigned long currentTime = gameTime - (millis() - startTime);
    if (currentTime > 0) {
        int seconds = (currentTime / 1000) % 60; 
        int minutes = (currentTime / 60000) % 60;

        char buffer[6];   //arreglo buffer
        sprintf(buffer, "%02d:%02d", minutes, seconds);  

        screen.setCursor(XMAX - 80, 10);
        screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        screen.setTextSize(2);
        screen.print(buffer);

        // Lógica de aparición y desaparición de objetos verdes
        unsigned long tiempoActual = millis();
        
        if (objetoVerdesExistente[0] == false &&  
            tiempoActual - ultimaAparicionObjetosVerdes >= intervaloObjetosVerdes) {   
            // Regenerar posiciones aleatorias para objetos verdes
            randomSeed(analogRead(0) + tiempoActual);
            generarPosicionesAleatoriasObjetosVerdes();
            
            // Dibujar objetos verdes
            dibujarObjetosVerdes();
            
            // Marcar tiempo de aparición
            ultimaAparicionObjetosVerdes = tiempoActual;
        }
        
        // Desaparecer objetos verdes después de 15 segundos
        if (objetoVerdesExistente[0] && 
            tiempoActual - ultimaAparicionObjetosVerdes >= duracionObjetosVerdes) {
            borrarObjetosVerdes();
        }
    } 
}


// Muestra el número de objetos recolectados
void mostrarObjetosRecolectados() {
    char buffer[10];
    sprintf(buffer, "%d/%d", objetosRecolectados, numObjetos);
    
    screen.setCursor(10, 10);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.setTextSize(2);
    screen.print(buffer);
}

// Función para comprobar si el jugador ha tocado un objeto amarillo
void comprobarColision() {
    int objetoSize = 16;
    for (int i = 0; i < numObjetos; i++) {
        if (objetoExistente[i] &&
            x1 < objetosX[i] + objetoSize && x1 + 18 > objetosX[i] &&  // Colisión horizontal
            y1 < objetosY[i] + objetoSize && y1 + 18 > objetosY[i]) {  // Colisión vertical

            // El jugador ha tocado el tesoro
            objetoExistente[i] = false;  
            objetosRecolectados++;  
            
            tone(BUZZER_PIN, 1500, 200); 
            delay(100);
            tone(BUZZER_PIN, 1200, 200); 
            delay(100);
            
            screen.fillRect(objetosX[i], objetosY[i], objetoSize, objetoSize, ILI9341_BLACK);  
        }
    }
}

// Función para comprobar si el Player1 ha tocado un objeto verde
void comprobarColisionVerde() {
    for (int i = 0; i < numObjetosVerdes; i++) {
        if (objetoVerdesExistente[i] &&
            x1 < objetosVerdesX[i] + 16 && x1 + 18 > objetosVerdesX[i] &&  
            y1 < objetosVerdesY[i] + 16 && y1 + 18 > objetosVerdesY[i]) {  

            // El jugador ha tocado el objeto verde
            objetoVerdesExistente[i] = false;  
            tone(BUZZER_PIN, 1200, 200); 
            delay(50);                  
            tone(BUZZER_PIN, 1500, 200); 
            delay(50);
            tone(BUZZER_PIN, 1800, 200); 
            delay(50);
           

            screen.fillRect(objetosVerdesX[i], objetosVerdesY[i], 16, 16, ILI9341_BLACK);  

            // Incrementar el tiempo en 30 segundos (30000 ms)
            gameTime += 30000;
        }
    }
}


//reiniciar el juego
void reiniciarJuego() {
    // Reiniciar variables de estado del juego
    x1 = 0;
    y1 = 0;
    objetosRecolectados = 0;
    juegoTerminado = false;

    // Reiniciar objetos
    for (int i = 0; i < numObjetos; i++) {
        objetoExistente[i] = false;
    }
    for (int i = 0; i < numObjetosVerdes; i++) {
        objetoVerdesExistente[i] = false;
    }

    // Limpiar pantalla
    screen.fillScreen(ILI9341_BLACK);

    // Redibujar laberinto
    dibujarLaberinto();

    // Reiniciar tiempo
    startTime = millis();
    gameTime = 180000; // Restablecer a 3 minutos
    ultimaAparicionObjetosVerdes = 0;

    // Regenerar posiciones de objetos
    generarPosicionesAleatoriasObjetos();
    dibujarObjetos();
    generarPosicionesAleatoriasObjetosVerdes();

    // Colocar jugador en posición inicial
    setPlayer1Position(0, YMAX - 32);
}

void mostrarGanaste() {

    tone(BUZZER_PIN, 1000, 300); // 1000 Hz 
    delay(300); 
    tone(BUZZER_PIN, 1500, 300); 
    delay(300);
    tone(BUZZER_PIN, 1200, 300); 
    delay(300);
    screen.fillScreen(ILI9341_BLACK);
    screen.setCursor(XMAX/2 - 80, YMAX/2 - 30);
    screen.setTextColor(ILI9341_GREEN);
    screen.setTextSize(3);
    screen.print("Ganaste!");


    // Añadir texto para reiniciar
    screen.setCursor(XMAX/2 - 120, YMAX/2 + 50);
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(2);
    screen.print("Presiona cualquier");
    screen.setCursor(XMAX/2 - 60, YMAX/2 + 80);
    screen.print("boton para");
    screen.setCursor(XMAX/2 - 60, YMAX/2 + 110);
    screen.print("reiniciar");
   

    // Esperar a que se presione un botón
    while(true) {
        if(digitalRead(botonRight) == HIGH || 
           digitalRead(botonLeft) == HIGH || 
           digitalRead(botonUp) == HIGH || 
           digitalRead(botonDown) == HIGH) {
            // Reiniciar el juego
            reiniciarJuego();
            break;
        }
    }
}

void mostrarPerdiste() {

    tone(BUZZER_PIN, 800, 300); // Tono inicial de 800 Hz
    delay(300); 
    tone(BUZZER_PIN, 500, 300); 
    delay(300);
    tone(BUZZER_PIN, 300, 300);
    delay(300);

    screen.fillScreen(ILI9341_BLACK);
    screen.setCursor(XMAX/2 - 80, YMAX/2 - 30);
    screen.setTextColor(ILI9341_RED);
    screen.setTextSize(3);
    screen.print("Perdiste!");

    // Añadir texto para reiniciar
    screen.setCursor(XMAX/2 - 120, YMAX/2 + 50);
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(2);
    screen.print("Presiona cualquier");
    screen.setCursor(XMAX/2 - 60, YMAX/2 + 80);
    screen.print("boton para");
    screen.setCursor(XMAX/2 - 60, YMAX/2 + 110);
    screen.print("reiniciar");

    // Esperar a que se presione un botón
    while(true) {
        if(digitalRead(botonRight) == HIGH || 
           digitalRead(botonLeft) == HIGH || 
           digitalRead(botonUp) == HIGH || 
           digitalRead(botonDown) == HIGH) {
            // Reiniciar el juego
            reiniciarJuego();
            break;
        }
    }
}
