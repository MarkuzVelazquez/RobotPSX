#include <stdio.h>
#include <stdlib.h>
#include <psx.h>
#include <psxgpu.h>

#define pad1Check(a) padbuf & a
#define pad2Check(a) padbuf2 & a
#define irandom(a, b) ((rand()%((absf(a-b))+1))+a)

#define instanceCreate(c, d) \
	if (d.Ini == NULL) { \
		d.Ini = (struct c *) malloc(sizeof(struct c)); \
		d.Ini->Ant = NULL; \
		d.Fin = d.Ini; \
	} \
	else { \
		d.Fin->Sig = (struct c *) malloc(sizeof(struct c)); \
		d.Fin->Sig->Ant = d.Fin; \
		d.Fin = d.Fin->Sig; \
	} \
	d.Fin->Sig = NULL;

#define instanceDestroy(a, b) \
	if (b.Ini->Sig == NULL) { \
		free(b.Ini); \
		b.Ini = NULL; \
		b.Fin = NULL; \
	} \
	else if (a->Sig == b.Fin) { \
		b.Fin->Ant = a->Ant; \
		*a = *b.Fin; \
		free(b.Fin); \
		b.Fin = a; \
	} \
	else if (a->Sig != NULL) { \
		a->Sig->Ant = a->Ant; \
		*a = *a->Sig; \
		free(a->Sig->Ant); \
		a->Sig->Ant = a;\
	} \
	else { \
		b.Fin = b.Fin->Ant; \
		free(b.Fin->Sig); \
		b.Fin->Sig = NULL; \
	}

#define crearBlock(_a, _b, c) \
	instanceCreate(escenario, globalE); \
	globalE.Fin->rect.x = _a; \
	globalE.Fin->rect.y = _b; \
	globalE.Fin->rect.h = 16; \
	globalE.Fin->rect.w = 16; \
	globalE.Fin->rect.r = 0; \
	globalE.Fin->rect.g = (c == 1) ? 255 : 0; \
	globalE.Fin->rect.b = (c == 1) ? 0 : 255; \
	globalE.Fin->rect.attribute = 80;

#define crearPlayer(_a, _b) \
	player.x = _a; \
	player.y = _b; \
	player.h = 16; \
	player.w = 16; \
	player.r = 255; \
	player.g = 0; \
	player.b = 0; \
	player.attribute = 0; \
	xstart = player.x; \
	ystart = player.y;

#define generaEscRandom() \
	generaEscenario(irandom(1, nLevel)); \
	state = 2;

#define restart() \
	step = 0; \
	seg = 0; \
	opcionX = 0; \
	state = 0; \
	player.x = xstart; \
	player.y = ystart;

#define checarNivel(_a) archivo = fopen("cdrom:\\Niveles\\nivel"#_a".lvl;1", "rb")

#define crearEscenario() \
	if (opcion == nLevel + 1) { \
		room = EDITOR; \
	} \
	generaEscenario(opcion);

#define escenarioDestroy() \
	while(globalE.Ini != NULL) { \
		instanceDestroy(globalE.Fin, globalE); \
	}

unsigned int primeList[0x40000];
volatile int displayOld = 1;
volatile int timeCounter = 0;
int dbuf = 0;
unsigned short padbuf;
unsigned char *data_buffer;
void blankHandler();

unsigned int distancia(int, int, int, int);
_Bool collisionRectangle(int, int, GsRectangle *, GsRectangle *);
int clamp(int, int, int);
int absf(int);

GsRectangle rectangulo[19];
GsRectangle player;

enum {
	MENU, NIVEL, EDITOR
};

struct escenario {
	GsRectangle rect;
	struct escenario *Sig;
	struct escenario *Ant;
};

struct listaEscenario {
	struct escenario *Ini;
	struct escenario *Fin;
} globalE;

void gameInit();
void generaEscenario(int);
unsigned short int numeroDeNivel();
void menuLevel(unsigned short int);
void logicaJuego();
void dibujarInterfaz();

unsigned char room = MENU;
unsigned char state = 0;
unsigned char seg = 0;
unsigned char step = 0;
unsigned char nLevel;

_Bool press_up = false;
_Bool press_down =  false;
_Bool press_right = false;
_Bool press_left = false;
_Bool press_cross = false;
_Bool press_circle = false;

int xstart;
int ystart;

unsigned char opcionX = 0;
unsigned char opcionY[14];
unsigned char opcion = 1;

int main() {
	PSX_Init();
	GsInit();
	GsSetList(primeList);
	GsClearMem();
	GsSetVideoMode(320, 240, VMODE_NTSC);
	GsLoadFont(768, 0, 768, 256);
	SetVBlankHandler(blankHandler);

	globalE.Ini = NULL;

	// Checa cuantos niveles existen
	nLevel = numeroDeNivel();

	while(1) {		
		if(displayOld) {
			dbuf = !dbuf;
			GsSetDispEnvSimple(0, dbuf ? 0: 256);
			GsSetDrawEnvSimple(0, dbuf ? 256 : 0, 320, 240);
			GsSortCls(0, 0, 0);
			PSX_ReadPad(&padbuf, NULL);
			switch(room) {
				case MENU:
					// Menu level
					menuLevel(nLevel);
				break;

				case NIVEL:
					// Juego
					logicaJuego();
				break;
				case EDITOR:
					// Info editor
					GsPrintFont(5, 10, "Mueva el player con las flechas");
					GsPrintFont(5, 30, "Cree un bloque con el boton CROSS");
					GsPrintFont(5, 50, "Precione el boton CROSS sobre un");
					GsPrintFont(5, 60, "bloque para indicar la meta");
					GsPrintFont(5, 80, "Borre un bloque con el boton CIRCLE");
					GsPrintFont(5, 100, "Presione START para probar el nivel");
					GsPrintFont(5, 120, "Presione CROSS para continuar");
					if (pad1Check(PAD_CROSS)) {	
						if (!press_cross) {					
							press_cross = true;
							room = NIVEL;
							generaEscRandom();
						}
					}
					else {
						press_cross = false;
					}
				break;
			}
			GsDrawList();
			while(GsIsDrawing());
			displayOld = 0;
		}
	}
	return 0;
}

void blankHandler() {
	displayOld = 1;
	timeCounter++;
}

unsigned int distancia(int x1, int y1, int x2, int y2) {
	unsigned int DisX, DisY;
	DisX = absf(x1-x2);
	DisY = absf(y1-y2);
	return DisX + DisY;
}

_Bool collisionRectangle(int hspeed, int vspeed, GsRectangle *Temp, GsRectangle *Temp2) {
	return (Temp->x+hspeed  < Temp2->x+Temp2->w 
	&& Temp->x+hspeed+Temp->w > Temp2->x 
	&& Temp->y+vspeed < Temp2->y+Temp2->h 
	&& Temp->y+vspeed+Temp->h > Temp2->y);
}

int absf(int a) {
	return (a < 0) ? (-(a)) : a;
}

int clamp(int x, int a, int b) {
	return (x < a) ? a : (x > b) ? b : x;
}

void generaEscenario(int _a) {
	FILE *archivo;
	int x = 0, y = 0;
	unsigned char i, size;
	switch(_a) {
		case 1: checarNivel(1); break;
		case 2: checarNivel(2); break;
		case 3: checarNivel(3); break;
		case 4: checarNivel(4); break;
		case 5: checarNivel(5); break;
		case 6: checarNivel(6); break;
		case 7: checarNivel(7); break;
		case 8: checarNivel(8); break;
	}
	fseek(archivo, 0, SEEK_END);
	size = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);
	data_buffer = (unsigned char *) malloc(size);
	fread(data_buffer, 1, size, archivo);
	fseek(archivo, 0, SEEK_SET);
	fclose(archivo);
	for(i = 0; i < size; i++) {
		switch(data_buffer[i]) {
			case 'f': crearBlock(x, y, 1);
			x+=16;
			break;
			case 'p': crearPlayer(x, y);
			case 'x': crearBlock(x, y, 0);
			case ' ': x+=16;
			break;
			case '\\':
				y+=16;
				x = 0;
			break;
		}
	}
	free(data_buffer);
}

unsigned short int numeroDeNivel() {
	FILE *archivo;
	unsigned char nLevel = 0;
	while(1) {
		switch(nLevel+1) {
		 case 1: checarNivel(1); break;
		 case 2: checarNivel(2); break;
		 case 3: checarNivel(3); break;
		 case 4: checarNivel(4); break;
		 case 5: checarNivel(5); break;
		 case 6: checarNivel(6); break;
		 case 7: checarNivel(7); break;
		 case 8: checarNivel(8); break;
		}
		if (archivo == NULL) {
			break;
		}
		fclose(archivo);
		nLevel++;
	}
	return nLevel;
}

void gameInit() {
	unsigned char i;
	/* Se crea la parte superior */
    for(i = 15; i <= 18; i++) {
    	rectangulo[i].x = 20;
    	rectangulo[i].y = -6 + ((i-15)*16) + i;
    	rectangulo[i].h = 16;
    	rectangulo[i].w = 16;
    	rectangulo[i].r = (irandom(0, 2) == 0) ? 255 : irandom(0, 255);
    	rectangulo[i].g = (irandom(0, 2) == 1) ? 255 : irandom(0, 255);
    	rectangulo[i].b = (irandom(0, 2) == 2) ? 255 : irandom(0, 255);
    	rectangulo[i].attribute = 0;
    }
	/* Crea rectangulos en la parte inferior */
	for(i = 0; i <= 14; i++) {
    	rectangulo[i].x = 10 + (i*16) + i;
    	rectangulo[i].y = 214;
    	rectangulo[i].h = 16;
    	rectangulo[i].w = 16;
    	rectangulo[i].r = rectangulo[15].r;
    	rectangulo[i].g = rectangulo[15].g;
    	rectangulo[i].b = rectangulo[15].b;
    	rectangulo[i].attribute = 0;
		opcionY[i] = 0;
    }
}

void menuLevel(unsigned short int nLevel) {
	GsPrintFont(10, 10, "Seleccione nivel");
	
	unsigned char i;
	// Dibuja cuantos niveles haya
	for(i = 1; i <= nLevel; i++) {
		GsPrintFont(20, 10 + (i * 10), "Nivel%d.lvl", i);
	}
	GsPrintFont(20, 10 + (i * 10), "Crear nivel");

	// Cursor
	GsPrintFont(10, 10 + (opcion * 10), ">");
	
	// Control
	if (pad1Check(PAD_UP)) {
		if (!press_up) {
			press_up = true;
			opcion = clamp(opcion-1, 1, nLevel + 1);
		}
	}
	else {
		press_up = false;
	}
	if (pad1Check(PAD_DOWN)) {
		if (!press_down) {
			press_down = true;
			opcion = clamp(opcion+1, 1, nLevel + 1);
		}
	}
	else {
		press_down = false;
	}

	if (pad1Check(PAD_CROSS)) {
		if (!press_cross) {
			press_cross = true;
			room = NIVEL;
			gameInit();

			// Genera escenario
			crearEscenario();
		}
	}
	else {
		press_cross = false;
	}
}

void logicaJuego() {
	GsSortRectangle(&player);
	struct escenario *temp;
	// Dibuja escenario
	temp = globalE.Ini;
	while(temp != NULL) {
		GsSortRectangle(&temp->rect);
		temp = temp->Sig;
	}
	switch(state) {
		case MENU:
			/**
			 * Menu principal
			 */

			// Control
			if (pad1Check(PAD_UP)) {
				if (!press_up) {
					press_up = true;
					opcionY[opcionX] = clamp(opcionY[opcionX]-1, 0, 3);
					rectangulo[opcionX].r = rectangulo[15+opcionY[opcionX]].r;
					rectangulo[opcionX].g = rectangulo[15+opcionY[opcionX]].g;
					rectangulo[opcionX].b = rectangulo[15+opcionY[opcionX]].b;
				}
			}
			else {
				press_up = false;
			}
			if (pad1Check(PAD_DOWN)) {
				if (!press_down) {
					press_down = true;
					opcionY[opcionX] = clamp(opcionY[opcionX]+1, 0, 3);
					rectangulo[opcionX].r = rectangulo[15+opcionY[opcionX]].r;
					rectangulo[opcionX].g = rectangulo[15+opcionY[opcionX]].g;
					rectangulo[opcionX].b = rectangulo[15+opcionY[opcionX]].b;
				}
			}
			else {
				press_down = false;
			}
			if (pad1Check(PAD_RIGHT)) {
				if (!press_right) {
					press_right = true;
					opcionX = clamp(opcionX+1, 0, 14);
				}
			}
			else {
				press_right = false;
			}
			if (pad1Check(PAD_LEFT)) {
				if (!press_left) {
					press_left = true;
					opcionX = clamp(opcionX-1, 0, 14);
				}
			}
			else {
				press_left = false;
			}
			if (pad1Check(PAD_CROSS)) {
				if (!press_cross) {
					press_cross = true;

					// Logica para mover player
					state = 1;
				}
			}
			else {
				press_cross = false;
			}

			// DIbujar Interfaz
			dibujarInterfaz();
		break;
		case NIVEL:
			// Dibuja posicion de step
			GsPrintFont(13 + (step * 16) + step, 236, "^");

			//Mover player
			if (seg++ == 25) {
				seg = 0;
				switch(opcionY[step]) {
					case 0:
						// Arriba
						temp = globalE.Ini;
						while(temp != NULL) {
							if (collisionRectangle(0, -16, &player, &temp->rect)) {
								player.y -= 16;
								if (temp->rect.g == 255) {
									state = 3;
									break;
								}
								break;
							}
							temp = temp->Sig;
						}
						if (temp == NULL) {
							state = 4;
							break;
						}
					break;
					case 1:
						// Abajo
						temp = globalE.Ini;
						while(temp != NULL) {
							if (collisionRectangle(0, 16, &player, &temp->rect)) {
								player.y += 16;
								if (temp->rect.g == 255) {
									state = 3;
									break;
								}
								break;
							}
							temp = temp->Sig;
						}
						if (temp == NULL) {
							state = 4;
							break;
						}
					break;
					case 2:
						// izquierda
						temp = globalE.Ini;
						while(temp != NULL) {
							if (collisionRectangle(-16, 0, &player, &temp->rect)) {
								player.x -= 16;
								if (temp->rect.g == 255) {
									state = 3;
									break;
								}
								break;
							}
							temp = temp->Sig;
						}
						if (temp == NULL) {
							state = 4;
							break;
						}
					break;
					case 3:			    			
						// derecha
						temp = globalE.Ini;
						while(temp != NULL) {
							if (collisionRectangle(16, 0, &player, &temp->rect)) {
								player.x += 16;
								if (temp->rect.g == 255) {
									state = 3;
									break;
								}
								break;
							}
							temp = temp->Sig;
						}
						if (temp == NULL) {
							state = 4;
							break;
						}
					break;
				}
				if (step < 14) {step++;} else {state = 0; step = 0;}
			}
			
			// DIbujar Interfaz
			dibujarInterfaz();
		break;
		case EDITOR:
			/**
			 * Editor de Niveles
			 */

			if (pad1Check(PAD_UP)) {
				if (!press_up) {
					press_up = true;
					player.y -= 16;
				}
			}
			else {
				press_up = false;
			}
			if (pad1Check(PAD_DOWN)) {
				if (!press_down) {
					press_down = true;
					player.y += 16;
				}
			}
			else {
				press_down = false;
			}
			if (pad1Check(PAD_RIGHT)) {
				if (!press_right) {
					press_right = true;
					player.x += 16;
				}
			}
			else {
				press_right = false;
			}
			if (pad1Check(PAD_LEFT)) {
				if (!press_left) {
					press_left = true;
					player.x -= 16;
				}
			}
			else {
				press_left = false;
			}
			if (pad1Check(PAD_START)) {
				temp = globalE.Ini;
				while(temp != NULL) {
					if (collisionRectangle(0, 0, &player, &temp->rect)) {
						if (temp->rect.b == 255) {
							break;
						}
					}
					temp = temp->Sig;
				}
				if (temp != NULL) {
					xstart = player.x;
					ystart = player.y;
						
					// Regresa al juego
					state = 0;
				}
			}
			if (pad1Check(PAD_CROSS)) {
				if (!press_cross) {
					press_cross = true;
					temp = globalE.Ini;
					while(temp != NULL) {
						if (collisionRectangle(0, 0, &player, &temp->rect)) {
							if (temp->rect.b == 0) {
								temp->rect.b = 255;
								temp->rect.g = 0;
							}
							else {
								temp->rect.b = 0;
								temp->rect.g = 255;
							}
							break;
						}
						temp = temp->Sig;
					}
					if (temp == NULL) {
						crearBlock(player.x, player.y, 0);
					}
				}
			}
			else {
				press_cross = false;
			}
			if (pad1Check(PAD_CIRCLE)) {
				if (!press_circle) {
					press_circle = true;
					temp = globalE.Ini;
					while(temp != NULL) {
						if (collisionRectangle(0, 0, &player, &temp->rect)) {
							instanceDestroy(temp, globalE);
						}
					temp = temp->Sig;
					}
				}
			}
			else {
				press_circle = false;
			}
		break;
		case 3:
			// Winer
			if (pad1Check(PAD_CROSS)) {
				if (!press_cross) {
					press_cross = true;
					opcion++;
					gameInit();
					escenarioDestroy();
					restart();
					if (opcion > nLevel) {
						// Menu level
						room = MENU;
						opcion = nLevel + 1;
						break;
					}
					// Next level
					room = NIVEL;
					crearEscenario();
				}
			}
			else {
				press_cross = false;
			}
			GsPrintFont(5, 110, "Ganaste!, Presione CROSS para continuar");

			// DIbujar Interfaz
			dibujarInterfaz();
		break;
		case 4:
			// Restart
			if (pad1Check(PAD_CROSS)) {
				if (!press_cross) {
					press_cross = true;
					restart();
					room = NIVEL;
				}
			}
			else {
				press_cross = false;
			}
			GsPrintFont(5, 110, "Perdiste, Presione CROSS para continuar");

			// DIbujar Interfaz
			dibujarInterfaz();
		break;
	}
}

void dibujarInterfaz() {
	// Dibujar Rectangulos
	unsigned char i;
	for(i = 0; i < 19; i++) {
		GsSortRectangle(&rectangulo[i]);
	}
	// Dibujar texto
	GsPrintFont(36, 10, "UP");
	GsPrintFont(36, 27, "Down");
	GsPrintFont(36, 44, "Left");
	GsPrintFont(36, 61, "Right");

	// Cursor Superior
	GsPrintFont(10, 16 + (opcionY[opcionX] * 16) + opcionY[opcionX], ">");

	// Cursor Inferior
	GsPrintFont(13 + (opcionX * 16) + opcionX, 204, "V");
}
